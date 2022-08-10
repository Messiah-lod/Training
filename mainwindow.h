#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QWidget>
#include <QtWidgets>
#include <QtGui>
#include <QPushButton>
#include <QTextStream>
#include <QAxObject>
#include <QQuickView>
#include <QQmlEngine>
#include <QSettings>
#include <QComboBox>
#include <QObject>
#include <QQuickItem>

#include "BaseSQL.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();


private:
    QWidget *centreWidget {nullptr};
    QGridLayout *gridLayout {nullptr};

    QTableView *tableView {nullptr};//создаем вью, который положим на грид вкладки
    QStandardItemModel* fileModel {nullptr};
    QPushButton *buttonLoad {nullptr};
    QPushButton *buttonUpLoad {nullptr};
    QPushButton *buttonExport {nullptr};

    void setDataToModel(QString fileName);
    void fillingToCommands();
    void fillingToActions();
    std::vector <std::any> getHeader(std::string table);

    std::string webserver = "192.168.37.3";
    std::string path = "D:\\ScadaPr\\Modeli\\SURGRES1_bl11+Model";
    std::string workerTable = "";

    Logger logs{ "logsSQL" };
    int whatTable = 0;


public slots:
    void on_buttonLoad_clicked();
    void on_buttonUpLoad_clicked();
    void on_buttonExport_clicked();

};

#endif // MAINWINDOW_H
