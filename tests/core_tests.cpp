#include "adaptive.hpp"
#include "pacing.hpp"
#include <iostream>
#include <limits>
void expect(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
template<class F> void rejects(F f) {
    bool rejected=false; try { f(); } catch(const std::exception&) { rejected=true; }
    expect(rejected,"Expected input rejection");
}
int main() {
    try {
        Options options;
        AdaptiveController controller;
        Features moving{.1,1,false,false},still{0,0,false,true},cut{1,1,true,false};
        expect(controller.decide(moving,options).mode=="interpolate","Generate moving frames");
        expect(controller.decide(still,options).mode=="repeat","Static frames repeat");
        expect(controller.decide(cut,options).mode=="passthrough","Do not interpolate cuts");
        controller.observe(9);
        expect(controller.decide(moving,options).reason=="processing_budget","Over-budget fallback");
        expect(controller.decide(moving,options).mode=="repeat","Cooldown");
        expect(controller.decide(moving,options).reason=="recovery_probe","Recover instead of permanent disabling");
        for(int i=0;i<8;i++) controller.observe(1);
        expect(controller.decide(moving,options).mode=="interpolate","Low-cost recovery");
        options.displayHz=144;
        expect(controller.decide(moving,options).reason=="refresh_limit","Display cap");
        rejects([&]{controller.observe(std::numeric_limits<double>::quiet_NaN());});
        std::vector<Pixel> a(16,Pixel{0,0,0,1}),b(16,Pixel{1,1,1,1});
        expect(analyzePair(a,b).cut,"Detect large luminance cut");
        expect(analyzePair(a,a).unchanged,"Detect exact repeat");
        rejects([&]{analyzePair(a,{});});

        FramePacer pacer(100,200); Telemetry telemetry;
        std::vector<ScheduledFrame> events;
        for(size_t i=0;i<=20;i++) {
            telemetry.source(i*10); events.push_back(pacer.real(i,i*10));
            if(i) {
                auto generated=pacer.between(i-1,(i-1)*10,i*10,i*10+1);
                expect(generated.has_value(),"On-time generated frame");
                events.push_back(*generated); telemetry.generationComplete(i*10+1);
            }
        }
        std::sort(events.begin(),events.end(),[](auto a,auto b){return a.dueMs<b.dueMs;});
        for(auto e:events) telemetry.submitted(e.dueMs);
        expect(eventRate(telemetry.sources)==100,"Source FPS must be distinct");
        expect(eventRate(telemetry.generated)==100,"Only new completed images count");
        expect(eventRate(telemetry.submissions)==200,"Ideal simulation has 200 submissions/s");
        expect(percentile(telemetry.intervals(),.99)==5,"Uniform 5ms model intervals");
        expect(!pacer.between(0,0,10,16),"Drop late synthetic frame");
        expect(pacer.real(1,10).dueMs==20,"Late work cannot move real deadline");
        expect(!FramePacer(100,144).between(0,0,10,11),"Respect display cap");
        expect(!pacer.between(0,0,2,2),"Respect variable source interval");
        rejects([&]{pacer.between(0,10,0,11);});
        rejects([&]{FramePacer invalid(0,200);});
        Telemetry repetitions; repetitions.source(0); repetitions.source(10);
        repetitions.submitted(10); repetitions.submitted(15,true);
        expect(repetitions.generated.empty() && repetitions.repeated==1,"Repetition is not generation");
        expect(repetitions.latencies.empty(),"Never invent input latency");
        repetitions.submitted(20,false,7);
        expect(repetitions.latencies[0]==13,"Input-to-submit latency needs a matched input timestamp");
        rejects([&]{repetitions.submitted(21,false,22);});
        rejects([&]{repetitions.source(10);});
        bool called=false;
        auto old=std::chrono::steady_clock::now()-std::chrono::milliseconds(100);
        expect(!FramePacer::dispatch({0,"generated",0},old,1,[&](auto){called=true;}),"Expired generated job dropped");
        expect(!called,"Do not dispatch expired work");
        expect(FramePacer::dispatch({0,"real",0},old,1,[&](auto){called=true;}),"Real job still submitted");
        expect(called,"Real callback executed");
        std::cout<<"PASS: adaptive selection/recovery, deadline pacing, counters, latency provenance, clock adapter, invalid inputs.\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
