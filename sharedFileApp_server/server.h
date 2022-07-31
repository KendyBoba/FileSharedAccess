#pragma once
#include <Windows.h>
#include <winsock.h>
#include <iostream>
#include "serverint.h"
#include <deque>
#include "general.h"
#include <utility>
#include <algorithm>
#include <list>

struct client_info {
	SOCKET* socket = nullptr;
	SOCKADDR* addr = nullptr;
	int addr_size = 0;
};

struct packetForThred {
	protoServer* server = nullptr;
	SOCKET* socket = nullptr;
	bool* work = nullptr;
};

struct file_struct{
	std::string file_name = "";
	buffer* buff = nullptr;
	int clients = 0;
};

struct access_struct {
	std::string file_name = "";
	std::deque<std::string> clients_names;
};

DWORD WINAPI read_event(LPVOID param);

class server : public protoServer
{
private:
	SOCKET listenSocket;
	SOCKADDR_IN listenAddres;
	int port = 3333;
	std::vector<bool*> threadsOfClients;
	std::deque<std::pair<SOCKET*,user*>> allClients;
	std::deque<file_struct*> allOpenedFiles;
	std::list<access_struct> accessList;
	buffer* clientsBuff = nullptr;
	std::string clientsFilePath = "";
	std::string filesDirPath = "";
	std::string clientsDirPath = "";
	HANDLE clientsFileMutex;
	HANDLE fileMutex;
	HANDLE accessFileMutex;
	HANDLE allClientsMutex;
	HANDLE ConsoleMutex;
private:
	bool initLib();
public:
	server();
	~server();
	void start(const std::string ip);
private:
	void Send(SOCKET sock,msg_type *type, const char* data,int size = 0);
	virtual void read(msg_type* type, int size,SOCKET * sock)override;
	void initDirs();
	void ReadFileClients(const std::string &path);
	void WriteFileClients(const std::string &path);
	void ReadAccessFile(const std::string& path);
	void addUserToAccessFile(const std::string& file_name, const std::string& user_name);
	void SaveAccessFile();
	bool CreateNewFile(const std::string &name);
	file_struct* OpenFile(const std::string &name);
	void CloseFile(const std::string& name);
	void UpdateFile(sumbol_info * sumbol, const std::string& file_name);
	void writeFile(const std::string & name,buffer * buff);
	bool deleteFile(const std::string& name);
	std::vector<std::string>* getAllFileNames();
	std::vector<std::string>* getFileNamesOfUser(const std::string& user_name);
	user* getUser(SOCKET sock);
	bool isNameInFileAccess(const std::string& user_name, const std::string& file_name);
	bool isUserExist(const std::string& user_name);
	void ConsolePrint(const std::string &text);
};

