#pragma comment(lib, "Ws2_32.lib")			// библиотека сокетов
#include <WinSock2.h>						// подключение сокетов
#include <iostream>							// подключение потоков
#include <WS2tcpip.h>						// библиотека для работы (подключение, получение информации от сервера, и т.д.)
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <thread>
#include <ctime>
#include <time.h>
#include <mysql.h>

SOCKET Connect;					// Создание сокета, отвечающего за подключаемых пользователей
SOCKET* Connections;			// Коллекция сокетов, подлючившихся пользователей
SOCKET Listen;					// Сокет для подключения
MYSQL* conn;					// Получаем дескриптор соединения

using namespace std;

int ClientCount = 0;	// Количество подключенных пользоваетелей
char strTime[80];

void LoadInfoServer();			// Проверка необходимого функционала после запуска сервера
void SetTimeForMess(char* strTime);	// Устанавливает текущую дату
void SeparateStr(const string& str, vector<string>& tokens, const string& delimiters);
void WriteToLog(vector<string> arrStr);
void SendMessageToClient(int id);

int main() {
	setlocale(LC_ALL, "Rus");
	WSAData data;
	WORD version = MAKEWORD(2, 2);						// Указываем версию сокетов 2.2
	int res = WSAStartup(version, &data);				/* WSAStartup(version, &data) инициализация сокетов
															res отвечает за результат: подключился сокет или нет */
	if (res != 0)
		return 0;

	struct addrinfo hints;
	struct addrinfo* result;

	Connections = (SOCKET*)calloc(64, sizeof(SOCKET));	// Инициализация и выделение памяти под коллекцию сокетов

	ZeroMemory(&hints, sizeof(hints));					// Очистка структуры hints

	hints.ai_family = AF_INET;							// Задаём тип сокета
	hints.ai_flags = AI_PASSIVE;						// Задаём флаг
	hints.ai_socktype = SOCK_STREAM;					// Задаём тип сокету TCP
	hints.ai_protocol = IPPROTO_TCP;

	getaddrinfo(NULL, "1234", &hints, &result);			// Получаем информацию о хосте

	Listen = socket(result->ai_family, result->ai_socktype, result->ai_protocol);	// Настройка сокета Listen
	::bind(Listen, result->ai_addr, result->ai_addrlen);							// Объявление сервера
	::listen(Listen, SOMAXCONN);													//	Настройка подключения

	freeaddrinfo(result);								// Удаление информации, т.к. все сокеты уже настроены

	SetTimeForMess(strTime);
	printf("<%s> Start server ...\n", strTime);

	const char* messToClient = new char[1024];	// Строка, отправляемая клиенту
	string strbuff;								// Буфер
	vector<string> arrStr;
	char buff[250];								// буфер промежуточного хранения считываемого из файла текста
	char* messFromClient = new char[1024];		// Строка, получаемая сервером

	LoadInfoServer();			// Подключение к БД

	// подключение пользователя
	for (;; Sleep(75)) {

		if (Connect = accept(Listen, NULL, NULL)) {				// Если есть подключение клиента
			SetTimeForMess(strTime);
			printf("<%s> Client connect (%u)...\n", strTime, ClientCount);
			Connections[ClientCount] = Connect;					// Сохраняем сокет подключаемого клиента
			strbuff = "setYourID;";
			_itoa_s(ClientCount, buff, 5, 10);					// Преобразование ClientCount в строку buff длиной 5, в 10-ой системе
			strbuff = strbuff + buff;
			messToClient = strbuff.c_str();						// Преобразуем из string в const char
			SetTimeForMess(strTime);
			printf("<%s> Команда: %s \n", strTime, messToClient);

			send(Connections[ClientCount], messToClient, strlen(messToClient), NULL);	// Отправляем сообщение подлючившемуся клиенту
			ClientCount++;										// Количество подключенных клиентов +1
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SendMessageToClient, (LPVOID)(ClientCount - 1), NULL, NULL);
		}
	}
		
	return 0;
}

void LoadInfoServer() {
	conn = mysql_init(NULL);
	SetTimeForMess(strTime);
	if (conn == NULL)
	{		
		printf("<%s> Ошибка! Невозможно создать MySQL-дискриптор\n", strTime);
	}
	if (!mysql_real_connect(conn, "localhost", "SarcVelL", "WcadQvse77291DW", "LogMessDB", 3306, NULL, 0))
	{
		printf("<%s> Ошибка! Невозможно подключиться к БД %s\n", strTime, mysql_error(conn));
	}
	if(conn)
	{
		printf("<%s> Соединение с БД установлено\n", strTime);
	}

}

void SetTimeForMess(char* strTime) {
	time_t t = time(0);   // получить текущее время
	struct tm now;
	localtime_s(&now, &t);
	const char* format = "%d.%m.%Y %H:%M:%S";
	strftime(strTime, 80, format, &now);
}

// Разбиение из string в vector<string>
void SeparateStr(const string& str, vector<string>& tokens, const string& delimiters)
{
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	string::size_type pos = str.find_first_of(delimiters, lastPos);
	while (string::npos != pos || string::npos != lastPos)
	{
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		lastPos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, lastPos);
	}
}

void WriteToLog(vector<string> arrStr/*string mess*/) {
	/*ofstream fout("Log.txt", ios::app);								// открыли файл для записи
	if (!fout.is_open()) {													// если файл не открыт
		SetTimeForMess(strTime);
		cout << "<" + (string)strTime + "> Файл Log.txt не может быть открыт!\n";	// сообщить об этом
	}
	else {
		fout << mess;
	}
	fout.close();*/

	SetTimeForMess(strTime);
	char command[255];
	strcpy_s(command, "INSERT INTO LogMess (date, name, message) VALUES ('");
	strcat_s(command, strTime);
	strcat_s(command, "', '");
	strcat_s(command, arrStr[2].c_str());
	strcat_s(command, "', '");
	strcat_s(command, arrStr[3].c_str());
	strcat_s(command, "')");

	if (mysql_query(conn, command)) {
		printf("<%s> Ошибка отправки команды в БД %s", strTime, command);
	}
}

// Функция для каждого клиента в отдельном потоке
void SendMessageToClient(int id) {
	char buffer[1024];
	vector<string> arrStr;
	string strbuff;

	for (;; Sleep(75)) {
		memset(buffer, 0, sizeof(buffer));
		if (recv(Connections[id], buffer, 1024, NULL)) {

			SetTimeForMess(strTime);
			printf("<%s> %s \n", strTime, buffer);
			arrStr.clear();
			strbuff = buffer;
			SeparateStr(strbuff, arrStr, ";");

			if (arrStr.size() > 2) {
				if (arrStr[1] == "sendMess") {
					SetTimeForMess(strTime);

					strbuff = "newMess;<";
					strbuff += strTime;
					strbuff += "> " + arrStr[2] + ": " + arrStr[3] + "\n";
					WriteToLog(arrStr);

					strncpy_s(buffer, strbuff.c_str(), sizeof(buffer) - 1);
					buffer[sizeof(buffer) - 1] = 0;

					for (int i = 0; i < ClientCount; i++) {
						send(Connections[i], buffer, strlen(buffer), NULL);
					}
				}
			}
		}
	}
}