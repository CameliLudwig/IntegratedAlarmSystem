// dio5856.cpp
#include "dio5856.h"
#include <iostream>

using namespace Automation::BDaq;

namespace adv {
namespace dio {

// FIX: 定义 Byte 类型，对应 uint8_t (unsigned char)
// 研华 SDK 示例中常用 Byte，但在标准 C++ 中需要定义
using Byte = uint8_t;

bool Device::check(ErrorCode ret, const char* where) {
    if (ret != Success) {
        std::cerr << "[BDaq Error] " << where << " -> code=" << ret << std::endl;
        return false;
    }
    return true;
}

Device::Device(const wchar_t* deviceDesc) {
    di_ = InstantDiCtrl::Create();
    do_ = InstantDoCtrl::Create();
    if (!di_ || !do_) throw std::runtime_error("Create InstantDi/Do failed");

    DeviceInformation devInfo;
    if (deviceDesc && *deviceDesc) {
        devInfo = DeviceInformation(deviceDesc);
    } else {
        // 自动选第一个支持的设备
        Array<DeviceTreeNode>* devs = di_->getSupportedDevices();
        if (!devs || devs->getCount() <= 0) {
            throw std::runtime_error("No supported BDaq devices");
        }
        // 注意：如果这里依然报错，说明头文件解析在前面就已经彻底失败了
        // 修复 header 后这里应该能通过
        DeviceTreeNode node = devs->getItem(0);
        devInfo = DeviceInformation(node.Description);
    }

    if (!check(di_->setSelectedDevice(devInfo), "di.setSelectedDevice") ||
        !check(do_->setSelectedDevice(devInfo), "do.setSelectedDevice")) {
        throw std::runtime_error("setSelectedDevice failed");
    }

    diPortCount_ = di_->getPortCount();
    doPortCount_ = do_->getPortCount();
}

Device::~Device() {
    if (di_) { di_->Dispose(); di_ = nullptr; }
    if (do_) { do_->Dispose(); do_ = nullptr; }
}

// ===== DI =====
bool Device::readDiPort(int port, uint8_t& value) const {
    if (port < 0 || port >= diPortCount_) return false;
    Byte b = 0;
    ErrorCode ret = di_->Read(port, 1, &b);
    value = static_cast<uint8_t>(b);
    return check(ret, "di.Read(port)");
}

bool Device::readAllDi(std::vector<uint8_t>& bytes) const {
    bytes.assign(diPortCount_, 0);
    if (diPortCount_ <= 0) return true;
    std::vector<Byte> buf(diPortCount_, 0);
    ErrorCode ret = di_->Read(0, diPortCount_, buf.data());
    for (int i = 0; i < diPortCount_; ++i) bytes[i] = static_cast<uint8_t>(buf[i]);
    return check(ret, "di.Read(all)");
}

bool Device::readDiBit(int channel, bool& level) const {
    int port, bit; channelToPortBit(channel, port, bit);
    uint8_t v = 0;
    if (!readDiPort(port, v)) return false;
    level = ((v >> bit) & 0x1) != 0;
    return true;
}

// ===== DO =====
bool Device::writeDoPort(int port, uint8_t value) {
    if (port < 0 || port >= doPortCount_) return false;
    Byte b = static_cast<Byte>(value);
    ErrorCode ret = do_->Write(port, 1, &b);
    return check(ret, "do.Write(port)");
}

bool Device::writeDoBit(int channel, bool level) {
    int port, bit; channelToPortBit(channel, port, bit);
    if (port < 0 || port >= doPortCount_) return false;

    Byte b = 0;
    if (!check(do_->Read(port, 1, &b), "do.Read(port-before-write)")) return false;

    if (level) b |=  (1 << bit);
    else       b &= ~(1 << bit);

    ErrorCode ret = do_->Write(port, 1, &b);
    return check(ret, "do.Write(bit)");
}

bool Device::readDoPort(int port, uint8_t& value) const {
    if (port < 0 || port >= doPortCount_) return false;
    Byte b = 0;
    ErrorCode ret = do_->Read(port, 1, &b);
    value = static_cast<uint8_t>(b);
    return check(ret, "do.Read(port)");
}

} // namespace dio
} // namespace adv
