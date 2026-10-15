#include "laserwarning.h"

LaserWarning::LaserWarning(QObject *parent) : QObject(parent)
{
    m_serial = new QSerialPort(this);
    // 串口配置：115200, 8数据位, 1停止位, 偶校验(Even)
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::EvenParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    connect(m_serial, &QSerialPort::readyRead, this, &LaserWarning::onReadyRead);
}

LaserWarning::~LaserWarning()
{
    if (m_serial->isOpen()) m_serial->close();
    closeLogFile();
}

void LaserWarning::setLogFile(const QString &path)
{
    if (m_logFile.isOpen())
        m_logFile.close();

    m_logFile.setFileName(path);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        qWarning().noquote() << QStringLiteral("Cannot open Laser log: %1").arg(path);
        return;
    }

    QString header = QStringLiteral("\r\n===== LaserWarning Log Start %1 =====\r\n\r\n")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
    m_logFile.write(header.toUtf8());
    m_logFile.flush();
}

void LaserWarning::closeLogFile()
{
    if (m_logFile.isOpen())
    {
        QString footer = QStringLiteral("\r\n===== LaserWarning Log End %1 =====\r\n")
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
        m_logFile.write(footer.toUtf8());
        m_logFile.close();
    }
}

void LaserWarning::writeLog(const QString &line)
{
    if (!m_logFile.isOpen()) return;
    m_logFile.write(line.toUtf8());
    m_logFile.flush();
}

bool LaserWarning::connectDevice(const QString &portName)
{
    if (m_serial->isOpen()) m_serial->close();
    m_serial->setPortName(portName);
    bool ok = m_serial->open(QIODevice::ReadWrite);
    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    if (ok) {
        writeLog(QStringLiteral("%1 [CONNECT] 激光告警设备连接成功: %2 (115200,8E1)\r\n").arg(ts, portName));
    } else {
        writeLog(QStringLiteral("%1 [CONNECT] 激光告警设备连接失败: %2 err=%3\r\n").arg(ts, portName, m_serial->errorString()));
    }
    return ok;
}

void LaserWarning::disconnectDevice()
{
    if (m_serial->isOpen()) {
        QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        writeLog(QStringLiteral("%1 [DISCONNECT] 激光告警设备断开连接\r\n").arg(ts));
        m_serial->close();
    }
}

void LaserWarning::sendSelfCheckCommand()
{
    if (!m_serial->isOpen()) return;

    QByteArray packet;
    packet.append(static_cast<char>(0xF0));
    packet.append(static_cast<char>(0xF0));
    packet.append(static_cast<char>(0x01)); // 类型：自检
    packet.append(static_cast<char>(0x06)); // 长度低位 (假设总长6)
    packet.append(static_cast<char>(0x00)); // 长度高位

    packet.append(static_cast<char>(calculateChecksum(packet, packet.size()))); // 校验和
    m_serial->write(packet);

    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    writeLog(QStringLiteral("%1 [TX] SelfCheck CMD: %2\r\n").arg(ts, QString::fromLatin1(packet.toHex(' '))));
}

quint8 LaserWarning::calculateChecksum(const QByteArray &data, int length)
{
    quint8 checksum = 0;
    for (int i = 0; i < length; ++i) {
        // 使用 .at() 避免老版本 Qt 报错
        checksum ^= static_cast<quint8>(data.at(i));
    }
    return checksum;
}

// 解析第1字节的通道位定义
ChannelFlags LaserWarning::parseChannelByte(quint8 byteVal)
{
    ChannelFlags flags;
    // 7-6 bit: 01=通道1有信号 (二进制 01 = 十进制 1)
    flags.ch1_active = ((byteVal >> 6) & 0x03) == 0x01;
    // 5-4 bit: 10=通道2有信号 (二进制 10 = 十进制 2)
    flags.ch2_active = ((byteVal >> 4) & 0x03) == 0x02;
    // 3-2 bit: 11=通道3有信号 (二进制 11 = 十进制 3)
    flags.ch3_active = ((byteVal >> 2) & 0x03) == 0x03;
    // 1-0 bit: 保留
    return flags;
}

void LaserWarning::onReadyRead()
{
    QByteArray newData = m_serial->readAll();
    m_buffer.append(newData);

    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    writeLog(QStringLiteral("%1 [RX RAW] %2 (%3 bytes, buffer=%4)\r\n")
             .arg(ts, QString::fromLatin1(newData.toHex(' ')))
             .arg(newData.size()).arg(m_buffer.size()));

    while (m_buffer.size() >= 6) {
        int headIdx = m_buffer.indexOf(QByteArray::fromHex("F0F0"));
        if (headIdx == -1) {
            writeLog(QStringLiteral("%1 [RX] No F0F0 header found, clearing buffer\r\n").arg(ts));
            m_buffer.clear();
            return;
        }
        if (headIdx > 0) {
            writeLog(QStringLiteral("%1 [RX] Skipped %2 bytes before F0F0\r\n").arg(ts).arg(headIdx));
            m_buffer.remove(0, headIdx);
        }

        if (m_buffer.size() < 6) return;

        // 解析长度：小端模式 (低位在前)
        quint8 lenLow = static_cast<quint8>(m_buffer.at(3));
        quint8 lenHigh = static_cast<quint8>(m_buffer.at(4));
        int packetLen = (lenHigh << 8) | lenLow;

        if (m_buffer.size() < packetLen) return; // 等待完整包

        QByteArray frame = m_buffer.mid(0, packetLen);
        m_buffer.remove(0, packetLen);

        // 校验
        quint8 recvSum = static_cast<quint8>(frame.at(packetLen - 1));
        quint8 calcSum = calculateChecksum(frame, packetLen - 1);

        writeLog(QStringLiteral("%1 [RX FRAME] type=0x%2 len=%3 hex=%4 sum:recv=0x%5 calc=0x%6 %7\r\n")
                 .arg(ts).arg(static_cast<uint>(frame.at(2)), 2, 16, QChar('0'))
                 .arg(packetLen)
                 .arg(QString::fromLatin1(frame.toHex(' ')))
                 .arg(recvSum, 2, 16, QChar('0'))
                 .arg(calcSum, 2, 16, QChar('0'))
                 .arg(recvSum == calcSum ? "OK" : "FAIL"));

        if (recvSum == calcSum) {
            parseFrame(frame);
        } else {
            qDebug() << "Checksum Failed: Recv" << recvSum << "Calc" << calcSum;
        }
    }
}

void LaserWarning::parseFrame(const QByteArray &frame)
{
    quint8 type = static_cast<quint8>(frame.at(2));
    // 数据区从第5个字节开始 (Frame Header 2 + Type 1 + Len 2 = 5)
    const int DATA_OFFSET = 5;

    // 安全检查：防止访问越界
    // frame.size() 包含了最后的校验位，所以实际有效数据长度应减去 Header(5) 和 Checksum(1)

    switch (type) {
    case 0x02: // 自检结果
    {
        if (frame.size() < DATA_OFFSET + 3) break;
        SelfCheckResult res;
        res.statusCh1 = static_cast<quint8>(frame.at(DATA_OFFSET + 0));
        res.statusCh2 = static_cast<quint8>(frame.at(DATA_OFFSET + 1));
        res.statusCh3 = static_cast<quint8>(frame.at(DATA_OFFSET + 2));
        writeLog(QStringLiteral("%1 [PARSE] SelfCheck: CH1=%2 CH2=%3 CH3=%4\r\n")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
                 .arg(res.statusCh1).arg(res.statusCh2).arg(res.statusCh3));
        emit selfCheckResultReceived(res);
        break;
    }
    case 0x04: // 脉冲激光告警信息
    {
        if (frame.size() < DATA_OFFSET + 10) break;
        PulseWarningData data;
        data.flags = parseChannelByte(static_cast<quint8>(frame.at(DATA_OFFSET + 0)));
        data.laserParam = (static_cast<quint8>(frame.at(DATA_OFFSET + 1)) << 8) |
                           static_cast<quint8>(frame.at(DATA_OFFSET + 2));
        data.channelParamReserved = static_cast<quint8>(frame.at(DATA_OFFSET + 3));
        writeLog(QStringLiteral("%1 [PARSE] PulseWarning: CH1=%2 CH2=%3 CH3=%4 laserParam=0x%5\r\n")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
                 .arg(data.flags.ch1_active).arg(data.flags.ch2_active).arg(data.flags.ch3_active)
                 .arg(data.laserParam, 4, 16, QChar('0')));
        emit pulseWarningReceived(data);
        break;
    }
    case 0x06: // 驾束激光告警信息
    {
        if (frame.size() < DATA_OFFSET + 8) break;
        BeamWarningData data;
        data.flags = parseChannelByte(static_cast<quint8>(frame.at(DATA_OFFSET + 0)));
        data.maxFrequency = static_cast<quint8>(frame.at(DATA_OFFSET + 1));
        data.paramCh1 = (static_cast<quint8>(frame.at(DATA_OFFSET + 2)) << 8) |
                         static_cast<quint8>(frame.at(DATA_OFFSET + 3));
        data.paramCh2 = (static_cast<quint8>(frame.at(DATA_OFFSET + 4)) << 8) |
                         static_cast<quint8>(frame.at(DATA_OFFSET + 5));
        data.paramCh3 = (static_cast<quint8>(frame.at(DATA_OFFSET + 6)) << 8) |
                         static_cast<quint8>(frame.at(DATA_OFFSET + 7));
        writeLog(QStringLiteral("%1 [PARSE] BeamWarning: CH1=%2 CH2=%3 CH3=%4 maxFreq=%5kHz\r\n")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
                 .arg(data.flags.ch1_active).arg(data.flags.ch2_active).arg(data.flags.ch3_active)
                 .arg(data.maxFrequency));
        emit beamWarningReceived(data);
        break;
    }
    default:
        writeLog(QStringLiteral("%1 [PARSE] Unknown type: 0x%2\r\n")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
                 .arg(type, 2, 16, QChar('0')));
        break;
    }
}
