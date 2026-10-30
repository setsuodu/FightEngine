#pragma once
#include "interfaces/IRenderer.h"
#include "rhi/dx12.h"
#include <wrl/client.h>

namespace eng {

// 最小 DX12 场景渲染：离屏 color RT + 两个纯色四边形（读 Demo GameState）。
class Renderer final : public IRenderer {
public:
    explicit Renderer(Dx12& dx);
    ~Renderer() override;

    void Resize(u32 w, u32 h) override;
    void Draw(ID3D12GraphicsCommandList* list, const RenderView& view) override;

    D3D12_GPU_DESCRIPTOR_HANDLE ColorSrv() const override { return srvGpu_; }
    bool HasColorTarget() const override { return color_ != nullptr; }
    u32  Width() const override { return w_; }
    u32  Height() const override { return h_; }

private:
    bool CreatePipeline();
    void DestroyTarget();
    bool CreateTarget(u32 w, u32 h);

    Dx12& dx_;
    Microsoft::WRL::ComPtr<ID3D12Resource> color_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_; // 独立 1 个 RTV，不碰 swapchain 堆
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_{};
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpu_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpu_{};
    bool srvAllocated_ = false;
    u32 w_ = 0, h_ = 0;
    D3D12_RESOURCE_STATES colorState_ = D3D12_RESOURCE_STATE_COMMON;
};

} // namespace eng
