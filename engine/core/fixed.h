#pragma once
// Q16.16 定点数。Sim 层只用它，禁止 float 运算。
// FromFloat / ToFloat 仅工具、初始化、渲染/调试使用。
#include "core/types.h"
#include <compare>
#include <cstdint>

namespace eng {

struct Fixed {
    i32 raw = 0;

    // ---------- 构造 ----------
    static constexpr Fixed FromInt(i32 v)  { return Fixed{v << 16}; }
    static constexpr Fixed FromRaw(i32 r)  { return Fixed{r}; }

    // 仅工具用：编辑器 / 初始化 / 调试。Sim 逻辑里不要调用。
    static Fixed FromFloat(float f) {
        return Fixed{(i32)(f * 65536.0f + (f >= 0.f ? 0.5f : -0.5f))};
    }

    // ---------- 一元 ----------
    constexpr Fixed operator+() const { return *this; }
    constexpr Fixed operator-() const { return Fixed{-raw}; }

    // ---------- 二元算术 ----------
    constexpr Fixed operator+(Fixed o) const { return Fixed{raw + o.raw}; }
    constexpr Fixed operator-(Fixed o) const { return Fixed{raw - o.raw}; }

    // 乘法：中间用 i64 防溢出
    constexpr Fixed operator*(Fixed o) const {
        return Fixed{(i32)(((i64)raw * o.raw) >> 16)};
    }

    // 除法：先左移再除，保持精度。除零返回 0。
    constexpr Fixed operator/(Fixed o) const {
        if (o.raw == 0) return Fixed{0};
        return Fixed{(i32)(((i64)raw << 16) / o.raw)};
    }

    // 整数缩放（无精度损失，常用）
    constexpr Fixed operator*(i32 s) const { return Fixed{raw * s}; }
    constexpr Fixed operator/(i32 s) const { return s ? Fixed{raw / s} : Fixed{0}; }
    friend constexpr Fixed operator*(i32 s, Fixed f) { return f * s; }

    // ---------- 复合赋值 ----------
    constexpr Fixed& operator+=(Fixed o) { raw += o.raw; return *this; }
    constexpr Fixed& operator-=(Fixed o) { raw -= o.raw; return *this; }
    constexpr Fixed& operator*=(Fixed o) { *this = *this * o; return *this; }
    constexpr Fixed& operator/=(Fixed o) { *this = *this / o; return *this; }
    constexpr Fixed& operator*=(i32 s)   { raw *= s; return *this; }
    constexpr Fixed& operator/=(i32 s)   { if (s) raw /= s; return *this; }

    // ---------- 比较 ----------
    auto operator<=>(const Fixed&) const = default;
    constexpr bool operator==(const Fixed&) const = default;

    // ---------- 转换 ----------
    constexpr i32 ToInt() const { return raw >> 16; }   // 向 0 截断
    float ToFloat() const { return raw / 65536.0f; }    // 仅渲染/调试

    // ---------- 常量 ----------
    static constexpr Fixed Zero() { return Fixed{0}; }
    static constexpr Fixed One()  { return FromInt(1); }
    static constexpr Fixed Half() { return Fixed{1 << 15}; } // 0.5
};

// ---------- 自由函数 ----------
constexpr Fixed Clamp(Fixed v, Fixed lo, Fixed hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
constexpr Fixed Abs(Fixed v) { return v.raw < 0 ? Fixed{-v.raw} : v; }
constexpr Fixed Min(Fixed a, Fixed b) { return a < b ? a : b; }
constexpr Fixed Max(Fixed a, Fixed b) { return a > b ? a : b; }

// 可选但推荐：2D 向量（位置 / 速度直接用）
struct FixedVec2 {
    Fixed x{}, y{};

    constexpr FixedVec2 operator+(FixedVec2 o) const { return {x + o.x, y + o.y}; }
    constexpr FixedVec2 operator-(FixedVec2 o) const { return {x - o.x, y - o.y}; }
    constexpr FixedVec2 operator*(Fixed s) const { return {x * s, y * s}; }
    constexpr FixedVec2 operator/(Fixed s) const { return {x / s, y / s}; }
    constexpr FixedVec2 operator*(i32 s) const { return {x * s, y * s}; }

    constexpr FixedVec2& operator+=(FixedVec2 o) { x += o.x; y += o.y; return *this; }
    constexpr FixedVec2& operator-=(FixedVec2 o) { x -= o.x; y -= o.y; return *this; }
    constexpr FixedVec2& operator*=(Fixed s) { x *= s; y *= s; return *this; }

    auto operator<=>(const FixedVec2&) const = default;
    constexpr bool operator==(const FixedVec2&) const = default;
};

} // namespace eng