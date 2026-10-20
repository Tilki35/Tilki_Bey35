#pragma once
struct Pixel { float r,g,b,a; };
static_assert(sizeof(Pixel)==16);
constexpr int W=256,H=144;
