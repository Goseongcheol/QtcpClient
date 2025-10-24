#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHostAddress>
#include <QDataStream>

#pragma pack(push, 1)
struct LoginPacket {
    quint16 size;
    quint16 type;
    char userId[32];
    char userName[32];
    char ip[16];
    quint16 port;
};
#pragma pack(pop)


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

    LoginPacket pkt = {};
    pkt.type = 1; // 로그인 패킷
    pkt.port = m_clientPort;

    // 문자열을 C-style로 변환 후 구조체에 복사
    QByteArray idBytes = m_userId.toUtf8();
    QByteArray nameBytes = m_userName.toUtf8();
    QByteArray ipBytes = m_clientIp.toUtf8();

    memcpy(pkt.userId, idBytes.constData(), qMin(idBytes.size(), 31));
    memcpy(pkt.userName, nameBytes.constData(), qMin(nameBytes.size(), 31));
    memcpy(pkt.ip, ipBytes.constData(), qMin(ipBytes.size(), 15));

    pkt.size = sizeof(LoginPacket);

    // 전송
    socket->write(reinterpret_cast<const char*>(&pkt), sizeof(pkt));
    socket->flush();

    qDebug() << "Sent LoginPacket:"
             << "\n  type =" << pkt.type
             << "\n  id =" << pkt.userId
             << "\n  name =" << pkt.userName
             << "\n  ip =" << pkt.ip
             << "\n  port =" << pkt.port
             << "\n  size =" << pkt.size;
}


void MainWindow::readyRead()
{
    QByteArray data = socket->readAll();
    qDebug() << "Received from server:" << data;
}
