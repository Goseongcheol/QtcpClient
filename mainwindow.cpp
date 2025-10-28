#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include <QDataStream>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QTextEdit>

MainWindow::MainWindow(const QString& serverIp, quint16 serverPort, const QString& clientIp, quint16 clientPort, QString userId, QString userName,  const QString& filePath, QWidget *parent)
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
        QString logoutData = QString("%1%2%3%4")
                              .arg(client_userId,
                                   client_userName,
                                   client_clientIp)
                              .arg(client_clientPort);
        socket->disconnectFromHost();
        socket->close();
        delete socket;
        ui->statusLabel->setText("연결 해제");
        ui->loginButton->setText("로그인");
        isLogin = false;
        writeLog(disConCmd,logoutData,logFilePath);
        //
        //여기에 타이머 비활성화(그냥 없애기)
        //
    }

}


void MainWindow::connected()
{
    //.arg는 3개씩 묶기 아니면 거슬리게 경고창 나옴
    QString dataStr = QString("%1%2%3%4")
                          .arg(client_userId,
                               client_userName,
                               client_clientIp)
                          .arg(client_clientPort);

    quint8 CMD = 0x01;

    sendProtocol(CMD,dataStr);

    writeLog(CMD,dataStr,logFilePath);

}


void MainWindow::readyRead()
{
    //
    // 추가할 내용
    // USER_LIST, USER_JOIN, USER_LEAVE,CHAT_MSG,Ack,Nack 각각 처리하기
    //

    QByteArray data = socket->readAll();
    qDebug() << "Received from server:" << data;
}


// 로그 표시 ( ui + 로그파일 같이 작성) 매개변수에 필요한 정보들 추려서 추가 ex) cmd ,data
// 2025-10-27 CMD는 추가 완료
void MainWindow::writeLog(quint8 cmd, QString data, const QString& filePath)
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
    QString uiLogData = QString("%1\n[%2:%3]\n%4 %5")
                          .arg(logTime,
                                client_clientIp,
                                QString::number(client_clientPort)) // port가 quint16 으로 작성했었음 오류 나옴
                          .arg(logCmd,
                                data);

    QString logData = QString("%1[%2:%3]%4 %5")
                            .arg(logTime,
                                client_clientIp,
                                QString::number(client_clientPort))
                            .arg(logCmd,
                                data);
    // ui->logText->append(logTime + "[" + client_clientIp + ":" + client_clientPort + "]" + cmd + data );

    ui->logText->append(uiLogData);



    //로그파일 열고 적기
    QFileInfo fileInfo(filePath);
    QDir dir;
    dir.mkpath(fileInfo.path());

    QFile File(filePath);

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
        // QString chatData = ui->chatText->toPlainText();
        QString chatData = QString ("%1%2").arg(client_userId,ui->chatText->toPlainText());
        qint8 cmd = 0x12;
        sendProtocol(cmd,chatData);
        writeLog(cmd,chatData,logFilePath);
        ui-> chatText -> clear();
    }

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
    packet.append(ETX);

    socket->write(packet);
    socket->flush();


}
