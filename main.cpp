#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include "UserAdminDialog.h"
#include "TestSelectionDialog.h"
#include "mainwindow.h"
#include "threadnaming.h"

int main(int argc, char *argv[])
{
    ThreadNaming::setCurrentThreadName(L"Main Thread");
    QApplication a(argc, argv);

    MainWindow mainWin;

    while (true) {
        UserAdminDialog loginDlg;
        if (loginDlg.exec() != QDialog::Accepted)
            break;
        QString userName = loginDlg.currentUser();

        bool backToLogin = false;
        while (!backToLogin) {
            // TestSelectionDialog testDlg(userName);
            // int result = testDlg.exec();
            // if (result != QDialog::Accepted) {
            //     backToLogin = true;
            //     continue;
            // }

            // mainWin.setTestMode(testDlg.selectedMode());
            mainWin.showMaximized();

            QEventLoop waitLoop;
            QObject::connect(&mainWin, &MainWindow::returnToSelection,
                             &waitLoop, &QEventLoop::quit);
            waitLoop.exec();
            QObject::disconnect(&mainWin, &MainWindow::returnToSelection,
                                &waitLoop, &QEventLoop::quit);

            mainWin.hide();
            QApplication::processEvents();
        }
    }

    mainWin.closeAllDevices();
    return 0;
}
