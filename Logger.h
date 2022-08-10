#pragma once

#include <fstream>
#include <string>
#include "windows.h"
#include <iostream>
#include <ctime> //либа для вывода текущего времени в лог
#include <QString>

#define DEBUG

class Logger
{
public:
	
	Logger(const char* nameLogFile);
	~Logger();
	
    bool operator<<(const QString message);
    bool warning(const QString message);
    bool error(const QString message);
    bool debug(const QString message);

private:
//    Logger() {}
    bool writeFile(const std::string message);
	std::string timeToLogs();
	std::string fileName;
	std::string m_msg;
	std::ofstream logs;
	//Набор переменных для вывода даты в лог
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80]; // строка, в которой будет храниться текущее время
};
