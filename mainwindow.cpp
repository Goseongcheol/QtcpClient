#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHostAddress>

MainWindow::MainWindow(const QString& serverIp, quint16 serverPort, const QString& clientIp, quint16 clientPort, QString userId, QString userName, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)

{
    ui->setupUi(this);
    // 멤버 변수 저장
    m_serverIp = serverIp;
    m_serverPort = serverPort;
    m_clientIp = clientIp;
    m_clientPort = clientPort;
    m_userId = userId;
    m_userName = userName;

    // QTcpSocket 생성 (서버와 동일한 패턴)
    socket = new QTcpSocket(this);

    // 시그널 연결
    connect(socket, &QTcpSocket::connected, this, &MainWindow::connected);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
    // connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onErrorOccurred);

    // 서버 접속 시도
    qDebug() << "Connecting to server:" << m_serverIp << ":" << m_serverPort;
    socket->connectToHost(QHostAddress(m_serverIp), m_serverPort);


}

MainWindow::~MainWindow()
{
    delete ui;
    socket->disconnectFromHost();
    socket->close();
    delete socket;
}


void MainWindow::connected()
{
    qDebug() << "Connected to server.";

    QString msg = QString("id=%1 name=%2 ip=%3 port=%4\n")
                      .arg(m_userId)
                      .arg(m_userName)
                      .arg(m_clientIp)
                      .arg(m_clientPort);

    socket->write(msg.toUtf8());
    socket->flush();

    qDebug() << "Sent to server:" << msg.trimmed();
}

void MainWindow::readyRead()
{
    QByteArray data = socket->readAll();
    qDebug() << "Received from server:" << data;
}
