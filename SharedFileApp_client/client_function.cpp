#include "client_function.h"

DWORD WINAPI cf::read_data(LPVOID param) {
	pth* p = (pth*)param;
	HWND wnd = p->wnd;
	SOCKET sock = p->sock;
	while (true) {
		msg_type type;
		recv(sock, (char*)&type, sizeof(msg_type), 0);
		int size = 0;
		recv(sock, (char*)&size, sizeof(int), 0);
		if (!size)continue;
		processing(sock,wnd, type, size);
	}
	return 0;
}

void cf::send_data(SOCKET sock, msg_type type, const char* data, int size)
{
	int n_size;
	if (!size)
		n_size = std::strlen(data);
	else
		n_size = size;
	send(sock, (char*)&type, sizeof(msg_type), 0);
	send(sock, (char*)&n_size, sizeof(int), 0);
	send(sock, (char*)data, n_size, 0);
}

void cf::processing(SOCKET sock,HWND wnd, msg_type type,int size)
{
	switch (type)
	{
	case msg_type::connect: {
		char* data = new char[size + 1];
		data[size] = '\0';
		recv(sock, (char*)data, size, 0);
		SendMessageA(wnd, MSG_CONNECT, 0, 0);
	}; break;
	case msg_type::access: {
		char* data = new char[size + 1];
		data[size] = '\0';
		recv(sock, (char*)data, size, 0);
		SendMessageA(wnd, MSG_ACCESS, (WPARAM)data, 0);
	}; break;
	case msg_type::user_reg: {
		char* data = new char[size + 1];
		data[size] = '\0';
		recv(sock, (char*)data, size, 0);
		SendMessageA(wnd, MSG_REG, (WPARAM)data, 0);
	}; break;
	case msg_type::n_access: {
		SendMessageA(wnd, MSG_NACCESS, 0, 0);
	}; break;
	case msg_type::file_name: {
		char* data = new char[size + 1];
		data[size] = '\0';
		recv(sock, (char*)data, size, 0);
		SendMessageA(wnd, MSG_FNAME, (WPARAM)data, 0);
	}; break;
	case msg_type::open_file: {
		if (size == -1) {
			char* file_data = new char[0];
			buffer* buff = new buffer(file_data, 0);
			SendMessageA(wnd, MSG_FDATA, (WPARAM)buff, 0);
			break;
		}
		char *file_data = new char[size];
		recv(sock, (char*)file_data, size, 0);
		buffer *buff = new buffer(file_data, size);
		SendMessageA(wnd, MSG_FDATA, (WPARAM)buff, 0);
	}; break;
	case msg_type::update_file: {
		sumbol_info* sumbol = new sumbol_info;
		recv(sock, (char*)sumbol, size, 0);
		SendMessageA(wnd, MSG_SUMBOL, (WPARAM)sumbol, 0);
	}; break;
	case msg_type::delete_file: {
		char* data = new char[size + 1];
		data[size] = '\0';
		recv(sock, (char*)data, size, 0);
		SendMessageA(wnd, MSG_FDELETE, 0, (LPARAM)data);
	}; break;
	case msg_type::message: {
		char* data = new char[size + 1];
		data[size] = '\0';
		recv(sock, (char*)data, size, 0);
		SendMessageA(wnd, MSG_MESSAGE, (WPARAM)data, 0);
	}; break;
	case msg_type::disconnect: {
		char* data = new char[size + 1];
		data[size] = '\0';
		recv(sock, (char*)data, size, 0);
		SendMessageA(wnd, MSG_DISCONNECT, (WPARAM)data, 0);
	}; break;
	}
}

bool cf::initLib()
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 1), &wsadata) != 0) {
		return false;
	}
	return true;
}

loginWnd* cf::initLoginWindow(HWND parent,bool show)
{
	static loginWnd *lwnd = new loginWnd;
	if (!show) {
		DestroyWindow(lwnd->loginLine);
		DestroyWindow(lwnd->passwordLine);
		DestroyWindow(lwnd->buttonLogin);
		DestroyWindow(lwnd->buttonReg);
		DestroyWindow(lwnd->staticLog);
		DestroyWindow(lwnd->staticPass);
		return nullptr;
	}
	lwnd->loginLine = CreateWindowA(WC_EDITA, "", WS_VISIBLE | WS_CHILD | ES_LEFT, 0, 0, 0, 0, parent, NULL, NULL, NULL);
	lwnd->passwordLine = CreateWindowA(WC_EDITA, "", WS_VISIBLE | WS_CHILD | ES_LEFT | ES_PASSWORD, 0, 0, 0, 0, parent, NULL, NULL, NULL);
	lwnd->buttonLogin = CreateWindowA(WC_BUTTONA, "login", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, NULL, NULL, NULL);
	lwnd->buttonReg = CreateWindowA(WC_BUTTONA, "register", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, NULL, NULL, NULL);
	lwnd->staticLog = CreateWindowA(WC_STATICA, "login", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, NULL, NULL, NULL);
	lwnd->staticPass = CreateWindowA(WC_STATICA, "password", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, NULL, NULL, NULL);
	return lwnd;
}

listWnd* cf::initListWindow(HWND parent,const char* full_name, bool show)
{
	std::string text = full_name;
	static listWnd *wnd = new listWnd;
	if (!show) {
		for (pageWnd & el : wnd->files) {
			DestroyWindow(el.wnd);
			DestroyWindow(el.wnd_text);
		}
		wnd->files.clear();
		DestroyMenu(wnd->menuBar);
		SetMenu(parent, NULL);
		DestroyWindow(wnd->findEdit);
		DestroyWindow(wnd->findButton);
		wnd->findEdit = NULL;
		wnd->findButton = NULL;
		return nullptr;
	}
	
	if (text.length()) {
		std::string name = cutSuffix(text);
		HWND new_wnd = CreateWindowA(WC_STATICA, "",WS_VISIBLE | WS_CHILD | SS_NOTIFY | BS_OWNERDRAW, 0, 0, 0, 0, parent, NULL, NULL, NULL);
		HWND new_text_wnd = CreateWindowA(WC_STATICA, name.c_str(), SS_CENTER | WS_VISIBLE | WS_CHILD | SS_NOTIFY, 0, 0, 0, 0, parent, NULL, NULL, NULL);
		wnd->files.push_back(pageWnd{new_wnd,new_text_wnd,full_name,name});
	}

	if(wnd->findEdit == NULL)
	wnd->findEdit = CreateWindowA(WC_EDITA, "", WS_VISIBLE | WS_CHILD | ES_LEFT, 0, 0, 0, 0, parent, NULL, NULL, NULL);
	if (wnd->findButton == NULL)
	wnd->findButton = CreateWindowA(WC_BUTTONA, "find", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, NULL, NULL, NULL);

	wnd->menuBar = CreateMenu();
	HMENU submenu = CreateMenu();
	HMENU sub_acc_menu = CreateMenu();
	AppendMenu(sub_acc_menu, MF_ENABLED, MENU_DELETEACC, "&delete");
	AppendMenu(sub_acc_menu, MF_ENABLED, MENU_EXITACC, "exit");
	AppendMenu(submenu, MF_ENABLED, MENU_CREATE, "create file");
	AppendMenu(submenu, MF_ENABLED, MENU_EXIT, "exit");
	AppendMenu(wnd->menuBar, MF_POPUP, (UINT_PTR)submenu, "menu");
	AppendMenu(wnd->menuBar, MF_POPUP, (UINT_PTR)sub_acc_menu, "account");
	SetMenu(parent, wnd->menuBar);
	return wnd;
}

void cf::drawImageOnWindow(HWND wnd, Gdiplus::Image *img)
{
	RECT r;
	GetWindowRect(wnd, &r);
	HDC dc = GetDC(wnd);
	InvalidateRect(wnd, NULL, true);
	Gdiplus::Graphics graphics(dc);
	int x = r.left, y = r.top, w = r.right - r.left, h = r.bottom - r.top;
	int textSize = 25;
	Gdiplus::Rect rect(0, textSize, w, h-textSize);
	graphics.DrawImage(img, rect);
	ReleaseDC(wnd, dc);
}

HANDLE cf::StartEditorProc()
{
	STARTUPINFOA sinfo{ 0 };
	sinfo.cb = sizeof(STARTUPINFOA);
	sinfo.dwFlags = STARTF_USESHOWWINDOW;
	sinfo.wShowWindow = SW_SHOWNORMAL;
	PROCESS_INFORMATION pinfo{ 0 };

	if (!CreateProcessA(NULL, (char*)"FSaccessEditor.exe", NULL, NULL, false, 0, NULL, NULL, &sinfo, &pinfo)) {
		return NULL;
	}
	Sleep(10);
	return pinfo.hProcess;
}

void cf::sendToWnd(HWND wnd, const char* data, unsigned long long size, DWORD msg_type)
{
	COPYDATASTRUCT cds;
	cds.lpData = (PVOID)data;
	cds.cbData = size;
	cds.dwData = msg_type;
	SendMessageA(wnd, WM_COPYDATA, 0, (LPARAM)&cds);
}

std::string cf::cutSuffix(const std::string& str)
{
	return str.substr(0,str.find_last_of('_'));
}

std::string cf::getFullName(const std::string& name, listWnd* listWnd)
{
	for (auto& el : listWnd->files) {
		if (name == el.name) {
			return el.full_name;
		}
	}
}

void cf::UpdateSize(HWND hwnd)
{
	RECT hwnd_rect;
	GetWindowRect(hwnd, &hwnd_rect);
	SendMessageA(hwnd,WM_SIZE,0,MAKELPARAM(hwnd_rect.right-hwnd_rect.left, hwnd_rect.bottom - hwnd_rect.top));
}

void cf::showFileCreateWnd(HWND parent,bool show,unsigned int type)
{
	static HWND createWnd;
	if (!show) {
		DestroyWindow(createWnd);
	}
	EnableWindow(parent, false);
	RECT r;
	GetWindowRect(parent, &r);
	switch (type)
	{
	case WND_CREATE:createWnd = CreateWindowA("childWnd", "Create File", WS_VISIBLE | WS_POPUP | WS_BORDER | WS_CHILD, r.left + (r.right - r.left) / 2 - 150, r.top + (r.bottom - r.top) / 2 - 25, 310, 80, parent, NULL, NULL, NULL); break;
	case WND_ADDUSER: createWnd = CreateWindowA("childWnd", "Add User", WS_VISIBLE | WS_POPUP | WS_BORDER | WS_CHILD, r.left + (r.right - r.left) / 2 - 150, r.top + (r.bottom - r.top) / 2 - 25, 310, 80, parent, NULL, NULL, NULL); break;
	case WND_DELUSER: createWnd = CreateWindowA("childWnd", "Delete User", WS_VISIBLE | WS_POPUP | WS_BORDER | WS_CHILD, r.left + (r.right - r.left) / 2 - 150, r.top + (r.bottom - r.top) / 2 - 25, 310, 80, parent, NULL, NULL, NULL); break;
	}
}
