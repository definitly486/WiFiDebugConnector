#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_get_ip_clicked();
    void on_pushButton_connect_clicked();
    void on_pushButton_disconnect_clicked();
  

private:
    Ui::MainWindow *ui;

    QProcess *adbProcess;        // single reusable process for interactive commands
    QProcess *oneShotProcess;    // for short commands that we need synchronously
    QString currentUsbDeviceId;  // cached device id when detected

    void ensureOneShotProcess();
    QString findUsbDeviceId();
    void runAdbShellCommand(const QString &deviceOrEmpty, const QString &command,
                             std::function<void(const QString&stdout, const QString&stderr)> callback);

    void setStatus(const QString &msg, int timeoutMs = 4000);
};

#endif // MAINWINDOW_H