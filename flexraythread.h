#ifndef FLEXRAYTHREAD_H
#define FLEXRAYTHREAD_H
#include<QMutex>
#include <QObject>
#include <QThread>
#include <QDebug>
#include <QWaitCondition>
#include "UBusCommLibWrapper.h"
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QDateTime>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <windows.h>
#include <atomic>

#define MaxSendNumber 5

class FlexRayThread : public QThread
{
    Q_OBJECT

public:
    FlexRayThread();  // 构造函数
    ~FlexRayThread();
    void startFlexRayOperation();
    void stop();
    void Init_instance();
    bool checkoutflexray();

    void setLogFile(const QString &path);
    void closeLogFile();
protected:
    void run() override;
signals:
    void flexRayDataReceived(QVector<uint16> rcvData,int num);  // 数据接收信号
    void error(QString flexerror);
    void connectionLost();           // 连接中断信号
    void connectionRestored();       // 自动重连成功
    void abChannelChanged(int chA, int chB);  // AB通道状态变化
    void abFrameTimeout();                      // 连续无帧, 强制灭AB


public slots:
    void flexRayDataSend(QVector<uint16> rcvData_MP5,QVector<uint16> rcvData_WP1,QVector<uint16> rcvData_WP4);
    void flexRayDataSend_MP5(QVector<uint16> sendData);
    void updatestate();

private:
    std::atomic<bool> m_running{false};
    QMutex m_mutex{QMutex::Recursive};  // 线程锁
    sint8 m_instance;//flexray句柄
    sint8 m_instance2;//flexray句柄
    uint16 m_rcvData[127];//接收的数据
    bool m_issend = 0;
    QVector<uint16> m_rcvData_MP5;
    QVector<uint16> m_rcvData_WP1;
    QVector<uint16> m_rcvData_WP4;
    QTimer *m_timer_state = nullptr;
    int m_errornum = 0;
    QTimer *m_timer_reconnect = nullptr; // 重连 timer
    QFile m_logFile;

    QString dataToHexStr(const QVector<uint16> &data) const;
    void writeLog(const QString &line);

};

#endif // FLEXRAYTHREAD_H
