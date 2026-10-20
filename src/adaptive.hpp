#pragma once
#include "types.hpp"
#include "options.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

struct Features { double meanDifference=0, changedFraction=0; bool cut=false, unchanged=false; };
inline Features analyzePair(const std::vector<Pixel>& a,const std::vector<Pixel>& b) {
    if(a.empty() || a.size()!=b.size()) throw std::invalid_argument("errors.frame_sizes_must_match_and_be_nonempty");
    Features f; size_t changed=0; double luminanceShift=0;
    for(size_t i=0;i<a.size();i++) {
        auto p=a[i],q=b[i];
        for(float value:{p.r,p.g,p.b,p.a,q.r,q.g,q.b,q.a}) if(!std::isfinite(value)) throw std::invalid_argument("errors.non_finite_frame_pixel");
        double d=std::max({std::abs(p.r-q.r),std::abs(p.g-q.g),std::abs(p.b-q.b)});
        if(!std::isfinite(d)) throw std::invalid_argument("errors.non_finite_frame_pixel");
        f.meanDifference+=d; changed+=d>1./255.;
        luminanceShift+=(q.r-p.r)*.2126+(q.g-p.g)*.7152+(q.b-p.b)*.0722;
    }
    f.meanDifference/=a.size(); f.changedFraction=double(changed)/a.size();
    f.unchanged=f.meanDifference==0;
    // Conservative cut heuristic, not a general shot-boundary detector.
    f.cut=f.changedFraction>.9 && std::abs(luminanceShift/a.size())>.35;
    return f;
}
struct Decision { std::string mode,reason; double budgetMs; };
class AdaptiveController {
    double costMs_=-1; int cooldown_=0;
public:
    Decision decide(const Features& f,const Options& opts) {
        const double budget=500./opts.sourceFps;
        if(f.cut) { cooldown_=0; return {"passthrough","scene_cut",budget}; }
        if(opts.displayHz+1e-6<2*opts.sourceFps) return {"passthrough","refresh_limit",budget};
        if(f.unchanged) return {"repeat","unchanged_frame",budget};
        if(costMs_>budget*.85) {
            if(cooldown_++<2) return {"repeat","processing_budget",budget};
            cooldown_=0;
            return {"interpolate","recovery_probe",budget};
        }
        cooldown_=0;
        return {"interpolate","motion_generation",budget};
    }
    void observe(double ms) {
        if(!std::isfinite(ms) || ms<0) throw std::invalid_argument("errors.invalid_processing_cost");
        // Conservative smoothed feedback; a long first-use/JIT cost can trigger cooldown.
        costMs_=costMs_<0 ? ms : .5*costMs_+.5*ms;
    }
};
