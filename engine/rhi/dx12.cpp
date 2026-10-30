#include "rhi/dx12.h"
#include "core/log.h"
#include "core/assert.h"
namespace eng {

bool Dx12::Init(HWND hwnd, u32 w, u32 h) {
    w_ = w; h_ = h;
    UINT dxgiFlags = 0;
#ifndef NDEBUG
    { ComPtr<ID3D12Debug> dbg;
      if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) { dbg->EnableDebugLayer(); dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG; } }
#endif
    if (FAILED(CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&factory_)))) { LogError("CreateDXGIFactory2 failed"); return false; }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; SUCCEEDED(factory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))); ++i) {
        DXGI_ADAPTER_DESC1 d; adapter->GetDesc1(&d);
        if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { adapter.Reset(); continue; }
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) break;
        adapter.Reset();
    }
    if (!adapter) { LogError("No DX12 adapter found"); return false; }
    { DXGI_ADAPTER_DESC1 d; adapter->GetDesc1(&d); LogInfo("GPU: %ls", d.Description); }
    if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)))) { LogError("D3D12CreateDevice failed"); return false; }

    D3D12_COMMAND_QUEUE_DESC qd = {}; qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ENG_HR(device_->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue_)));

    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = w; sd.Height = h; sd.Format = BackbufferFormat(); sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.BufferCount = kFrames;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    ComPtr<IDXGISwapChain1> sc1;
    ENG_HR(factory_->CreateSwapChainForHwnd(queue_.Get(), hwnd, &sd, nullptr, nullptr, &sc1));
    ENG_HR(sc1.As(&swap_));
    factory_->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    D3D12_DESCRIPTOR_HEAP_DESC hd = {};
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; hd.NumDescriptors = kFrames;
    ENG_HR(device_->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtvHeap_)));
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; hd.NumDescriptors = kSrvCount; hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ENG_HR(device_->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&srvHeap_)));
    rtvInc_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    srvInc_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (u32 i = 0; i < kFrames; ++i) ENG_HR(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc_[i])));
    ENG_HR(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc_[0].Get(), nullptr, IID_PPV_ARGS(&list_)));
    list_->Close();
    ENG_HR(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
    fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CreateRenderTargets();
    return true;
}

void Dx12::CreateRenderTargets() {
    D3D12_CPU_DESCRIPTOR_HANDLE h = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    for (u32 i = 0; i < kFrames; ++i) {
        ENG_HR(swap_->GetBuffer(i, IID_PPV_ARGS(&rt_[i])));
        device_->CreateRenderTargetView(rt_[i].Get(), nullptr, h);
        h.ptr += rtvInc_;
    }
}

void Dx12::WaitFence(u64 v) {
    if (fence_->GetCompletedValue() < v) { fence_->SetEventOnCompletion(v, fenceEvent_); WaitForSingleObject(fenceEvent_, INFINITE); }
}
void Dx12::WaitIdle() {
    if (!queue_ || !fence_) return;
    queue_->Signal(fence_.Get(), ++fenceCounter_);
    WaitFence(fenceCounter_);
}
void Dx12::Shutdown() {
    WaitIdle();
    for (auto& r : rt_) r.Reset();
    if (fenceEvent_) { CloseHandle(fenceEvent_); fenceEvent_ = nullptr; }
}
void Dx12::Resize(u32 w, u32 h) {
    if (!swap_ || !w || !h || (w == w_ && h == h_)) return;
    WaitIdle();
    for (auto& r : rt_) r.Reset();
    ENG_HR(swap_->ResizeBuffers(kFrames, w, h, DXGI_FORMAT_UNKNOWN, 0));
    w_ = w; h_ = h;
    CreateRenderTargets();
}

bool Dx12::AllocSrv(D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu) {
    if (srvUsed_ >= kSrvCount) return false;
    *cpu = srvHeap_->GetCPUDescriptorHandleForHeapStart(); cpu->ptr += (SIZE_T)srvUsed_ * srvInc_;
    *gpu = srvHeap_->GetGPUDescriptorHandleForHeapStart(); gpu->ptr += (UINT64)srvUsed_ * srvInc_;
    ++srvUsed_;
    return true;
}

static D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* r, D3D12_RESOURCE_STATES a, D3D12_RESOURCE_STATES b) {
    D3D12_RESOURCE_BARRIER bar = {};
    bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    bar.Transition.pResource = r; bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    bar.Transition.StateBefore = a; bar.Transition.StateAfter = b;
    return bar;
}

ID3D12GraphicsCommandList* Dx12::BeginFrame(const float clear[4]) {
    frameIdx_ = swap_->GetCurrentBackBufferIndex();
    WaitFence(fenceVals_[frameIdx_]);
    alloc_[frameIdx_]->Reset();
    list_->Reset(alloc_[frameIdx_].Get(), nullptr);
    auto b = Transition(rt_[frameIdx_].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    list_->ResourceBarrier(1, &b);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart(); rtv.ptr += (SIZE_T)frameIdx_ * rtvInc_;
    list_->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    list_->ClearRenderTargetView(rtv, clear, 0, nullptr);
    D3D12_VIEWPORT vp = {0, 0, (float)w_, (float)h_, 0.f, 1.f};
    D3D12_RECT sc = {0, 0, (LONG)w_, (LONG)h_};
    list_->RSSetViewports(1, &vp); list_->RSSetScissorRects(1, &sc);
    return list_.Get();
}

void Dx12::EndFrame(bool vsync) {
    auto b = Transition(rt_[frameIdx_].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    list_->ResourceBarrier(1, &b);
    list_->Close();
    ID3D12CommandList* lists[] = {list_.Get()};
    queue_->ExecuteCommandLists(1, lists);
    swap_->Present(vsync ? 1 : 0, 0);
    fenceVals_[frameIdx_] = ++fenceCounter_;
    queue_->Signal(fence_.Get(), fenceCounter_);
}
}
