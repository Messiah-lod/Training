#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QtWidgets>
#include <QApplication>
#include <QPluginLoader>
#include <QQuickStyle>
#include <QSqlDatabase>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("fusion"));
    const QIcon mainIcon = QIcon::fromTheme("mainIcon", QIcon(":/pic/icon.ico"));

    qDebug() << "Used drivers: :" << QSqlDatabase::drivers();

    a.setWindowIcon(mainIcon);

    MainWindow excelExportScada;
    excelExportScada.setMinimumWidth(800);
    excelExportScada.setMinimumHeight(600);
    excelExportScada.show();

    return a.exec();
}
