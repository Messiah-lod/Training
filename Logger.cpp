#include "Logger.h"



Logger::Logger(const char* nameLogFile)
{
    CreateDirectoryW(L"logs", nullptr);
	fileName = nameLogFile;
}

bool Logger::writeFile(const std::string message)
{
    logs.open("./logs/"+ fileName, std::ios_base::app);//открываем(создаем) файл с записью в конец
    if(!logs.is_open()) return false;
    logs << message << std::endl;
    logs.close();
    return true;//вернет тру при успешном открытии файла и записи туда
}

bool Logger::operator<<(const QString message)
{
    return writeFile(timeToLogs() + message.toStdString());
}

bool Logger::warning(const QString message)
{
    return writeFile(timeToLogs() + "[WARNING] " + message.toStdString());
}

bool Logger::error(const QString message)
{
    return writeFile(timeToLogs() + "[ERROR] " + message.toStdString());
}

bool Logger::debug(const QString message)
{
#ifdef DEBUG
    return writeFile(timeToLogs() + "[DEBUG] " + message.toStdString());
#else
    Q_UNUSED(message)
    return true;
#endif
}

std::string Logger::timeToLogs()
{
	time(&rawtime); // текущая дата в секундах
	timeinfo = localtime(&rawtime);// текущее локальное время, представленное в структуре
	strftime(buffer, sizeof(buffer), "%d.%m.%Y %X", timeinfo); // форматируем строку времени
	return "(" + std::string(buffer) + "." + std::to_string(GetTickCount() % 1000) + ")	";
}


Logger::~Logger()
{
}

