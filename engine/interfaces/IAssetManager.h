#pragma once
#include "core/types.h"
namespace eng {
// TODO: 句柄 + 引用计数 + 异步加载 + 热重载; 离线 cooker: cgltf/ufbx -> 自有二进制格式。
using AssetHandle = u32;
class IAssetManager { public: virtual ~IAssetManager() = default; virtual AssetHandle Load(const char* path) = 0; };
}
