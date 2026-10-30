#include "runtime/state_arena.h"
#include <malloc.h>
#include <cstring>
namespace eng {
bool StateArena::Create(u64 size) {
    Destroy();
    data_ = (u8*)_aligned_malloc((size_t)size, 64);
    if (!data_) return false;
    size_ = size; Clear();
    return true;
}
void StateArena::Destroy() { if (data_) { _aligned_free(data_); data_ = nullptr; } size_ = 0; }
void StateArena::Clear() { if (data_) memset(data_, 0, (size_t)size_); }
void StateArena::Save(std::vector<u8>& out) const { out.assign(data_, data_ + size_); }
bool StateArena::Load(const std::vector<u8>& in) {
    if (in.size() != size_) return false;
    memcpy(data_, in.data(), (size_t)size_);
    return true;
}
}
