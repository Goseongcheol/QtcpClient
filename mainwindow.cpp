#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include <QDataStream>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QTextEdit>
#include <QTimer>

MainWindow::MainWindow(const QString& serverIp, quint16 serverPort, const QString& clientIp, quint16 clientPort, QString userId, QString userName,  const QString& filePath, quint16 reconnectTime, QWidget *parent)
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
    client_reconnectTime = reconnectTime;

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
        connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::connectFail);

        socket->connectToHost(QHostAddress(client_serverIp), client_serverPort);
    }else{
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
        ui->userTableWidget->clearContents();
        ui->userTableWidget->setRowCount(0);

        if (reconnectTimer) {
            reconnectTimer->stop();
            reconnectTimer->deleteLater();
            reconnectTimer = nullptr;
        }
        ui->reconnectLabel->clear();
    }
}

void MainWindow::readyRead()
{

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
        ackOrNack(client,0x09,0x01,0x07);
        return;
    }


    QByteArray data = packet.mid(4, LEN);
    quint8 checksum = quint8(packet[4 + LEN]);
    quint8 ETX      = quint8(packet[5 + LEN]);

    // 체크섬 계산 (클라이언트와 동일)
    quint32 sum = CMD + lenH + lenL;
    for (unsigned char c : data)
        sum += c;
    quint8 calChecksum = quint8(sum % 256);

    // STX, ETX 검증
    if(STX != 2 || ETX != 3){
        ackOrNack(client,0x09,0x01,0x06);
        return;
    }

    // 받은 checksum과 계산한 checksum 확인하기
    if(checksum != calChecksum)
    {
        ackOrNack(client,0x09,0x01,0x01);
        return;
    }
    // CMD 에 맞게 따로 처리
    switch (CMD){
    // CONNECT (0x01)
    case 2 :{
        //user_list
        ui->userTableWidget->clearContents();
        ui->userTableWidget->setRowCount(0);

        QByteArray userCount = data.mid(0,1).trimmed();


        for (auto it = 0; it !=userCount; ++it)
        {
            int packetSize = 37;
            int base = it *packetSize ;
            QByteArray ID   = data.mid(base + 1, 4).trimmed();
            QByteArray NAME = data.mid(base + 5, 16).trimmed();
            QByteArray IP   = data.mid(base + 21, 15).trimmed();
            QByteArray portBytes = data.mid(base + 36, 2);
            quint16 port = (quint8)portBytes[0] << 8 | (quint8)portBytes[1];

            auto * userList =ui->userTableWidget;
            const int row = userList->rowCount();
            userList->insertRow(row);

            userList->setItem(row, 0, new QTableWidgetItem(ID));
            userList->setItem(row, 1, new QTableWidgetItem(NAME));
            userList->setItem(row, 2, new QTableWidgetItem(IP));
            userList->setItem(row, 3, new QTableWidgetItem(QString::number(port)));
        }
        QString joinData = QString("USER_LIST");

        writeLog(0X02,joinData);

        ackOrNack(client,0x08,0x02,0x00);

        break;
    }
    case 3 :
    {
        //user_join
        // try{
        // user_join
        QByteArray ID   = data.mid(0, 4).trimmed();
        QByteArray NAME = data.mid(4, 16).trimmed();
        QByteArray IP   = data.mid(20, 15).trimmed();
        QByteArray portBytes = data.mid(35, 2);
        quint16 port = (quint8)portBytes[0] << 8 | (quint8)portBytes[1];

        QString idStr   = QString::fromUtf8(ID);
        QString nameStr = QString::fromUtf8(NAME);
        QString ipStr   = QString::fromUtf8(IP);

        auto *userList = ui->userTableWidget;

        //테이블에서 userlist 행 찾기
        int foundRow = -1;
        for (int r = 0; r < userList->rowCount(); ++r) {
            QTableWidgetItem *idItem = userList->item(r, 0);
            if (!idItem) continue;

            if (idItem->text() == idStr) {
                foundRow = r;
                break;
            }
        }

        //찾았으면 업데이트
        if (foundRow >= 0) {
            userList->setItem(foundRow, 1, new QTableWidgetItem(nameStr));
            userList->setItem(foundRow, 2, new QTableWidgetItem(ipStr));
            userList->setItem(foundRow, 3, new QTableWidgetItem(QString::number(port)));
        } else {
        //없으면 새로 추가
            const int row = userList->rowCount();
            userList->insertRow(row);

            userList->setItem(row, 0, new QTableWidgetItem(idStr));
            userList->setItem(row, 1, new QTableWidgetItem(nameStr));
            userList->setItem(row, 2, new QTableWidgetItem(ipStr));
            userList->setItem(row, 3, new QTableWidgetItem(QString::number(port)));
        }

        QString joinData = QString("%1|%2 USER JOIN!").arg(idStr, nameStr);
        writeLog(0x03, joinData);

        ackOrNack(client, 0x08, 0x03, 0x00);

        break;
    }
    case 4 :
    {

        QByteArray ID   = data.mid(0, 4).trimmed();

        auto *userList = ui->userTableWidget;

        for (int row = 0; row < userList->rowCount(); ++row) {
            QTableWidgetItem *item = userList->item(row, 0); // 0번 컬럼이 ID 컬럼
            if (item && item->text() == ID) {
                userList->removeRow(row);
                break; // 하나만 지우면 됨
            }
        }


        QString leaveData = QString("%1 USER LEAVE!").arg(ID);

        writeLog(0X04,leaveData);

        ackOrNack(client,0x08,0x04,0x00);

        break;
    }
    case 8 :
    {
        quint8 ackCMD = quint8(packet[4]);
        QString ackMessage = "해당 CMD 성공";
        writeLog(ackCMD, ackMessage);
        break;
    }
    case 9 :
    {
        quint8 nackCMD = quint8(packet[4]);
        quint8 nackErrorCode = quint8(packet[5]);
        if(nackErrorCode == 1)
        {
            QString nackMessage = "checksum error";
            writeLog(nackCMD, nackMessage);
        }else if(nackErrorCode == 2)
        {
            QString nackMessage = "Unknown CMD";
            writeLog(nackCMD, nackMessage);
        }else if(nackErrorCode == 3)
        {
            QString nackMessage = "Invalid Data";
            writeLog(nackCMD, nackMessage);
        }else if(nackErrorCode == 4)
        {
            QString nackMessage = "Time Out";
            writeLog(nackCMD, nackMessage);
        }else if(nackErrorCode == 5)
        {
            QString nackMessage = "Permossion Denied";
            writeLog(nackCMD, nackMessage);
        }else{
            QString nackMessage = "undefind code";
            writeLog(nackCMD, nackMessage);
        }
        break;
   }
    case 18 :
    {
        //chat으로 메세지 송수신 + 브로드캐스트
        QString ID = data.mid(0,16).trimmed();
        QString MSG = data.mid(16);

        QString chatLogData = QString("%1:%2").arg(ID,MSG);

        writeLog(CMD,chatLogData);

        ackOrNack(client,0x08,0x12,0x00);

        break;
    }
    default :
    {
        break;
    }
    }
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



void MainWindow::connected()
{

    qDebug() << "reconnect";
    ui->loginButton->setText("로그아웃");

    isLogin = true;
    if (reconnectTimer) {
        reconnectTimer->stop();
        reconnectTimer->deleteLater();
        reconnectTimer = nullptr;
    }

    ui->reconnectLabel->setText("");

    quint8 CMD = 0x01;

    QByteArray idBytes = client_userId.toUtf8();
    if (idBytes.size() > 4) idBytes.truncate(4);
    else idBytes.append(QByteArray(4 - idBytes.size(), ' '));

    QByteArray nameBytes = client_userName.toUtf8();
    if (nameBytes.size() > 16) nameBytes.truncate(16);
    else nameBytes.append(QByteArray(16 - nameBytes.size(), ' '));

    QByteArray data = idBytes + nameBytes;  // 총 20바이트

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
    // socket->flush();
}


//전송
void MainWindow::on_sendButton_clicked()
{
    if(!isLogin)
    {
        ui-> chatText -> clear();
    }else{


        QByteArray nameBytes = client_userName.toUtf8();
        if (nameBytes.size() > 16)
        {
            nameBytes.truncate(16);
        }
        else
        {
            nameBytes.append(QByteArray(16 - nameBytes.size(), ' '));
        }
        QString chatData = ui->chatText->toPlainText();
        QString logData = QString("%1 : %2").arg(client_userName,
                                                 ui->chatText->toPlainText());
        qint8 cmd = 0x12;
        sendProtocol(cmd,nameBytes,chatData);
        writeLog(cmd,logData);
        ui-> chatText -> clear();
    }
}

//
void MainWindow::sendProtocol(quint8 CMD, QByteArray nameBytes, QString dataStr)
{
    QByteArray chatBytes = dataStr.toUtf8();
    QByteArray packet;
    quint8 STX = 0x02;
    QByteArray data = nameBytes + chatBytes;

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
    // socket->flush();
}



void MainWindow::ackOrNack(QTcpSocket* client, quint8 cmd, quint8 refCMD, quint8 code)
{
    QByteArray data;
    data.append(refCMD);
    data.append(code);

    quint16 len = data.size();

    quint8 STX = 0x02;
    quint8 ETX = 0x03;

    QByteArray packet;
    packet.append(STX);
    packet.append(cmd);
    packet.append((char)((len >> 8) & 0xFF));
    packet.append((char)(len & 0xFF));
    packet.append(data);

    quint32 sum = cmd + ((len >> 8) & 0xFF) + (len & 0xFF);
    for (unsigned char c : data)
        sum += c;

    quint8 checksum = sum % 256;
    packet.append(checksum);
    packet.append(ETX); // ETX

    client->write(packet);
    // client->flush();
}

void MainWindow::connectFail()
{
    ui->statusLabel->setText("연결 실패");
    ui->loginButton->setText("로그인");
    isLogin = false;
    ui->userTableWidget->clearContents();
    ui->userTableWidget->setRowCount(0);

    remainingTime = client_reconnectTime;
    reconnectTimer = new QTimer(this);

    connect(reconnectTimer, &QTimer::timeout, this, [this]() {
        remainingTime--;

        if (remainingTime <= 0) {

            // 윗부분이 재연결 시간이 경과하면 여기서 연결해제와 재 연결을 해서 밑의 reconnectTimer 이 누적돼서 다음 timer 가 한번에 2초씩 감소하는 현상으로 추가
            if (reconnectTimer) {
                reconnectTimer->stop();
                reconnectTimer->deleteLater();
                reconnectTimer = nullptr;
            }

            socket = new QTcpSocket(this);

            connect(socket, &QTcpSocket::connected, this, &MainWindow::connected);
            connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
            connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::connectFail);

            socket->connectToHost(QHostAddress(client_serverIp), client_serverPort);
            remainingTime = client_reconnectTime;
        }

        ui->reconnectLabel->setText(
            QString("재연결까지 남은 시간: %1초").arg(remainingTime));
    });

    reconnectTimer->start(1000);

    if (socket) {
        socket->deleteLater();
        socket = nullptr;
    }
}



