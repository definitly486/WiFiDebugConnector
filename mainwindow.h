#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private:
    // Универсальная функция безопасного запуска adb
    void runAdb(const QStringList &args,
                std::function<void(QString,QString)> callback);

    // Поиск USB-устройства
    QString findUsbDeviceId();

    // Отображение сообщения в статус-баре
    void setStatus(const QString &msg, int timeoutMs = 2000);

private slots:
    void on_pushButton_get_ip_clicked();
    void on_pushButton_connect_clicked();
    void on_pushButton_disconnect_clicked();
};

#endif // MAINWINDOW_H
