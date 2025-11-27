#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QRegularExpression>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), adbProcess(new QProcess(this)), oneShotProcess(nullptr)
{
    ui->setupUi(this);

    connect(ui->pushButton_get_ip, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_getIp_clicked);
    connect(ui->pushButton_connect, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_connect_clicked);
    connect(ui->pushButton_disconnect, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_disconnect_clicked);
    /* pushButton_silent отсутствует в .ui — строка удалена */
}

MainWindow::~MainWindow()
{
    if (adbProcess && adbProcess->state() != QProcess::NotRunning)
        adbProcess->kill();
    delete ui;
}

void MainWindow::ensureOneShotProcess()
{
    if (!oneShotProcess) {
        oneShotProcess = new QProcess(this);
    }
}

QString MainWindow::findUsbDeviceId()
{
    QProcess p;
    p.start("adb", {"devices"});
    p.waitForFinished();

    QString out = p.readAllStandardOutput();
    QStringList lines = out.split("\n", Qt::SkipEmptyParts);

    for (const QString &l : lines) {
        if (l.contains("	device") && !l.contains(":")) {
            return l.split("\t").first(); // Исправлено на "\t"
        }
    }
    return "";
}

void MainWindow::runAdbShellCommand(const QString &device,
                                    const QString &command,
                                    std::function<void(const QString&, const QString&)> callback)
{
    ensureOneShotProcess();
    QProcess *p = oneShotProcess;

    // 1️⃣ — если процесс ещё выполняется, убиваем
    if (p->state() != QProcess::NotRunning) {
        p->kill();
        p->waitForFinished(300);
    }

    // 2️⃣ — сброс старых сигналов, иначе будет "дублирование" callback
    p->disconnect();

    // 3️⃣ — собрали adb kill-server / start-server
    QProcess::execute("adb", {"kill-server"});
    QProcess::execute("adb", {"start-server"});

    // 4️⃣ — формируем команду adb shell
    QStringList args;
    if (!device.isEmpty())
        args << "-s" << device;

    args << "shell" << command;

    connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int, QProcess::ExitStatus) {
                callback(p->readAllStandardOutput(), p->readAllStandardError());
            });

    // 5️⃣ — запускаем команду
    p->start("adb", args);
}



void MainWindow::setStatus(const QString &msg, int timeoutMs)
{
    statusBar()->showMessage(msg, timeoutMs);
}

// --- BUTTON HANDLERS --------------------------------------------------------

void MainWindow::on_pushButton_getIp_clicked()
{
    setStatus("Получаем IP...");

    currentUsbDeviceId = findUsbDeviceId();
    if (currentUsbDeviceId.isEmpty()) {
        setStatus("USB устройство не найдено");
        return;
    }

    runAdbShellCommand(currentUsbDeviceId,
        "ip addr show wlan0 | awk '/inet /{print $2}' | cut -d/ -f1",
        [&](const QString &out, const QString &err) {
            QString ip = out.trimmed();
            if (ip.isEmpty()) {
                setStatus("Не удалось получить IP. Wi-Fi включен?");
                return;
            }

            ui->lineEdit_ip->setText(ip);
            setStatus("IP получен: " + ip);
        }
    );
}

void MainWindow::on_pushButton_connect_clicked()
{
    QString ip = ui->lineEdit_ip->text().trimmed();
    if (ip.isEmpty()) {
        setStatus("Введите IP или нажмите 'Получить IP'");
        return;
    }
    if (!ip.contains(':')) ip += ":5555";

    currentUsbDeviceId = findUsbDeviceId();
    if (currentUsbDeviceId.isEmpty()) {
        setStatus("USB устройство не найдено");
        return;
    }

    // Enable TCP/IP mode
    QProcess::execute("adb", {"-s", currentUsbDeviceId, "tcpip", "5555"});

    QTimer::singleShot(800, this, [=]() {
        ensureOneShotProcess();
        QProcess *p = oneShotProcess;

        connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [=](int, QProcess::ExitStatus){
            QString out = p->readAllStandardOutput() + p->readAllStandardError();

            if (out.contains("connected") || out.contains("already")) {
                ui->checkBox_wireless->setChecked(true);
                setStatus("Подключено к " + ip);
            } else {
                ui->checkBox_wireless->setChecked(false);
                setStatus("Ошибка подключения: " + out);
            }
        });

        p->start("adb", {"connect", ip});
    });
}

void MainWindow::on_pushButton_disconnect_clicked()
{
    ensureOneShotProcess();
    QProcess *p = oneShotProcess;

    if (p->state() != QProcess::NotRunning) {
        p->kill();
        p->waitForFinished(300);
    }

    p->disconnect();

    connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=]{
                ui->checkBox_wireless->setChecked(false);
                setStatus("Отключено");
            });

    p->start("adb", {"disconnect"});
}
