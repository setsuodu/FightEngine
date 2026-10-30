#pragma once
namespace eng {
// TODO: miniaudio 封装 + mixer/bus。回滚期间音效必须走事件队列，只在确认帧触发。
class IAudio { public: virtual ~IAudio() = default; virtual void Play(unsigned soundId) = 0; };
}
