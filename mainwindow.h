#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>

#include "flexraythread.h"
#include "circlelineedit.h"
#include "qtmaterialflatbutton.h"
#include "applogger.h"
#include "TestSelectionDialog.h"

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
    void closeAllDevices();

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void returnToSelection();
    void flexSend(QVector<uint16> rcvData_MP5, QVector<uint16> rcvData_WP1, QVector<uint16> rcvData_WP4);

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

    Ui::MainWindow *ui;

    FlexRayThread *m_Flexthread = nullptr;
    TestSelectionDialog::TestMode m_testMode = TestSelectionDialog::ModeNone;

    uint8_t m_cnt_SelfCheck = 0;
    QtMaterialFlatButton *pushButton_selfcheck = nullptr;
    QLineEdit *lineEdit_self = nullptr;

    QVector<double> m_TemprcvData4 = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

    QDateTime m_sessionStartTime;
    QString m_currentUser;

    bool m_abNodeAOk = false;
    bool m_abNodeBOk = false;
    bool m_returning = false;
};

#endif // MAINWINDOW_H
