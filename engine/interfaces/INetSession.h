#pragma once
namespace eng {
// TODO: 包装 GGPO。save/load 回调直接调用 StateArena::Save/Load; advance 回调调用 GameAPI::Tick。
class INetSession { public: virtual ~INetSession() = default; virtual void Update() = 0; };
}
