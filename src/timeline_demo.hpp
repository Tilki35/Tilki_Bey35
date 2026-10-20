#pragma once
#include "pacing.hpp"
#include "adaptive.hpp"
#include <fstream>
#include <filesystem>

inline void timelineDemo(const Options& options,bool pacing,double processingMs,const std::filesystem::path& out) {
    validTime(processingMs);
    FramePacer pacer(options.sourceFps,options.displayHz);
    Telemetry telemetry; AdaptiveController adaptive;
    std::vector<ScheduledFrame> events;
    double period=1000./options.sourceFps;
    for(size_t i=0;i<=20;i++) {
        double source=i*period;
        telemetry.source(source); events.push_back(pacer.real(i,source));
        if(i==0) continue;
        std::string selected=options.mode;
        if(selected=="adaptive") selected=adaptive.decide(Features{.1,1,false,false},options).mode;
        bool repeated=selected=="repeat";
        bool synthesized=selected=="interpolate" || selected=="blend";
        if(!synthesized && !repeated) continue;
        double ready=synthesized ? source+processingMs : source-period;
        if(synthesized) { telemetry.generationComplete(ready); adaptive.observe(processingMs); }
        auto event=pacer.between(i-1,source-period,source,ready,repeated);
        if(pacing && event) events.push_back(*event);
        else telemetry.dropped++;
    }
    std::sort(events.begin(),events.end(),[](const auto& a,const auto& b) { return a.dueMs<b.dueMs; });
    for(const auto& event:events) telemetry.submitted(event.dueMs,event.kind=="repeat");
    auto intervals=telemetry.intervals();
    std::ofstream report(out/"timeline.json");
    report<<"{\n\"timing_basis\":\"simulated\",\"presentation_backend\":\"none\","
          <<"\"real_fps\":null,\"displayed_fps\":null,\"input_latency_ms\":null,"
          <<"\"simulated_source_fps\":"<<eventRate(telemetry.sources)
          <<",\"simulated_generation_completion_fps\":"<<eventRate(telemetry.generated)
          <<",\"simulated_submission_fps\":"<<eventRate(telemetry.submissions)
          <<",\"simulated_generated_count\":"<<telemetry.generated.size()
          <<",\"simulated_repeat_count\":"<<telemetry.repeated<<",\"simulated_dropped_count\":"<<telemetry.dropped
          <<",\"simulated_processing_ms\":"<<processingMs<<",\"interpolation_buffer_ms\":"<<period
          <<",\"simulated_interval_p50_ms\":"<<percentile(intervals,.5)
          <<",\"simulated_interval_p95_ms\":"<<percentile(intervals,.95)
          <<",\"simulated_interval_p99_ms\":"<<percentile(intervals,.99)<<",\"events\":[\n";
    for(size_t i=0;i<events.size();i++) {
        const auto& e=events[i];
        report<<"{\"due_ms\":"<<e.dueMs<<",\"kind\":\""<<e.kind<<"\",\"source_id\":"<<e.sourceId<<"}"<<(i+1<events.size()?",":"")<<"\n";
    }
    report<<"]}\n";
    if(!report) throw std::runtime_error("errors.cannot_write_timeline_report");
}
