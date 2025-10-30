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
    MainWindow(const QString& serverIp, quint16 serverPort, const QString& clientIp, quint16 clientPort, QString userId, QString userName, const QString& filePath, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void connected();
    void readyRead();

    void writeLog(quint8 CMD, QString data);

    void on_loginButton_clicked();
    void on_sendButton_clicked();
    void sendProtocol(quint8 CMD, QString datastr);
    void sendProtocol(quint8 CMD, QByteArray nameBytes, QString dataStr);
    // void sendProtocol(quint8 CMD, QString ID, QString NAME);

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;

    bool isLogin = false;
    QString client_serverIp;
    quint16 client_serverPort;
    QString client_clientIp;
    quint16 client_clientPort;
    QString client_userId;
    QString client_userName;
    QString logFilePath;
    void ackOrNack(QTcpSocket* client, quint8 cmd, quint8 refCMD, quint8 code);
};
#endif // MAINWINDOW_H
