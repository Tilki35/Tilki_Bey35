#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <chrono>
#include <thread>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

inline void validTime(double t) { if(!std::isfinite(t) || t<0) throw std::invalid_argument("errors.invalid_timestamp"); }
inline double eventRate(const std::vector<double>& times) {
    return times.size()>1 && times.back()>times.front() ? (times.size()-1)*1000./(times.back()-times.front()) : 0;
}
inline double percentile(std::vector<double> values,double p) {
    if(values.empty()) return 0;
    std::sort(values.begin(),values.end());
    return values[size_t(std::ceil((values.size()-1)*p))];
}
struct Telemetry {
    std::vector<double> sources,generated,submissions,latencies;
    size_t repeated=0, dropped=0;
    static void append(std::vector<double>& dst,double t) {
        validTime(t);
        if(!dst.empty() && t<=dst.back()) throw std::invalid_argument("errors.timestamps_must_increase");
        dst.push_back(t);
    }
    void source(double t) { append(sources,t); }
    void generationComplete(double t) { append(generated,t); }
    void submitted(double t,bool repeat=false,std::optional<double> inputEvent=std::nullopt) {
        if(inputEvent) { validTime(*inputEvent); if(*inputEvent>t) throw std::invalid_argument("errors.input_event_after_submission"); }
        append(submissions,t); repeated+=repeat;
        if(inputEvent) latencies.push_back(t-*inputEvent);
    }
    std::vector<double> intervals() const {
        std::vector<double> result;
        for(size_t i=1;i<submissions.size();i++) result.push_back(submissions[i]-submissions[i-1]);
        return result;
    }
};
struct ScheduledFrame { double dueMs; std::string kind; size_t sourceId; };
class FramePacer {
    double sourcePeriod_,displayPeriod_;
public:
    FramePacer(double sourceFps,double displayHz) {
        if(!std::isfinite(sourceFps)||!std::isfinite(displayHz)||sourceFps<=0||displayHz<=0)
            throw std::invalid_argument("errors.rates_must_be_finite_and_positive");
        sourcePeriod_=1000./sourceFps; displayPeriod_=1000./displayHz;
    }
    ScheduledFrame real(size_t id,double captureMs) const {
        validTime(captureMs);
        // One source-period interpolation buffer; never extended by late generated work.
        return {captureMs+sourcePeriod_,"real",id};
    }
    std::optional<ScheduledFrame> between(size_t id,double a,double b,double readyMs,bool repeated=false) const {
        validTime(a); validTime(b); validTime(readyMs);
        if(b<=a || readyMs<a) throw std::invalid_argument("errors.invalid_frame_pair_timeline");
        double half=(b-a)*.5,deadline=(a+b)*.5+sourcePeriod_;
        // Respect actual adjacent real-frame spacing and the configured display cap.
        if(half+1e-6<displayPeriod_ || readyMs>deadline) return std::nullopt;
        return ScheduledFrame{deadline,repeated?"repeat":"generated",id};
    }
    // Monotonic clock adapter for a future presentation callback. This does not
    // present to a screen itself. Drops an expired synthetic job rather than
    // queueing it in front of real frames; real-frame deadlines stay absolute.
    static bool dispatch(const ScheduledFrame& frame,std::chrono::steady_clock::time_point epoch,
                         double maxSyntheticLatenessMs,const std::function<void(const ScheduledFrame&)>& submit) {
        validTime(frame.dueMs); validTime(maxSyntheticLatenessMs);
        auto due=epoch+std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double,std::milli>(frame.dueMs));
        std::this_thread::sleep_until(due);
        double late=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-due).count();
        if(frame.kind!="real" && late>maxSyntheticLatenessMs) return false;
        submit(frame); return true;
    }
};
