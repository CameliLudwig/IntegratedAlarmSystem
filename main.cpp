#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include "UserAdminDialog.h"
#include "TestSelectionDialog.h"
#include "mainwindow.h"
// #include "threadnaming.h"

int main(int argc, char *argv[])
{
    // ThreadNaming::setCurrentThreadName(L"Main Thread");
    QApplication a(argc, argv);
    bool exitApplication = false;

    while (!exitApplication) {
        UserAdminDialog loginDlg;
        if (loginDlg.exec() != QDialog::Accepted)
            break;

        MainWindow mainWin;
        mainWin.showMaximized();
        mainWin.startFlexRayConnection();

        QEventLoop waitLoop;
        QMetaObject::Connection returnConnection = QObject::connect(
            &mainWin, &MainWindow::returnToSelection,
            &waitLoop, &QEventLoop::quit);
        QMetaObject::Connection exitConnection = QObject::connect(
            &mainWin, &MainWindow::exitApplication,
            [&waitLoop, &exitApplication]() {
                exitApplication = true;
                waitLoop.quit();
            });
        waitLoop.exec();
        QObject::disconnect(returnConnection);
        QObject::disconnect(exitConnection);

        mainWin.closeAllDevices();
    }

    return 0;
}
