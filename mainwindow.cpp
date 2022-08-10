#include "mainwindow.h"

MainWindow::MainWindow()
{
    //инициализируем элементы графического интерфейса
    centreWidget = new QWidget(this);
    gridLayout = new QGridLayout(centreWidget);
    tableView = new QTableView;
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileModel = new QStandardItemModel();
    tableView->setModel(fileModel);

    buttonLoad = new QPushButton;
    buttonUpLoad = new QPushButton;
    buttonExport = new QPushButton;

    buttonLoad->setText(QApplication::translate("buttonLoad", "Открыть файл", nullptr));
    buttonUpLoad->setText(QApplication::translate("buttonUpLoad", "Выгрузить", nullptr));
    buttonExport->setText(QApplication::translate("buttonUpLoad", "Экспортировать", nullptr));

    gridLayout->addWidget(buttonExport, 0,0,1,1);
    gridLayout->addWidget(buttonLoad, 0,1,1,1);
    gridLayout->addWidget(buttonUpLoad, 0, 2, 1, 1);
    gridLayout->addWidget(tableView, 1, 0, 1, 3);

    //Связь сигналов кнопок с методами
    QObject::connect(buttonExport, SIGNAL(clicked()), this, SLOT(on_buttonExport_clicked()));
    QObject::connect(buttonLoad, SIGNAL(clicked()), this, SLOT(on_buttonLoad_clicked()));
    QObject::connect(buttonUpLoad, SIGNAL(clicked()), this, SLOT(on_buttonUpLoad_clicked()));

    this->setCentralWidget(centreWidget);
}

void MainWindow::setDataToModel(QString fileName)
{
    QAxObject *mExcel = new QAxObject("Excel.Application", nullptr);
    QAxObject *workbooks = mExcel->querySubObject("Workbooks");
    QAxObject *workbook = workbooks->querySubObject("Open(const QString&)", fileName);
    QAxObject *mSheets = workbook->querySubObject("Sheets");
    QAxObject *StatSheet = mSheets->querySubObject("Item(int)", 1);

    //Количество используемых на листе строк можно получить с помощью такого кода
    QAxObject* usedRange = StatSheet->querySubObject("UsedRange");
    QAxObject* rows = usedRange->querySubObject("Rows");
    int countRows = rows->property("Count").toInt() - 1;//вычитаем 1, т.к.заголовок оставим

    //Соответственно, столбцов:
    QAxObject* columns = usedRange->querySubObject("Columns");
    int countCols = columns->property("Count").toInt();

    logs << QString::number(countRows);
    logs << QString::number(countCols);

    QAxObject* cell = StatSheet->querySubObject("Cells(int,int)", 1, 1);
    QVariant value = cell->property("Value");
    webserver = value.toString().toStdString();

    cell = StatSheet->querySubObject("Cells(int,int)", 1, 2);
    value = cell->property("Value");
    path = value.toString().toStdString();

    //узнаем какую таблицу будем выгружать в БД
    cell = StatSheet->querySubObject("Cells(int,int)", 1, 3);
    value = cell->property("Value");
    workerTable = value.toString().toStdString();
    if(workerTable == "TRAININGSCRIPTS_COMMANDS") whatTable = 1;
    else if (workerTable == "TRAININGSCRIPTS_ACTIONS") whatTable = 2;
    else whatTable = 0;

    qDebug() << "Сюда дошли";
    qDebug() << QString::fromStdString(webserver) + " " + QString::fromStdString(path)+ " " + QString::fromStdString(workerTable);

    fileModel->clear();

    for (int nRow = 0; nRow < countRows; ++nRow)
    {
        for (int nCol = 0; nCol < countCols; ++nCol)
        {
            QAxObject* cell = StatSheet->querySubObject("Cells(int,int)", nRow + 2, nCol+1);
            QVariant value = cell->property("Value");
            QStandardItem *item = new QStandardItem(value.toString());

            if (nRow == 0)
                fileModel->setHorizontalHeaderItem(nCol, item);
            else
                fileModel->setItem(nRow-1, nCol, item);
        }
    }

    workbook->dynamicCall("Close(Boolean)", false);
    mExcel->dynamicCall("Quit(void)");    //закрываем приложение
    delete(mExcel);

    tableView->resizeColumnsToContents();
}

void MainWindow::fillingToCommands()
{
    BaseSQL DB(webserver, path, "SYSDBA", "masterkey");
    if(!DB.connected()){
        qDebug() << "Не успешный коннект";
        return;
    }
    std::string query = "";

    for(int i = 0; i < fileModel->rowCount(); i++){
        QModelIndex index = fileModel->index(i, 0);
        QString text = index.data(Qt::DisplayRole).toString();
        std::string id = text.toStdString();

        index = fileModel->index(i, 1);
        text = index.data(Qt::DisplayRole).toString();
        std::string training = text.toStdString();

        index = fileModel->index(i, 2);
        text = index.data(Qt::DisplayRole).toString();
        std::string name = text.toStdString();

        index = fileModel->index(i, 3);
        text = index.data(Qt::DisplayRole).toString();
        std::string mode = text.toStdString();

        //проверим если уже такой ID в БД
        std::string table = "TRAININGSCRIPTS_COMMANDS";
        std::string coll_answer = "ID";
        std::any answer_;
        query = "Select " + coll_answer + " from " + table + " where ID = '" + id + "'";
        DB.read(query, table, coll_answer, answer_);

        //если нет - вставляем данные, в противном случае - обновляем
        if(DB.any_to_int(answer_) == 0){
            query = "INSERT INTO TRAININGSCRIPTS_COMMANDS (ID, TRAININGSCRIPTID, NAME, MODE) VALUES (" + id + ", "+ training + ", '"+ name +"' ," + mode + ")";
            logs << QString::fromStdString(query);
            DB.write(query);
        }
        else {
            query = "UPDATE TRAININGSCRIPTS_COMMANDS SET TRAININGSCRIPTID = "+ training + ", NAME = '"+ name +"', MODE = " + mode + " WHERE ID = " + id;
            logs << QString::fromStdString(query);
            DB.write(query);
        }
    }
}

void MainWindow::fillingToActions()
{
    std::string query = "";
    std::vector <std::any> headerTable = getHeader(workerTable);
    std::vector <std::string> rowData;

    BaseSQL DB(webserver, path, "SYSDBA", "masterkey");
    if(!DB.connected()){
        qDebug() << "Не успешный коннект";
        return;
    }
    //начнем подготовку запросов
    for(int i = 0; i < fileModel->rowCount(); i++){
        //набъем вектор строки
        rowData.clear();
        for(unsigned long long j = 0; j < headerTable.size(); j++) {
            QModelIndex index = fileModel->index(i, j);
            std::string text = index.data(Qt::DisplayRole).toString().toStdString();
            qDebug() << QString::fromStdString(text);
            rowData.push_back(text);
        }

        //проверим если уже такой ID в БД
        std::string coll_answer = "ID";
        std::any answer_;
        query = "Select " + coll_answer + " from " + workerTable + " where ID = '" + rowData[0] + "'";
        DB.read(query, workerTable, coll_answer, answer_);

            qDebug() << "Зашли во внешний цикл, пробуем узнать ID";

        //если нет - вставляем данные, в противном случае - обновляем
        if(DB.any_to_int(answer_) == 0){
            query = "INSERT INTO "+workerTable+" (ID, TRAININGSCRIPTID, PID, NAME, DESC, ACTTYPE,USERID,"
                    "SENDTOUSERID,COMMANDID,ORDERNUM,PARAMID,PARAM_OPER,PARAM_VALUE,CHECK_PARAMID,CHECK_OPER,CHECK_VALUE,PREVALLOWED) "
                    "VALUES (" + rowData[0] + ", "+ rowData[1] + ", "+ rowData[2] + ", '"+ rowData[3] + "', '"+ rowData[4] + "', "+
                    rowData[5] + ", "+ rowData[6] + ", "+ rowData[7] + ", "+ rowData[8] + ", "+ rowData[9] + ", "+ rowData[10] + ", "+
                    rowData[11] + ", '"+ rowData[12] + "', "+ rowData[13] + ", "+ rowData[14] + ", '"+ rowData[15] + "', "+ rowData[16] + ")";
            logs << QString::fromStdString(query);
            DB.write(query);
        }
        else {
            query = "UPDATE "+workerTable+" SET "
                    " TRAININGSCRIPTID = " + rowData[1] +
                    ", PID = " + rowData[2] +
                    ", NAME = '" + rowData[3] +
                    "', DESC = '" + rowData[4] +
                    "', ACTTYPE = " + rowData[5] +
                    ", USERID = " + rowData[6] +
                    ", SENDTOUSERID = " + rowData[7] +
                    ", COMMANDID = " + rowData[8] +
                    ", ORDERNUM = " + rowData[9] +
                    ", PARAMID = " + rowData[10] +
                    ", PARAM_OPER = " + rowData[11] +
                    ", PARAM_VALUE = '" + rowData[12] +
                    "', CHECK_PARAMID = " + rowData[13] +
                    ", CHECK_OPER = " + rowData[14] +
                    ", CHECK_VALUE = '" + rowData[15] +
                    "', PREVALLOWED = " + rowData[16] +
                    " WHERE ID = " + rowData[0];
            logs << QString::fromStdString(query);
            DB.write(query);
        }
    }
}

std::vector<std::any> MainWindow::getHeader(std::string table)
{
    std::vector <std::any> header;
    //подключимся к БД
    BaseSQL DB(webserver, path, "SYSDBA", "masterkey");
    if(!DB.connected()) return header;

    //получим вектор с хедерами запрошенной таблицы
    std::string query = "select R.RDB$RELATION_NAME,  R.RDB$FIELD_NAME, F.RDB$FIELD_TYPE from RDB$FIELDS F, RDB$RELATION_FIELDS R "
                        "where F.RDB$FIELD_NAME = R.RDB$FIELD_SOURCE and R.RDB$SYSTEM_FLAG = 0 and R.RDB$RELATION_NAME = '"+table+
                         "' order by R.RDB$RELATION_NAME, R.RDB$FIELD_POSITION";
    DB.read(query, table, "RDB$FIELD_NAME", header);
    return header;
}

void MainWindow::on_buttonLoad_clicked()
{
    QString fileName;
    try
    {
        fileName = QFileDialog::getOpenFileName(nullptr,
                                                tr("Open file"), QDir::currentPath(), tr("Data Files (*.xls *.xlsx *.xlsm)"));
    }
    catch (const std::exception&)
    {
        fileName = "";
        return;
    }

    if (!fileName.isEmpty())
    {
        //запуск заполнения модели в отдельном потоке
        std::thread t1(&MainWindow::setDataToModel, this, fileName);
        t1.detach();
    }
}

void MainWindow::on_buttonUpLoad_clicked()
{
    BaseSQL DB(webserver, path, "SYSDBA", "masterkey");
    if(!DB.connected()){
        qDebug() << "Не успешный коннект";
        return;
    }
    switch(whatTable){
        case 1: fillingToCommands(); break;
        case 2: fillingToActions(); break;
        default: break;
    }

 //   std::thread t1(&MainWindow::fillingToDB, this);
    //   t1.detach();
}

void MainWindow::on_buttonExport_clicked()
{
    //блок подключения к файлу экселя
    QString fileName;
    try  {
        fileName = QFileDialog::getOpenFileName(nullptr,
                                                tr("Open file"), QDir::currentPath(), tr("Data Files (*.xls *.xlsx *.xlsm)"));
    }  catch (const std::exception&)  {
        fileName = "";
        return;
    }

    //блок подключения к экселю
    QAxObject *mExcel = new QAxObject("Excel.Application", nullptr);
    mExcel->setProperty("DisplayAlerts", 0); //говорим, что выводить сообщения эксель не надо
    QAxObject *workbooks = mExcel->querySubObject("Workbooks");
    QAxObject *workbook = workbooks->querySubObject("Open(const QString&)", fileName);
    QAxObject *mSheets = workbook->querySubObject("Sheets");
    QAxObject *StatSheet = mSheets->querySubObject("Item(int)", 1);
    QAxObject* cell = StatSheet->querySubObject("Cells(int,int)", 1, 1);
    QVariant value = cell->property("Value");
    webserver = value.toString().toStdString();

    cell = StatSheet->querySubObject("Cells(int,int)", 1, 2);
    value = cell->property("Value");
    path = value.toString().toStdString();

    cell = StatSheet->querySubObject("Cells(int,int)", 1, 3);
    value = cell->property("Value");
    std::string table = value.toString().toStdString();

    //очистим документ, через обращение к диапазону
    QAxObject * user_range = StatSheet->querySubObject("Range (const QString &)", "A2: GZ1000"); // диапазон
    user_range->setProperty("Value", QVariant(""));

    //получим вектор с хедерами запрошенной таблицы
    std::vector <std::any> headerTable = getHeader(table);
    std::vector <std::any> dataColumnTable;
    std::string query;

    //подключимся к БД
    BaseSQL DB(webserver, path, "SYSDBA", "masterkey");
    if(!DB.connected()) return;

    for(unsigned long long numColl = 0; numColl < headerTable.size(); numColl++)
    {
        //получим имя хедера и запишем его в файл
        std::string header = DB.any_to_str(headerTable[numColl]);
        header.erase(remove(header.begin(),header.end(),' '),header.end());
        cell = StatSheet->querySubObject("Cells(int,int)", 2, numColl+1);
        cell->setProperty("Value", QVariant(QString::fromStdString(header)));

        //заполним вектор данных из таблицы
        dataColumnTable.clear();
        query = "select " + header + " from " + table + " ORDER BY ID";
        DB.read(query, table, header, dataColumnTable);

        //побежим по данным и будем их записывать в эксель
        for(unsigned long long numRow = 0; numRow < dataColumnTable.size(); numRow++) {
            cell = StatSheet->querySubObject("Cells(int,int)", numRow +3, numColl+1);
            QVariant data = QString::fromStdString(DB.any_to_str(dataColumnTable[numRow]));
            cell->setProperty("Value", data);
        }
    }

    qDebug() << "Вышли из всех циклов";
//    delete cell;
//    delete StatSheet;
//    delete mSheets;
//    workbook->dynamicCall("Save()");
//    delete workbook;
//    //закрываем книги
//    delete workbooks;
//    //закрываем Excel
//    mExcel->querySubObject("ActiveDocument")->dynamicCall("Close()");
//    mExcel->dynamicCall("Quit()");
//    delete mExcel;

    workbook->dynamicCall("Save()"); // Сохраняем файл
    workbook->dynamicCall("Close(Boolean)", false); // Закрываем файл
    mExcel->dynamicCall("Quit(void)"); // Закрываем excel

    delete(mExcel);
}

