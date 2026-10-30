// 游戏模块示例: 只依赖 runtime/game_api.h + core/fixed.h。改完保存 -> 编辑器自动热重载, 状态不丢。
#include <cstring>
#include <type_traits>
#include "runtime/game_api.h"
#include "core/fixed.h"
using namespace eng;

struct Fighter { Fixed x, y; i16 hp; u16 pad; };
struct GameState {
    GameStateHeader hdr;        // 必须是第一个成员
    Fighter f[2];
};
static_assert(std::is_trivially_copyable_v<GameState>, "状态必须是 POD, 才能 memcpy 快照");

static void Init(void* s, u64 size) {
    memset(s, 0, (size_t)size);
    auto* g = (GameState*)s;
    g->f[0].x = Fixed::FromInt(-200); g->f[1].x = Fixed::FromInt(200);
    g->f[0].hp = g->f[1].hp = 1000;
}
static void Tick(void* s, const GameInput* in) {
    auto* g = (GameState*)s;
    g->hdr.frame++;
    for (int i = 0; i < 2; ++i) {
        Fixed spd = Fixed::FromInt(3);
        if (in->buttons[i] & BTN_LEFT)  g->f[i].x = g->f[i].x - spd;
        if (in->buttons[i] & BTN_RIGHT) g->f[i].x = g->f[i].x + spd;
        g->f[i].x = Clamp(g->f[i].x, Fixed::FromInt(-600), Fixed::FromInt(600));
    }
}

extern "C" __declspec(dllexport) bool GetGameAPI(GameAPI* out) {
    out->version = kGameApiVersion; out->stateSize = sizeof(GameState);
    out->Init = &Init; out->Tick = &Tick;
    return true;
}
