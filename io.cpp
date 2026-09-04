
/**
 * @file    dio5856.cpp
 * @brief   研华 BDaq 数字量 I/O 简易封装实现（USB-5856 等）
 *
 * 关键点：
 *  1) 设备选择
 *     - 若调用方提供 deviceDesc（如 L"USB-5856,BID#0"），直接用它构造 DeviceInformation
 *     - 否则，从 getSupportedDevices() 中取第一个节点的 Description 构造 DeviceInformation
 *
 *  2) DI 开路默认态(OpenState)/反相(Inverse) 复位
 *     - 很多隔离 DI 在“开路”时会按 OpenState 掩码返回 1，导致“未接线也读 1”
 *     - 构造后对每个 DI 端口：setDiOpenState(0)、setDiInversePort(0)，改成开路=0、正常极性
 *
 *  3) DO 单点写的读改写策略
 *     - 先读端口字节 -> 修改目标位 -> 整口写回
 *     - 若设备不支持回读，用影子缓存 doShadow_ 维护端口字节，避免破坏其他位
 *
 *  4) 错误与警告
 *     - BDaq 的 ErrorCode 并非非 0 即失败；通常 >= 0xC0000000 才是“失败”
 *     - 这里区分打印 Error（失败）与 Warn（非成功但未达失败阈值）
 */

#include "io.h"
#include <algorithm>   // std::max
#include <cstdio>

namespace adv {
namespace dio {

using namespace Automation::BDaq;

// ============= 内部工具：区分失败/告警/成功 =============
static inline bool isFailed(ErrorCode ec) {
    // 参考 biodaq 的错误码风格：最高位为 1 时多为失败
    return (static_cast<unsigned>(ec) >= 0xC0000000u);
}
static inline bool isWarning(ErrorCode ec) {
    // 非成功且未达失败阈值，按告警处理
    return (ec != Success) && !isFailed(ec);
}

bool Device::check(ErrorCode ret, const char* where) {
    if (isFailed(ret)) {
        std::cerr << "[BDaq Error] " << where << " -> code=" << ret << std::endl;
        return false;
    }
    if (isWarning(ret)) {
        std::cerr << "[BDaq Warn ] " << where << " -> code=" << ret << std::endl;
    }
    return true;
}

// ======================== 构造/析构 ========================
Device::Device(const wchar_t* deviceDesc) {
    // 1) 创建 DI/DO 即时控制器
    di_ = InstantDiCtrl::Create();
    do_ = InstantDoCtrl::Create();
    if (!di_ || !do_) {
        throw std::runtime_error("Create InstantDi/Do failed");
    }

    // 2) 选择设备
    DeviceInformation devInfo;
    if (deviceDesc && *deviceDesc) {
        // 显式传入设备描述（推荐：准确选中指定 BID 的设备）
        devInfo = DeviceInformation(deviceDesc);
    } else {
        // 自动选择：拿“第一个支持的设备”
        // 注意：getSupportedDevices() 返回的是 DeviceTreeNode 数组，
        // 需要用节点的 Description 构造 DeviceInformation
        Array<DeviceTreeNode>* devs = (Array<DeviceTreeNode>*)di_->getSupportedDevices();
        if (!devs || devs->getCount() <= 0) {
            throw std::runtime_error("No supported BDaq devices");
        }
        devInfo = DeviceInformation(devs->getItem(0).Description);
    }

    // 3) 把同一个设备绑定到 DI/DO 控制器
    if (!check(di_->setSelectedDevice(devInfo), "di.setSelectedDevice") ||
        !check(do_->setSelectedDevice(devInfo), "do.setSelectedDevice")) {
        throw std::runtime_error("setSelectedDevice failed");
    }

    // 4) 查询端口数量（以 USB-5856 为例，通常 DI/DO 各 4 个端口）
    diPortCount_ = di_->getPortCount();
    doPortCount_ = do_->getPortCount();

    // 5) === 关键修复：将 DI 的 OpenState/Inverse 复位为“开路=0，不反相” ===
    if (diPortCount_ > 0) {
        Array<DioPort>* ports = di_->getPorts();               // 端口数组
        if (ports) {
            for (int i = 0; i < ports->getCount(); ++i) {
                DioPort& p = ports->getItem(i);
                // 开路默认态置 0（开路→读 0），反相清 0（正常极性）
                // 失败不视为致命，仅打印警告
                ErrorCode r1 = p.setDiOpenState(0);            // 若机型不支持，会返回警告/错误
                ErrorCode r2 = p.setDiInversePort(0);
                check(r1, "DioPort.setDiOpenState(0)");
                check(r2, "DioPort.setDiInversePort(0)");
            }
        }
    }

    // 6) 初始化 DO 影子缓存，并探测是否支持回读
    doShadow_.assign(std::max(0, doPortCount_), 0);
    if (doPortCount_ > 0) {
        uint8 tmp = 0;
        // 尝试单口 Read 探测；若失败就认为不支持回读
        ErrorCode r = do_->Read(0, tmp);
        doHasReadback_ = !isFailed(r);

        // 如支持回读，把各端口当前值读入 shadow，避免“未知初值”
        if (doHasReadback_) {
            std::vector<uint8> buf(doPortCount_, 0);
            if (check(do_->Read(0, doPortCount_, buf.data()), "do.Read(all-init)")) {
                for (int i = 0; i < doPortCount_; ++i) doShadow_[i] = buf[i];
            }
        }
    }
}

Device::~Device() {
    // 先处置 DO、后处置 DI（次序通常无强约束；主要确保都调用 Dispose）
    if (di_) { di_->Dispose(); di_ = nullptr; }
    if (do_) { do_->Dispose(); do_ = nullptr; }
}

// ======================== DI（输入） ========================

bool Device::readDiPort(int port, uint8& value) const {
    // 边界检查：端口号必须在可用范围内
    if (port < 0 || port >= diPortCount_) return false;

    // 读取单个端口的 8 位值；value 的 bit0..bit7 对应该端口的 8 路
    ErrorCode ret = di_->Read(port, value);  // 若库无此重载，可改为 di_->Read(port, 1, &value)
    return check(ret, "di.Read(port)");
}

bool Device::readAllDi(std::vector<uint8>& bytes) const {
    // 为返回数组分配空间（端口数可能为 0）
    bytes.assign(std::max(0, diPortCount_), 0);
    if (diPortCount_ <= 0) return true;

    // 从起始端口 0 连续读取 diPortCount_ 个端口
    ErrorCode ret = di_->Read(0, diPortCount_, bytes.data());
    return check(ret, "di.Read(all)");
}

bool Device::readDiBit(int channel, bool& level) const {
    // 通道号 -> 端口与位
    int port, bit; channelToPortBit(channel, port, bit);
    if (port < 0 || port >= diPortCount_) return false;

    // 用 ReadBit 直接读取该位，避免手动移位与位序/极性歧义
    uint8 v = 0;
    ErrorCode ret = di_->ReadBit(port, bit, &v);
    if (!check(ret, "di.ReadBit(port,bit)")) return false;

    level = (v != 0);   // 非 0 即 true
    return true;
}

// ======================== DO（输出） ========================

bool Device::writeDoPort(int port, uint8 value) {
    if (port < 0 || port >= doPortCount_) return false;

    // 整口写入（一次性写 8 位）
    ErrorCode ret = do_->Write(port, value);
    if (!check(ret, "do.Write(port)")) return false;

    // 写成功后，同步影子缓存（即便设备支持回读，也保持 shadow 一致）
    if ((size_t)port < doShadow_.size()) doShadow_[port] = value;
    return true;
}

bool Device::writeDoBit(int channel, bool level) {
    int port, bit; channelToPortBit(channel, port, bit);
    if (port < 0 || port >= doPortCount_) return false;

    uint8 b = 0;

    // 读端口当前值：优先使用真实回读；不支持则落回 shadow
    if (doHasReadback_) {
        if (!check(do_->Read(port, b), "do.Read(port-before-write)")) return false;
    } else {
        b = doShadow_.empty() ? 0 : doShadow_[port];
    }

    // 修改目标位
    if (level) b = static_cast<uint8>(b |  (1u << bit));
    else       b = static_cast<uint8>(b & ~(1u << bit));

    // 写回整口
    ErrorCode ret = do_->Write(port, b);
    if (!check(ret, "do.Write(bit)")) return false;

    // 同步影子缓存
    if ((size_t)port < doShadow_.size()) doShadow_[port] = b;
    return true;
}

bool Device::readDoPort(int port, uint8& value) const {
    if (port < 0 || port >= doPortCount_) return false;

    if (doHasReadback_) {
        // 设备支持回读 -> 直接读取硬件口
        ErrorCode ret = do_->Read(port, value);
        return check(ret, "do.Read(port)");
    } else {
        // 不支持回读 -> 返回影子值（能反映本进程的最近写入状态）
        value = (port < (int)doShadow_.size()) ? doShadow_[port] : 0;
        return true;
    }
}

// ======================== 调试辅助 ========================

void Device::debugDumpDiConfigAndState() const {
    std::cout << "[DI debug] portCount=" << diPortCount_ << std::endl;
    Array<DioPort>* ports = di_->getPorts();
    if (!ports) { std::cout << "  (no ports)" << std::endl; return; }

    for (int i = 0; i < ports->getCount(); ++i) {
        DioPort& p = ports->getItem(i);
        uint8 openMask = p.getDiOpenState();
        uint8 invMask  = p.getDiInversePort();
        uint8 val      = 0;
        di_->Read(i, val);  // 端口字节
        std::printf("  Port%d  open=0x%02X  inv=0x%02X  val=0x%02X\n",
                    i, (unsigned)openMask, (unsigned)invMask, (unsigned)val);
    }
}

} // namespace dio
} // namespace adv

