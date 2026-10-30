#pragma once
namespace eng {
// TODO: 网格/材质/蒙皮/描边/后处理/2D batch。实现放 engine/render/。
class IRenderer { public: virtual ~IRenderer() = default; virtual void BeginScene() = 0; virtual void EndScene() = 0; };
}
