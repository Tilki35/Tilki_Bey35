#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#include "options.hpp"
#include "types.hpp"
#include "adaptive.hpp"
#include "pacing.hpp"

#include "vulkan_engine.hpp"
#include "reference.hpp"
#include "timeline_demo.hpp"
#include "localization.hpp"
#include "stream.hpp"
#include <iomanip>
#include <sstream>
#include <cctype>
std::vector<Pixel> scene(float dx,float dy,bool flat=false) {
    std::vector<Pixel> result(W*H);
    for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
        float u=x-dx,v=y-dy;
        float c=flat ? .4f : .5f+.19f*std::sin(u*.27f+v*.13f)+.17f*std::cos(u*.11f-v*.31f)+.10f*std::sin(u*.49f+v*.37f);
        result[y*W+x]={c,c*.8f,c*.6f,1};
    }
    return result;
}
double mae(const std::vector<Pixel>& a,const std::vector<Pixel>& b,int margin) {
    double sum=0; size_t n=0;
    for(int y=margin;y<H-margin;y++) for(int x=margin;x<W-margin;x++) {
        const auto& p=a[y*W+x]; const auto& q=b[y*W+x];
        if(!std::isfinite(p.r)||!std::isfinite(p.g)||!std::isfinite(p.b)||std::abs(p.a-1)>1e-5)
            throw std::runtime_error("errors.invalid_output_pixel");
        sum+=std::abs(p.r-q.r)+std::abs(p.g-q.g)+std::abs(p.b-q.b); n+=3;
    }
    return sum/n;
}
void ppm(const std::filesystem::path& path,const std::vector<Pixel>& pixels) {
    std::ofstream out(path,std::ios::binary); out<<"P6\n"<<W<<" "<<H<<"\n255\n";
    for(auto p:pixels) for(float c:{p.r,p.g,p.b}) {
        unsigned char byte=static_cast<unsigned char>(std::round(std::fmax(0.f,std::fmin(1.f,c))*255)); out.put(static_cast<char>(byte));
    }
    if(!out) throw std::runtime_error("errors.cannot_write|"+path.string());
}
std::vector<Pixel> readPPM(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary);
    if(!in) throw std::runtime_error("errors.cannot_open|"+path.string());
    auto token=[&]() {
        std::string word; char ch;
        while(in.get(ch)) {
            if(ch=='#') { in.ignore(65536,'\n'); continue; }
            if(!std::isspace(static_cast<unsigned char>(ch))) { word+=ch; break; }
        }
        while(in.get(ch)) {
            if(std::isspace(static_cast<unsigned char>(ch))) {
                if(ch=='\r' && in.peek()=='\n') in.get();
                break;
            }
            if(word.size()>32) throw std::runtime_error("errors.ppm_header_token_too_long");
            word+=ch;
        }
        return word;
    };
    if(token()!="P6" || token()!=std::to_string(W) || token()!=std::to_string(H) || token()!="255")
        throw std::runtime_error("errors.expected_256x144_p6_ppm_with_maxval_255");
    std::vector<unsigned char> bytes(W*H*3);
    in.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    if(in.gcount()!=static_cast<std::streamsize>(bytes.size())) throw std::runtime_error("errors.truncated_ppm_pixels");
    std::vector<Pixel> result(W*H);
    for(size_t i=0;i<result.size();i++) result[i]={bytes[i*3]/255.f,bytes[i*3+1]/255.f,bytes[i*3+2]/255.f,1};
    return result;
}
double number(const std::string& text,double minimum,double maximum) {
    size_t count=0; double value=std::stod(text,&count);
    if(count!=text.size() || !std::isfinite(value) || value<minimum || value>maximum)
        throw std::runtime_error("errors.numeric|"+text);
    return value;
}
void flowCSV(const std::filesystem::path& path,const Pixel* flow,bool dense) {
    std::ofstream out(path); out<<"x,y,forward_x,forward_y,backward_x,backward_y\n";
    int width=dense?W:(W+7)/8,height=dense?H:(H+7)/8;
    for(int y=0;y<height;y++) for(int x=0;x<width;x++) {
        auto v=flow[y*width+x];
        out<<x*(dense?1:8)<<','<<y*(dense?1:8)<<','<<v.r<<','<<v.g<<','<<v.b<<','<<v.a<<'\n';
    }
    if(!out) throw std::runtime_error("errors.cannot_write_flow_csv");
}
int runBackend(int argc,char** argv) {
    try {
        std::filesystem::path out="results";
        std::string backend="auto";
        Options options; bool pacing=true,demo=false; double simulatedCost=1;
        std::string sceneName="all",inputA,inputB;
        bool stream=false;int multiplier=2,radius=12;float blendWeight=.5f;
        if(argc==2 && std::string(argv[1])=="--help") {
            std::cout<<ui("app.help"); return 0;
        }
        for(int i=1;i<argc;i++) {
            std::string arg=argv[i];
            if(arg=="--output" && i+1<argc) out=std::filesystem::u8path(argv[++i]);
            else if(arg=="--backend" && i+1<argc) backend=argv[++i];
            else if(arg=="--mode" && i+1<argc) options.mode=argv[++i];
            else if(arg=="--estimator" && i+1<argc) options.estimator=argv[++i];
            else if(arg=="--scene" && i+1<argc) sceneName=argv[++i];
            else if(arg=="--input-a" && i+1<argc) inputA=argv[++i];
            else if(arg=="--input-b" && i+1<argc) inputB=argv[++i];
            else if(arg=="--source-fps" && i+1<argc) options.sourceFps=number(argv[++i],1,1000);
            else if(arg=="--display-hz" && i+1<argc) options.displayHz=number(argv[++i],1,1000);
            else if(arg=="--simulated-processing-ms" && i+1<argc) simulatedCost=number(argv[++i],0,10000);
            else if(arg=="--pacing-demo") demo=true;
            else if(arg=="--stream") stream=true;
            else if(arg=="--multiplier" && i+1<argc) {double value=number(argv[++i],1,8);if(value!=std::floor(value))throw std::runtime_error("errors.stream_protocol");multiplier=int(value);}
            else if(arg=="--search-radius" && i+1<argc) {double value=number(argv[++i],1,24);if(value!=std::floor(value))throw std::runtime_error("errors.stream_protocol");radius=int(value);}
            else if(arg=="--blend-weight" && i+1<argc) blendWeight=float(number(argv[++i],0,1));
            else if((arg=="--artifact-reduction" || arg=="--pacing") && i+1<argc) {
                std::string value=argv[++i];
                if(value!="on" && value!="off") throw std::runtime_error("errors.expected_on_or_off");
                if(arg=="--pacing") pacing=value=="on"; else options.artifacts=value=="on";
            }
            else throw std::runtime_error("errors.unknown_or_incomplete_option_run_help");
        }
        if(backend!="auto" && backend!="vulkan" && backend!="cpu") throw std::runtime_error("errors.backend|"+backend);
        const std::vector<std::string> modes{"repeat","blend","interpolate","motion","optical-flow","adaptive"};
        if(std::find(modes.begin(),modes.end(),options.mode)==modes.end()) throw std::runtime_error("errors.unknown_mode");
        if(options.estimator!="block" && options.estimator!="optical-flow") throw std::runtime_error("errors.unknown_estimator");
        if(inputA.empty()!=inputB.empty()) throw std::runtime_error("errors.both_input_frames_are_required");
        if(stream) {
            if(demo || !inputA.empty() || options.mode=="motion" || options.mode=="optical-flow")throw std::runtime_error("errors.stream_protocol");
            return runStream(options,backend,multiplier,blendWeight,radius);
        }
        if(multiplier!=2)throw std::runtime_error("errors.stream_protocol");
        bool external=!inputA.empty();
        std::filesystem::create_directories(out);
        if(demo) { timelineDemo(options,pacing,simulatedCost,out); std::cout<<ui("pacing.result",{{"path",(out/"timeline.json").u8string()}})<<'\n'; return 0; }
        std::unique_ptr<Engine> gpu;
        // Cheap modes need no Vulkan initialization unless the user explicitly requests it.
        if(backend=="auto" && (options.mode=="repeat" || options.mode=="blend")) backend="cpu";
        auto initStart=std::chrono::steady_clock::now();
        if(backend!="cpu") {
            gpu=std::make_unique<Engine>();
            try { gpu->init(activeLocalizer->name()); }
            catch(const std::exception& e) {
                if(backend=="vulkan") throw;
                std::cerr<<ui("warning.vulkan",{{"detail",uiError(e.what())}})<<'\n';
                gpu.reset(); backend="cpu";
            }
        }
        double initMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-initStart).count();
        std::ofstream report(out/"metrics.json");
        if(!report) throw std::runtime_error("errors.cannot_create_metrics_json");
        report<<std::setprecision(9)<<"{\n  \"real_fps\":null,\"generated_fps\":null,\"displayed_fps\":null,\"input_latency_ms\":null,"
              <<"\"frame_submission_interval_ms\":null,\"presentation_backend\":\"none\",\"measurement_scope\":\"offline_pairs\","
              <<"\"requested_mode\":\""<<options.mode<<"\",\"estimator\":\""<<options.estimator<<"\",\"artifact_reduction\":"<<(options.artifacts?"true":"false")
              <<",\"quality_reference_known\":"<<(external?"false":"true")<<",\"initialization_wall_ms\":"<<initMs<<",\"cases\":[\n";
        struct Case { const char* name; float dx,dy; bool flat; };
        std::vector<Case> cases{{"horizontal",8,0,false},{"vertical",0,6,false},{"diagonal",6,-4,false},{"static",0,0,false},{"flat",0,0,true},{"hud",8,0,false},{"scene_cut",0,0,true}};
        if(external) cases={{"external",0,0,false}};
        else if(sceneName=="fractional") cases={{"fractional",5.5f,2.5f,false}};
        else if(sceneName!="all") {
            auto it=std::find_if(cases.begin(),cases.end(),[&](const Case& c) {return sceneName==c.name;});
            if(it==cases.end()) throw std::runtime_error("errors.unknown_scene");
            cases={*it};
        }
        AdaptiveController adaptive; bool passed=true; size_t generatedPairs=0,repeatedPairs=0;
        for(size_t i=0;i<cases.size();i++) {
            auto c=cases[i]; auto a=scene(0,0,c.flat),b=scene(c.dx,c.dy,c.flat),truth=scene(c.dx*.5f,c.dy*.5f,c.flat);
            if(external) { a=readPPM(std::filesystem::u8path(inputA)); b=readPPM(std::filesystem::u8path(inputB)); truth=b; }
            bool hud=std::string(c.name)=="hud",cut=std::string(c.name)=="scene_cut";
            if(hud) for(int y=32;y<56;y++) for(int x=32;x<224;x++) {
                float value=((x/2+y/4)%2) ? 1.f : 0.f;
                a[y*W+x]=b[y*W+x]=truth[y*W+x]={value,value,value,1};
            }
            if(cut) { std::fill(a.begin(),a.end(),Pixel{0,0,0,1}); std::fill(b.begin(),b.end(),Pixel{1,1,1,1}); truth=b; }
            std::vector<Pixel> blend(a.size()),generated;
            for(size_t j=0;j<a.size();j++) blend[j]={a[j].r*(1-blendWeight)+b[j].r*blendWeight,a[j].g*(1-blendWeight)+b[j].g*blendWeight,a[j].b*(1-blendWeight)+b[j].b*blendWeight,1};
            auto processingStart=std::chrono::steady_clock::now();
            Features features=analyzePair(a,b);
            Decision decision{options.mode,"explicit_mode",500./options.sourceFps};
            if(options.mode=="adaptive") decision=adaptive.decide(features,options);
            int flags=options.flags(decision.mode);
            std::vector<Pixel> cpuFlows;
            float alpha=decision.mode=="blend"?blendWeight:.5f;
            double computeMs=gpu ? gpu->run(a,b,generated,flags,alpha,radius) : runCPU(a,b,generated,cpuFlows,flags,alpha,radius);
            size_t changing=0,rejected=0;
            for(size_t j=0;j<a.size();j++) {
                if(a[j].r==b[j].r && a[j].g==b[j].g && a[j].b==b[j].b) continue;
                changing++;
                rejected+=generated[j].r==b[j].r && generated[j].g==b[j].g && generated[j].b==b[j].b;
            }
            double fallbackFraction=changing ? double(rejected)/changing : 0;
            if(options.mode=="adaptive" && decision.mode=="interpolate" && fallbackFraction>.7) {
                decision.mode="passthrough"; decision.reason="unreliable_warp"; generated=b;
            }
            double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-processingStart).count();
            if(needsFlow(flags) && options.mode=="adaptive") adaptive.observe(ms);
            generatedPairs+=decision.mode=="interpolate" || decision.mode=="blend";
            repeatedPairs+=decision.mode=="repeat";
            // Validate finite RGBA across the entire image, then score an interior crop.
            double fullError=mae(generated,truth,0),error=mae(generated,truth,24),baseline=mae(blend,truth,24);
            auto* flow=gpu ? static_cast<Pixel*>(gpu->buffers[(flags&DENSE_FLOW)?4:2].mapped) : cpuFlows.data();
            Pixel center{}; bool flowComputed=needsFlow(flags);
            if(flowComputed) center=flow[(flags&DENSE_FLOW)?(H/2)*W+W/2:(H/16)*((W+7)/8)+W/16];
            float flowTolerance=sceneName=="fractional" ? .65f : .02f;
            bool exactFlow=std::abs(center.r-c.dx)<flowTolerance && std::abs(center.g-c.dy)<flowTolerance &&
                           std::abs(center.b+c.dx)<flowTolerance && std::abs(center.a+c.dy)<flowTolerance;
            bool hudExact=true;
            if(hud) for(int y=32;y<56;y++) for(int x=32;x<224;x++) {
                const auto p=generated[y*W+x],t=truth[y*W+x];
                hudExact &= p.r==t.r && p.g==t.g && p.b==t.b;
            }
            int synthesis=options.flags(decision.mode)&15; bool ok=true;
            if(synthesis==REPEAT) ok=mae(generated,a,0)<1e-7;
            else if(synthesis==BLEND) ok=mae(generated,blend,0)<1e-7;
            else if(synthesis==PASSTHROUGH) ok=mae(generated,b,0)<1e-7 && (!flowComputed || external || exactFlow);
            else if(!external && options.artifacts) {
                double threshold=sceneName=="fractional" ? .05 : .005;
                ok=exactFlow && (hud ? hudExact : error<threshold && (baseline<1e-6 || error<baseline*(sceneName=="fractional"?.8:.25)));
            }
            passed&=ok;
            std::cout<<ui("result.line",{{"scene",c.name},{"status",ui(ok?"status.pass":"status.fail")},{"mode",decision.mode},
                {"reason",decision.reason},{"mae",external?ui("value.unknown"):std::to_string(error)},{"blend",std::to_string(baseline)},
                {"ms",std::to_string(ms)},{"device",gpu?gpu->deviceName:ui("device.cpu")}})<<'\n';
            report<<"    {\"name\":\""<<c.name<<"\",\"passed\":"<<(ok?"true":"false")
                  <<",\"backend\":\""<<(gpu?"vulkan":"cpu")<<"\",\"software_device\":"<<(!gpu||gpu->software?"true":"false")<<",\"mae_interior\":"<<(external?"null":std::to_string(error))
                  <<",\"mae_full\":"<<(external?"null":std::to_string(fullError))<<",\"blend_mae_interior\":"<<(external?"null":std::to_string(baseline))<<",\"processing_wall_ms\":"<<ms
                  <<",\"compute_wall_ms\":"<<computeMs<<",\"gpu_timestamp_ms\":"<<(gpu && gpu->gpuTimestampMs>=0?std::to_string(gpu->gpuTimestampMs):"null")
                  <<",\"selected_mode\":\""<<decision.mode<<"\",\"decision_reason\":\""<<decision.reason<<"\",\"budget_ms\":"<<decision.budgetMs
                  <<",\"flow_computed\":"<<(flowComputed?"true":"false")<<",\"flow_center\":["<<center.r<<','<<center.g<<','<<center.b<<','<<center.a<<"]"
                  <<",\"fallback_proxy_fraction\":"<<fallbackFraction
                  <<",\"validation_kind\":\""<<(external?"finite_pixels_and_mode_operation":synthesis!=0?"mode_operation_and_optional_flow":!options.artifacts?"finite_pixels_only":"analytic_flow_and_image")<<"\""
                  <<",\"reference_kind\":\""<<(external?"next_input_not_ground_truth":"analytic_midpoint_or_cut_policy")<<"\"}"<<(i+1<cases.size()?",":"")<<"\n";
            if(flowComputed && (flags&ANALYSIS_ONLY)) flowCSV(out/(std::string(c.name)+".flow.csv"),flow,(flags&DENSE_FLOW)!=0);
            ppm(out/(std::string(c.name)+".generated.ppm"),generated);
            if(i==0) { ppm(out/"previous.ppm",a); ppm(out/"next.ppm",b); ppm(out/"generated.ppm",generated); ppm(out/"reference.ppm",truth); ppm(out/"blend.ppm",blend); }
        }
        report<<"  ],\n\"offline_pairs_generated\":"<<generatedPairs<<",\"offline_pairs_repeated\":"<<repeatedPairs<<",\"passed\":"<<(passed?"true":"false")<<"\n}\n";
        report.close(); if(!report) throw std::runtime_error("errors.failed_writing_metrics");
        return passed ? 0 : 2;
    } catch(const std::exception& e) {
        auto detail=uiError(e.what());
        std::cerr<<ui("error.prefix")<<": "<<detail<<'\n';return 1;
    }
}
int main(int argc,char** argv) {
    try {
        auto arguments=unicodeArguments(argc,argv);
        auto localized=localeArguments(arguments);
        Localizer locale(localized,executableDirectory(arguments.at(0)));
        activeLocalizer=&locale;
        if(localized.menu) {locale.menu();return 0;}
        if(localized.show) {std::cout<<locale.summary().dump(2)<<'\n';return 0;}
        if(localized.list) {std::cout<<locale.available().dump(2)<<'\n';return 0;}
        std::vector<char*> pointers;
        for(auto& arg:localized.remaining) pointers.push_back(arg.data());
        return runBackend(int(pointers.size()),pointers.data());
    } catch(const std::exception& e) {
        // English embedded fallback is available even if settings cannot initialize.
        activeLocalizer=nullptr;
        std::cerr<<ui("error.prefix")<<": "<<uiError(e.what())<<'\n';return 1;
    }
}
