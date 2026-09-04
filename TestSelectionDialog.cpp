#include "TestSelectionDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <QDebug>

TestSelectionDialog::TestSelectionDialog(const QString &userName, QWidget *parent)
    : QDialog(parent)
    , m_selectedMode(ModeNone)
    , m_userName(userName)
{
    setupUI();
    applyStyle();
}

TestSelectionDialog::~TestSelectionDialog() {}

void TestSelectionDialog::setupUI()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setWindowState(Qt::WindowFullScreen);
    setObjectName("TestSelectionDialog");

    {
        QFont f = this->font();
        f.setPointSize(12);
#ifdef Q_OS_WIN
        f.setFamily(QStringLiteral("Microsoft YaHei UI"));
#endif
        this->setFont(f);
    }

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setAlignment(Qt::AlignCenter);

    m_panelFrame = new QFrame(this);
    m_panelFrame->setObjectName("panelFrame");
    m_panelFrame->setFixedSize(700, 620);

    QVBoxLayout *panelLayout = new QVBoxLayout(m_panelFrame);
    panelLayout->setAlignment(Qt::AlignCenter);
    panelLayout->setSpacing(12);
    panelLayout->setContentsMargins(40, 30, 40, 30);

    m_titleLabel = new QLabel(QString::fromUtf8("综合告警与干扰检测设备"), this);
    m_titleLabel->setObjectName("title");
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_userLabel = new QLabel(QString::fromUtf8("当前用户：%1").arg(m_userName), this);
    m_userLabel->setObjectName("userLabel");
    m_userLabel->setAlignment(Qt::AlignCenter);

    m_subtitleLabel = new QLabel(QString::fromUtf8("请选择测试项目"), this);
    m_subtitleLabel->setObjectName("subtitleLabel");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(12);
    buttonLayout->setContentsMargins(30, 5, 30, 5);

    QHBoxLayout *row1 = new QHBoxLayout();
    row1->setSpacing(15);
    m_laserButton = new QPushButton(QString::fromUtf8("激光告警自检"), this);
    m_laserButton->setObjectName("laserButton");
    m_laserButton->setMinimumHeight(60);
    m_laserButton->setCursor(Qt::PointingHandCursor);

    m_hostStatusButton = new QPushButton(QString::fromUtf8("主机状态"), this);
    m_hostStatusButton->setObjectName("hostStatusButton");
    m_hostStatusButton->setMinimumHeight(60);
    m_hostStatusButton->setCursor(Qt::PointingHandCursor);

    m_mp5Button = new QPushButton(QString::fromUtf8("任务核心处理机设置"), this);
    m_mp5Button->setObjectName("mp5Button");
    m_mp5Button->setMinimumHeight(60);
    m_mp5Button->setCursor(Qt::PointingHandCursor);

    row1->addWidget(m_laserButton);
    row1->addWidget(m_hostStatusButton);
    row1->addWidget(m_mp5Button);

    QHBoxLayout *row2 = new QHBoxLayout();
    row2->setSpacing(15);
    m_answerButton = new QPushButton(QString::fromUtf8("应答诸元"), this);
    m_answerButton->setObjectName("answerButton");
    m_answerButton->setMinimumHeight(60);
    m_answerButton->setCursor(Qt::PointingHandCursor);

    m_iffButton = new QPushButton(QString::fromUtf8("敌我识别器"), this);
    m_iffButton->setObjectName("iffButton");
    m_iffButton->setMinimumHeight(60);
    m_iffButton->setCursor(Qt::PointingHandCursor);

    m_identifyButton = new QPushButton(QString::fromUtf8("识别诸元"), this);
    m_identifyButton->setObjectName("identifyButton");
    m_identifyButton->setMinimumHeight(60);
    m_identifyButton->setCursor(Qt::PointingHandCursor);

    row2->addWidget(m_answerButton);
    row2->addWidget(m_iffButton);
    row2->addWidget(m_identifyButton);

    buttonLayout->addLayout(row1);
    buttonLayout->addLayout(row2);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(30, 8, 30, 0);
    m_returnButton = new QPushButton(QString::fromUtf8("返回登录"), this);
    m_returnButton->setObjectName("returnButton");
    m_returnButton->setMinimumHeight(42);
    m_returnButton->setFixedWidth(200);
    m_returnButton->setCursor(Qt::PointingHandCursor);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_returnButton);
    bottomLayout->addStretch();

    panelLayout->addSpacing(5);
    panelLayout->addWidget(m_titleLabel);
    panelLayout->addSpacing(3);
    panelLayout->addWidget(m_userLabel);
    panelLayout->addSpacing(3);
    panelLayout->addWidget(m_subtitleLabel);
    panelLayout->addSpacing(15);
    panelLayout->addLayout(buttonLayout);
    panelLayout->addSpacing(8);
    panelLayout->addLayout(bottomLayout);
    panelLayout->addSpacing(5);

    outerLayout->addWidget(m_panelFrame);

    connect(m_laserButton,      &QPushButton::clicked, this, &TestSelectionDialog::onLaserClicked);
    connect(m_hostStatusButton, &QPushButton::clicked, this, &TestSelectionDialog::onHostStatusClicked);
    connect(m_mp5Button,        &QPushButton::clicked, this, &TestSelectionDialog::onMP5Clicked);
    connect(m_answerButton,     &QPushButton::clicked, this, &TestSelectionDialog::onAnswerClicked);
    connect(m_iffButton,        &QPushButton::clicked, this, &TestSelectionDialog::onIFFClicked);
    connect(m_identifyButton,   &QPushButton::clicked, this, &TestSelectionDialog::onIdentifyClicked);
    connect(m_returnButton,     &QPushButton::clicked, this, &TestSelectionDialog::onReturnClicked);

    if (auto *shadow = new QGraphicsDropShadowEffect(this)) {
        shadow->setBlurRadius(40);
        shadow->setOffset(0, 10);
        shadow->setColor(QColor(0, 0, 0, 80));
        m_panelFrame->setGraphicsEffect(shadow);
    }
}

void TestSelectionDialog::applyStyle()
{
    this->setStyleSheet(R"(
        QDialog#TestSelectionDialog {
            border-image: url(:/test.png);
        }
        #panelFrame {
            background-color: rgba(255, 255, 255, 0.96);
            border-radius: 16px;
        }
        #title {
            color: #0f2027;
            font-size: 24px;
            font-weight: bold;
            letter-spacing: 2px;
            background: transparent;
        }
        #userLabel {
            color: #555;
            font-size: 15px;
            background: transparent;
            font-weight: normal;
        }
        #subtitleLabel {
            color: #888;
            font-size: 17px;
            background: transparent;
            font-weight: normal;
            letter-spacing: 1px;
        }
        #laserButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #1E88E5, stop:1 #42A5F5);
            color: white;
            border: none;
            border-radius: 12px;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 2px;
        }
        #laserButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #1565C0, stop:1 #1E88E5);
        }
        #laserButton:pressed {
            background: #0D47A1;
            padding-top: 4px;
        }
        #hostStatusButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #2E7D32, stop:1 #43A047);
            color: white;
            border: none;
            border-radius: 12px;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 2px;
        }
        #hostStatusButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #1B5E20, stop:1 #2E7D32);
        }
        #hostStatusButton:pressed {
            background: #1B5E20;
            padding-top: 4px;
        }
        #mp5Button {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #1565C0, stop:1 #1E88E5);
            color: white;
            border: none;
            border-radius: 12px;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 2px;
        }
        #mp5Button:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #0D47A1, stop:1 #1565C0);
        }
        #mp5Button:pressed {
            background: #0D47A1;
            padding-top: 4px;
        }
        #answerButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #E65100, stop:1 #FF9800);
            color: white;
            border: none;
            border-radius: 12px;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 2px;
        }
        #answerButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #BF360C, stop:1 #E65100);
        }
        #answerButton:pressed {
            background: #BF360C;
            padding-top: 4px;
        }
        #iffButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #6A1B9A, stop:1 #AB47BC);
            color: white;
            border: none;
            border-radius: 12px;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 2px;
        }
        #iffButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #4A148C, stop:1 #8E24AA);
        }
        #iffButton:pressed {
            background: #4A148C;
            padding-top: 4px;
        }
        #identifyButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #C62828, stop:1 #EF5350);
            color: white;
            border: none;
            border-radius: 12px;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 2px;
        }
        #identifyButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #B71C1C, stop:1 #E53935);
        }
        #identifyButton:pressed {
            background: #B71C1C;
            padding-top: 4px;
        }
        #returnButton {
            background: rgba(0, 0, 0, 0.08);
            color: #666;
            border: 1px solid #ddd;
            border-radius: 8px;
            font-size: 15px;
            font-weight: bold;
        }
        #returnButton:hover {
            background: rgba(0, 0, 0, 0.15);
            color: #333;
        }
        #returnButton:pressed {
            background: rgba(0, 0, 0, 0.25);
        }
    )");
}

void TestSelectionDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) return;
    QDialog::keyPressEvent(event);
}

void TestSelectionDialog::onLaserClicked()
{
    m_selectedMode = ModeLaser;
    emit testModeSelected(ModeLaser);
    accept();
}
void TestSelectionDialog::onHostStatusClicked()
{
    m_selectedMode = ModeHostStatus;
    emit testModeSelected(ModeHostStatus);
    accept();
}
void TestSelectionDialog::onMP5Clicked()
{
    m_selectedMode = ModeMP5;
    emit testModeSelected(ModeMP5);
    accept();
}
void TestSelectionDialog::onAnswerClicked()
{
    m_selectedMode = ModeAnswer;
    emit testModeSelected(ModeAnswer);
    accept();
}
void TestSelectionDialog::onIFFClicked()
{
    m_selectedMode = ModeIFF;
    emit testModeSelected(ModeIFF);
    accept();
}
void TestSelectionDialog::onIdentifyClicked()
{
    m_selectedMode = ModeIdentify;
    emit testModeSelected(ModeIdentify);
    accept();
}
void TestSelectionDialog::onReturnClicked()
{
    m_selectedMode = ModeNone;
    emit requestReturnToLogin();
    reject();
}
