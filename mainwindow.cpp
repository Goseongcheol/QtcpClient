#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include <QDataStream>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QTextEdit>
#include <QTimer>

MainWindow::MainWindow(const QString& serverIp, quint16 serverPort, const QString& clientIp, quint16 clientPort, QString userId, QString userName,  const QString& filePath, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)

{
    ui->setupUi(this);

    //client IP와 Port사용 안함
    //나중에도 계속 써야하는 정보들
    client_serverIp = serverIp; // 주기적으로 재접속
    client_serverPort = serverPort; // 주기적으로 재접속
    client_clientIp = clientIp; // 정보전송시
    client_clientPort = clientPort; // 정보전송시
    client_userId = userId; // 정보전송시
    client_userName = userName; // 정보전송시
    logFilePath = filePath; // 로그 파일 기록용 경로
}

MainWindow::~MainWindow()
{

    if(!isLogin){
        delete ui;
    }else{
        socket->disconnectFromHost();
        socket->close();
        delete socket;
        delete ui;
    }
}


//로그인 버튼 클릭으로 연결시도
void MainWindow::on_loginButton_clicked()
{
    if(!isLogin)
    {
        socket = new QTcpSocket(this);

        connect(socket, &QTcpSocket::connected, this, &MainWindow::connected);
        connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
        // connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onErrorOccurred);

        // qDebug() << "Connecting to server:" << client_serverIp << ":" << client_serverPort;
        socket->connectToHost(QHostAddress(client_serverIp), client_serverPort);
        ui->statusLabel->setText("연결 중");
        ui->loginButton->setText("로그아웃");
        isLogin = true;
        //
        //여기에 타이머? 활성화 추가( 접속후 자동 재연결 시도)
        //

    }else{
        // 위치 조정 생각해보기
        quint8 disConCmd = 0x13;
        QString logoutData = QString("%1%2")
                              .arg(client_userId
                                      , client_userName);
        socket->disconnectFromHost();
        socket->close();
        delete socket;
        ui->statusLabel->setText("연결 해제");
        ui->loginButton->setText("로그인");
        isLogin = false;
        writeLog(disConCmd,logoutData);
        //
        //여기에 타이머 비활성화(그냥 없애기)
        //
    }
}

void MainWindow::readyRead()
{
    //
    // 추가할 내용
    // USER_LIST, USER_JOIN, USER_LEAVE,CHAT_MSG,Ack,Nack 각각 처리하기
    //

    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray packet = client->readAll();

    quint8 STX  = quint8(packet[0]);
    quint8 CMD  = quint8(packet[1]);
    quint8 lenH = quint8(packet[2]);
    quint8 lenL = quint8(packet[3]);
    quint16 LEN = (quint16(lenH) << 8) | lenL;

    // 패킷 형식(사이즈로) 검증
    if (packet.size() < 1 + 1 + 2 + LEN + 1 + 1) {
        qDebug() << "packet size error";
        //
        // nack 보내기
        //
        return;
    }

    QByteArray data = packet.mid(4, LEN);
    quint8 checksum = static_cast<quint8>(packet[4 + LEN]);
    quint8 ETX      = static_cast<quint8>(packet[5 + LEN]);

    // 체크섬 계산 (클라이언트와 동일)
    quint32 sum = CMD + lenH + lenL;
    for (unsigned char c : data)
        sum += c;
    quint8 calChecksum = static_cast<quint8>(sum % 256);

    // STX, ETX 검증
    if(STX != 2 || ETX != 3){
        qDebug() << "STX OR ETX error";
        //
        // nack보내기
        //
        return;
    }

    // 받은 checksum과 계산한 checksum 확인하기
    if(checksum != calChecksum)
    {
        qDebug() << "checksum error";
        //
        // nack보내기
        //
        return;
    }
    // CMD 에 맞게 따로 처리
    switch (CMD){
    // CONNECT (0x01)
    case 2 :{

        break;
    }
    case 3 :
    {

        QByteArray ID   = data.mid(0, 4).trimmed();
        QByteArray NAME = data.mid(4, 16).trimmed();
        QByteArray IP   = data.mid(20, 15).trimmed();
        QByteArray PORT = data.mid(35, 2).trimmed();

        qDebug() << "ID : " << ID ;
        qDebug() << "NAME : " << NAME ;
        qDebug() << "IP : " << IP ;
        qDebug() << "PORT : " << PORT ;


        // 필드 해석


        break;
    }
    case 4 :
    {
        //Nack
        qDebug() << "nack";

        break;
    }
    case 8 :
    {
        qDebug() << "ACK message" ;

        qDebug() << packet;

        qDebug() << data;

        break;
    }
    case 9 :
    {
        qDebug() << "NACK message" ;

        qDebug() << packet;

        qDebug() << data;

        break;
    }
    case 12 :
    {
        //chat으로 메세지 송수신 + 브로드캐스트
        qDebug() << "real DATA:" << data;
        qDebug() << "chat ";

        QString ID = data.mid(0,4);
        QString MSG = data.mid(4);

        //ID와 MSG 로 받은 메세지 구분해서 처리하기

        QString chatLogData = QString("%1:%2").arg(ID,MSG);

        writeLog(CMD,chatLogData);


        //
        // 받은 메세지 처리하기 추가 전체 클라이언트에게 브로드캐스트
        //


        break;
    }
    default :
    {
        qDebug() << "none";
        break;
    }
    }
    qDebug() << "readyRead end";
}




// 로그 표시 ( ui + 로그파일 같이 작성) 매개변수에 필요한 정보들 추려서 추가 ex) cmd ,data
// 2025-10-27 CMD는 추가 완료
void MainWindow::writeLog(quint8 cmd, QString data)
{
    //data 처리 방식이나 후반 작성 해보다가 switch 고려중

    QString logCmd = "";
    if( cmd == 0x01){
        logCmd = "[CONNECT]";
    }else if(cmd == 0x02){
        logCmd = "[LIST]";
    }else if(cmd == 0x03){
        logCmd = "[JOIN]";
    }else if(cmd == 0x04){
        logCmd = "[LEAVE]";
    }else if(cmd == 0x08){
        logCmd = "[ack]";
    }else if(cmd == 0x09){
        logCmd = "[nack]";
    }else if(cmd == 0x12){
        logCmd = "[CHAT_MSG]";
    }else if(cmd == 0x13){
        logCmd = "[DISCONNECT]";
    }else {
        logCmd = "[NONE]";
    }

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString logTime = currentDateTime.toString("[yyyy-MM-dd HH:mm:ss]"); //폴더에 날짜가 표시 되지만 프로그램을 며칠동안 종료하지 않을 경우에 날짜를 명확하게 확인하려고 yyyy-MM-dd 표시
    QString uiLogData = QString("%1\n%2 %3")
                          .arg(logTime
                                , logCmd
                                , data);

    QString logData = QString("%1%2 %3")
                            .arg(logTime
                               , logCmd
                               , data);
    // ui->logText->append(logTime + "[" + client_clientIp + ":" + client_clientPort + "]" + cmd + data );

    ui->logText->append(uiLogData);

    //로그파일 열고 적기
    QFileInfo fileInfo(logFilePath);
    QDir dir;
    dir.mkpath(fileInfo.path());

    QFile File(logFilePath);

    if (File.open(QFile::WriteOnly | QFile::Append | QFile::Text))
    {
        //log에 데이터 형식 가공해서 바꿔 넣기
        QTextStream SaveFile(&File);
        SaveFile << logData << "\n";
        File.close();
    }
    else
    {
        //error 처리
    }
}

//전송
void MainWindow::on_sendButton_clicked()
{
    if(!isLogin)
    {
        //로그아웃 상태 채팅 보낼때 효과 추가하기
    }else{
        QString chatData = QString ("%1%2").arg(client_userId,
                                               ui->chatText->toPlainText());
        qint8 cmd = 0x12;
        sendProtocol(cmd,chatData);
        writeLog(cmd,chatData);
        ui-> chatText -> clear();
    }
}

void MainWindow::connected()
{
    //.arg는 3개씩 묶기 아니면 거슬리게 경고창 나옴
    // QString dataStr = QString("%1%2")
    //                       .arg(client_userId,
    //                            client_userName);
    quint8 CMD = 0x01;

    QByteArray idBytes = client_userId.toUtf8();
    if (idBytes.size() > 4) idBytes.truncate(4);
    else idBytes.append(QByteArray(4 - idBytes.size(), ' '));

    QByteArray nameBytes = client_userName.toUtf8();
    if (nameBytes.size() > 16) nameBytes.truncate(16);
    else nameBytes.append(QByteArray(16 - nameBytes.size(), ' '));

    QByteArray data = idBytes + nameBytes;  // 총 20바이트

    qDebug() << data;
    qDebug() << idBytes;

    sendProtocol(CMD,data);
    writeLog(CMD,data);
}

void MainWindow::sendProtocol(quint8 CMD, QString dataStr)
{
    QByteArray data = dataStr.toUtf8();
    QByteArray packet;
    quint8 STX = 0x02;

    quint16 len = data.size();

    quint8 ETX = 0x03;

    packet.append(STX);
    packet.append(CMD);

    //append 는 1바이트만 들어감 그래서 서버에서 인식이 안됨.
    //packet.append(len);

    packet.append((len >> 8) & 0xFF);
    packet.append(len & 0xFF);

    //여기서 data는 QByteArray로 지정해놔서 1바이트가 아닌데도 append 사용가능
    packet.append(data);

    //체크섬 계산 LEN_L 하위 8비트 , LEN_H 상위 8비트
    //0xFF = 11111111 로 LEN_L과 LEN_H와 AND연산하여 정해진 8비트만 추출, LEN_H같은 경우 LEN >> 8 로도 상위 8비트를 추출가능하지만 안정성을 위해 한번더 AND연산으로 확실한 상위8비트 추출
    quint8 lenL = len & 0xFF;
    quint8 lenH = (len >> 8) & 0xFF;
    quint32 sum = CMD + lenH + lenL ;

    // data의 크기를 순수 바이트 처리용 unsigned char로 for문을 돌려서 하나씩 크기를 더함 기존 sum에다가
    for (unsigned char c : std::as_const(data)) {
        sum += c;
    }

    //체크섬 크기 1바이트 로 quint8
    quint8 checksum = static_cast<quint8>(sum % 256);

    packet.append(checksum);
    packet.append(ETX);

    socket->write(packet);
    socket->flush();
}

