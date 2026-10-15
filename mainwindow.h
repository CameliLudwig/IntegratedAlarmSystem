#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QFileInfo>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include "flexraythread.h"
#include "circlelineedit.h"
#include "qtmaterialflatbutton.h"
#include "applogger.h"
#include "TestSelectionDialog.h"
#include "gaugeCompass.h"
#include "pdfreportgenerator.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setTestMode(TestSelectionDialog::TestMode mode);
    void startFlexRayConnection();
    void closeAllDevices();

    // 生成会话PDF报告
    QString generateSessionPDF();

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void returnToSelection();
    void exitApplication();
    // void flexSend(QVector<uint16> rcvData_MP5, QVector<uint16> rcvData_WP1, QVector<uint16> rcvData_WP4);
    void flexSend(QVector<uint16> masterData, QVector<uint16> driveData);

public slots:
    void show_data(QVector<uint16> rcvData, int channel);
    void flexrayerror(QString error);
    void on_flexraysend();

private slots:
    void on_pushButton_close_clicked(bool checked);
    void on_pushButton_selfcheck_clicked(bool checked);

private:
    void applyVisibilityForMode();
    void generateModePDF();
    void initFlexRayConnections();
    void initDevices();
    void setAlarmWindows(int tmp1, int tmp2, QString mes1, QString mes2);

    Ui::MainWindow *ui;

    // FlexRayThread *m_Flexthread = nullptr;
    TestSelectionDialog::TestMode m_testMode = TestSelectionDialog::ModeNone;

    uint8_t m_cnt_SelfCheck = 0;
    QtMaterialFlatButton *pushButton_selfcheck = nullptr;
    QLineEdit *lineEdit_self = nullptr;

    // FlexRay worker thread
    QThread *m_flexRayWorkerThread = nullptr;
    FlexRayThread *m_flexRayThread = nullptr; // 启动任务 future （用于后台执行 startFlexRayOperation），保留以检查后台启动完成
    // FlexRay timers（由 MainWindow 管理）
    QTimer *m_flexray_reconnect_timer = nullptr;  // 3s 重连尝试
    QFuture<void> m_flexray_start_future;
    // Guard to prevent reentrant start requests from timers
    std::atomic<bool> m_flexStartInProgress{false};

    QVector<double> m_TemprcvData1 = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    QVector<uint16> m_TemprcvData4 = {0xFFFF, 0xFFFF};
    QVector<double> m_TemprcvData5 = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

    QDateTime m_sessionStartTime;
    QString m_currentUser;

    bool m_abNodeAOk = false;
    bool m_abNodeBOk = false;
    bool m_returning = false;
    bool m_flexRayStartRequested = false;
    std::atomic<bool> m_flexRayStartCancelled{false};
    QFuture<void> m_flexRayStartupFuture;

    GaugeCompass *gaugeCompass_ = nullptr;
    bool startTest_ = false;
};

#endif // MAINWINDOW_H
