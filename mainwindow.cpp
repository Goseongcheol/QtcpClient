#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHostAddress>
#include <QDataStream>


MainWindow::MainWindow(const QString& serverIp, quint16 serverPort, const QString& clientIp, quint16 clientPort, QString userId, QString userName, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)

{
    ui->setupUi(this);

    //나중에도 계속 써야하는 정보들
    client_serverIp = serverIp; // 주기적으로 재접속
    client_serverPort = serverPort; // 주기적으로 재접속
    client_clientIp = clientIp; // 정보전송시
    client_clientPort = clientPort; // 정보전송시
    client_userId = userId; // 정보전송시
    client_userName = userName; // 정보전송시


    socket = new QTcpSocket(this);


    connect(socket, &QTcpSocket::connected, this, &MainWindow::connected);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
    // connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onErrorOccurred);


    //서버와 연결 시도중 잠깐이라도 표시하기
    qDebug() << "Connecting to server:" << client_serverIp << ":" << client_serverPort;
    socket->connectToHost(QHostAddress(client_serverIp), client_serverPort);


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
    //연결된 콘솔 대신에 접속 중인 표시
    qDebug() << "Connected to server.";


    //.arg는 3개씩 묶기 아니면 거슬리게 경고창 나옴
    QString dataStr = QString("%1;%2;%3;%4")
                          .arg(client_userId,
                               client_userName,
                               client_clientIp)
                          .arg(client_clientPort);
    QByteArray data = dataStr.toUtf8();


    QByteArray packet;
    quint8 STX = 0x02;
    quint8 CMD = 0x01; // connect CMD 로 0X01
    quint16 len = data.size();
    quint8 ETX = 0x03;


    packet.append(STX);
    packet.append(CMD);


    packet.append(static_cast<char>(len & 0xFF));
    packet.append(static_cast<char>((len >> 8) & 0xFF));


    packet.append(data);

    quint8 lenL = len & 0xFF;
    quint8 lenH = (len >> 8) & 0xFF;

    quint32 sum = CMD + lenH + lenL;


    // 기존 c:data 였지만 std::as_const(data) 로 변경 : 이유 - for문에서 data를 수정하지않으려는 안정장치. 기능적차이 x 코드 안정성 상승
    for (unsigned char c : std::as_const(data))
        sum += c;

    quint8 checksum = static_cast<quint8>(sum % 256);
    packet.append(checksum);

    // ETX
    packet.append(ETX);



    socket->write(packet);
    socket->flush();

    qDebug() << "Sent Packet (hex):" << packet.toHex(' ');
    qDebug() << "Data:" << dataStr;
}


void MainWindow::readyRead()
{

    // 추가할 내용
    // USER_LIST, USER_JOIN, USER_LEAVE 각각 처리하기
    //

    QByteArray data = socket->readAll();
    qDebug() << "Received from server:" << data;
}





// 추가할 내용
// 로그아웃 버튼으로 연결 끊을 시 UI접속중 아이콘? 이나 상태 변경 해주기
//

