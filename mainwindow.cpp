// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    if (adbProcess) {
        adbProcess->kill();
        adbProcess->deleteLater();
    }
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    // Если уже запущен процесс — убиваем старый
    if (adbProcess && adbProcess->state() != QProcess::NotRunning) {
        adbProcess->kill();
    } else if (!adbProcess) {
        adbProcess = new QProcess(this);
    }

    // Подключаем сигналы (один раз достаточно, но можно и каждый раз)
    connect(adbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
                Q_UNUSED(exitCode);
                Q_UNUSED(exitStatus);

                QString output = adbProcess->readAllStandardOutput().trimmed();
                QStringList lines = output.split('\n', Qt::SkipEmptyParts);

                if (!lines.isEmpty()) {
                    QString ip = lines.first(); // берём первую строку (обычно одна)
                    ui->lineEdit_ip->setText(ip);
                    statusBar()->showMessage("IP получен: " + ip, 3000);
                } else {
                    ui->lineEdit_ip->setText("Нет IP");
                    statusBar()->showMessage("wlan0 не найден или нет IP", 5000);
                }
            });

    connect(adbProcess, &QProcess::errorOccurred, this, [=](QProcess::ProcessError error) {
        Q_UNUSED(error);
        ui->lineEdit_ip->setText("Ошибка ADB");
        statusBar()->showMessage("ADB не запущен или устройство не подключено", 7000);
    });

    // Запускаем команду
    adbProcess->start("adb", QStringList() << "shell"
                                           << "ip addr show wlan0 2>/dev/null | awk '/inet /{print $2}' | cut -d/ -f1");
}

void MainWindow::on_pushButton_2_clicked()
{
    // Здесь будет подключение по IP, например:
    QString ip = ui->lineEdit_ip->text().trimmed();
    if (ip.isEmpty() || ip == "Нет IP" || ip == "Ошибка ADB") {
        statusBar()->showMessage("Сначала получите IP", 3000);
        return;
    }

    QProcess::execute("adb", QStringList() << "tcpip" << "5555");
    QProcess::execute("adb", QStringList() << "connect" << ip + ":5555");

    statusBar()->showMessage("Пытаемся подключиться к " + ip + ":5555", 5000);
}

void MainWindow::on_lineEdit_ip_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    // Можно добавить подсветку валидного IP и т.д.
}
