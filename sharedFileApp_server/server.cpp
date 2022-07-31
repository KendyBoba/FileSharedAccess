#include "server.h"
#include <string>
#include <vector>

#pragma warning(disable : 4996)

DWORD WINAPI read_event(LPVOID param) {
	packetForThred* pth = (packetForThred*)param;
	while (*pth->work) {
		msg_type *type = new msg_type;
		recv(*pth->socket, (char*)type, sizeof(msg_type), 0);
		int size = 0;
		recv(*pth->socket, (char*)&size, sizeof(int), 0);
		if (!size) continue;
		pth->server->read(type,size,pth->socket);
	}
	return 0;
}

bool server::initLib()
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 1), &wsadata)!=0) {
		ConsolePrint("WsaSturtup failed");
		return false;
	}
	return true;
}

server::server()
{
	clientsFileMutex = CreateMutex(NULL, false, NULL);
	fileMutex = CreateMutex(NULL, false, NULL);
	accessFileMutex = CreateMutex(NULL, false, NULL);
	allClientsMutex = CreateMutex(NULL, false, NULL);
	ConsoleMutex = CreateMutex(NULL, false, NULL);
}

server::~server()
{
	for (std::pair<SOCKET*, user*> &el : allClients) {
		msg_type type = msg_type::disconnect;
		Send(*el.first, &type, " ");
		closesocket(*el.first);
		delete el.first;
		delete el.second;
	}
	allClients.clear();
	for (auto el : allOpenedFiles) {
		delete el->buff;
		delete el;
	}
	allOpenedFiles.clear();
	closesocket(listenSocket);
	for (bool* el : threadsOfClients) {
		*el = false;
	}
	threadsOfClients.clear();
	delete clientsBuff;
	WSACleanup();
}

void server::start(std::string ip)
{
	if (!initLib()) {
		return;
	}
	ConsolePrint("server starts...");
	initDirs();
	ZeroMemory(&listenAddres, sizeof(SOCKADDR_IN));
	listenAddres.sin_family = AF_INET;
	listenAddres.sin_port = htons(port);
	if(ip.empty())
		listenAddres.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	else
		listenAddres.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
	listenSocket = socket(AF_INET, SOCK_STREAM, NULL);
	if (listenSocket == INVALID_SOCKET) {
		ConsolePrint("Socket not created");
		return;
	}
	if (bind(listenSocket, (SOCKADDR*)&listenAddres, sizeof(listenAddres)) == SOCKET_ERROR) {
		ConsolePrint("Bind failed");
		return;
	}
	ConsolePrint("server started");
	while (true) {
		listen(listenSocket, SOMAXCONN);
		SOCKET *new_socket = new SOCKET;
		*new_socket = accept(listenSocket, NULL,NULL);
		if (*new_socket == INVALID_SOCKET) {
			ConsolePrint("accept failed");
			return;
		}

		WaitForSingleObject(ConsoleMutex, INFINITE);
		SYSTEMTIME sys_time;
		GetLocalTime(&sys_time);
		std::cout << "Client connected time: " << sys_time.wDay << "-" << sys_time.wMonth << "-" << sys_time.wYear << " Socket: " << std::to_string(*new_socket) << std::endl;
		ReleaseMutex(ConsoleMutex);
		
		bool work = true;
		packetForThred* pth = new packetForThred{ this,new_socket ,&work};
		CreateThread(NULL, 0, read_event, (LPVOID*)pth, 0, NULL);
		threadsOfClients.push_back(&work);
		allClients.push_back(std::make_pair(new_socket,nullptr));
		
		Send(*new_socket, new msg_type(msg_type::connect), "connect");
	}
}

void server::Send(SOCKET sock,msg_type *type, const char* data,int size)
{
	int n_size;
	if (!size)
		n_size = std::strlen(data);
	else
		n_size = size;
	send(sock, (char*)type, sizeof(msg_type), 0);
	send(sock, (char*)&n_size, sizeof(int), 0);
	send(sock, (char*)data, n_size, 0);
}

void server::read(msg_type* type, int size, SOCKET * sock)
{
	switch (*type)
	{
	case msg_type::add_user: {
		char* data = new char[size];
		recv(*sock, (char*)data, size, 0);
		buffer buff(data, size);
		int* sizeUser = (int*)buff.read(sizeof(int));
		char* user_name = (char*)buff.read(*sizeUser);
		user_name[*sizeUser] = '\0';
		int* sizeFile_name = (int*)buff.read(sizeof(int));
		char* file_name = (char*)buff.read(*sizeFile_name);
		file_name[*sizeFile_name] = '\0';
		user *u = getUser(*sock);
		if (u->login == std::string(user_name)) {
			msg_type type = msg_type::message;
			Send(*sock, &type, "you cant add yourself");
			break;
		}
		if (isNameInFileAccess(u->login, file_name) && this->isUserExist(user_name)) {
			WaitForSingleObject(accessFileMutex, INFINITE);
			addUserToAccessFile(file_name, user_name);
			ReleaseMutex(accessFileMutex);
			msg_type type = msg_type::message;
			Send(*sock, &type, "user added");
		}
		else {
			msg_type type = msg_type::message;
			Send(*sock, &type, "user does not exist or you don't own the file");
		}
	}; break;
	case msg_type::del_user: {
		char* data = new char[size];
		recv(*sock, (char*)data, size, 0);
		buffer buff(data, size);
		int* sizeUser = (int*)buff.read(sizeof(int));
		char* user_name = (char*)buff.read(*sizeUser);
		user_name[*sizeUser] = '\0';
		int* sizeFile_name = (int*)buff.read(sizeof(int));
		char* file_name = (char*)buff.read(*sizeFile_name);
		file_name[*sizeFile_name] = '\0';
		std::string fileName = file_name;
		std::string userName = user_name;
		user* u = getUser(*sock);
		WaitForSingleObject(accessFileMutex, INFINITE);
		auto el = std::find_if(accessList.begin(), accessList.end(), [fileName](const access_struct & cur_el)->bool{
			if (cur_el.file_name == fileName) return true;
			return false;
		});
		if (u->login == userName) {
			msg_type type = msg_type::message;
			Send(*sock, &type, "you cant remove yourself");
			ReleaseMutex(accessFileMutex);
			break;
		}
		if (el == accessList.end()) {
			msg_type type = msg_type::message;
			Send(*sock, &type, "file not found");
			ReleaseMutex(accessFileMutex);
			break;
		}
		if (!isUserExist(user_name)) {
			msg_type type = msg_type::message;
			Send(*sock, &type, "user does not exist");
			ReleaseMutex(accessFileMutex);
			break;
		}
		if (el->clients_names.front() != u->login) {
			msg_type type = msg_type::message;
			Send(*sock, &type, "you are not the owner of the file");
			ReleaseMutex(accessFileMutex);
			break;
		}
		auto delIt = std::find_if(el->clients_names.begin(), el->clients_names.end(), [userName](const std::string &name)->bool {
			if (name == userName) return true;
			return false;
		});
		el->clients_names.erase(delIt);
		SaveAccessFile();
		ReleaseMutex(accessFileMutex);
		msg_type type = msg_type::message;
		Send(*sock, &type, "user deleted");
	}; break;
	case msg_type::user_log: {
		bool access = false;
		user* cur_user = new user;
		recv(*sock, (char*)cur_user, size,0);
		WaitForSingleObject(clientsFileMutex, INFINITE);
		clientsBuff->seekR(0);
		while (clientsBuff->getLength() > clientsBuff->getPointR()) {
			user* temp_user = (user*)clientsBuff->read(sizeof(user));
			if (temp_user->login == cur_user->login && temp_user->password == cur_user->password) {
				access = true;
				for (auto& el : allClients) {
					if (*sock == *el.first) { el.second = temp_user; break; };
				}
				break;
			}
		}
		ReleaseMutex(clientsFileMutex);
		if (!access) {
			msg_type type = msg_type::n_access;
			Send(*sock, &type, "n_access");
			return;
		}
		msg_type type = msg_type::access;
		Send(*sock, &type, (char*)(cur_user->login.c_str()));
		msg_type t = msg_type::file_name;
		for (const auto& el : *this->getFileNamesOfUser(cur_user->login)) {
			Send(*sock, &t, (char*)(el.c_str()));
		}
		SYSTEMTIME sys_time;
		GetSystemTime(&sys_time);
		ConsolePrint(cur_user->login + " log in" + " " + std::to_string(sys_time.wHour) + "-" + std::to_string(sys_time.wDay) + "-" + std::to_string(sys_time.wMonth) + "-" + std::to_string(sys_time.wYear));
	}; break;
	case msg_type::user_reg: {
		user* cur_user = new user;
		recv(*sock, (char*)cur_user, size, 0);
		WaitForSingleObject(clientsFileMutex, INFINITE);
		bool is_exist_user = false;
		clientsBuff->seekR(0);
		while (clientsBuff->getPointR() < clientsBuff->getLength()) {
			user* temp_user = (user*)clientsBuff->read(sizeof(user));
			if (temp_user->login == cur_user->login) {
				is_exist_user = true;
				break;
			}
		}
		if (is_exist_user) {
			msg_type type = msg_type::message;
			Send(*sock, &type,"user already exists");
			ReleaseMutex(clientsFileMutex);
			break;
		}
		clientsBuff->write((char*)cur_user, sizeof(user));
		WriteFileClients(clientsFilePath);
		ReleaseMutex(clientsFileMutex);
		msg_type type = msg_type::user_reg;
		Send(*sock, &type, "account registered");
	}; break;
	case msg_type::delete_acc: {
		user* cur_user = new user;
		recv(*sock, (char*)cur_user, size, 0);
		WaitForSingleObject(clientsFileMutex, INFINITE);
		clientsBuff->seekR(0);
		while (clientsBuff->getSize() > clientsBuff->getPointR()) {
			user* temp_user = (user*)clientsBuff->read(sizeof(user));
			if (temp_user->login == cur_user->login) {
				int pos = clientsBuff->getPointR() - sizeof(user);
				clientsBuff->cut(pos, sizeof(user));
				clientsBuff->seekW(pos);
				WriteFileClients(clientsFilePath);
				break;
			}
		}
		ReleaseMutex(clientsFileMutex);
	}; break;
	case msg_type::create_file: {
		char* file_name = new char[FILENAME_MAX];
		file_name[size] = '\0';
		recv(*sock, file_name, size, 0);
		std::string name = file_name;
		if (!CreateNewFile(name)) {
			msg_type type = msg_type::message;
			Send(*sock, &type, "file_exist", 0);
		}
		else {
			msg_type type = msg_type::file_name;
			for (const auto& el : allClients) {
				Send(*el.first, &type, file_name, 0);
			}
			WaitForSingleObject(accessFileMutex, INFINITE);
			user * cur_user = getUser(*sock);
			if(cur_user)
			addUserToAccessFile(name, cur_user->login);
			ReleaseMutex(accessFileMutex);
		}
	}; break;
	case msg_type::open_file: {
		char* file_name = new char[size+1];
		file_name[size] = '\0';
		recv(*sock, file_name, size, 0);
		std::string name = file_name;
		file_struct * info = OpenFile(name);
		msg_type type = msg_type::open_file;
		if (!info  || !isNameInFileAccess(getUser(*sock)->login, name)) {
			type = msg_type::message;
			Send(*sock, &type, "file not found");
			return;
		}
		if(!info->buff->getSize())
			Send(*sock, &type, info->buff->data(), -1);
		else
			Send(*sock, &type, info->buff->data(), info->buff->getSize());
	}; break;
	case msg_type::close_file: {
		char* file_name = new char[FILENAME_MAX];
		file_name[size] = '\0';
		recv(*sock, file_name, size, 0);
		std::string name = file_name;
		CloseFile(name);
	}; break;
	case msg_type::update_file: {
		char* data = new char[size];
		recv(*sock, data, size, 0);
		buffer buff(data, size);
		size_t* size_name = (size_t*)buff.read(sizeof(size_t));
		char* file_name = (char*)buff.read(*size_name);
		sumbol_info* symbol = (sumbol_info*)buff.read(sizeof(sumbol_info));
		file_name[*size_name] = '\0';
		std::string fileName = file_name;
		UpdateFile(symbol,fileName);
		msg_type type = msg_type::update_file;
		for (const auto& el : allClients) {
			if (*el.first != *sock && this->isNameInFileAccess(el.second->login, fileName))
				Send(*el.first, &type, (char*)symbol, sizeof(sumbol_info));
		}
	}; break;
	case msg_type::delete_file: {
		char* file_name = new char[FILENAME_MAX];
		file_name[size] = '\0';
		recv(*sock, file_name, size, 0);
		std::string name = file_name;
		if (!isNameInFileAccess(getUser(*sock)->login, name) || !deleteFile(name)) {
			msg_type type = msg_type::message;
			Send(*sock, &type, "file not found");
		}
		else {
			msg_type type = msg_type::delete_file;
			for (const auto& el : allClients) {
				if (this->isNameInFileAccess(el.second->login, name))
					Send(*el.first, &type, file_name);
			}
		}
		WaitForSingleObject(accessFileMutex, INFINITE);
		for (auto it = accessList.begin(); it != accessList.end(); ++it) {
			if (it->file_name != name) continue;
			accessList.erase(it);
			break;
		}
		DeleteFileA((clientsDirPath + "\\access.bin").c_str());
		SaveAccessFile();
		ReleaseMutex(accessFileMutex);
	}; break;
	case msg_type::get_all_filenames: {
		char* data = new char[FILENAME_MAX];
		data[size] = '\0';
		recv(*sock, data, size, 0);
		msg_type t = msg_type::file_name;
		for (const auto& el : *getFileNamesOfUser(getUser(*sock)->login)) {
			Send(*sock, &t, (char*)(el.c_str()));
		}
	}; break;
	case msg_type::disconnect: {
		char* data = new char[FILENAME_MAX];
		data[size] = '\0';
		recv(*sock, data, size, 0);
		WaitForSingleObject(allClientsMutex, INFINITE);
		int i = -1;
		std::deque<std::pair<SOCKET*, user*>>::iterator find_it = allClients.end();
		for (auto it = allClients.begin(); it != allClients.end(); ++it) {
			++i;
			if (*it->first != *sock) continue;
			*threadsOfClients[i] = false;
			closesocket(*sock);
			find_it = it;
		}
		if (find_it != allClients.end()) {
			allClients.erase(find_it);
			threadsOfClients.erase(threadsOfClients.begin() + i);
		}
		ReleaseMutex(allClientsMutex);

		WaitForSingleObject(ConsoleMutex, INFINITE);
		SYSTEMTIME sys_time;
		GetLocalTime(&sys_time);
		std::cout << "Client disconnected time: " << sys_time.wDay << "-" << sys_time.wMonth << "-" << sys_time.wYear << " Socket: " << std::to_string(*sock) << std::endl;
		ReleaseMutex(ConsoleMutex);
	}; break;
	}
}

void server::initDirs()
{
	char *cur_path = new char[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH,cur_path);
	std::string clientsDir = cur_path; clientsDir += +"\\clients";
	CreateDirectoryA(clientsDir.c_str(), NULL);
	clientsDirPath = clientsDir;
	std::string fileName = clientsDir + "\\clients.bin";
	clientsFilePath = fileName;
	ReadFileClients(fileName);
	ReadAccessFile(clientsDir + "\\access.bin");
	std::string filesDir = cur_path;filesDir +="\\files";
	CreateDirectoryA(filesDir.c_str(), NULL);
	filesDirPath = filesDir + "\\";
}

void server::ReadFileClients(const std::string& path)
{
	HANDLE clientsFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() == ERROR_FILE_NOT_FOUND) {
		clientsFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	if (clientsFile == INVALID_HANDLE_VALUE) {
		return;
	}
	DWORD size = GetFileSize(clientsFile, NULL);
	char* cstr_buff = new char[size];
	DWORD cur_size = 0;
	if (size) {
		if (!ReadFile(clientsFile, cstr_buff, size, &cur_size, NULL)) {
			CloseHandle(clientsFile);
			return;
		}
	}
	clientsBuff = new buffer(cstr_buff,cur_size);
	clientsBuff->seekW(size);
	CloseHandle(clientsFile);
}

void server::WriteFileClients(const std::string& path)
{
	HANDLE clientsFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() == ERROR_FILE_NOT_FOUND) {
		clientsFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	if (clientsFile == INVALID_HANDLE_VALUE) {
		return;
	}
	if(clientsBuff)
		WriteFile(clientsFile, (LPVOID)clientsBuff->data(), clientsBuff->getLength(), NULL, NULL);
	CloseHandle(clientsFile);
}

void server::ReadAccessFile(const std::string& path)
{
	HANDLE fileAccess = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() == ERROR_FILE_NOT_FOUND) {
		fileAccess = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	if (fileAccess == INVALID_HANDLE_VALUE) {
		return;
	}
	DWORD size = GetFileSize(fileAccess, NULL);
	char* cstr_buff = new char[size];
	DWORD cur_size = 0;
	if (size) {
		if (!ReadFile(fileAccess, cstr_buff, size, &cur_size, NULL)) {
			CloseHandle(fileAccess);
			return;
		}
	}
	buffer accessBuff(cstr_buff, cur_size);
	while (accessBuff.getLength() > accessBuff.getPointR()) {
		access_struct new_access_el;
		unsigned int* fname_s = (unsigned int*)accessBuff.read(sizeof(unsigned int));
		char* fname = (char*)accessBuff.read(*fname_s);
		fname[*fname_s] = '\0';
		new_access_el.file_name = fname;
		size_t* el_s = (size_t*)accessBuff.read(sizeof(size_t));
		for (int i = 0; i < *el_s; ++i) {
			unsigned int* uname_s = (unsigned int*)accessBuff.read(sizeof(unsigned int));
			char* uname = (char*)accessBuff.read(*uname_s);
			uname[*uname_s] = '\0';
			new_access_el.clients_names.push_back(std::string(uname));
		}
		accessList.push_back(new_access_el);
	}
	CloseHandle(fileAccess);
}

bool server::CreateNewFile(const std::string& name)
{
	std::string path = filesDirPath + name;
	HANDLE new_file = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() == ERROR_FILE_EXISTS) {
		return false;
	}
	CloseHandle(new_file);
	return true;
}

file_struct* server::OpenFile(const std::string& name)
{

	WIN32_FIND_DATAA  find_file_data;
	HANDLE hFind = FindFirstFileA((filesDirPath + "*").c_str(), &find_file_data);
	HANDLE resFile = NULL;
	do {
		std::string fname = find_file_data.cFileName;
		if (fname == "." || fname == "..") continue;
		if (fname == name) {
			resFile = CreateFileA((filesDirPath + fname).c_str(), GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
			break;
		}
	} while (FindNextFileA(hFind,&find_file_data) );
	if (resFile == INVALID_HANDLE_VALUE || !resFile) {
		return nullptr;
	}
	DWORD size = GetFileSize(resFile, NULL);
	char* data = new char[size];
	DWORD cur_size = 0;
	if (size) {
		if (!ReadFile(resFile, data, size, &cur_size, NULL)) {
			CloseHandle(resFile);
			return nullptr;
		}
	}
	CloseHandle(resFile);
	buffer* fileBuff = new buffer(data, cur_size);

	file_struct* res = new file_struct;
	res->buff = fileBuff;
	res->file_name = name;
	res->clients = 1;

	WaitForSingleObject(fileMutex, INFINITE);
	for (auto& el : allOpenedFiles) {
		if (el->file_name == name) {
			el->clients += 1;
			ReleaseMutex(fileMutex);
			return el;
			break;
		}
	}
	allOpenedFiles.push_back(res);
	ReleaseMutex(fileMutex);
	return res;
}

void server::CloseFile(const std::string& name)
{
	auto it = allOpenedFiles.begin();
	for (auto& el : allOpenedFiles) {
		if (el->file_name == name) {
			WaitForSingleObject(fileMutex, INFINITE);
			el->clients -= 1;
			if (el->clients <= 0) { writeFile(el->file_name, el->buff); allOpenedFiles.erase(it); };
			ReleaseMutex(fileMutex);
			break;
		}
		std::advance(it, 1);
	}
}

void server::UpdateFile(sumbol_info* symbol,const std::string &file_name)
{
	for (const auto& el : allOpenedFiles) {
		if (file_name != el->file_name) continue;
		WaitForSingleObject(fileMutex, INFINITE);
		el->buff->write((char*)symbol, sizeof(sumbol_info), symbol->pos * sizeof(sumbol_info));
		ReleaseMutex(fileMutex);
	}
}

void server::writeFile(const std::string& name,buffer* buff)
{
	HANDLE file = CreateFileA((filesDirPath + name).c_str(), GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		return;
	}
	WriteFile(file, buff->data(), buff->getLength(), NULL, NULL);
	CloseHandle(file);
}

bool server::deleteFile(const std::string& name)
{
	std::vector<std::string>* all_file_names = getAllFileNames();
	std::vector<std::string>::iterator it = std::find(all_file_names->begin(), all_file_names->end(), name);
	if (it == all_file_names->end()) return false;
	std::string path = filesDirPath + name;
	WaitForSingleObject(fileMutex, INFINITE);
	bool res = DeleteFileA(path.c_str());
	ReleaseMutex(fileMutex);
	return res;
}

void server::addUserToAccessFile(const std::string& file_name, const std::string& user_name)
{
	auto it = accessList.begin();
	for (it; it != accessList.end(); ++it) {
		if (it->file_name == file_name) {
			auto el = std::find_if(it->clients_names.begin(), it->clients_names.end(), [&user_name](const std::string& el) {
				if (el == user_name) return true;
				return false;
			});
			if(el == it->clients_names.end())
				it->clients_names.push_back(user_name);
			break;
		}
	}
	if (it == accessList.end()) {
		access_struct temp; temp.file_name = file_name; temp.clients_names.push_back(user_name);
		accessList.push_back(temp);
	}
	SaveAccessFile();
}

std::vector<std::string>* server::getAllFileNames()
{
	WIN32_FIND_DATAA find_data;
	HANDLE hFind = FindFirstFileA((filesDirPath + "*").c_str(), &find_data);
	std::vector<std::string> *res = new std::vector<std::string>;
	do {
		std::string name = find_data.cFileName;
		if (name == "." || name == "..") continue;
		res->push_back(name);
	} while (FindNextFileA(hFind,&find_data));
	return res;
}

void server::SaveAccessFile()
{
	HANDLE file = CreateFileA((clientsDirPath + "\\access.bin").c_str(), GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() == ERROR_FILE_NOT_FOUND) {
		file = CreateFileA((clientsDirPath + "\\access.bin").c_str(), GENERIC_ALL, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	if (file == INVALID_HANDLE_VALUE) return;
	char* data = new char[0];
	buffer buff(data, 0);
	for (auto it = accessList.begin(); it != accessList.end(); ++it) {
		unsigned int fname_s = it->file_name.length();
		buff.write((char*)&fname_s, sizeof(unsigned int));
		buff.write((char*)it->file_name.c_str(), fname_s);
		size_t el_s = it->clients_names.size();
		buff.write((char*)&el_s, sizeof(size_t));
		for (const auto& el : it->clients_names) {
			unsigned int uname_s = el.length();
			buff.write((char*)&uname_s, sizeof(unsigned int));
			buff.write((char*)el.c_str(), uname_s);
		}
	}
	WriteFile(file, buff.data(), buff.getLength(), NULL, NULL);
	SetFilePointer(file, buff.getLength(), 0, FILE_BEGIN);
	SetEndOfFile(file);
	CloseHandle(file);
}

std::vector<std::string>* server::getFileNamesOfUser(const std::string& user_name)
{
	std::vector<std::string>* res = new std::vector<std::string>;
	for (auto it = accessList.begin(); it != accessList.end(); ++it) {
		auto el = std::find_if(it->clients_names.begin(), it->clients_names.end(), [user_name](const std::string& uname) {
			if (user_name == uname) return true;
			return false;
		});
		if (el != it->clients_names.end()) res->push_back(it->file_name);
	}
	return res;
}

user* server::getUser(SOCKET sock)
{
	auto it = std::find_if(allClients.begin(), allClients.end(), [&sock](const std::pair<SOCKET*, user*>& el) {
		if (*el.first == sock) return true;
		return false;
	});
	if (it != allClients.end()) return it->second;
	return nullptr;
}

bool server::isNameInFileAccess(const std::string& user_name, const std::string& file_name) {
	for (auto it = accessList.begin(); it != accessList.end(); ++it) {
		if (it->file_name != file_name) continue;
		for (auto& el : it->clients_names) {
			if (el == user_name) return true;
		}
	}
	return false;
}

bool server::isUserExist(const std::string& user_name)
{
	WaitForSingleObject(clientsFileMutex, INFINITE);
	clientsBuff->seekR(0);
	while (clientsBuff->getLength() > clientsBuff->getPointR()) {
		user* temp_user = (user*)clientsBuff->read(sizeof(user));
		if (temp_user->login == user_name) {
			ReleaseMutex(clientsFileMutex);
			return true;
		}
	}
	ReleaseMutex(clientsFileMutex);
	return false;
}

void server::ConsolePrint(const std::string& text)
{
	WaitForSingleObject(ConsoleMutex, INFINITE);
	std::cout << text << std::endl;
	ReleaseMutex(ConsoleMutex);
}
