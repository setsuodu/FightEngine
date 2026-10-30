#pragma once
#include <vector>
#include "core/types.h"
namespace eng {
// 引擎持有的连续状态内存。快照 = memcpy。后续 GGPO 的 save/load 回调直接调用 Save/Load。
class StateArena {
public:
    ~StateArena() { Destroy(); }
    bool Create(u64 size);
    void Destroy();
    void Clear();                                   // 清零
    void* Data() { return data_; }
    const void* Data() const { return data_; }
    u64 Size() const { return size_; }
    void Save(std::vector<u8>& out) const;
    bool Load(const std::vector<u8>& in);
private:
    u8* data_ = nullptr; u64 size_ = 0;
};
}
