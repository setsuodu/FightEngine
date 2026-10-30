#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "core/types.h"
namespace eng {
using Microsoft::WRL::ComPtr;
constexpr u32 kFrames = 3;   // in-flight 帧数 / 交换链缓冲数

// 最小 DX12 封装: 设备 / 队列 / 交换链 / 围栏 / 描述符堆 / 清屏。
// 后续由别的 AI 在此之上补: 上传环形缓冲、PSO 缓存、资源状态跟踪、DXC 着色器等。
class Dx12 {
public:
    bool Init(HWND hwnd, u32 w, u32 h);
    void Shutdown();
    void Resize(u32 w, u32 h);
    ID3D12GraphicsCommandList* BeginFrame(const float clear[4]);  // 已设置 RT/视口/清屏
    void EndFrame(bool vsync);
    void WaitIdle();

    bool AllocSrv(D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu); // 简易线性分配
    ID3D12Device* Device() const { return device_.Get(); }
    ID3D12CommandQueue* Queue() const { return queue_.Get(); }
    ID3D12DescriptorHeap* SrvHeap() const { return srvHeap_.Get(); }
    static constexpr DXGI_FORMAT BackbufferFormat() { return DXGI_FORMAT_R8G8B8A8_UNORM; }
    u32 Width() const { return w_; }
    u32 Height() const { return h_; }
private:
    void CreateRenderTargets();
    void WaitFence(u64 v);
    ComPtr<IDXGIFactory6> factory_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<IDXGISwapChain3> swap_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_, srvHeap_;
    ComPtr<ID3D12Resource> rt_[kFrames];
    ComPtr<ID3D12CommandAllocator> alloc_[kFrames];
    ComPtr<ID3D12GraphicsCommandList> list_;
    ComPtr<ID3D12Fence> fence_;
    HANDLE fenceEvent_ = nullptr;
    u64 fenceCounter_ = 0, fenceVals_[kFrames] = {};
    u32 frameIdx_ = 0, rtvInc_ = 0, srvInc_ = 0, srvUsed_ = 0, w_ = 0, h_ = 0;
    static constexpr u32 kSrvCount = 64;
};
}
