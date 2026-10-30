#include "render/renderer.h"
#include "core/assert.h"
#include "core/fixed.h"
#include "core/log.h"
#include "runtime/game_api.h"
#include <cstring>
#include <cstddef>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace eng {
using Microsoft::WRL::ComPtr;

// ---------- Demo 状态布局（与 game/game_main.cpp 对齐；正式项目应走 DebugDraw 契约）----------
struct DemoFighter { Fixed x, y; i16 hp; u16 pad; };
struct DemoGameState {
    GameStateHeader hdr;
    DemoFighter f[2];
};
static_assert(sizeof(DemoFighter) == 12, "DemoFighter layout");
static_assert(offsetof(DemoGameState, f) == sizeof(GameStateHeader), "header must be first");

// VS 输入：NDC 位置 + 颜色
struct Vertex {
    float x, y;
    float r, g, b, a;
};

static const char* kShaderSrc = R"HLSL(
struct VSIn {
    float2 pos : POSITION;
    float4 col : COLOR;
};
struct VSOut {
    float4 pos : SV_Position;
    float4 col : COLOR;
};
VSOut VSMain(VSIn i) {
    VSOut o;
    o.pos = float4(i.pos, 0, 1);
    o.col = i.col;
    return o;
}
float4 PSMain(VSOut i) : SV_Target {
    return i.col;
}
)HLSL";

static D3D12_RESOURCE_BARRIER MakeBarrier(ID3D12Resource* r, D3D12_RESOURCE_STATES a, D3D12_RESOURCE_STATES b) {
    D3D12_RESOURCE_BARRIER bar{};
    bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    bar.Transition.pResource = r;
    bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    bar.Transition.StateBefore = a;
    bar.Transition.StateAfter = b;
    return bar;
}

Renderer::Renderer(Dx12& dx) : dx_(dx) {
    if (!CreatePipeline()) {
        LogError("Renderer: pipeline create failed");
    }
}

Renderer::~Renderer() {
    DestroyTarget();
}

bool Renderer::CreatePipeline() {
    // Root signature: empty（顶点色走 IA）
    D3D12_ROOT_SIGNATURE_DESC rsd{};
    rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> sig, err;
    HRESULT hr = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err);
    if (FAILED(hr)) {
        if (err) LogError("SerializeRootSignature: %s", (const char*)err->GetBufferPointer());
        return false;
    }
    ENG_HR(dx_.Device()->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&root_)));

    ComPtr<ID3DBlob> vs, ps, cerr;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifndef NDEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    hr = D3DCompile(kShaderSrc, strlen(kShaderSrc), "renderer.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", flags, 0, &vs, &cerr);
    if (FAILED(hr)) {
        if (cerr) LogError("VS compile: %s", (const char*)cerr->GetBufferPointer());
        return false;
    }
    cerr.Reset();
    hr = D3DCompile(kShaderSrc, strlen(kShaderSrc), "renderer.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", flags, 0, &ps, &cerr);
    if (FAILED(hr)) {
        if (cerr) LogError("PS compile: %s", (const char*)cerr->GetBufferPointer());
        return false;
    }

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pd{};
    pd.pRootSignature = root_.Get();
    pd.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    pd.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    pd.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pd.SampleMask = UINT_MAX;
    pd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pd.DepthStencilState.DepthEnable = FALSE;
    pd.InputLayout = { layout, 2 };
    pd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pd.NumRenderTargets = 1;
    pd.RTVFormats[0] = Dx12::BackbufferFormat();
    pd.SampleDesc.Count = 1;
    ENG_HR(dx_.Device()->CreateGraphicsPipelineState(&pd, IID_PPV_ARGS(&pso_)));
    LogInfo("Renderer: pipeline ready");
    return true;
}

void Renderer::DestroyTarget() {
    color_.Reset();
    rtvHeap_.Reset();
    w_ = h_ = 0;
    colorState_ = D3D12_RESOURCE_STATE_COMMON;
    // SRV 槽不回收（Dx12 线性分配器无 free）；重建时复用同一 slot
}

bool Renderer::CreateTarget(u32 w, u32 h) {
    DestroyTarget();
    if (!w || !h) return false;

    D3D12_DESCRIPTOR_HEAP_DESC hd{};
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    hd.NumDescriptors = 1;
    ENG_HR(dx_.Device()->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtvHeap_)));
    rtv_ = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

    D3D12_RESOURCE_DESC rd{};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rd.Width = w;
    rd.Height = h;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.Format = Dx12::BackbufferFormat();
    rd.SampleDesc.Count = 1;
    rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE cv{};
    cv.Format = rd.Format;
    cv.Color[0] = 0.12f; cv.Color[1] = 0.12f; cv.Color[2] = 0.16f; cv.Color[3] = 1.f;

    D3D12_HEAP_PROPERTIES hp{};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    ENG_HR(dx_.Device()->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
        D3D12_RESOURCE_STATE_COMMON, &cv, IID_PPV_ARGS(&color_)));
    colorState_ = D3D12_RESOURCE_STATE_COMMON;

    dx_.Device()->CreateRenderTargetView(color_.Get(), nullptr, rtv_);

    if (!srvAllocated_) {
        if (!dx_.AllocSrv(&srvCpu_, &srvGpu_)) {
            LogError("Renderer: SRV heap full");
            DestroyTarget();
            return false;
        }
        srvAllocated_ = true;
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC svd{};
    svd.Format = rd.Format;
    svd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    svd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    svd.Texture2D.MipLevels = 1;
    dx_.Device()->CreateShaderResourceView(color_.Get(), &svd, srvCpu_);

    w_ = w; h_ = h;
    LogInfo("Renderer: color RT %ux%u", w, h);
    return true;
}

void Renderer::Resize(u32 w, u32 h) {
    if (!w || !h) return;
    if (w == w_ && h == h_ && color_) return;
    // 尺寸变化时可能仍有帧在飞，先等 GPU
    dx_.WaitIdle();
    CreateTarget(w, h);
}

void Renderer::Draw(ID3D12GraphicsCommandList* list, const RenderView& view) {
    if (!list || !pso_ || !root_) return;
    if (view.width && view.height) {
        if (view.width != w_ || view.height != h_ || !color_) {
            // Resize 可能在 UI 线程语义上调用过；这里兜底
            dx_.WaitIdle();
            if (!CreateTarget(view.width, view.height)) return;
        }
    }
    if (!color_) return;

    // COMMON/PS -> RT
    if (colorState_ != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        auto b = MakeBarrier(color_.Get(), colorState_, D3D12_RESOURCE_STATE_RENDER_TARGET);
        list->ResourceBarrier(1, &b);
        colorState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }

    list->OMSetRenderTargets(1, &rtv_, FALSE, nullptr);
    const float clear[4] = { 0.12f, 0.12f, 0.16f, 1.f };
    list->ClearRenderTargetView(rtv_, clear, 0, nullptr);

    D3D12_VIEWPORT vp{ 0, 0, (float)w_, (float)h_, 0.f, 1.f };
    D3D12_RECT sc{ 0, 0, (LONG)w_, (LONG)h_ };
    list->RSSetViewports(1, &vp);
    list->RSSetScissorRects(1, &sc);

    // ---- 从 Demo 状态取两个 fighter，画成色块 ----
    Vertex verts[12]; // 2 quads * 6 verts
    int nVert = 0;
    auto pushQuad = [&](float cx, float cy, float hw, float hh, float r, float g, float b) {
        // 世界 x∈[-600,600] → NDC；y 暂固定中线附近
        auto toNdcX = [](float x) { return x / 600.f; };
        auto toNdcY = [](float y) { return y / 400.f; };
        float x0 = toNdcX(cx - hw), x1 = toNdcX(cx + hw);
        float y0 = toNdcY(cy - hh), y1 = toNdcY(cy + hh);
        Vertex q[6] = {
            {x0,y0, r,g,b,1}, {x1,y0, r,g,b,1}, {x1,y1, r,g,b,1},
            {x0,y0, r,g,b,1}, {x1,y1, r,g,b,1}, {x0,y1, r,g,b,1},
        };
        for (int i = 0; i < 6 && nVert < 12; ++i) verts[nVert++] = q[i];
    };

    const bool okState = view.state && view.stateSize >= sizeof(DemoGameState);
    if (okState) {
        const auto* cur = (const DemoGameState*)view.state;
        const DemoGameState* prev = nullptr;
        if (view.prevState && view.stateSize >= sizeof(DemoGameState))
            prev = (const DemoGameState*)view.prevState;

        auto lerpF = [](Fixed a, Fixed b, float t) {
            return a.ToFloat() + (b.ToFloat() - a.ToFloat()) * t;
        };
        const float t = view.alpha;
        for (int i = 0; i < 2; ++i) {
            float x = cur->f[i].x.ToFloat();
            float y = cur->f[i].y.ToFloat();
            if (prev) {
                x = lerpF(prev->f[i].x, cur->f[i].x, t);
                y = lerpF(prev->f[i].y, cur->f[i].y, t);
            }
            // P1 青 / P2 橙；高度用 hp 简单示意
            float hp01 = cur->f[i].hp / 1000.f;
            if (hp01 < 0.1f) hp01 = 0.1f;
            if (i == 0) pushQuad(x, y, 40.f, 60.f * hp01, 0.2f, 0.85f, 0.9f);
            else        pushQuad(x, y, 40.f, 60.f * hp01, 0.95f, 0.55f, 0.2f);
        }
    } else {
        // 无状态时画两个占位块，证明 RT 通路通
        pushQuad(-200.f, 0.f, 40.f, 60.f, 0.3f, 0.3f, 0.35f);
        pushQuad( 200.f, 0.f, 40.f, 60.f, 0.3f, 0.3f, 0.35f);
    }

    if (nVert > 0) {
        // 上传：临时用 upload heap 每帧建（第一刀够用；后续改环形缓冲）
        const u32 bytes = (u32)(sizeof(Vertex) * nVert);
        D3D12_HEAP_PROPERTIES upHp{}; upHp.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC ub{};
        ub.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        ub.Width = bytes;
        ub.Height = 1; ub.DepthOrArraySize = 1; ub.MipLevels = 1;
        ub.SampleDesc.Count = 1;
        ub.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> upload;
        if (SUCCEEDED(dx_.Device()->CreateCommittedResource(&upHp, D3D12_HEAP_FLAG_NONE, &ub,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)))) {
            void* mapped = nullptr;
            D3D12_RANGE rng{0,0};
            if (SUCCEEDED(upload->Map(0, &rng, &mapped))) {
                memcpy(mapped, verts, bytes);
                upload->Unmap(0, nullptr);
            }
            list->SetGraphicsRootSignature(root_.Get());
            list->SetPipelineState(pso_.Get());
            list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            D3D12_VERTEX_BUFFER_VIEW vbv{};
            vbv.BufferLocation = upload->GetGPUVirtualAddress();
            vbv.SizeInBytes = bytes;
            vbv.StrideInBytes = sizeof(Vertex);
            list->IASetVertexBuffers(0, 1, &vbv);
            list->DrawInstanced(nVert, 1, 0, 0);
            // upload 在本帧 list 关闭前必须活着 → 用 WaitIdle 不现实；
            // 第一刀：把 lifetime 挂到静态环形（简单延迟释放）
            static ComPtr<ID3D12Resource> keep[3];
            static int keepI = 0;
            keep[keepI] = upload;
            keepI = (keepI + 1) % 3;
        }
    }

    // RT -> PS resource（ImGui 采样）
    {
        auto b = MakeBarrier(color_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        list->ResourceBarrier(1, &b);
        colorState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    // 恢复主 RT（swapchain），供后续 ImGui 画
    // 注意：BeginFrame 已把 swapchain 设为 RT；我们改写了 OM，这里不重设——
    // ImGuiRender 前由调用方再次 OMSet 到 backbuffer。见 main.cpp。
}

} // namespace eng
