#include "mainwindow.h"
// #include "threadnaming.h"
#include "ui_mainwindow.h"
#include "PdfReport.h"
#include "applogger.h"

#include <QStyleFactory>
#include <QFile>
#include <QCloseEvent>
#include <QVBoxLayout>

#include "qtmaterialflatbutton.h"

uint16_t SetBit(uint16_t num, int bitIndex, bool b)
{
    if (bitIndex < 0 || bitIndex > 15)
        return num;
    if (b)
        num |= (1 << bitIndex);
    else
        num &= ~(1 << bitIndex);
    return num;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_sessionStartTime = QDateTime::currentDateTime();
    LOG("=== 综合告警检测设备启动 ===");

    setWindowTitle("15综合告警与干扰系统检测设备(增强型)");
    ui->statusBar->deleteLater();

    qApp->setStyle(QStyleFactory::create("Fusion"));

    QFile *styleFile = new QFile(":/style.qss", this);
    styleFile->open(QFile::ReadOnly);
    QString styleSheet = QString(styleFile->readAll());
    this->setStyleSheet(styleSheet);
    styleFile->close();

    foreach (QLineEdit *lineEdit, findChildren<QLineEdit *>())
    {
        lineEdit->setText("-1");
        lineEdit->setAlignment(Qt::AlignCenter);
    }

    // flexray线程
    // m_Flexthread = new FlexRayThread(this);
    // m_Flexthread->setLogFile(QDir::currentPath() + "/FlexRay_log.txt");
    // connect(m_Flexthread, &FlexRayThread::error, this, &MainWindow::flexrayerror);
    // connect(this, &MainWindow::flexSend, m_Flexthread, &FlexRayThread::flexRayDataSend);
    // connect(m_Flexthread, &FlexRayThread::flexRayDataReceived, this,
    //         static_cast<void (MainWindow::*)(QVector<short unsigned int>, int)>(&MainWindow::show_data));
    // connect(m_Flexthread, &FlexRayThread::abChannelChanged, this, [this](int chA, int chB)
    //         {
    //     m_abNodeAOk = (chA != 0);
    //     m_abNodeBOk = (chB != 0); });
    // connect(m_Flexthread, &FlexRayThread::abFrameTimeout, this, [this]()
    //         {
    //     m_abNodeAOk = false;
    //     m_abNodeBOk = false; });
    // connect(m_Flexthread, &FlexRayThread::connectionLost, this, []()
    //         { LOG("FlexRay连接丢失"); });
    // connect(m_Flexthread, &FlexRayThread::connectionRestored, this, []()
    //         { LOG("FlexRay连接恢复"); });

    // FlexRay信号连接 (不启动)
    initFlexRayConnections();

    auto flatButtons = this->findChildren<QtMaterialFlatButton *>();
    for (QtMaterialFlatButton *btn : flatButtons)
    {
        btn->setUseThemeColors(false);
        btn->setBackgroundMode(Qt::OpaqueMode);
        btn->setBackgroundColor(QColor("#374151"));
        btn->setForegroundColor(Qt::white);
        btn->setCornerRadius(6);
        btn->setCheckable(true);
        btn->setChecked(false);
    }

    // 自检按钮
    pushButton_selfcheck = new QtMaterialFlatButton(ui->groupBox_27);
    pushButton_selfcheck->setText("发送自检");
    pushButton_selfcheck->setObjectName("pushButton_selfcheck");
    pushButton_selfcheck->setUseThemeColors(false);
    pushButton_selfcheck->setBackgroundMode(Qt::OpaqueMode);
    pushButton_selfcheck->setBackgroundColor(QColor("#374151"));
    pushButton_selfcheck->setForegroundColor(Qt::white);
    pushButton_selfcheck->setCornerRadius(6);
    pushButton_selfcheck->setCheckable(true);
    pushButton_selfcheck->setChecked(false);

    QLabel *label_self = new QLabel("自检计数", ui->groupBox_27);
    label_self->setFont(QFont("微软雅黑", 8));
    lineEdit_self = new QLineEdit(ui->groupBox_27);
    lineEdit_self->setObjectName("lineEdit_self");
    lineEdit_self->setText("0");
    lineEdit_self->setAlignment(Qt::AlignCenter);
    lineEdit_self->setReadOnly(true);

    QHBoxLayout *hLayout_selfcheck = new QHBoxLayout();
    hLayout_selfcheck->addWidget(label_self);
    hLayout_selfcheck->addWidget(lineEdit_self);
    hLayout_selfcheck->addWidget(pushButton_selfcheck);
    ui->gridLayout_23->addLayout(hLayout_selfcheck, 2, 0, 1, 2);

    connect(pushButton_selfcheck, &QtMaterialFlatButton::clicked,
            this, &MainWindow::on_pushButton_selfcheck_clicked);

    ui->lineEdit_15->setText("-1");
    ui->lineEdit_16->setText("-1");

    auto *gaugeCompassLayout = new QVBoxLayout(ui->widget);
    gaugeCompassLayout->setContentsMargins(0, 0, 0, 0);

    gaugeCompass_ = new GaugeCompass(ui->widget);
    gaugeCompassLayout->addWidget(gaugeCompass_);
    gaugeCompass_->setValue(0);

    ui->groupBox_2->hide();
    ui->groupBox_23->hide();
    ui->groupBox_30->hide();
    ui->groupBox_42->hide();
    ui->groupBox_46->hide();
    ui->groupBox_28->hide();
    ui->label_33->hide();
    ui->comboBox_DriverAvoidDirection->hide();

    connect(ui->pushButton_cancel, &QPushButton::clicked, [&]()
            {
        QList<CircleLineEdit*> vecCircleLineEdits = ui->groupBox_40->findChildren<CircleLineEdit*>();
        for(auto tmpLineEdit : vecCircleLineEdits)
        {
            tmpLineEdit->setText("2");
        } });

    ui->pushButton_cancel->click();

    connect(ui->pushButton_testStart, &QPushButton::clicked, [&]()
            {
                 QString errStr = "告警头安装位置为：";
        if (ui->lineEdit_jgLeftFront->text().toInt() == 2)
            errStr += "左前 ";
        if (ui->lineEdit_jgRightFront->text().toInt() == 2)
            errStr += "右前 ";
        if (ui->lineEdit_jgLeftRear->text().toInt() == 2)
            errStr += "左后 ";
        if (ui->lineEdit_jgRightRear->text().toInt() == 2)
            errStr += "右后 ";
        switch (ui->comboBox->currentIndex()){
        case 0:
            setAlarmWindows(ui->lineEdit_jgLeftFront->text().toInt(),
                            ui->lineEdit_jgRightFront->text().toInt(),
                            ",所选告警头位置为:左前 右前", errStr);
            break;
        case 1:
            setAlarmWindows(ui->lineEdit_jgRightFront->text().toInt(),
                            ui->lineEdit_jgRightRear->text().toInt(),
                            ",所选告警头位置为:右前 右后",errStr);
            break;
        case 2:
            setAlarmWindows(ui->lineEdit_jgLeftRear->text().toInt(),
                            ui->lineEdit_jgRightRear->text().toInt(),
                            ",所选告警头位置为:右后 左后", errStr);
            break;
        case 3:
            setAlarmWindows(ui->lineEdit_jgLeftFront->text().toInt(),
                            ui->lineEdit_jgLeftRear->text().toInt(),
                            ",所选告警头位置为:左后 左前", errStr);
            break;
        default:
            break;
        } });

    connect(ui->pushButton_testEnd, &QPushButton::clicked, [&]()
            {
        startTest_ = false;
        ui->lineEdit_jgLeftFront->setText("0");
        ui->lineEdit_jgRightFront->setText("0");
        ui->lineEdit_jgLeftRear->setText("0");
        ui->lineEdit_jgRightRear->setText("0");
        ui->pushButton_cancel->click();
        // 生成会话PDF报告
        generateSessionPDF(); });

    // 所有阻塞操作延后到事件循环, 界面先出来
    QTimer::singleShot(100, this, [this]()
                       { initDevices(); });

    LOG("=== MainWindow 构造完成（设备初始化已调度）===");
}

void MainWindow::setAlarmWindows(int tmp1, int tmp2, QString mes1, QString mes2)
{
    if (tmp1 == 2 && tmp2 == 2)
    {
        startTest_ = true;
    }
    else
    {
        startTest_ = false;
        QMessageBox::warning(this, "告警头错误", mes2 + mes1);
    }
};

void MainWindow::startFlexRayConnection()
{
    // if (m_flexRayStartRequested || !m_Flexthread)
    //     return;

    // m_flexRayStartRequested = true;
    // m_flexRayStartCancelled = false;
    // m_flexRayStartupFuture = QtConcurrent::run([this]()
    //                                            {
    //     // ThreadNaming::setCurrentThreadName(L"FlexRay Startup Worker");
    //     LOG("开始启动FlexRay...");
    //     m_Flexthread->startFlexRayOperation(&m_flexRayStartCancelled); });
}

void MainWindow::initFlexRayConnections()
{
    qRegisterMetaType<QVector<uint16>>("QVector<uint16>");
    LOG("FlexRay 初始化连接...");
    m_flexRayThread = new FlexRayThread();
    LOG("FlexRayThread 已创建");

    // 创建并启动用于承载 FlexRay worker 的 QThread，并将 worker 移到该线程
    m_flexRayWorkerThread = new QThread(this);
    m_flexRayThread->moveToThread(m_flexRayWorkerThread);
    // 确保线程退出时自动删除 worker 对象由 MainWindow 负责删除
    m_flexRayWorkerThread->start();
    LOG(QString("moveToThread done: workerThread=%1 isRunning=%2 workerAffinity=%3")
            .arg((quintptr)m_flexRayWorkerThread, 0, 16)
            .arg(m_flexRayWorkerThread->isRunning())
            .arg((quintptr)m_flexRayThread->thread(), 0, 16));

    // 创建并管理由 MainWindow 负责的定时器（仅重连定时器）
    m_flexray_reconnect_timer = new QTimer(this);
    m_flexray_reconnect_timer->setInterval(3000);
    m_flexray_reconnect_timer->setSingleShot(false);
    {
        QPointer<MainWindow> safeThis(this);
        connect(m_flexray_reconnect_timer, &QTimer::timeout, this, [safeThis]()
                {
            if (!safeThis) return;
            // 如果线程正在运行则不需要重连
            if (!safeThis->m_flexRayThread) return;
            if (safeThis->m_flexRayThread->isRunning()) return;
            // 如果已有启动进行中则跳过本次重连触发
            if (safeThis->m_flexStartInProgress.load()) {
                LOG("FlexRay reconnect timer: 忽略，已有启动进行中");
                return;
            }
            LOG("FlexRay reconnect timer: 尝试启动 (queued)...");
            // 标记正在启动并停止重连定时器，避免重复排队
            safeThis->m_flexStartInProgress.store(true);
            if (safeThis->m_flexray_reconnect_timer) safeThis->m_flexray_reconnect_timer->stop();
            // 将 start 请求排入 FlexRay worker 的事件队列（安全，非阻塞 UI）
            QMetaObject::invokeMethod(safeThis->m_flexRayThread, "startFlexRayOperation", Qt::QueuedConnection);
            // 超时保护：若15s内未完成，则清除 in-progress 并重启重连定时器
            QTimer::singleShot(15000, safeThis.data(), [safeThis]() {
                if (!safeThis) return;
                if (safeThis->m_flexStartInProgress.load()) {
                    LOG("FlexRay reconnect 超时：清除启动标志并重启重连定时器");
                    safeThis->m_flexStartInProgress.store(false);
                    if (safeThis->m_flexray_reconnect_timer) safeThis->m_flexray_reconnect_timer->start();
                }
            }); });
    }

    bool connected = QObject::connect(m_flexRayThread, &FlexRayThread::flexRayDataReceived,
                                      this, &MainWindow::show_data, Qt::QueuedConnection);
    if (connected)
    {
        LOG("FlexRay data信号连接: 成功");
    }
    else
    {
        LOG("FlexRay data信号连接: 失败!!! 类型不匹配!");
        QMessageBox::critical(nullptr, "FlexRay 错误", "信号连接失败! 类型不匹配");
    }
    QObject::connect(m_flexRayThread, &FlexRayThread::error, this, [this](const QString &err)
                     {
            qDebug() << "FlexRay错误:" << err;
            LOG("FlexRay错误: " + err);
            // 清理启动中标志（启动失败）
            m_flexStartInProgress.store(false);
            // 确保在 GUI 线程创建对话框: 使用 queued connection 或者再次通过 invokeMethod 到主线程执行
            QMessageBox *msgBox = new QMessageBox(QMessageBox::Warning, "FlexRay错误", err, QMessageBox::Ok, this);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();
            // 启动重连定时器以便稍后重试
            if (m_flexray_reconnect_timer) m_flexray_reconnect_timer->start(); }, Qt::QueuedConnection);
    QObject::connect(m_flexRayThread, &FlexRayThread::connectionLost, this, [this]()
                     {
            qDebug() << "FlexRay连接丢失";
            LOG("FlexRay连接丢失");
            // 清理启动中标志（若在尝试启动时发生断开）
            m_flexStartInProgress.store(false);
            ui->lineEdit_16->setText("0");
            ui->lineEdit_15->setText("0");

            QMessageBox *msgBox = new QMessageBox(QMessageBox::Critical, "FlexRay", "连接丢失，正在尝试重连...", QMessageBox::Ok, this);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();

            // 启动重连定时器
            if (m_flexray_reconnect_timer) m_flexray_reconnect_timer->start(); }, Qt::QueuedConnection);
    QObject::connect(m_flexRayThread, &FlexRayThread::connectionRestored, this, [this]()
                     {
            qDebug() << "FlexRay连接恢复";
            LOG("FlexRay连接恢复");

            QMessageBox *msgBox = new QMessageBox(QMessageBox::Information, "FlexRay", "连接已恢复", QMessageBox::Ok, this);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();

            // 停止重连定时器
            if (m_flexray_reconnect_timer) m_flexray_reconnect_timer->stop(); }, Qt::QueuedConnection);

    // AB灯: 节点状态做门控 + 超时强制灭
    QObject::connect(m_flexRayThread, &FlexRayThread::abChannelChanged, this, [this](int chA, int chB)
                     {
        m_abNodeAOk = chA;
        m_abNodeBOk = chB;
        if (!chA) ui->lineEdit_16->setText("0");
        if (!chB) ui->lineEdit_15->setText("0"); }, Qt::QueuedConnection);
    QObject::connect(m_flexRayThread, &FlexRayThread::abFrameTimeout, this, [this]()
                     {
        ui->lineEdit_16->setText("0");
        ui->lineEdit_15->setText("0"); }, Qt::QueuedConnection);

    QObject::connect(this, &MainWindow::flexSend,
                     m_flexRayThread, &FlexRayThread::flexRayDataSend);
    LOG("FlexRay 信号连接完成");
}

void MainWindow::initDevices()
{
    LOG("initDevices 开始...");

    // Flex/Ray 启动 — 单独延后, 不阻塞
    {
        QPointer<MainWindow> safeThis(this);
        QTimer::singleShot(500, this, [safeThis]()
                           {
            if (!safeThis) return;
            // 防止重入：如果已有启动进行中则跳过
            if (safeThis->m_flexStartInProgress.load()) {
                LOG("FlexRay 启动请求被忽略：已有启动进行中");
                return;
            }
            LOG("FlexRay 开始启动 (queued)...");
            // 如果没有 worker 或 worker 已被删除则不调用
            if (!safeThis->m_flexRayThread) return;
            // 标记启动中，避免定时器重复触发造成重入
            safeThis->m_flexStartInProgress.store(true);
            // 停止重连定时器在启动期间避免重复排队
            if (safeThis->m_flexray_reconnect_timer) safeThis->m_flexray_reconnect_timer->stop();
            // 将 start 请求排入 FlexRay worker 的事件队列（安全，非阻塞 UI）
            QMetaObject::invokeMethod(safeThis->m_flexRayThread, "startFlexRayOperation", Qt::QueuedConnection);
            // 启动一个超时看门器：若在超时内未能建立连接，则清理 in-progress 标记并启动重连定时器
            QTimer::singleShot(15000, safeThis.data(), [safeThis]() {
                if (!safeThis) return;
                if (safeThis->m_flexStartInProgress.load()) {
                    LOG("FlexRay 启动超时：清除启动标志并启动重连定时器");
                    safeThis->m_flexStartInProgress.store(false);
                    if (safeThis->m_flexray_reconnect_timer) safeThis->m_flexray_reconnect_timer->start();
                }
            }); });
    }
    LOG("initDevices 完成");
}

void MainWindow::setTestMode(TestSelectionDialog::TestMode mode)
{
    m_testMode = mode;
    m_returning = false;
    LOG(QString("设置测试模式: %1").arg(static_cast<int>(mode)));
    applyVisibilityForMode();
}

void MainWindow::applyVisibilityForMode()
{
    // 先隐藏所有顶层groupBox
    ui->groupBox_2->setVisible(false);  // 激光告警
    ui->groupBox_23->setVisible(false); // 烟幕弹状态
    ui->groupBox_30->setVisible(false); // 主机状态(含MP5)
    ui->groupBox_42->setVisible(false); // 诸元类型等
    ui->groupBox_46->setVisible(false); // 应答诸元
    ui->groupBox_28->setVisible(false); // 敌我识别器
    ui->groupBox_32->setVisible(false); // 识别诸元
    ui->groupBox_41->setVisible(false); // 告警诸元

    // 隐藏groupBox_30的非MP5子元素 (用于MP5模式)
    // groupBox_27是groupBox_30的子元素
    ui->groupBox_27->setVisible(true);

    auto hideGroup30Children = [this](bool hide)
    {
        // 隐藏groupBox_30中非groupBox_27的直接子控件
        QLayout *lay = ui->groupBox_30->layout();
        if (!lay)
            return;
        for (int i = 0; i < lay->count(); i++)
        {
            QLayoutItem *item = lay->itemAt(i);
            if (!item)
                continue;
            QWidget *w = item->widget();
            if (w && w != ui->groupBox_27)
            {
                w->setVisible(!hide);
            }
        }
    };

    switch (m_testMode)
    {
    case TestSelectionDialog::ModeLaser:
        ui->groupBox_2->setVisible(true);
        break;
    case TestSelectionDialog::ModeHostStatus:
        ui->groupBox_30->setVisible(true);
        break;
    case TestSelectionDialog::ModeMP5:
        ui->groupBox_30->setVisible(true);
        hideGroup30Children(true);
        break;
    case TestSelectionDialog::ModeAnswer:
        ui->groupBox_46->setVisible(true);
        ui->groupBox_42->setVisible(true); // 诸元类型/序号/批次/截获时间
        break;
    case TestSelectionDialog::ModeIFF:
        ui->groupBox_28->setVisible(true);
        break;
    case TestSelectionDialog::ModeIdentify:
        ui->groupBox_32->setVisible(true);
        break;
    default:
        break;
    }

    // "返回"按钮始终可见
    ui->pushButton_close->setVisible(true);
}

void MainWindow::flexrayerror(QString error)
{
    // LOG("FlexRay Error: " + error);
    // QMessageBox::warning(this, "FlexRay错误", error);
    // m_Flexthread->stop();
}

void MainWindow::show_data(QVector<uint16> rcvData, int channel)
{
    if (channel == 1)
    {
        /*ui->lineEdit_ReadKeyGroupParameters->setText(QString::number(rcvData.at(2)));
        ui->lineEdit_ReadTheWorkingStatusOfFriendlyAndEnemyForces->setText(QString::number(rcvData.at(3)));
        if (rcvData.at(4) >= 1 && rcvData.at(4) <= 4)
            ui->comboBox_setTopMode->setCurrentIndex(static_cast<int>(rcvData.at(4)));
        else
            ui->comboBox_setTopMode->setCurrentIndex(0);
        ui->lineEdit_ReadStealthCoating->setText(rcvData.at(6) == 1 ? "无涂层" : "有涂层");

        ui->lineEdit_jgLeftFront->setText(QString::number((rcvData.at(12) >> 4) & 0x03));
        ui->lineEdit_jgLeftRear->setText(QString::number(rcvData.at(12) & 0x03));
        ui->lineEdit_jgRightFront->setText(QString::number((rcvData.at(13) >> 4) & 0x03));
        ui->lineEdit_jgRightRear->setText(QString::number(rcvData.at(13) & 0x03));

        ui->lineEdit_ReadTopMode->setText(QString::number((rcvData.at(15) >> 4) & 0x03));
        ui->lineEdit_dwsbInquiry->setText(QString::number(rcvData.at(16) >> 4 & 0x03));
        ui->lineEdit_dwsbAnswer->setText(QString::number(rcvData.at(16) & 0x03));
        ui->lineEdit_dwsbHost->setText(QString::number(rcvData.at(17) >> 4 & 0x03));
        ui->lineEdit_hmbLeftFront->setText(QString::number(rcvData.at(18) >> 4 & 0x03));
        ui->lineEdit_hmbLeftRear->setText(QString::number(rcvData.at(18) & 0x03));
        ui->lineEdit_hmbRightFront->setText(QString::number(rcvData.at(19) >> 4 & 0x03));
        ui->lineEdit_hmbRightRear->setText(QString::number(rcvData.at(19) & 0x03));*/
        bool changed = false;
        for (int i = 0; i < qMin(rcvData.size(), m_TemprcvData1.size()); i++)
        {
            if ((double)rcvData.at(i) != m_TemprcvData1.at(i))
            {
                changed = true;
                m_TemprcvData1[i] = rcvData.at(i);
            }
        }

        if (!changed)
            return;
        LOG("show_data ch=1 数据变化!");

        QStringList hexList;
        for (uint16 value : rcvData)
        {
            hexList << QString("%1").arg(value, 4, 16, QChar('0')).toUpper();
        }
        QString result = hexList.join(" ");
        LOG(QString("接收到ch1的数据: %1").arg(result));

        auto setJgAndTrack = [this](CircleLineEdit *w, int val, const QString &name)
        {
            if (val)
                w->setText("2");
        };

        quint8 lef = rcvData.at(6) & 0xff;
        quint8 rig = (rcvData.at(6) >> 8) & 0xff;
        setJgAndTrack(ui->lineEdit_jgLeftFront, (lef >> 4) & 0x03, QString::fromUtf8("左前"));
        setJgAndTrack(ui->lineEdit_jgLeftRear, lef & 0x03, QString::fromUtf8("左后"));
        setJgAndTrack(ui->lineEdit_jgRightFront, (rig >> 4) & 0x03, QString::fromUtf8("右前"));
        setJgAndTrack(ui->lineEdit_jgRightRear, rig & 0x03, QString::fromUtf8("右后"));
    }
    else if (channel == 3)
    {
        QStringList type = {"告警诸元", "识别诸元", "应答诸元", "火警诸元"};
        int typeID = rcvData.at(0);
        ui->lineEdit_TypesOfElements->setText(type[rcvData.at(0)]);
        ui->lineEdit_ZhuyuanSerialNumber->setText(QString::number(rcvData.at(1)));
        ui->lineEdit_ZhuyuanBatch->setText(QString::number(rcvData.at(2)));
        ui->lineEdit_ZhuyuanInterceptionh->setText(QString::number(rcvData.at(3)));
        ui->lineEdit_ZhuyuanInterceptionm->setText(QString::number(rcvData.at(4)));
        ui->lineEdit_ZhuyuanInterceptions->setText(QString::number(rcvData.at(5)));

        switch (typeID)
        {
        case 0:
        {
            uint16_t azimuth = (uchar)rcvData.at(6) | ((uchar)rcvData.at(7) << 8);
            // ui->lineEdit_AlarmDirection->setText(azimuth == 0xFFFF ? "无效" : QString::number(azimuth));
            QStringList gjTypes = {"NULL", "单脉冲", "重频", "连续波", "编码", "毫米波", "综合", "保留"};
            int gjTypeVal = (uchar)rcvData.at(9);
            // ui->lineEdit_AlarmType->setText(gjTypeVal < gjTypes.size() ? gjTypes[gjTypeVal] : "其他");
            ui->comboBox_DriverAvoidDirection->setCurrentText(QString::number((uchar)rcvData.at(10)));
            break;
        }
        case 1:
        {
            uchar byte7 = (uchar)rcvData.at(7);
            int prop = (byte7 >> 6) & 0x03;
            ui->lineEdit_TargetAttribute->setText(prop == 1 ? "我方" : (prop == 2 ? "不名" : "未知"));
            ui->lineEdit_NumberOfTargetMatches->setText(QString::number((byte7 >> 3) & 0x07));
            ui->lineEdit_NumberOfTargets->setText(QString::number(byte7 & 0x07));
            uint16_t laserDist = (uchar)rcvData.at(8) | ((uchar)rcvData.at(9) << 8);
            ui->lineEdit_LaserDistance->setText(QString::number(laserDist));
            ui->lineEdit_MillimeterWaveDistance1->setText(QString::number((uchar)rcvData.at(10) | ((uchar)rcvData.at(11) << 8)));
            ui->lineEdit_MillimeterWaveDistance2->setText(QString::number((uchar)rcvData.at(12) | ((uchar)rcvData.at(13) << 8)));
            ui->lineEdit_MillimeterWaveDistance3->setText(QString::number((uchar)rcvData.at(14) | ((uchar)rcvData.at(15) << 8)));
            ui->lineEdit_MillimeterWaveDistance4->setText(QString::number((uchar)rcvData.at(16) | ((uchar)rcvData.at(17) << 8)));
            ui->lineEdit_MillimeterWaveDistance5->setText(QString::number((uchar)rcvData.at(18) | ((uchar)rcvData.at(19) << 8)));
            break;
        }
        case 2:
        {
            uint32_t id = ((uint32_t)(uchar)rcvData.at(6) << 16) +
                          ((uint32_t)(uchar)rcvData.at(9) << 8) +
                          (uint32_t)(uchar)rcvData.at(8);
            ui->lineEdit_RecognizerID->setText(QString::number(id));
            break;
        }
        default:
            break;
        }
    }
    else if (channel == 4)
    {
        // QStringList diwsbTips = {"无提示", "我方", "不名", "识别中", "故障"};
        // int dIdx = (uchar)rcvData.at(0);
        // ui->lineEdit_IdentificationOfFriendOrFoePrompt->setText(dIdx < diwsbTips.size() ? diwsbTips[dIdx] : "未知");
        // QStringList gjTips = {"无显示", "告警", "火警", "告警且火警"};
        // int gIdx = (uchar)rcvData.at(1);
        // ui->lineEdit_WarningPrompt->setText(gIdx < gjTips.size() ? gjTips[gIdx] : "未知");
        if (rcvData.size() < 2)
            return;
        if (rcvData.at(0) == m_TemprcvData4.at(0) && rcvData.at(1) == m_TemprcvData4.at(1))
            return;
        QString logMsg = QString("show_data ch=4 AB变化! ch=%1 slot=%2").arg(rcvData.at(0)).arg(rcvData.at(1));
        LOG(logMsg);
        m_TemprcvData4[0] = rcvData.at(0);
        m_TemprcvData4[1] = rcvData.at(1);

        int abChannel = rcvData.at(0);
        ui->lineEdit_16->setText(m_abNodeAOk && (abChannel & 0x01) ? "2" : "0"); // A
        ui->lineEdit_15->setText(m_abNodeBOk && (abChannel & 0x02) ? "2" : "0"); // B
    }
    else if (channel == 2)
    {
        if (rcvData.size() < 2)
            return;
        if (rcvData.at(0) == m_TemprcvData4.at(0) && rcvData.at(1) == m_TemprcvData4.at(1))
            return;
        m_TemprcvData4[0] = rcvData.at(0);
        m_TemprcvData4[1] = rcvData.at(1);
        int abChannel = rcvData.at(0);
        ui->lineEdit_15->setText(m_abNodeAOk && (abChannel & 0x01) ? "1" : "0");
        ui->lineEdit_16->setText(m_abNodeBOk && (abChannel & 0x02) ? "1" : "0");
    }
    else if (channel == 5)
    {
        bool changed = false;
        for (int i = 1; i < qMin(rcvData.size(), m_TemprcvData5.size()); i++)
        {
            if ((double)rcvData.at(i) != m_TemprcvData5.at(i))
            {
                changed = true;
                m_TemprcvData5[i] = rcvData.at(i);
            }
        }
        if (!changed)
            return;
        LOG("show_data ch=5 数据变化!");

        QStringList hexList;
        for (uint16 value : rcvData)
        {
            hexList << QString("%1").arg(value, 4, 16, QChar('0')).toUpper();
        }
        QString result = hexList.join(" ");
        LOG(QString("接收到ch5的数据: %1").arg(result));

        quint8 typeID = (rcvData.at(0) >> 8) & 0xff;
        const char *kindText = "";
        switch (typeID)
        {
        case 0x00:
            kindText = "NULL";
            break;
        case 0x01:
            kindText = "单脉冲激光";
            break;
        case 0x02:
            kindText = "重频激光";
            break;
        case 0x03:
            kindText = "架束制导激光";
            break;
        case 0x04:
            kindText = "编码激光告警";
            break;
        case 0x05:
            kindText = "毫米波告警";
            break;
        case 0x06:
            kindText = "综合告警";
            break;
        default:
            kindText = "未知";
            break;
        }

        quint8 high = static_cast<quint8>(rcvData.at(1) & 0xFF);
        quint8 low = static_cast<quint8>((rcvData.at(1) >> 8) & 0xFF);
        quint16 azimuth = high * 256 + low;

        quint8 dir = (rcvData.at(4) >> 8) & 0xFF;
        const char *dirText = "";
        switch (dir)
        {
        case 0x00:
            dirText = "正常驾驶";
            break;
        case 0x01:
            dirText = "正前方";
            break;
        case 0x02:
            dirText = "右前方";
            break;
        case 0x03:
            dirText = "右方";
            break;
        case 0x04:
            dirText = "右后方";
            break;
        case 0x05:
            dirText = "正后方";
            break;
        case 0x06:
            dirText = "左后方";
            break;
        case 0x07:
            dirText = "左方";
            break;
        case 0x08:
            dirText = "左前方";
            break;
        case 0xFF:
            dirText = "无法提示";
            break;
        default:
            dirText = "未知";
            break;
        }

        if (startTest_)
        {
            quint8 smk = rcvData.at(4) & 0xFF;
            const char *smkText = "";
            switch (smk)
            {
            case 0x00:
                smkText = "NULL";
                break;
            case 0x01:
                smkText = "左12";
                ui->lineEdit_smokeLeft1->setText(QString("0"));
                ui->lineEdit_smokeLeft2->setText(QString("0"));
                break;
            case 0x02:
                smkText = "左34";
                ui->lineEdit_smokeLeft3->setText(QString("0"));
                ui->lineEdit_smokeLeft4->setText(QString("0"));
                break;
            case 0x03:
                smkText = "右12";
                ui->lineEdit_smokeRight1->setText(QString("0"));
                ui->lineEdit_smokeRight2->setText(QString("0"));
                break;
            case 0x04:
                smkText = "右34";
                ui->lineEdit_smokeRight3->setText(QString("0"));
                ui->lineEdit_smokeRight4->setText(QString("0"));
                break;
            case 0x05:
                smkText = "左4";
                ui->lineEdit_smokeLeft1->setText(QString("0"));
                ui->lineEdit_smokeLeft2->setText(QString("0"));
                ui->lineEdit_smokeLeft3->setText(QString("0"));
                ui->lineEdit_smokeLeft4->setText(QString("0"));
                break;
            case 0x06:
                smkText = "右4";
                ui->lineEdit_smokeRight1->setText(QString("0"));
                ui->lineEdit_smokeRight2->setText(QString("0"));
                ui->lineEdit_smokeRight3->setText(QString("0"));
                ui->lineEdit_smokeRight4->setText(QString("0"));
                break;
            case 0x07:
                smkText = "前4";
                ui->lineEdit_smokeLeft1->setText(QString("0"));
                ui->lineEdit_smokeLeft2->setText(QString("0"));
                ui->lineEdit_smokeRight1->setText(QString("0"));
                ui->lineEdit_smokeRight2->setText(QString("0"));
                break;
            case 0x08:
                smkText = "八发齐射";
                ui->lineEdit_smokeLeft1->setText(QString("0"));
                ui->lineEdit_smokeLeft2->setText(QString("0"));
                ui->lineEdit_smokeLeft3->setText(QString("0"));
                ui->lineEdit_smokeLeft4->setText(QString("0"));
                ui->lineEdit_smokeRight1->setText(QString("0"));
                ui->lineEdit_smokeRight2->setText(QString("0"));
                ui->lineEdit_smokeRight3->setText(QString("0"));
                ui->lineEdit_smokeRight4->setText(QString("0"));
                break;
            case 0xFF:
                smkText = "无法提示";
                break;
            default:
                smkText = "未知";
                break;
            }
            ui->lineEdit_AlarmType->setText(QString(kindText));
            LOG(QString("告警类型: %1").arg(QString(kindText)));
            LOG(QString("告警角度high: %1").arg(QString::number(high)));
            LOG(QString("告警角度low: %1").arg(QString::number(low)));
            LOG(QString("告警角度: %1").arg(QString::number(azimuth)));
            ui->lineEdit_AlarmDirection->setText(QString::number(azimuth));
            gaugeCompass_->setValue(azimuth);
            LOG(QString("规避方向: %1").arg(QString(dirText)));
            LOG(QString("烟幕弹: %1").arg(QString(smkText)));
        }

        // switch (typeID)
        // {
        //     case 0:
        //     {
        //         uint16_t azimuth = (uchar)rcvData.at(6) | ((uchar)rcvData.at(7) << 8);
        //         LOG(QString("告警角度: %1").arg(azimuth == 0xFFFF ? "无效" : QString::number(azimuth)));
        //         QStringList gjTypes = {"NULL", "单脉冲", "重频", "连续波", "编码", "毫米波", "综合", "保留"};
        //         int gjTypeVal = (uchar)rcvData.at(9);
        //         LOG(QString("告警类型: %1").arg(gjTypeVal < gjTypes.size() ? gjTypes[gjTypeVal] : "其他"));
        //         LOG(QString("规避方向: %1").arg(QString::number((uchar)rcvData.at(10))));
        //         break;
        //     }
        // }
    }
}

QString MainWindow::generateSessionPDF()
{
    QString reportDir = QApplication::applicationDirPath() + "/reports";
    QDir().mkpath(reportDir);

    QString timestamp = m_sessionStartTime.toString("yyyyMMdd_hhmmss");
    QString fileName = QString("%1/%2_%3_%4.pdf")
                           .arg(reportDir)
                           .arg(timestamp)
                           .arg("suibian")
                           .arg(m_currentUser);

    qDebug() << "[MainWindow] 正在生成会话PDF报告:" << fileName;
    LOG(QString("正在生成PDF报告: %1").arg(fileName));

    PDFReportGenerator generator;
    generator.setCompanyInfo(QString::fromUtf8("中国人民解放军第二七六五厂"));
    generator.setReportTitle(QString::fromUtf8("15综合告警与干扰系统检测设备  检测报告"));
    generator.setDeviceName(QString::fromUtf8("15综合告警与干扰系统"));
    generator.setPartNumber(QString(""));
    generator.setInspector(m_currentUser);
    generator.setTestDate(m_sessionStartTime.toString("yyyy-MM-dd"));

    // 收集 ReportRow
    // QList<ReportRow> rows = collectReportRows();
    // for (const auto &r : rows)
    //     generator.addReportRow(r);

    bool ok = generator.generateFormalReport(fileName);
    if (ok)
    {
        LOG(QString("PDF报告已保存: %1").arg(fileName));
        qDebug() << "[MainWindow] PDF报告生成成功:" << fileName;
    }
    else
    {
        LOG("PDF报告生成失败!");
        qWarning() << "[MainWindow] PDF报告生成失败!";
    }

    return ok ? fileName : QString();
}

void MainWindow::on_flexraysend()
{
    // QVector<uint16_t> rcvData_MP5, rcvData_WP1, rcvData_WP4;
    // uint16_t Data_MP5[5] = {0};
    // uint16_t Data_WP1[5] = {0};
    // uint16_t Data_WP4[2] = {0};

    // for (int i = 0; i < 8; i++)
    //     Data_MP5[0] = SetBit(Data_MP5[0], i, (m_cnt_SelfCheck >> i) & 1);

    // uint8_t val_Key = ui->comboBox_KeyGroup->currentIndex();
    // for (int i = 0; i < 8; i++)
    //     Data_MP5[1] = SetBit(Data_MP5[1], i, (val_Key >> i) & 1);

    // uint8_t val_DiWSBStatus = ui->comboBox_setWorkingCondition->currentIndex();
    // for (int i = 0; i < 8; i++)
    //     Data_MP5[1] = SetBit(Data_MP5[1], 8 + i, (val_DiWSBStatus >> i) & 1);

    // uint8_t val_DingBu = ui->comboBox_setTopMode->currentIndex();
    // for (int i = 0; i < 8; i++)
    //     Data_MP5[2] = SetBit(Data_MP5[2], i, (val_DingBu >> i) & 1);

    // for (int i = 0; i < 8; i++)
    //     Data_MP5[2] = SetBit(Data_MP5[2], 8 + i, (0 >> i) & 1);

    // for (int i = 0; i < 4; i++)
    // {
    //     Data_MP5[4] = SetBit(Data_MP5[4], i, (1 >> i) & 1);
    //     Data_MP5[4] = SetBit(Data_MP5[4], 4 + i, (1 >> i) & 1);
    // }
    // for (int i = 0; i < 4; i++)
    //     Data_MP5[4] = SetBit(Data_MP5[4], 12 + i, (1 >> i) & 1);

    // for (int i = 0; i < 5; i++)
    //     rcvData_MP5.append(Data_MP5[i]);

    // for (int i = 0; i < 8; i++)
    //     Data_WP1[2] = SetBit(Data_WP1[2], i, (0 >> i) & 1);
    // for (int i = 0; i < 8; i++)
    //     Data_WP1[4] = SetBit(Data_WP1[4], i, (0 >> i) & 1);
    // for (int i = 0; i < 8; i++)
    //     Data_WP1[4] = SetBit(Data_WP1[4], 8 + i, (0 >> i) & 1);
    // for (int i = 0; i < 5; i++)
    //     rcvData_WP1.append(Data_WP1[i]);

    // for (int i = 0; i < 2; i++)
    //     rcvData_WP4.append(Data_WP4[i]);

    // emit flexSend(rcvData_MP5, rcvData_WP1, rcvData_WP4);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_returning)
        return;
    m_returning = true;
    LOG("用户点击了窗口关闭(X)按钮");
    generateModePDF();
    closeAllDevices();
    event->accept();
    emit exitApplication();
}

MainWindow::~MainWindow()
{
    closeAllDevices();
    delete ui;
    LOG("=== 程序退出 ===");
}

void MainWindow::closeAllDevices()
{
    LOG("=== 开始关闭所有设备 ===");
    // if (m_Flexthread)
    // {
    //     LOG("停止 FlexRay...");
    //     m_flexRayStartCancelled = true;
    //     m_Flexthread->stop();
    //     m_flexRayStartupFuture.waitForFinished();
    //     m_Flexthread->wait(3000);
    //     m_flexRayStartRequested = false;
    // }
    // 停止并停用由 MainWindow 管理的 FlexRay 定时器（仅重连定时器）
    if (m_flexray_reconnect_timer)
    {
        m_flexray_reconnect_timer->stop();
    }

    // 1. 停止 FlexRay
    if (m_flexRayThread)
    {
        LOG("停止 FlexRay...");
        // 请求停止（通过 queued 调用进入 worker 线程）
        QMetaObject::invokeMethod(m_flexRayThread, "stop", Qt::QueuedConnection);

        // 等待 worker 状态变为不运行（m_running==false），分步等待以便记录进度（总计 15s）
        const int stepMs = 1000;
        const int maxTotalMs = 15000;
        int waited = 0;
        bool exited = false;
        while (waited < maxTotalMs)
        {
            if (!m_flexRayThread->isRunning())
            {
                exited = true;
                break;
            }
            waited += stepMs;
            LOG(QString("等待 FlexRay 退出... %1/%2 ms").arg(waited).arg(maxTotalMs));
            QCoreApplication::processEvents();
            QThread::msleep(stepMs);
        }

        if (!exited)
        {
            // 超时，询问用户是否强制终止
            QMessageBox::StandardButton btn = QMessageBox::warning(this,
                                                                   "FlexRay 线程未退出",
                                                                   "FlexRay 线程在 15 秒内未退出。\n强制终止可能导致资源泄露或不稳定。\n是否要强制终止？",
                                                                   QMessageBox::Abort | QMessageBox::Retry,
                                                                   QMessageBox::Retry);

            if (btn == QMessageBox::Abort)
            {
                LOG("用户选择强制终止 FlexRay 线程");
                // 强制终止 worker 所在的 QThread（最后手段）
                if (m_flexRayWorkerThread)
                {
                    m_flexRayWorkerThread->terminate();
                    if (m_flexRayWorkerThread->wait(2000))
                    {
                        LOG("强制终止后 FlexRay worker 线程已退出");
                    }
                    else
                    {
                        LOG("强制终止后 FlexRay worker 线程仍未退出");
                    }
                }
            }
            else
            {
                // 再等 10s（用户选择再等等）
                LOG("用户选择再等待 10 秒");
                int extraWait = 10000;
                int waitedExtra = 0;
                while (waitedExtra < extraWait)
                {
                    if (!m_flexRayThread->isRunning())
                        break;
                    QCoreApplication::processEvents();
                    QThread::msleep(500);
                    waitedExtra += 500;
                }
                if (m_flexRayThread->isRunning())
                {
                    LOG("额外等待后仍未退出，准备强制终止 worker 线程");
                    if (m_flexRayWorkerThread)
                    {
                        m_flexRayWorkerThread->terminate();
                        m_flexRayWorkerThread->wait(2000);
                    }
                }
            }
        }

        // 删除 worker 对象并清理线程
        if (m_flexRayWorkerThread)
        {
            // 停止事件循环并等待线程退出（给予短超时）
            m_flexRayWorkerThread->quit();
            if (!m_flexRayWorkerThread->wait(1000))
            {
                LOG("警告: worker 线程在 quit 后仍未退出");
            }
        }

        delete m_flexRayThread;
        m_flexRayThread = nullptr;
        if (m_flexRayWorkerThread)
        {
            delete m_flexRayWorkerThread;
            m_flexRayWorkerThread = nullptr;
        }

        LOG("FlexRay 已停止");
    }
    LOG("=== 所有设备已关闭 ===");
}

void MainWindow::on_pushButton_close_clicked(bool checked)
{
    Q_UNUSED(checked);
    if (m_returning)
        return;
    m_returning = true;
    LOG("用户点击了「返回」按钮");
    generateModePDF();
    hide();
    emit returnToSelection();
}

void MainWindow::on_pushButton_selfcheck_clicked(bool checked)
{
    Q_UNUSED(checked);
    m_cnt_SelfCheck++;
    m_cnt_SelfCheck %= 255;
    lineEdit_self->setText(QString::number(m_cnt_SelfCheck));
    LOG(QString("自检发送, 计数=%1").arg(m_cnt_SelfCheck));
    on_flexraysend();
}

static QString makeReportPath(const QString &testName)
{
    QString dir = QApplication::applicationDirPath() + "/reports";
    QDir().mkpath(dir);
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1/%2_%3.pdf").arg(dir, ts, testName);
}

static void addTableRowIf(QList<QStringList> &rows, int idx, const QString &name,
                          const QString &range, const QString &val, bool ok)
{
    QStringList row;
    row << QString::number(idx) << name << range << val;
    row << (ok ? "合格" : "未触发");
    rows.append(row);
}

void MainWindow::generateModePDF()
{
    using Mode = TestSelectionDialog::TestMode;
    QString modeName;
    QStringList headers = {"序号", "检测项目", "指标范围", "检测结果", "状态"};
    QString path;

    switch (m_testMode)
    {
    case Mode::ModeLaser:
    {
        modeName = "激光告警自检";
        QList<QStringList> rows;
        auto add = [&](int idx, const QString &name, QLineEdit *w)
        {
            QString v = w->text();
            addTableRowIf(rows, idx, name, "0/1/2/3", v, (v != "-1" && v != "0"));
        };
        add(1, "激光左前", ui->lineEdit_jgLeftFront);
        add(2, "激光左后", ui->lineEdit_jgLeftRear);
        add(3, "激光右前", ui->lineEdit_jgRightFront);
        add(4, "激光右后", ui->lineEdit_jgRightRear);

        PdfReport report;
        report.setTitle("15综合告警检测设备  激光告警自检测试");
        report.setSubtitle(QString("检测时间: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        report.addParagraph("一、激光告警自检结果");
        report.addTable(headers, rows);
        path = makeReportPath(modeName);
        report.exportPdf(path);
        break;
    }
    case Mode::ModeHostStatus:
    {
        modeName = "主机状态";
        QList<QStringList> rows;
        auto add = [&](int idx, const QString &name, const QString &range, const QString &val, bool ok)
        {
            addTableRowIf(rows, idx, name, range, val, ok);
        };
        int n = 1;
        QString v = ui->lineEdit_ReadKeyGroupParameters->text();
        add(n++, "密钥组别参数", "1/2/3/4", v, (v != "-1"));
        v = ui->lineEdit_ReadTheWorkingStatusOfFriendlyAndEnemyForces->text();
        add(n++, "敌我工作状态", "1/2/3", v, (v != "-1"));
        v = ui->lineEdit_ReadStealthCoating->text();
        add(n++, "隐身涂层", "无涂层/有涂层", v, (!v.isEmpty() && v != "-1"));
        v = ui->lineEdit_ReadTopMode->text();
        add(n++, "顶部模式", "0/1/2/3", v, (v != "-1"));
        v = ui->lineEdit_70->text();
        add(n++, "被动8mm波", "—", v, (v != "-1"));

        PdfReport report;
        report.setTitle("15综合告警检测设备  主机状态测试");
        report.setSubtitle(QString("检测时间: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        report.addParagraph("一、主机状态");
        report.addTable(headers, rows);

        QList<QStringList> mmRows;
        auto addMM = [&](int idx, const QString &name, QLineEdit *w)
        {
            addTableRowIf(mmRows, idx, name, "0/1/2/3", w->text(), (w->text() != "-1" && w->text() != "0"));
        };
        addMM(1, "毫米波左前", ui->lineEdit_hmbLeftFront);
        addMM(2, "毫米波左后", ui->lineEdit_hmbLeftRear);
        addMM(3, "毫米波右前", ui->lineEdit_hmbRightFront);
        addMM(4, "毫米波右后", ui->lineEdit_hmbRightRear);
        report.addParagraph("二、毫米波告警");
        report.addTable(headers, mmRows);

        QList<PdfReport::KeyValueRow> kv;
        kv.append({"主机", ui->lineEdit_dwsbHost->text()});
        kv.append({"询问", ui->lineEdit_dwsbInquiry->text()});
        kv.append({"应答", ui->lineEdit_dwsbAnswer->text()});
        report.addParagraph("三、敌我识别状态");
        report.addKeyValueGrid(kv);
        path = makeReportPath(modeName);
        report.exportPdf(path);
        break;
    }
    case Mode::ModeMP5:
    {
        modeName = "任务核心处理机设置";
        QList<PdfReport::KeyValueRow> kv;
        kv.append({"工作状态", ui->comboBox_setWorkingCondition->currentText()});
        kv.append({"顶部模式", ui->comboBox_setTopMode->currentText()});
        kv.append({"密钥组别", ui->comboBox_KeyGroup->currentText()});
        kv.append({"自检计数", lineEdit_self ? lineEdit_self->text() : "0"});

        PdfReport report;
        report.setTitle("15综合告警检测设备  任务核心处理机设置测试");
        report.setSubtitle(QString("检测时间: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        report.addParagraph("一、任务核心处理机设置（MP-5）");
        report.addKeyValueGrid(kv);
        path = makeReportPath(modeName);
        report.exportPdf(path);
        break;
    }
    case Mode::ModeAnswer:
    {
        modeName = "应答诸元";
        QList<PdfReport::KeyValueRow> kv;
        kv.append({"被询问对象ID", ui->lineEdit_RecognizerID->text()});
        kv.append({"敌我识别提示", ui->lineEdit_IdentificationOfFriendOrFoePrompt->text()});
        kv.append({"告警提示", ui->lineEdit_WarningPrompt->text()});

        PdfReport report;
        report.setTitle("15综合告警检测设备  应答诸元测试");
        report.setSubtitle(QString("检测时间: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        report.addParagraph("一、应答诸元");
        report.addKeyValueGrid(kv);
        path = makeReportPath(modeName);
        report.exportPdf(path);
        break;
    }
    case Mode::ModeIFF:
    {
        modeName = "敌我识别器";
        QList<QStringList> rows;
        auto addCombo = [&](int idx, const QString &name, QComboBox *cb)
        {
            QString v = cb->currentText();
            addTableRowIf(rows, idx, name, "正常/故障", v, (v == "正常"));
        };
        addCombo(1, "主机", ui->comboBox_5);
        addCombo(2, "询问", ui->comboBox_6);
        addCombo(3, "应答", ui->comboBox_7);

        PdfReport report;
        report.setTitle("15综合告警检测设备  敌我识别器测试");
        report.setSubtitle(QString("检测时间: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        report.addParagraph("一、敌我识别器状态");
        report.addTable(headers, rows);
        path = makeReportPath(modeName);
        report.exportPdf(path);
        break;
    }
    case Mode::ModeIdentify:
    {
        modeName = "识别诸元";
        QList<QStringList> rows;
        auto add = [&](int idx, const QString &name, const QString &range, const QString &val, bool ok)
        {
            addTableRowIf(rows, idx, name, range, val, ok);
        };
        int n = 1;
        add(n++, "目标属性", "我方/不名", ui->lineEdit_TargetAttribute->text(),
            (!ui->lineEdit_TargetAttribute->text().isEmpty() && ui->lineEdit_TargetAttribute->text() != "-1"));
        add(n++, "目标个数", "0~7", ui->lineEdit_NumberOfTargets->text(),
            (ui->lineEdit_NumberOfTargets->text() != "-1"));
        add(n++, "目标匹配个数", "0~7", ui->lineEdit_NumberOfTargetMatches->text(),
            (ui->lineEdit_NumberOfTargetMatches->text() != "-1"));
        add(n++, "激光距离(m)", "uint16", ui->lineEdit_LaserDistance->text(),
            (ui->lineEdit_LaserDistance->text() != "-1"));

        QList<QStringList> mmRows;
        auto addMM = [&](int idx, const QString &name, QLineEdit *w)
        {
            addTableRowIf(mmRows, idx, name, "uint16", w->text(), (w->text() != "-1"));
        };
        addMM(1, "毫米波1距离(m)", ui->lineEdit_MillimeterWaveDistance1);
        addMM(2, "毫米波2距离(m)", ui->lineEdit_MillimeterWaveDistance2);
        addMM(3, "毫米波3距离(m)", ui->lineEdit_MillimeterWaveDistance3);
        addMM(4, "毫米波4距离(m)", ui->lineEdit_MillimeterWaveDistance4);
        addMM(5, "毫米波5距离(m)", ui->lineEdit_MillimeterWaveDistance5);

        PdfReport report;
        report.setTitle("15综合告警检测设备  识别诸元测试");
        report.setSubtitle(QString("检测时间: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        report.addParagraph("一、识别诸元基础参数");
        report.addTable(headers, rows);
        report.addParagraph("二、毫米波距离");
        report.addTable(headers, mmRows);
        path = makeReportPath(modeName);
        report.exportPdf(path);
        break;
    }
    default:
        return;
    }

    LOG(QString("%1报告已保存: %2").arg(modeName, path));
}
