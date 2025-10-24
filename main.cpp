#include "mainwindow.h"

#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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


    MainWindow w(serverIp,serverPort,clientIp,clientPort,userId,userName);
    w.show();




    return a.exec();
}
