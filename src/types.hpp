#pragma once
struct Pixel { float r,g,b,a; };
static_assert(sizeof(Pixel)==16);
// One fixed geometry per native worker process. Offline fixtures retain 256x144.
inline int W=256,H=144;
