#include "mainwindow.h"
#include "threadnaming.h"
#include "ui_mainwindow.h"
#include "PdfReport.h"
#include "applogger.h"

#include <QStyleFactory>
#include <QFile>
#include <QCloseEvent>

#include "qtmaterialflatbutton.h"

uint16_t SetBit(uint16_t num, int bitIndex, bool b) {
    if (bitIndex < 0 || bitIndex > 15) return num;
    if (b) num |= (1 << bitIndex);
    else  num &= ~(1 << bitIndex);
    return num;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
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

    foreach (QLineEdit *lineEdit, findChildren<QLineEdit*>()) {
        lineEdit->setText("-1");
        lineEdit->setAlignment(Qt::AlignCenter);
    }

    // flexray线程
    m_Flexthread = new FlexRayThread();
    m_Flexthread->setLogFile(QDir::currentPath() + "/FlexRay_log.txt");
    connect(m_Flexthread, &FlexRayThread::error, this, &MainWindow::flexrayerror);
    connect(this, &MainWindow::flexSend, m_Flexthread, &FlexRayThread::flexRayDataSend);
    connect(m_Flexthread, &FlexRayThread::flexRayDataReceived, this,
            static_cast<void (MainWindow::*)(QVector<short unsigned int>, int)>(&MainWindow::show_data));
    connect(m_Flexthread, &FlexRayThread::abChannelChanged, this, [this](int chA, int chB) {
        m_abNodeAOk = (chA != 0);
        m_abNodeBOk = (chB != 0);
    });
    connect(m_Flexthread, &FlexRayThread::abFrameTimeout, this, [this]() {
        m_abNodeAOk = false;
        m_abNodeBOk = false;
    });
    connect(m_Flexthread, &FlexRayThread::connectionLost, this, []() {
        LOG("FlexRay连接丢失");
    });
    connect(m_Flexthread, &FlexRayThread::connectionRestored, this, []() {
        LOG("FlexRay连接恢复");
    });

    QTimer::singleShot(500, this, [this]() {
        QtConcurrent::run([this]() {
            ThreadNaming::setCurrentThreadName(L"FlexRay Startup Worker");
            LOG("开始启动FlexRay...");
            m_Flexthread->startFlexRayOperation();
        });
    });

    auto flatButtons = this->findChildren<QtMaterialFlatButton*>();
    for (QtMaterialFlatButton *btn : flatButtons) {
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

    LOG("=== MainWindow 构造完成 ===");
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
    ui->groupBox_2->setVisible(false);   // 激光告警
    ui->groupBox_23->setVisible(false);  // 烟幕弹状态
    ui->groupBox_30->setVisible(false);  // 主机状态(含MP5)
    ui->groupBox_42->setVisible(false);  // 诸元类型等
    ui->groupBox_46->setVisible(false);  // 应答诸元
    ui->groupBox_28->setVisible(false);  // 敌我识别器
    ui->groupBox_32->setVisible(false);  // 识别诸元
    ui->groupBox_41->setVisible(false);  // 告警诸元

    // 隐藏groupBox_30的非MP5子元素 (用于MP5模式)
    // groupBox_27是groupBox_30的子元素
    ui->groupBox_27->setVisible(true);

    auto hideGroup30Children = [this](bool hide) {
        // 隐藏groupBox_30中非groupBox_27的直接子控件
        QLayout *lay = ui->groupBox_30->layout();
        if (!lay) return;
        for (int i = 0; i < lay->count(); i++) {
            QLayoutItem *item = lay->itemAt(i);
            if (!item) continue;
            QWidget *w = item->widget();
            if (w && w != ui->groupBox_27) {
                w->setVisible(!hide);
            }
        }
    };

    switch (m_testMode) {
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
    LOG("FlexRay Error: " + error);
    QMessageBox::warning(this, "FlexRay错误", error);
    m_Flexthread->stop();
}

void MainWindow::show_data(QVector<uint16> rcvData, int channel)
{
    if (channel == 1) {
        ui->lineEdit_ReadKeyGroupParameters->setText(QString::number(rcvData.at(2)));
        ui->lineEdit_ReadTheWorkingStatusOfFriendlyAndEnemyForces->setText(QString::number(rcvData.at(3)));
        if (rcvData.at(4) >= 1 && rcvData.at(4) <= 4)
            ui->comboBox_setTopMode->setCurrentIndex(static_cast<int>(rcvData.at(4)));
        else
            ui->comboBox_setTopMode->setCurrentIndex(0);
        ui->lineEdit_ReadStealthCoating->setText(rcvData.at(6) == 1 ? "无涂层" : "有涂层");

        ui->lineEdit_sl1->setText(QString::number((rcvData.at(10) >> 0) & 0x01));
        ui->lineEdit_sl2->setText(QString::number((rcvData.at(10) >> 1) & 0x01));
        ui->lineEdit_sl3->setText(QString::number((rcvData.at(10) >> 2) & 0x01));
        ui->lineEdit_sl4->setText(QString::number((rcvData.at(10) >> 3) & 0x01));
        ui->lineEdit_sr1->setText(QString::number((rcvData.at(11) >> 0) & 0x01));
        ui->lineEdit_sr2->setText(QString::number((rcvData.at(11) >> 1) & 0x01));
        ui->lineEdit_sr3->setText(QString::number((rcvData.at(11) >> 2) & 0x01));
        ui->lineEdit_sr4->setText(QString::number((rcvData.at(11) >> 3) & 0x01));

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
        ui->lineEdit_hmbRightRear->setText(QString::number(rcvData.at(19) & 0x03));
    }
    else if (channel == 3) {
        QStringList type = {"告警诸元","识别诸元","应答诸元","火警诸元"};
        int typeID = rcvData.at(0);
        ui->lineEdit_TypesOfElements->setText(type[rcvData.at(0)]);
        ui->lineEdit_ZhuyuanSerialNumber->setText(QString::number(rcvData.at(1)));
        ui->lineEdit_ZhuyuanBatch->setText(QString::number(rcvData.at(2)));
        ui->lineEdit_ZhuyuanInterceptionh->setText(QString::number(rcvData.at(3)));
        ui->lineEdit_ZhuyuanInterceptionm->setText(QString::number(rcvData.at(4)));
        ui->lineEdit_ZhuyuanInterceptions->setText(QString::number(rcvData.at(5)));

        switch (typeID) {
        case 0: {
            uint16_t azimuth = (uchar)rcvData.at(6) | ((uchar)rcvData.at(7) << 8);
            ui->lineEdit_AlarmDirection->setText(azimuth == 0xFFFF ? "无效" : QString::number(azimuth));
            QStringList gjTypes = {"NULL","单脉冲","重频","连续波","编码","毫米波","综合","保留"};
            int gjTypeVal = (uchar)rcvData.at(9);
            ui->lineEdit_AlarmType->setText(gjTypeVal < gjTypes.size() ? gjTypes[gjTypeVal] : "其他");
            ui->comboBox_DriverAvoidDirection->setCurrentText(QString::number((uchar)rcvData.at(10)));
            break;
        }
        case 1: {
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
        case 2: {
            uint32_t id = ((uint32_t)(uchar)rcvData.at(6) << 16) +
                          ((uint32_t)(uchar)rcvData.at(9) << 8) +
                          (uint32_t)(uchar)rcvData.at(8);
            ui->lineEdit_RecognizerID->setText(QString::number(id));
            break;
        }
        default: break;
        }
    }
    else if (channel == 4) {
        QStringList diwsbTips = {"无提示","我方","不名","识别中","故障"};
        int dIdx = (uchar)rcvData.at(0);
        ui->lineEdit_IdentificationOfFriendOrFoePrompt->setText(dIdx < diwsbTips.size() ? diwsbTips[dIdx] : "未知");
        QStringList gjTips = {"无显示","告警","火警","告警且火警"};
        int gIdx = (uchar)rcvData.at(1);
        ui->lineEdit_WarningPrompt->setText(gIdx < gjTips.size() ? gjTips[gIdx] : "未知");
    }
    else if (channel == 2) {
        if (rcvData.size() < 2) return;
        if (rcvData.at(0) == m_TemprcvData4.at(0) && rcvData.at(1) == m_TemprcvData4.at(1)) return;
        m_TemprcvData4[0] = rcvData.at(0);
        m_TemprcvData4[1] = rcvData.at(1);
        int abChannel = rcvData.at(0);
        ui->lineEdit_15->setText(m_abNodeAOk && (abChannel & 0x01) ? "1" : "0");
        ui->lineEdit_16->setText(m_abNodeBOk && (abChannel & 0x02) ? "1" : "0");
    }
}

void MainWindow::on_flexraysend()
{
    QVector<uint16_t> rcvData_MP5, rcvData_WP1, rcvData_WP4;
    uint16_t Data_MP5[5] = {0};
    uint16_t Data_WP1[5] = {0};
    uint16_t Data_WP4[2] = {0};

    for (int i = 0; i < 8; i++)
        Data_MP5[0] = SetBit(Data_MP5[0], i, (m_cnt_SelfCheck >> i) & 1);

    uint8_t val_Key = ui->comboBox_KeyGroup->currentIndex();
    for (int i = 0; i < 8; i++)
        Data_MP5[1] = SetBit(Data_MP5[1], i, (val_Key >> i) & 1);

    uint8_t val_DiWSBStatus = ui->comboBox_setWorkingCondition->currentIndex();
    for (int i = 0; i < 8; i++)
        Data_MP5[1] = SetBit(Data_MP5[1], 8 + i, (val_DiWSBStatus >> i) & 1);

    uint8_t val_DingBu = ui->comboBox_setTopMode->currentIndex();
    for (int i = 0; i < 8; i++)
        Data_MP5[2] = SetBit(Data_MP5[2], i, (val_DingBu >> i) & 1);

    for (int i = 0; i < 8; i++)
        Data_MP5[2] = SetBit(Data_MP5[2], 8 + i, (0 >> i) & 1);

    for (int i = 0; i < 4; i++) {
        Data_MP5[4] = SetBit(Data_MP5[4], i, (1 >> i) & 1);
        Data_MP5[4] = SetBit(Data_MP5[4], 4 + i, (1 >> i) & 1);
    }
    for (int i = 0; i < 4; i++)
        Data_MP5[4] = SetBit(Data_MP5[4], 12 + i, (1 >> i) & 1);

    for (int i = 0; i < 5; i++) rcvData_MP5.append(Data_MP5[i]);

    for (int i = 0; i < 8; i++)
        Data_WP1[2] = SetBit(Data_WP1[2], i, (0 >> i) & 1);
    for (int i = 0; i < 8; i++)
        Data_WP1[4] = SetBit(Data_WP1[4], i, (0 >> i) & 1);
    for (int i = 0; i < 8; i++)
        Data_WP1[4] = SetBit(Data_WP1[4], 8 + i, (0 >> i) & 1);
    for (int i = 0; i < 5; i++) rcvData_WP1.append(Data_WP1[i]);

    for (int i = 0; i < 2; i++) rcvData_WP4.append(Data_WP4[i]);

    emit flexSend(rcvData_MP5, rcvData_WP1, rcvData_WP4);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    if (m_returning) return;
    m_returning = true;
    LOG("用户点击了窗口关闭(X)按钮");
    generateModePDF();
    emit returnToSelection();
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
    if (m_Flexthread) {
        LOG("停止 FlexRay...");
        m_Flexthread->stop();
        m_Flexthread->wait(3000);
    }
    LOG("=== 所有设备已关闭 ===");
}

void MainWindow::on_pushButton_close_clicked(bool checked)
{
    Q_UNUSED(checked);
    if (m_returning) return;
    m_returning = true;
    LOG("用户点击了「返回」按钮");
    generateModePDF();
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

static QString makeReportPath(const QString& testName)
{
    QString dir = QApplication::applicationDirPath() + "/reports";
    QDir().mkpath(dir);
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1/%2_%3.pdf").arg(dir, ts, testName);
}

static void addTableRowIf(QList<QStringList>& rows, int idx, const QString& name,
                          const QString& range, const QString& val, bool ok)
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

    switch (m_testMode) {
    case Mode::ModeLaser: {
        modeName = "激光告警自检";
        QList<QStringList> rows;
        auto add = [&](int idx, const QString& name, QLineEdit* w) {
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
    case Mode::ModeHostStatus: {
        modeName = "主机状态";
        QList<QStringList> rows;
        auto add = [&](int idx, const QString& name, const QString& range, const QString& val, bool ok) {
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
        auto addMM = [&](int idx, const QString& name, QLineEdit* w) {
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
    case Mode::ModeMP5: {
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
    case Mode::ModeAnswer: {
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
    case Mode::ModeIFF: {
        modeName = "敌我识别器";
        QList<QStringList> rows;
        auto addCombo = [&](int idx, const QString& name, QComboBox* cb) {
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
    case Mode::ModeIdentify: {
        modeName = "识别诸元";
        QList<QStringList> rows;
        auto add = [&](int idx, const QString& name, const QString& range, const QString& val, bool ok) {
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
        auto addMM = [&](int idx, const QString& name, QLineEdit* w) {
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
