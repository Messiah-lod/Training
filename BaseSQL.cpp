#include "BaseSQL.h"

BaseSQL::BaseSQL(std::string webserver_, std::string path_, std::string user_, std::string pass_)
{
    setConnectParam(webserver_, path_, user_, pass_);
    connect();
}

BaseSQL::~BaseSQL()
{
	disconnect();
}

int BaseSQL::connect()
{//После получения данных о базе и пользователе создаем подключение
    Database.setHostName(QString::fromStdString(webserver));
    Database.setDatabaseName(QString::fromStdString(path));
    Database.setUserName(QString::fromStdString(user));
    Database.setPassword(QString::fromStdString(pass));

    logs.debug("Попытка подключения к базе данных");
    logs.debug(QString::fromStdString("webserver: " + webserver + ", path: " + path));

    bool isSuccessConnect;
    try{
         isSuccessConnect = Database.open();
    }
    catch(...) {
        logs.warning("Исключение при попытке подключения к БД!");
        return 1;
    }

    if(!isSuccessConnect){
        logs.warning("Заданы неверные параметры подключения к базе данных!");
        logs << Database.lastError().text();
		return 1;
    }
    return 0;
}

void BaseSQL::disconnect()
{
    Database.close();
}

bool BaseSQL::connected()
{
    bool isSuccessConnected = Database.isOpen();
    if (!isSuccessConnected)logs << "Нет подключения к базе данных!";
    return isSuccessConnected;
}

void BaseSQL::setConnectParam(std::string webserver_, std::string path_, std::string user_, std::string pass_)
{
    webserver = webserver_;
    std::transform(path_.begin(), path_.end(),path_.begin(), ::toupper);
    if(path_.find("SCADABD.GDB")==std::string::npos) path_+= "/SCADABD.GDB";
    path = path_;
    user = user_;
    pass = pass_;
}

std::string BaseSQL::any_to_str(std::any parameter)
{
	try {
        if (parameter.type() == typeid(int))
		{
			return std::to_string(std::any_cast <int> (parameter));
		}
        else if (parameter.type() == typeid(double))
		{
			return std::to_string(std::any_cast <double> (parameter));
		}
        else if (parameter.type() == typeid(const char*))
		{
			return std::any_cast <const char*> (parameter);
		}
        else if (parameter.type() == typeid(bool))
		{
			return std::to_string(std::any_cast <bool> (parameter));
		}
        else if (parameter.type() == typeid(std::string))
		{
			return std::any_cast<std::string> (parameter);
		}
		else
		{
            logs.error("Исключение в методе any_to_str: Попытка вывода неизвестного или пустого типа переменной! "
             "Наименование типа параметра: " + QString::fromStdString(parameter.type().name()));
            return "";
		}
	}
	catch (const std::bad_any_cast & e) {
        logs << "Исключение в методе any_to_str: " + QString::fromStdString(e.what());
        return std::string("");
    }
}

int BaseSQL::any_to_int(std::any parameter)
{
    try {
        if (parameter.type() == typeid(int))
        {
            return std::any_cast <int> (parameter);
        }
        else if (parameter.type() == typeid(double))
        {
            return std::any_cast <int>(std::any_cast <double> (parameter));
        }
        else if (parameter.type() == typeid(bool))
        {
            return std::any_cast <int>(std::any_cast <bool> (parameter));
        }
        else
        {
            logs.error("Исключение в методе any_to_int: Попытка вывода неизвестного, пустого или строкового типа переменной! "
             "Наименование типа параметра: " + QString::fromStdString(parameter.type().name()));
            return 1;
        }
    }
    catch (const std::bad_any_cast & e) {
        logs << "Исключение в методе any_to_int: " + QString::fromStdString(e.what());
        return 2;
    }
}

void BaseSQL::IntToBinary(int num, std::string  &buf)
{
	do
	{
		if (num % 2 == 0) buf += '0';
		else buf += '1';
		num /= 2;
	} while (num > 0);

	while (buf.size() < 8)
	{
		buf += '0';
	}

	std::reverse(buf.begin(), buf.end());
}

void BaseSQL::write(std::string query)
{
    if (!connected()) return;
    qDebug() << QString::fromStdString(query);
    QSqlQuery sqlquery(Database);
    if(!sqlquery.exec(QString::fromStdString(query))){
        logs << sqlquery.lastError().text();
    }
}

void BaseSQL::read(std::string query, std::string table, std::string collAnswer, std::any &answ)
{
    std::vector <std::any> vectorAnsw;
    read(query, table, collAnswer, vectorAnsw);
    if(vectorAnsw.size() > 0) answ = vectorAnsw[0];
    else answ = 0;
    qDebug() << "Метод Read вернул следующее значение: " + QString::number(any_to_int(answ));
}

void BaseSQL::read(std::string query, std::string table, std::string collAnswer, std::vector <std::any> &answ)
{
    if (!connected()) return;

    logs.debug(QString::fromStdString(query));


    QSqlQuery sqlquery(QString::fromStdString(query), Database); //объект запроса с указанием на БД
    QSqlRecord sqlrecord = sqlquery.record();
    int indexNameCol = sqlrecord.indexOf(QString::fromStdString(collAnswer)); // index of the field coll_answer

    //приводим any к типу данных по запросу к БД
    std::string type_answ = get_type_name(table, collAnswer); //получим тип параметра из БД
    qDebug() << QString::fromStdString(query);
    qDebug() << QString::fromStdString(type_answ);

    if (type_answ == "VARCHAR" || type_answ == "") {
        while (sqlquery.next()) {//получаем данные и приводим их сразу к строке
            QVariant tmpVariant = sqlquery.value(indexNameCol);
 //           qDebug() << tmpVariant.toString();
            std::string tmpString = tmpVariant.toString().toStdString();
    //        qDebug() << QString::fromStdString(tmpString);
            answ.push_back(tmpString);
        }
//        if(answ.size() == 0) answ.push_back("");
    }
    else if (type_answ == "INTEGER" || type_answ == "SHORT") {
        while (sqlquery.next()) {//получаем данные и приводим их сразу к числу
            QVariant tmpVariant = sqlquery.value(indexNameCol);
            int tmpInt = tmpVariant.toInt();
        //    qDebug() << QString::number(tmpInt);
            answ.push_back(tmpInt);
        }
//        if(answ.size() == 0) answ.push_back(0);
    }
    else logs << "Исключение в методе read: Тип Параметра не найден!";
    qDebug() << "Длина вектора с вычитанными данными равна: " + QString::number(answ.size());
    /*    //приводим any к типу данных по QVariant
        switch(answerVariant.userType()){
        case QMetaType::Int: {
            int a = answerVariant.toInt();
            answ = a;
            break;
        }
        case QMetaType::QString: {
            std::string b = answerVariant.toString().toStdString();
            answ = b;
            break;
        }
        default: break;
        }
    */
}

std::string BaseSQL::get_type_name(std::string table, std::string coll)
{
    std::transform(table.begin(), table.end(), table.begin(), toupper); //переведем в верхний регистр строку с таблицей
    std::transform(coll.begin(), coll.end(), coll.begin(), toupper);

    //затираем все кавычки в названии колонок
    for (unsigned int i = 0; i < coll.length(); i++)
        if (coll[i] == '"')	coll.erase(i, 1);

    //подготовим запрос на тип данных колонки в таблице
    std::string  query1, query2;
    query.clear();
    query1 = "select R.RDB$RELATION_NAME,  R.RDB$FIELD_NAME, F.RDB$FIELD_TYPE from RDB$FIELDS F, RDB$RELATION_FIELDS R where F.RDB$FIELD_NAME = R.RDB$FIELD_SOURCE and R.RDB$SYSTEM_FLAG = 0";
    query2 = "order by R.RDB$RELATION_NAME, R.RDB$FIELD_POSITION";
    query = query1 + ' ' + "and R.RDB$RELATION_NAME = '" + table.c_str() + "' and R.RDB$FIELD_NAME = '"  + coll.c_str() + "' " + query2;
//    qDebug() <<  QString::fromStdString(query);

    QSqlQuery sqlquery(QString::fromStdString(query), Database); //объект запроса с указанием на БД
    QSqlRecord sqlrecord = sqlquery.record();
    QVariant answerVariant;

    while (sqlquery.next())//пока получаем данные...
        answerVariant = sqlquery.value(2);

//    qDebug() << answerVariant;
    int columnType = 0;
    if (answerVariant.canConvert<int>())
        columnType = answerVariant.toInt();
    else logs.error("Недопустимый формат данных в таблице: " + QString::fromStdString(table) + ", колонка: " + QString::fromStdString(coll));

    switch (columnType) {
        case 7: return "SHORT";
        case 8: return "INTEGER";
        case 9: return "QUAD";
        case 10: return "FLOAT";
        case 12: return "DATE";
        case 13: return "TIME";
        case 14: return "TEXT";
        case 16: return "INT64";
        case 23: return "BOOLEAN";
        case 27: return "DOUBLE";
        case 35: return "TIMESTAMP";
        case 37: return "VARCHAR";
        case 40: return "CSTRING";
        case 45: return "BLOB_ID";
        case 261: return "BLOB";
        default: {
            logs.error("Не удалось определить тип данных " + QString::fromStdString(table) + ", колонка: " + QString::fromStdString(coll));
            return "";
        }
    }
}

int BaseSQL::color_convet(std::string FONCOLOR)
{
    std::string tmp_, color_str;
    int colorR, colorG, colorB, count;
    count = colorR = colorG = colorB = 0;

    //цикл для выделения цвета из строки параметра в R:G:B
    for (size_t i = 0; i < FONCOLOR.size(); i++)
    {
        tmp_ += FONCOLOR[i];
        if (FONCOLOR[i] == ':' || i == (FONCOLOR.size() - 1))
        {
            if (count == 0)
            {
                colorR = std::stoi(tmp_);
            }
            else if (count == 1)
            {
                colorG = std::stoi(tmp_);
            }
            else
            {
                colorB = std::stoi(tmp_);
            }
            count++;
            tmp_.clear();
        }
    }
    //запишем в бинарном виде параметры цвета и склеим в одну строку
    IntToBinary(colorB, tmp_);
    color_str += tmp_;
    tmp_.clear();
    IntToBinary(colorG, tmp_);
    color_str += tmp_;
    tmp_.clear();
    IntToBinary(colorR, tmp_);
    color_str += tmp_;
    tmp_.clear();

    return std::bitset<32>(color_str).to_ulong();//переведем бинарную строку в десятичный INT
}

bool BaseSQL::checkingIsCorrectName(std::string m_name)
{
    qDebug() << "Выполним проверку имени на допустимость";
    if (m_name.size() == 0){ 						//Проверка на пустое имя
        logs << "Наименование не корректно: пустое имя!";
        return false;
    }
    QString name = QString::fromStdString(m_name); //переведем в utf-8 для работы с кирилицей
    if(name.contains(QRegularExpression("[А-Яа-я]"))){
        logs.warning("Наименование не корректно: кирилица в имени!");
        return false;
    }
    if(name.contains(QRegularExpression("[!-/:-@[-^`{-~]"))){
        logs.warning("Наименование не корректно: имя содержит не корректные символы!");
        return false;
    }
    if(QString(name[0]).contains(QRegularExpression("[0-9]"))){
        logs.warning("Наименование не корректно: имя начинается с цифры!");
        return false;
    }
    return true;
}
