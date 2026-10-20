#pragma once
#include "types.hpp"
#include "options.hpp"
#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>
// Portable scalar reference/fallback. No GPU, Vulkan loader, or vendor SDK needed.
// Intentionally slow: it demonstrates correctness, not real-time CPU frame generation.
double runCPU(const std::vector<Pixel>& a,const std::vector<Pixel>& b,
              std::vector<Pixel>& output,std::vector<Pixel>& flows,int flags=ARTIFACT_REDUCTION,float alpha=.5f,int radius=12,bool reuseFlow=false) {
    const auto start=std::chrono::steady_clock::now();
    constexpr int gw=(W+7)/8,gh=(H+7)/8;
    if(reuseFlow && flows.size()!=size_t((flags&DENSE_FLOW)?W*H:gw*gh))throw std::runtime_error("errors.stream_protocol");
    if(!reuseFlow)flows.resize(gw*gh);
    output.resize(W*H);
    int synthesis=flags&15;
    if(!needsFlow(flags)) {
        for(size_t i=0;i<a.size();i++) {
            output[i]=synthesis==REPEAT ? a[i] : synthesis==BLEND ?
                Pixel{a[i].r*(1-alpha)+b[i].r*alpha,a[i].g*(1-alpha)+b[i].g*alpha,a[i].b*(1-alpha)+b[i].b*alpha,1} : b[i];
        }
        return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    }
    auto texel=[&](int x,int y,bool next) {
        return (next?b:a)[std::clamp(y,0,H-1)*W+std::clamp(x,0,W-1)];
    };
    auto lum=[](Pixel p) { return p.r*.2126f+p.g*.7152f+p.b*.0722f; };
    for(int by=0;!reuseFlow && by<gh;by++) for(int bx=0;bx<gw;bx++) {
        std::array<float,4> f{};
        for(int direction=0;direction<2;direction++) {
            float best=1e30f;
            for(int dy=-radius;dy<=radius;dy++) for(int dx=-radius;dx<=radius;dx++) {
                float error=0;
                for(int y=0;y<8;y+=2) for(int x=0;x<8;x+=2) {
                    float diff=lum(texel(bx*8+x,by*8+y,direction!=0))-
                               lum(texel(bx*8+x+dx,by*8+y+dy,direction==0));
                    error+=diff*diff;
                }
                float previous=f[direction*2]*f[direction*2]+f[direction*2+1]*f[direction*2+1];
                if(error<best || (error==best && dx*dx+dy*dy<previous)) {
                    best=error; f[direction*2]=float(dx); f[direction*2+1]=float(dy);
                }
            }
        }
        flows[by*gw+bx]={f[0],f[1],f[2],f[3]};
    }
    auto mix=[](Pixel p,Pixel q,float t) { return Pixel{p.r*(1-t)+q.r*t,p.g*(1-t)+q.g*t,p.b*(1-t)+q.b*t,1}; };
    auto sample=[&](float x,float y,bool next) {
        int ix=int(std::floor(x)),iy=int(std::floor(y)); float tx=x-ix,ty=y-iy;
        return mix(mix(texel(ix,iy,next),texel(ix+1,iy,next),tx),
                   mix(texel(ix,iy+1,next),texel(ix+1,iy+1,next),tx),ty);
    };
    if((flags&DENSE_FLOW) && !reuseFlow) {
        std::vector<Pixel> dense(W*H);
        for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
            auto seed=flows[(y/8)*gw+x/8];
            std::array<float,4> f{seed.r,seed.g,seed.b,seed.a};
            for(int direction=0;direction<2;direction++) {
                float& mx=f[direction*2]; float& my=f[direction*2+1];
                for(int iteration=0;iteration<3;iteration++) {
                    float xx=0,xy=0,yy=0,rx=0,ry=0;
                    for(int oy=-2;oy<=2;oy++) for(int ox=-2;ox<=2;ox++) {
                        float tx=x+ox+mx,ty=y+oy+my; bool target=direction==0;
                        float residual=lum(texel(x+ox,y+oy,!target))-lum(sample(tx,ty,target));
                        float gx=.5f*(lum(sample(tx+1,ty,target))-lum(sample(tx-1,ty,target)));
                        float gy=.5f*(lum(sample(tx,ty+1,target))-lum(sample(tx,ty-1,target)));
                        xx+=gx*gx; xy+=gx*gy; yy+=gy*gy; rx+=gx*residual; ry+=gy*residual;
                    }
                    float determinant=xx*yy-xy*xy;
                    if(determinant<1e-6f) break;
                    mx=std::clamp(mx+std::clamp((yy*rx-xy*ry)/determinant,-1.f,1.f),-float(radius),float(radius));
                    my=std::clamp(my+std::clamp((xx*ry-xy*rx)/determinant,-1.f,1.f),-float(radius),float(radius));
                }
            }
            dense[y*W+x]={f[0],f[1],f[2],f[3]};
        }
        flows=std::move(dense);
    }
    auto field=[&](float x,float y) {
        if(flags&DENSE_FLOW) return flows[std::clamp(int(std::floor(y)),0,H-1)*W+std::clamp(int(std::floor(x)),0,W-1)];
        return flows[std::clamp(int(std::floor(y/8)),0,gh-1)*gw+std::clamp(int(std::floor(x/8)),0,gw-1)];
    };
    bool protect=(flags&ARTIFACT_REDUCTION)!=0;
    for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
        if(synthesis==PASSTHROUGH) { output[y*W+x]=b[y*W+x]; continue; }
        auto old=a[y*W+x],next=b[y*W+x];
        if(protect && old.r==next.r && old.g==next.g && old.b==next.b && old.a==next.a) {
            output[y*W+x]=next; continue;
        }
        float ax=float(x),ay=float(y),bx=float(x),by=float(y);
        for(int i=0;i<2;i++) {
            auto f=field(ax,ay),r=field(bx,by);
            ax=x-alpha*f.r; ay=y-alpha*f.g; bx=x-(1-alpha)*r.b; by=y-(1-alpha)*r.a;
        }
        auto f=field(ax,ay),r=field(bx,by),ca=sample(ax,ay,false),cb=sample(bx,by,true);
        float disagreement=std::hypot(f.r+r.b,f.g+r.a);
        float delta=std::max({std::abs(ca.r-cb.r),std::abs(ca.g-cb.g),std::abs(ca.b-cb.b)});
        bool outside=ax<0||ay<0||bx<0||by<0||ax>W-1||bx>W-1||ay>H-1||by>H-1;
        output[y*W+x]=(protect && (outside||disagreement>2||delta>1.f/255.f)) ? texel(x,y,true) : mix(ca,cb,alpha);
    }
    return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
}
