#pragma once
#include "vulkan_engine.hpp"
#include "reference.hpp"
#include "localization.hpp"
#include "adaptive.hpp"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

// Local, versioned binary pipe. One process/device is retained for the session.
// Request: JSON line {id,timestamp_ms} + 256*144*3 RGB bytes.
// Response: JSON line + payload_frame_count fixed RGB frames. Not a network API.
inline int runStream(const Options& options,std::string backend,int multiplier,float blendWeight,int radius) {
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY); _setmode(_fileno(stdout),_O_BINARY);
#endif
    std::unique_ptr<Engine> gpu;
    if(backend=="auto" && (options.mode=="repeat"||options.mode=="blend"))backend="cpu";
    if(backend!="cpu") {
        gpu=std::make_unique<Engine>();
        try {gpu->init(activeLocalizer->name());if(backend=="auto" && gpu->software)gpu.reset();}
        catch(const std::exception&) {if(backend=="vulkan")throw;gpu.reset();}
    }
    AdaptiveController adaptive;
    std::vector<Pixel> previous,current(W*H),result,flow;
    std::vector<unsigned char> bytes(W*H*3);
    uint64_t lastId=0;double lastTime=0;std::string line;
    while(std::getline(std::cin,line)) {
        if(line.size()>4096)throw std::runtime_error("errors.stream_protocol");
        auto request=Json::parse(line);auto id=request.at("id").get<uint64_t>();auto time=request.at("timestamp_ms").get<double>();
        validTime(time);
        if(!previous.empty() && (id<=lastId || time<=lastTime))throw std::runtime_error("errors.stream_protocol");
        std::cin.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
        if(std::cin.gcount()!=std::streamsize(bytes.size()))throw std::runtime_error("errors.stream_protocol");
        for(size_t i=0;i<current.size();i++)current[i]={bytes[3*i]/255.f,bytes[3*i+1]/255.f,bytes[3*i+2]/255.f,1};
        auto start=std::chrono::steady_clock::now();
        Decision decision{options.mode,"explicit_mode",500./options.sourceFps};
        if(previous.empty())decision={"passthrough","first_frame",500./options.sourceFps};
        else if(options.mode=="adaptive")decision=adaptive.decide(analyzePair(previous,current),options);
        std::vector<std::vector<Pixel>> payload;
        Json kinds=Json::array();int generated=0,repeated=0,slots=1;
        if(decision.mode=="repeat") {
            payload.push_back(current);slots=multiplier;repeated=multiplier-1;
            kinds.push_back("real");for(int i=1;i<multiplier;i++)kinds.push_back("repeat");
        } else if(decision.mode=="passthrough" || decision.mode=="motion" || decision.mode=="optical-flow") {
            payload.push_back(current);kinds.push_back("real");
        } else {
            payload.push_back(previous);kinds.push_back("real");slots=multiplier;
            for(int i=1;i<multiplier;i++) {
                float alpha=decision.mode=="blend"?blendWeight:float(i)/multiplier;
                int flags=options.flags(decision.mode);
                bool reuse=i>1 && needsFlow(flags);
                if(gpu)gpu->run(previous,current,result,flags,alpha,radius,reuse);
                else runCPU(previous,current,result,flow,flags,alpha,radius,reuse);
                // A fallback/copy is not a genuinely new image, even when an
                // interpolation dispatch ran. Keep these counters distinct.
                bool sameA=true,sameB=true;
                auto quantize=[](float channel) {return int(std::round(std::clamp(channel,0.f,1.f)*255));};
                for(size_t pixel=0;pixel<result.size();pixel++) {
                    const auto r=result[pixel],a=previous[pixel],b=current[pixel];
                    sameA&=quantize(r.r)==quantize(a.r) && quantize(r.g)==quantize(a.g) && quantize(r.b)==quantize(a.b);
                    sameB&=quantize(r.r)==quantize(b.r) && quantize(r.g)==quantize(b.g) && quantize(r.b)==quantize(b.b);
                }
                payload.push_back(result);
                if(sameA || sameB) {kinds.push_back("repeat");repeated++;}
                else {kinds.push_back("generated");generated++;}
            }
            double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
            if(options.mode=="adaptive")adaptive.observe(elapsed);
        }
        double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        Json response={{"protocol",1},{"id",id},{"source_id",decision.mode=="interpolate"||decision.mode=="blend"?lastId:id},
            {"payload_frame_count",payload.size()},{"slot_count",slots},{"kinds",kinds},{"generated_count",generated},{"repeated_count",repeated},
            {"processing_ms",elapsed},{"backend",gpu?"vulkan":"cpu"},{"device",gpu?gpu->deviceName:"CPU"},
            {"software_device",!gpu||gpu->software},{"selected_mode",decision.mode},{"decision_reason",decision.reason},
            {"lookahead_ms",generated?time-lastTime:0},{"gpu_timestamp_ms",gpu&&gpu->gpuTimestampMs>=0?Json(gpu->gpuTimestampMs):Json(nullptr)},
            {"width",W},{"height",H},{"real_game_fps",nullptr},{"displayed_fps",nullptr},{"input_latency_ms",nullptr}};
        std::cout<<response.dump()<<'\n';
        for(const auto& frame:payload) {
            for(size_t i=0;i<frame.size();i++) {
                auto p=frame[i];int k=0;
                for(float value:{p.r,p.g,p.b})bytes[i*3+k++]=static_cast<unsigned char>(std::round(std::clamp(value,0.f,1.f)*255));
            }
            std::cout.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
        }
        std::cout.flush();if(!std::cout)throw std::runtime_error("errors.stream_protocol");
        previous=current;lastId=id;lastTime=time;
    }
    return 0;
}
