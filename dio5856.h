// dio5856.h
// 适用于 Advantech USB-5856（32DI + 32DO）或同系列 BDaq 设备
#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <stdint.h>

#ifndef HMODULE
typedef HANDLE HMODULE;
#endif
#include "bdaqctrl.h"
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace adv {
namespace dio {

class Device {
public:
    // 设备描述例：L"USB-5856,BID#0"
    // 传空字符串或 nullptr 将自动选择第一个支持的设备
    explicit Device(const wchar_t* deviceDesc = L"");
    ~Device();

    // 禁拷贝，允许移动（如需）
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    // 端口数量（通常 4：每端口 8 路）
    int  diPortCount() const { return diPortCount_; }
    int  doPortCount() const { return doPortCount_; }

    // ===== DI 读取 =====
    // 读一个 DI 端口（0..diPortCount-1），返回一个字节（bit0..bit7）
    bool readDiPort(int port, uint8_t& value) const;

    // 读全部 DI 端口（按端口顺序填充）
    bool readAllDi(std::vector<uint8_t>& bytes) const;

    // 读某一路 DI（channel = 0..31）
    bool readDiBit(int channel, bool& level) const;

    // ===== DO 写入/读取 =====
    // 写一个 DO 端口整字节（0..doPortCount-1）
    bool writeDoPort(int port, uint8_t value);

    // 只改某一路 DO（读改写，不影响同端口其它位）
    bool writeDoBit(int channel, bool level);

    // 读回一个 DO 端口当前值
    bool readDoPort(int port, uint8_t& value) const;

private:
    // 工具
    static bool check(Automation::BDaq::ErrorCode ret, const char* where);

    static inline void channelToPortBit(int channel, int& port, int& bit) {
        port = channel / 8; bit = channel % 8;
    }

    Automation::BDaq::InstantDiCtrl* di_ = nullptr;
    Automation::BDaq::InstantDoCtrl* do_ = nullptr;
    int diPortCount_ = 0;
    int doPortCount_ = 0;
};

} // namespace dio
} // namespace adv
