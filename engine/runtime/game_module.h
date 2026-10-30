#pragma once
#include <windows.h>
#include <filesystem>
#include "runtime/game_api.h"
namespace eng {
// 热重载: 把 game.dll 复制成 game_live_N.dll 再加载 (避免锁住原文件, 编译器可随时覆盖原 dll)。
class GameModule {
public:
    ~GameModule() { Shutdown(); }
    bool Init(const std::filesystem::path& dll);
    void Shutdown();
    bool Poll(float dt);          // 检测 dll 变化, 稳定后重载; 本次发生重载返回 true
    bool ForceReload() { return TryLoad(); }
    const GameAPI* Api() const { return lib_ ? &api_ : nullptr; }
    u32 ReloadCount() const { return reloadCount_; }
    const std::filesystem::path& SourcePath() const { return src_; }
private:
    bool TryLoad();
    std::filesystem::path src_, copy_;
    std::filesystem::file_time_type loadedStamp_{}, pendingStamp_{};
    HMODULE lib_ = nullptr; GameAPI api_{};
    float pollTimer_ = 0.f; u32 counter_ = 0, reloadCount_ = 0;
};
}
