#pragma once
#include "core/types.h"
namespace eng {
// 约定: 姿势 = f(clipId, frame)，按帧号直接采样，不累加 dt。
class IAnimation { public: virtual ~IAnimation() = default; virtual void Sample(u32 clipId, u32 frame, void* outPose) = 0; };
}
