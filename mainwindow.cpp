#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QTimer>
#include <QDebug>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton_get_ip, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_get_ip_clicked);
    connect(ui->pushButton_connect, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_connect_clicked);
    connect(ui->pushButton_disconnect, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_disconnect_clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* ----------------------------------------------------------------------
 *  ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
 * --------------------------------------------------------------------*/

/// Запуск ADB с callback’ом, каждый вызов — свой процесс (безопасно)
void MainWindow::runAdb(const QStringList &args,
                        std::function<void(QString,QString)> callback)
{
    QProcess *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::SeparateChannels);

    logCmd(args.join(" "));  // ЛОГИРУЕМ КОМАНДУ

    connect(p,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, p, callback](int exitCode, QProcess::ExitStatus status){

        QString out = p->readAllStandardOutput();
        QString err = p->readAllStandardError();

        if (!out.trimmed().isEmpty())
            logInfo(out.trimmed());

        if (!err.trimmed().isEmpty())
            logError(err.trimmed());

        if (exitCode != 0)
            logError("Exit code: " + QString::number(exitCode));

        p->deleteLater();
        callback(out, err);
    });

    p->start("adb", args);
}


/// Поиск USB-устройства
QString MainWindow::findUsbDeviceId()
{
    QProcess p;
    p.start("adb", {"devices"});
    p.waitForFinished();

    QString out = p.readAllStandardOutput();
    QStringList lines = out.split("\n", Qt::SkipEmptyParts);

    for (const QString &l : lines) {
        if (l.contains("\tdevice") && !l.contains(":")) {
            return l.split("\t").first();
        }
    }
    return "";
}

/// Вывод в статус-бар
void MainWindow::setStatus(const QString &msg, int timeoutMs)
{
    statusBar()->showMessage(msg, timeoutMs);
}

/* ----------------------------------------------------------------------
 *  ОБРАБОТЧИКИ КНОПОК
 * --------------------------------------------------------------------*/

/// Получение IP через USB
void MainWindow::on_pushButton_get_ip_clicked()
{
    setStatus("Получаем IP...");

    QString usb = findUsbDeviceId();
    if (usb.isEmpty()) {
        setStatus("USB устройство не найдено");
        return;
    }

 logInfo("Получение IP через USB…");

    runAdb({ "-s", usb, "shell",
             "ip addr show wlan0 | awk '/inet /{print $2}' | cut -d/ -f1"
           },
           [this](QString out, QString err){

        QString ip = out.trimmed();
        if (ip.isEmpty()) {
            setStatus("Не удалось получить IP. Wi-Fi включен?");
            return;
        }

        ui->lineEdit_ip->setText(ip);
        setStatus("IP получен: " + ip);
    });
}

/// Подключение по Wi-Fi
void MainWindow::on_pushButton_connect_clicked()
{

logInfo("Настройка TCP/IP режима...");

    QString ip = ui->lineEdit_ip->text().trimmed();
    if (ip.isEmpty()) {
        setStatus("Введите IP или нажмите 'Получить IP'");
        return;
    }

    if (!ip.contains(":"))
        ip += ":5555";

    QString usb = findUsbDeviceId();
    if (usb.isEmpty()) {
        setStatus("USB устройство не найдено");
        return;
    }

    // 1. Включаем TCP/IP режим
    runAdb({ "-s", usb, "tcpip", "5555" },
           [this, ip](QString out, QString err) {

        Q_UNUSED(out)
        Q_UNUSED(err)

        // 2. Даём устройству время переключиться в tcpip
        QTimer::singleShot(600, this, [this, ip]() {

            runAdb({ "adb", "connect", ip },
                   [this, ip](QString out, QString err){

                QString res = out + err;

                if (res.contains("connected") || res.contains("already")) {
                    ui->checkBox_wireless->setChecked(true);
                    setStatus("Подключено к " + ip);
                } else {
                    ui->checkBox_wireless->setChecked(false);
                    setStatus("Ошибка подключения: " + res);
                }
            });
        });
    });
}

/// Отключение Wi-Fi соединения
void MainWindow::on_pushButton_disconnect_clicked()
{

logInfo("Отключение устройства...");
    runAdb({ "disconnect" },
           [this](QString out, QString err) {

        Q_UNUSED(out)
        Q_UNUSED(err)

        ui->checkBox_wireless->setChecked(false);
        setStatus("Отключено");
    });
}

void MainWindow::logInfo(const QString &msg)
{
    ui->textEdit_log->append(
        "<span style='color:#2ECC71;'>🟢 " + msg + "</span>");
}

void MainWindow::logError(const QString &msg)
{
    ui->textEdit_log->append(
        "<span style='color:#E74C3C;'>🔴 " + msg + "</span>");
}

void MainWindow::logCmd(const QString &cmd)
{
    ui->textEdit_log->append(
        "<span style='color:#F1C40F;'>💻 adb " + cmd + "</span>");
}

