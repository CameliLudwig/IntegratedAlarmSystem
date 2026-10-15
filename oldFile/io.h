#pragma once
/**
 * @file    dio5856.h
 * @brief   研华 BDaq 数字量 I/O 简易封装（以 USB-5856 为例：32DI + 32DO）
 *
 * 设计目标：
 *  - 提供“按端口/按位”的 DI 读取与 DO 写入接口
 *  - 屏蔽设备选择、DI 开路默认态(OpenState)/反相(Inverse) 等差异
 *  - 不依赖 Qt，纯 Console/服务进程可直接使用
 *
 * 使用说明：
 *  - 设备描述：例如 L"USB-5856,BID#0"；若传空字符串或使用默认构造，则自动选择第一个可用设备
 *  - 通道/端口映射：channel = port * 8 + bit，端口号从 0 连续编号，每端口 8 位（bit0..bit7）
 *  - DO“只改某一位”采用【读改写】策略；若设备不支持 DO 回读，则使用影子缓存（shadow）保持端口其他位不被破坏
 */

#include <stdexcept>
#include <vector>
#include <cstdint>
#include <iostream>
#include <windows.h>
#ifndef HMODULE
typedef HANDLE HMODULE;
#endif
#include "bdaqctrl.h"   // 研华官方头文件，定义了 BDaq 接口与类型（如 uint8）

namespace adv {
namespace dio {

using namespace Automation::BDaq;

/**
 * @class  Device
 * @brief  简化的 DI/DO 设备访问封装
 */
class Device {
public:
    /**
     * @brief  构造并绑定设备
     * @param  deviceDesc 设备描述（如 L"USB-5856,BID#0"），为空则自动选第一个可用设备
     * @throws std::runtime_error 设备创建或选择失败
     */
    explicit Device(const wchar_t* deviceDesc = L"");

    /**
     * @brief  析构：释放 BDaq 资源
     */
    ~Device();

    // ======================= DI（输入） =======================

    /**
     * @brief  读取一个 DI 端口（8 位）
     * @param  port   端口号（0..diPortCount()-1）
     * @param  value  返回端口的 8 位数据，bit0 为最低位
     * @return 成功/失败
     */
    bool readDiPort(int port, uint8& value) const;

    /**
     * @brief  读取所有 DI 端口
     * @param  bytes  输出数组，大小为 diPortCount()，每个元素为一个端口字节
     * @return 成功/失败
     */
    bool readAllDi(std::vector<uint8>& bytes) const;

    /**
     * @brief  读取某一路 DI（按“通道号”）
     * @param  channel 通道号（如 0..31），channel = port*8 + bit
     * @param  level   返回该位电平（true=1，false=0）
     * @return 成功/失败
     */
    bool readDiBit(int channel, bool& level) const;

    // ======================= DO（输出） =======================

    /**
     * @brief  写一个 DO 端口（整字节）
     * @param  port   端口号（0..doPortCount()-1）
     * @param  value  要写入的 8 位值（一次性写 8 路）
     * @return 成功/失败
     *
     * 注意：某些设备对 DO 写入有“掩码/只读位”限制，按需在上层做屏蔽
     */
    bool writeDoPort(int port, uint8 value);

    /**
     * @brief  只写某一路 DO（读改写）
     * @param  channel 通道号（如 0..31），channel = port*8 + bit
     * @param  level   目标电平 true=1/false=0
     * @return 成功/失败
     *
     * 说明：先读取端口当前值，再修改对应那一位，最后整口写回。
     *      若设备不支持回读，则使用影子缓存保持端口其他位不被破坏。
     */
    bool writeDoBit(int channel, bool level);

    /**
     * @brief  读回一个 DO 端口当前值
     * @param  port   端口号
     * @param  value  返回端口 8 位值；若设备不支持回读，则返回影子缓存
     * @return 成功/失败（不支持回读时也会返回 true，但 value 来自缓存）
     */
    bool readDoPort(int port, uint8& value) const;

    // ======================= 辅助/调试 =======================

    /// DI 端口数量（通常 4）
    int diPortCount() const { return diPortCount_; }
    /// DO 端口数量（通常 4）
    int doPortCount() const { return doPortCount_; }

    /**
     * @brief  （可选）调试：打印每个 DI 端口的 OpenState/Inverse 以及当前读值
     */
    void debugDumpDiConfigAndState() const;

private:
    // 工具：通道号 -> 端口/位
    static inline void channelToPortBit(int channel, int& port, int& bit) {
        port = channel / 8; bit = channel % 8;
    }

    // 工具：错误/告警打印（失败返回 false）
    static bool check(ErrorCode ret, const char* where);

private:
    // BDaq 控制器（即时模式）
    InstantDiCtrl* di_ = nullptr;
    InstantDoCtrl* do_ = nullptr;

    // 端口数量
    int diPortCount_ = 0;
    int doPortCount_ = 0;

    // 当 DO 不支持回读时使用的影子缓存（每端口 1 字节）
    bool doHasReadback_ = true;
    mutable std::vector<uint8> doShadow_;
};

} // namespace dio
} // namespace adv


