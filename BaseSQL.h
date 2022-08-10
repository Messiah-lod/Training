#pragma once

#include "Logger.h"

#include <exception>
#include "windows.h"
#include <map>
#include <iostream>
#include <any>
#include <bitset>
#include <ctime> //либа для вывода текущего времени в лог
#include <vector>
#include <QtSql>
#include <QString>
#include <algorithm>

class BaseSQL
{
public:
	//методы, позволяющие получить простой доступ к БД (чтение/запись)
	BaseSQL(std::string webserver_, std::string path_, std::string user_, std::string pass_);

	void setConnectParam(std::string webserver_, std::string path_, std::string user_, std::string pass_);
	~BaseSQL();
	bool connected();
	void disconnect(); //отключение от БД

    void write(std::string query);
    void read(std::string query, std::string table, std::string collAnswer, std::any &answ);
    void read(std::string query, std::string table, std::string collAnswer, std::vector <std::any> &answ);

    std::string any_to_str(std::any parameter);//сервисный метод конвертации
    int any_to_int(std::any parameter);//сервисный метод конвертации

protected:
	//методы, предназначенные для дочерних классов
	Logger logs{ "logsSQL" };
	std::string query; //общая переменная для записи запросов


	void IntToBinary(int num, std::string  &buf);//сервисный метод конвертации
    int color_convet(std::string FONCOLOR);
    bool checkingIsCorrectName(std::string m_name); //проверка корректности имени/марки и т.п.

private:
	//локальные переменные, необходимые для работы класса
	std::string webserver;
	std::string path;
	std::string user;
	std::string pass;

    QSqlDatabase Database = QSqlDatabase::addDatabase("QIBASE");

    int connect();//подключение к БД
	
	std::string get_type_name(std::string table, std::string coll);//метод, возвращающий тип значения из таблицы БД
	
};
