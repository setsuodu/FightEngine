#pragma once
// 16.16 定点数占位实现。Sim 层只用它，不用 float。后续由别的 AI 补全 (除法/三角/sqrt 等)。
#include "core/types.h"
namespace eng {
struct Fixed {
    i32 raw = 0;
    static constexpr Fixed FromInt(i32 v) { return Fixed{v << 16}; }
    static constexpr Fixed FromRaw(i32 r) { return Fixed{r}; }
    constexpr Fixed operator+(Fixed o) const { return Fixed{raw + o.raw}; }
    constexpr Fixed operator-(Fixed o) const { return Fixed{raw - o.raw}; }
    constexpr Fixed operator*(Fixed o) const { return Fixed{(i32)(((i64)raw * o.raw) >> 16)}; }
    constexpr i32 ToInt() const { return raw >> 16; }
    float ToFloat() const { return raw / 65536.0f; }   // 仅供渲染/调试
    auto operator<=>(const Fixed&) const = default;
};
constexpr Fixed Clamp(Fixed v, Fixed lo, Fixed hi) { return v < lo ? lo : (v > hi ? hi : v); }
}
