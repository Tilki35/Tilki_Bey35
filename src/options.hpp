#pragma once
#include <string>
#include <stdexcept>

// Shared with the GLSL push-constant flags. Synthesis and analysis are orthogonal.
constexpr int REPEAT=1, BLEND=2, PASSTHROUGH=3;
constexpr int ARTIFACT_REDUCTION=256, DENSE_FLOW=512, ANALYSIS_ONLY=1024;
struct Options {
    std::string mode="interpolate", estimator="block";
    bool artifacts=true;
    double sourceFps=100, displayHz=200;
    int flags(const std::string& selected) const {
        int f=artifacts ? ARTIFACT_REDUCTION : 0;
        if(estimator=="optical-flow" || selected=="optical-flow") f|=DENSE_FLOW;
        if(selected=="repeat") f|=REPEAT;
        else if(selected=="blend") f|=BLEND;
        else if(selected=="motion" || selected=="optical-flow") f|=PASSTHROUGH|ANALYSIS_ONLY;
        else if(selected=="passthrough") f|=PASSTHROUGH;
        return f;
    }
};
inline bool needsFlow(int flags) { return (flags&15)==0 || (flags&ANALYSIS_ONLY)!=0; }
