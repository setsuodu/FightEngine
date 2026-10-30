#include "runtime/game_module.h"
#include "core/log.h"
namespace eng {
namespace fs = std::filesystem;

bool GameModule::Init(const fs::path& dll) { src_ = dll; return TryLoad(); }

void GameModule::Shutdown() {
    std::error_code ec;
    if (lib_) { FreeLibrary(lib_); lib_ = nullptr; }
    if (!copy_.empty()) { fs::remove(copy_, ec); copy_.clear(); }
    api_ = {};
}

bool GameModule::TryLoad() {
    std::error_code ec;
    auto stamp = fs::last_write_time(src_, ec);
    if (ec) { LogWarn("game.dll not found: %s", src_.string().c_str()); return false; }
    fs::path copy = src_.parent_path() / (src_.stem().wstring() + L"_live_" + std::to_wstring(++counter_) + L".dll");
    fs::copy_file(src_, copy, fs::copy_options::overwrite_existing, ec);
    if (ec) { LogWarn("copy dll failed: %s", ec.message().c_str()); return false; }
    HMODULE lib = LoadLibraryW(copy.c_str());
    if (!lib) { LogWarn("LoadLibrary failed (%lu)", GetLastError()); fs::remove(copy, ec); return false; }
    GameAPI api{};
    auto fn = (GetGameAPIFn)GetProcAddress(lib, "GetGameAPI");
    if (!fn || !fn(&api) || api.version != kGameApiVersion) {
        LogError("game.dll has no valid GetGameAPI / version mismatch");
        FreeLibrary(lib); fs::remove(copy, ec); return false;
    }
    HMODULE oldLib = lib_; fs::path oldCopy = copy_;
    lib_ = lib; copy_ = copy; api_ = api; loadedStamp_ = stamp; pendingStamp_ = stamp; ++reloadCount_;
    if (oldLib) { FreeLibrary(oldLib); fs::remove(oldCopy, ec); }
    LogInfo("game module loaded (#%u), stateSize=%llu", reloadCount_, (unsigned long long)api.stateSize);
    return true;
}

bool GameModule::Poll(float dt) {
    pollTimer_ += dt;
    if (pollTimer_ < 0.25f) return false;
    pollTimer_ = 0.f;
    std::error_code ec;
    auto stamp = fs::last_write_time(src_, ec);
    if (ec || stamp == loadedStamp_) return false;
    if (stamp != pendingStamp_) { pendingStamp_ = stamp; return false; }   // 等文件写完 (两次轮询时间戳一致)
    return TryLoad();
}
}
