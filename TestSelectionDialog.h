#ifndef TESTSELECTIONDIALOG_H
#define TESTSELECTIONDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

class TestSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    enum TestMode {
        ModeNone = 0,
        ModeLaser,
        ModeHostStatus,
        ModeMP5,
        ModeAnswer,
        ModeIFF,
        ModeIdentify
    };

    explicit TestSelectionDialog(const QString &userName, QWidget *parent = nullptr);
    ~TestSelectionDialog();

    TestMode selectedMode() const { return m_selectedMode; }

signals:
    void testModeSelected(TestMode mode);
    void requestReturnToLogin();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onLaserClicked();
    void onHostStatusClicked();
    void onMP5Clicked();
    void onAnswerClicked();
    void onIFFClicked();
    void onIdentifyClicked();
    void onReturnClicked();

private:
    void setupUI();
    void applyStyle();

    TestMode m_selectedMode;
    QString  m_userName;

    QFrame     *m_panelFrame;
    QLabel     *m_titleLabel;
    QLabel     *m_userLabel;
    QLabel     *m_subtitleLabel;
    QPushButton *m_laserButton;
    QPushButton *m_hostStatusButton;
    QPushButton *m_mp5Button;
    QPushButton *m_answerButton;
    QPushButton *m_iffButton;
    QPushButton *m_identifyButton;
    QPushButton *m_returnButton;
};

#endif // TESTSELECTIONDIALOG_H
