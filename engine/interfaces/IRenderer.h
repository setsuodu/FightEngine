#pragma once
#include <d3d12.h>
#include "core/types.h"

namespace eng {

// 一帧要画进 Viewport 离屏 RT 的输入。表现层只读，不回写 Sim。
struct RenderView {
    const void* state = nullptr;     // 当前逻辑状态（StateArena）
    const void* prevState = nullptr; // 上一逻辑帧（可空 = 不插值）
    u64         stateSize = 0;
    float       alpha = 0.f;         // 主循环固定步长插值系数 [0,1)
    u32         width = 0;           // Viewport 内容区像素
    u32         height = 0;
};

// 实现放 engine/render/。第一刀：离屏 RT + 纯色矩形（Demo Fighter）。
// 后续：网格/材质/蒙皮/描边/后处理/2D batch。
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // 按 Viewport 尺寸建/重建 color RT。w/h 为 0 时忽略。
    virtual void Resize(u32 w, u32 h) = 0;

    // 清屏并画场景到离屏 RT。须在 Dx12::BeginFrame 之后、ImGui 提交之前，共用同一 command list。
    virtual void Draw(ID3D12GraphicsCommandList* list, const RenderView& view) = 0;

    // 给 ImGui::Image 用的 SRV（GPU handle 的 ptr 可直接当 ImTextureID）
    virtual D3D12_GPU_DESCRIPTOR_HANDLE ColorSrv() const = 0;
    virtual bool HasColorTarget() const = 0;
    virtual u32  Width() const = 0;
    virtual u32  Height() const = 0;
};

} // namespace eng
