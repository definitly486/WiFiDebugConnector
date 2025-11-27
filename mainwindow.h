// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>          // ← ЭТО ОБЯЗАТЕЛЬНО!

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
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_lineEdit_ip_textChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
    QProcess *adbProcess = nullptr;   // ← вот сюда объявляем указатель
};

#endif // MAINWINDOW_H
