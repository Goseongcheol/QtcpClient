#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QString& serverIp, quint16 serverPort, const QString& clientIp, quint16 clientPort, QString userId, QString userName,QWidget *parent = nullptr);
    ~MainWindow();



private slots:
    void connected();
    void readyRead();


private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;

    QString client_serverIp;
    quint16 client_serverPort;
    QString client_clientIp;
    quint16 client_clientPort;
    QString client_userId;
    QString client_userName;
};
#endif // MAINWINDOW_H
