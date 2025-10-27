#include "mainwindow.h"

#include <QApplication>
#include <QSettings>
#include <QTextStream>
#include <QFile>
#include <QDateTime>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    QDateTime currentDateTime = QDateTime::currentDateTime();

    QString yyyymmdd = currentDateTime.toString("yyyyMMdd");
    QString HH = currentDateTime.toString("HH");
    QString HHmmss = currentDateTime.toString("HHmmss");


    QString filePath = QString("\n%1%2%3").arg(yyyymmdd,HH,HHmmss);

    // 실행 경로의 config.ini 파일 읽어오기
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("SERVER");
    QString serverIp = settings.value("IP").toString();
    quint16 serverPort = static_cast<quint16>(settings.value("PORT").toUInt());
    settings.endGroup();
    settings.beginGroup("CLIENT");
    QString clientIp = settings.value("IP").toString();
    quint16 clientPort = static_cast<quint16>(settings.value("PORT").toUInt());
    settings.endGroup();
    settings.beginGroup("USERINFO");
    QString userId = settings.value("USERID").toString();
    QString userName = settings.value("USERNAME").toString();
    settings.endGroup();




    //여기에 로그 파일 경로 생성 만들기

    MainWindow w(serverIp,serverPort,clientIp,clientPort,userId,userName,filePath);
    w.show();




    return a.exec();
}
