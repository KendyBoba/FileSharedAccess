#include "client_function.h"
#include <sstream>
#include <deque>
#define _CRT_SECURE_NO_WARNINGS

static std::string cmd_str = "";

void WINAPI timerFunc(HWND hwnd, UINT msg, UINT id, DWORD time) {
	RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
}

LRESULT CALLBACK secondProc(HWND wnd, unsigned int msg, WPARAM wp, LPARAM lp) {
	static HWND input;
	static HWND createBtn;
	static HWND close;
	static HWND parent;
	static HWND wnd_text;
	static	std::string titleText = "";
	switch (msg)
	{
	case WM_CREATE: {
		wnd_text = CreateWindowA(WC_STATICA, "", WS_VISIBLE | WS_CHILD, 5, 5, 150, 20, wnd, NULL, NULL, NULL);
		char* title = new char[FILENAME_MAX];
		GetWindowTextA(wnd, title, FILENAME_MAX);
		SetWindowTextA(wnd_text, title);
		titleText += title;
		parent  = GetParent(wnd);
		input = CreateWindowA(WC_EDITA, "", WS_VISIBLE | WS_CHILD, 5, 25, 300, 20, wnd, NULL, NULL, NULL);
		if(titleText == "Create File")
			createBtn = CreateWindowA(WC_BUTTONA, "Create", WS_VISIBLE | WS_CHILD, 5, 50, 150, 20, wnd, NULL, NULL, NULL);
		else if(titleText == "Add User")
			createBtn = CreateWindowA(WC_BUTTONA, "Add", WS_VISIBLE | WS_CHILD, 5, 50, 150, 20, wnd, NULL, NULL, NULL);
		else if (titleText == "Delete User")
			createBtn = CreateWindowA(WC_BUTTONA, "Delete", WS_VISIBLE | WS_CHILD, 5, 50, 150, 20, wnd, NULL, NULL, NULL);
		close = CreateWindowA(WC_BUTTONA, "Cancel", WS_VISIBLE | WS_CHILD, 150, 50, 155, 20, wnd, NULL, NULL, NULL);
	}; break;
	case WM_COMMAND: {
		char* name = new char[FILENAME_MAX];
		GetWindowText(input, name, FILENAME_MAX);
		std::string n = name;
		if (createBtn == (HWND)lp && titleText == "Create File") {
			SendMessageA(parent, MSG_CREATE, (WPARAM)n.c_str(), 0);
			EnableWindow(parent, true);
			DestroyWindow(wnd);
		}
		if (createBtn == (HWND)lp && titleText == "Add User") {
			SendMessageA(parent, MSG_ADDUSER, (WPARAM)n.c_str(), 0);
			EnableWindow(parent, true);
			DestroyWindow(wnd);
		}
		if (createBtn == (HWND)lp && titleText == "Delete User") {
			SendMessageA(parent, MSG_DELUSER, (WPARAM)n.c_str(), 0);
			EnableWindow(parent, true);
			DestroyWindow(wnd);
		}
		else if (close == (HWND)lp) {
			DestroyWindow(wnd);
			EnableWindow(parent, true);
		}
	}; break;
	case WM_DESTROY: {
		titleText = "";
	}; break;
	}
	return  DefWindowProc(wnd, msg, wp, lp);
}

LRESULT CALLBACK mainProc(HWND wnd,unsigned int msg,WPARAM wp,LPARAM lp) {
	HDC hdc;
	PAINTSTRUCT ps;
	static char* acc_name = nullptr;
	static loginWnd *logWnd = nullptr;
	static SOCKET client;
	static HWND statusBar = NULL;
	static listWnd *lWnd = nullptr;
	static ULONG_PTR token;
	static std::string cur_name_file = "";
	static HANDLE editProc = NULL;
	static pageWnd chouse_wnd = {0};

	static unsigned int width;
	static unsigned int height;
	static HWND edit;

	static Gdiplus::Image *img_file = nullptr;

	static int scroll = 0;
	switch (msg)
	{
	case WM_CREATE: {
		SetTimer(wnd, 0, 1000 / 60, (TIMERPROC)timerFunc);
		SOCKADDR_IN addr{};
		if(cmd_str.empty())
			addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		else
			addr.sin_addr.S_un.S_addr = inet_addr(cmd_str.c_str());
		addr.sin_family = AF_INET;
		addr.sin_port = htons(3333);
		client = socket(AF_INET, SOCK_STREAM, NULL);
		connect(client, (SOCKADDR*)&addr, sizeof(addr));
		pth p{ wnd,client };
		CreateThread(NULL, 0, cf::read_data, (LPVOID)&p, 0, NULL);
		Sleep(1);

		Gdiplus::GdiplusStartupInput input;
		Gdiplus::GdiplusStartup(&token, &input, NULL);

		statusBar = CreateWindowA(STATUSCLASSNAMEA, "", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, wnd, NULL, NULL, NULL);
		int psize[1] = { 400 };
		SendMessageA(statusBar, SB_SETPARTS, (WPARAM)1, (LPARAM)psize);

		img_file = new Gdiplus::Image(L"file.png");
	}; break;
	case WM_VSCROLL: {
		int event = LOWORD(wp);
		switch (event)
		{
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION: {
			RECT r;
			GetWindowRect(wnd, &r);
			int pos = HIWORD(wp);
			SetScrollPos(wnd, SB_VERT, pos, true);
			scroll = pos * (((r.bottom - r.top) / 2) - 60);
			cf::UpdateSize(wnd);
		}; break;
		}
	}; break;
	case WM_PAINT: {
		RECT r;
		GetWindowRect(wnd, &r);
		int w = r.right - r.left;
		int h = r.bottom - r.top;
		if (lWnd) {
			int blockWidth = (w - 25) / 4;
			int blockHeight = (h / 2) - 60;
			int tempX = 5;
			int tempY = 30 - scroll;
			int count = 1;
			for (auto& el : lWnd->files) {
				if (el.name != lWnd->cur_find && lWnd->cur_find != "") {
					ShowWindow(el.wnd, 0);
					ShowWindow(el.wnd_text, 0);
					continue;
				}else if(tempY >= h - 100 || tempY < 25){
					ShowWindow(el.wnd, 0);
					ShowWindow(el.wnd_text, 0);
				}
				else {
					MoveWindow(el.wnd_text, tempX, tempY, blockWidth, 20, true);
					SetWindowTextA(el.wnd_text, el.name.c_str());
					MoveWindow(el.wnd, tempX, tempY+20, blockWidth, blockHeight-20, true);
					cf::drawImageOnWindow(el.wnd, img_file);
					ShowWindow(el.wnd_text, 1);
					ShowWindow(el.wnd, 1);
				}
				tempX += blockWidth + 5;
				if (count % 4 == 0) {
					tempY += blockHeight + 5;
					tempX = 5;
				}
				++count;
			}
		}
	}; break;
	case WM_GETMINMAXINFO: {
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lp;
		lpMMI->ptMinTrackSize.x = 800;
		lpMMI->ptMinTrackSize.y = 600;
	}; break;
	case WM_SIZE: {
		width = LOWORD(lp);
		height = HIWORD(lp);
		if (statusBar != NULL) {
			MoveWindow(statusBar, 0, height - 20, width, 20, true);
		}
		if (logWnd) {
			MoveWindow(logWnd->staticLog, width / 3, height / 3, 80, 20, true);
			MoveWindow(logWnd->loginLine, width / 3 + 80, height / 3, width / 3 - 80, 20, true);
			MoveWindow(logWnd->staticPass, width / 3, height / 2, 80, 20, true);
			MoveWindow(logWnd->passwordLine, width / 3 + 80, height / 2, width / 3 - 80, 20, true);
			MoveWindow(logWnd->buttonLogin, width / 3, height / 1.5, (width / 3) / 2, 20, true);
			MoveWindow(logWnd->buttonReg, (width / 3) + (width / 3) / 2, height / 1.5, (width / 3) / 2, 20, true);
		}
		if (lWnd) {
			SetScrollRange(wnd, SB_VERT, 0, (lWnd->files.size()/4)-1, true);
			MoveWindow(lWnd->findEdit, 5, 5, width * (1.0 - 1.0 / 5), 20, true);
			MoveWindow(lWnd->findButton, width * (1.0 - 1.0 / 5), 5, (width / 5) - 5, 20, true);
			for (auto& el : lWnd->files) {
				HDC hdc = GetDC(el.wnd);
				RECT r;
				GetWindowRect(el.wnd, &r);
				HBRUSH brush = CreateSolidBrush(RGB(200, 200, 200));
				FillRect(hdc, new RECT{ 0,0,r.right,r.bottom }, brush);
				DeleteObject(brush);
				ReleaseDC(el.wnd, hdc);
			}
		}
	}; break;
	case WM_COMMAND: {
		int cEvent = HIWORD(wp);
		int menuEvent = LOWORD(wp);
		HWND test = (HWND)lp;
		if (menuEvent == MENU_CREATE) {
			cf::showFileCreateWnd(wnd, true,WND_CREATE);
		}else if (menuEvent == MENU_EXIT) {
			SendMessageA(wnd, WM_DESTROY, 0, 0);
		}
		else if (menuEvent == MENU_ADDUSER) {
			cf::showFileCreateWnd(wnd, true, WND_ADDUSER);
		}
		else if (menuEvent == MENU_DELETE) {
			cf::send_data(client, msg_type::delete_file, chouse_wnd.full_name.c_str());
		}
		else if (menuEvent == MENU_OPENFILE) {
			cur_name_file = chouse_wnd.full_name;
			cf::send_data(client, msg_type::open_file, chouse_wnd.full_name.c_str());
		}
		else if (menuEvent == MENU_DELUSER) {
			cf::showFileCreateWnd(wnd, true, WND_DELUSER);
		}
		else if (menuEvent == MENU_EXITACC) {
			lWnd = cf::initListWindow(wnd, "", false);
			logWnd = cf::initLoginWindow(wnd, true);
			cf::UpdateSize(wnd);
		}
		else if (menuEvent == MENU_DELETEACC) {
			user user;
			user.login = acc_name;
			cf::send_data(client, msg_type::delete_acc, (char*)&user,sizeof(user));
			lWnd = cf::initListWindow(wnd, "", false);
			logWnd = cf::initLoginWindow(wnd, true);
			cf::UpdateSize(wnd);
		}
		else if (logWnd->buttonReg == (HWND)lp || logWnd->buttonLogin == (HWND)lp) {
			char login[30];
			GetWindowTextA(logWnd->loginLine, (char*)login, 30);
			char pass[30];
			GetWindowTextA(logWnd->passwordLine, (char*)pass, 30);
			user cur_user{ login,pass };
			if(logWnd->buttonLogin == (HWND)lp)
				cf::send_data(client, msg_type::user_log, (char*)&cur_user, sizeof(user));
			else
				cf::send_data(client, msg_type::user_reg, (char*)&cur_user, sizeof(user));
		}
		else if (lWnd != nullptr && lWnd->findButton == (HWND)lp) {
			int len = SendMessageA(lWnd->findEdit, WM_GETTEXTLENGTH, 0, 0);
			char* filename = new char[len+1];
			SendMessageA(lWnd->findEdit, EM_GETLINE, 0, (LPARAM)filename);
			filename[len] = '\0';
			lWnd->cur_find = filename;
			scroll = 0;
			SetScrollPos(wnd, SB_VERT, 0, true);
		}
		else if (lWnd != nullptr && lWnd->findEdit == (HWND)lp && cEvent == EN_CHANGE) {
			int len = SendMessageA(lWnd->findEdit, WM_GETTEXTLENGTH, 0, 0);
			if (!len) {
				lWnd->cur_find = "";
				scroll = 0;
				SetScrollPos(wnd, SB_VERT, 0, true);
				cf::UpdateSize(wnd);
			}
		}
		if (!lWnd) break;
		for (const auto& el : lWnd->files) {
			if ((el.wnd == (HWND)lp || el.wnd_text == (HWND)lp) && cEvent == STN_CLICKED && editProc == NULL) {
				chouse_wnd = el;
				HMENU menu = CreatePopupMenu();
				AppendMenuA(menu, MF_ENABLED, MENU_DELETE, "Delete");
				AppendMenuA(menu, MF_ENABLED, MENU_OPENFILE, "Open");
				AppendMenuA(menu, MF_ENABLED, MENU_ADDUSER, "Add User");
				AppendMenuA(menu, MF_ENABLED, MENU_DELUSER, "Delete User");
				POINT* mouse_pos = new POINT;
				GetCursorPos(mouse_pos);
				TrackPopupMenu(menu, TPM_LEFTBUTTON, mouse_pos->x, mouse_pos->y, 0, wnd , NULL);
			}
		}
	}; break;
	case MSG_MESSAGE: {
		char* message = (char*)wp;
		SendMessageA(statusBar, SB_SETTEXTA, 0, (WPARAM)message);
	}; break;
	case MSG_CONNECT: {
		logWnd = cf::initLoginWindow(wnd, true);
		ShowScrollBar(wnd, SB_VERT, false);
	}; break;
	case MSG_DISCONNECT: {
		char* data = (char*)wp;
		closesocket(client);
		SendMessageA(statusBar, SB_SETTEXTA, 0, (WPARAM)"server closed");
		Sleep(1000);
		PostQuitMessage(0);
	}; break;
	case MSG_ACCESS: {
		acc_name = (char*)wp;
		SendMessageA(statusBar, SB_SETTEXTA, 0, (WPARAM)"access\0");
		cf::initLoginWindow(wnd, false);
		cf::initListWindow(wnd, "", true);
		ShowScrollBar(wnd, SB_VERT, true);
	}; break;
	case MSG_REG: {
		char *info = (char*)wp;
		SendMessageA(statusBar, SB_SETTEXTA, 0, (WPARAM)info);
	}; break;
	case MSG_NACCESS: {
		const char* info = "wrong login or password\0";
		SendMessageA(statusBar, SB_SETTEXTA, 0, (WPARAM)info);
	}; break;
	case MSG_FNAME: {
		const char* name = (char*)wp;
		lWnd = cf::initListWindow(wnd,name,true);
		cf::UpdateSize(wnd);
	}; break;
	case MSG_FDATA: {
		editProc = cf::StartEditorProc();
		buffer * buff = (buffer*)wp;
		SendMessageA(statusBar, SB_SETTEXTA, 0, (WPARAM)"data file loaded"); //TO DO
		edit = FindWindowA("file_editor", 0);
		cf::sendToWnd(edit, buff->data(), buff->getLength(), PROC_DATA);
	}; break;
	case MSG_CREATE: {
		char * name = (char*)wp;
		std::string send_name; send_name += name; send_name += "_" + std::string(acc_name) += ".fs";
		cf::send_data(client, msg_type::create_file, send_name.c_str());
	}; break;
	case MSG_FDELETE: {
		char* name = (char*)lp;
		std::string n = name;
		if (lWnd) {
			auto it = lWnd->files.begin();
			for (auto& el : lWnd->files) {
				if (el.full_name == n) {
					DestroyWindow(el.wnd);
					DestroyWindow(el.wnd_text);
					lWnd->files.erase(it);
					break;
				}
				++it;
			}
		}
		TerminateProcess(editProc, 0);
		editProc = NULL;
		edit = NULL;
		cur_name_file = "";
	}; break;
	case MSG_ADDUSER: {
		char *user_name = (char*)wp;
		buffer send_data;
		int sizeUser = std::strlen(user_name);
		int sizeFileName = chouse_wnd.full_name.length();
		send_data.write((char*)&sizeUser, sizeof(int));
		send_data.write(user_name,sizeUser);
		send_data.write((char*)&sizeFileName, sizeof(int));
		send_data.write((char*)chouse_wnd.full_name.c_str(), sizeFileName);
		cf::send_data(client, msg_type::add_user, (char*)send_data.data(), send_data.getLength());
	}; break;
	case MSG_DELUSER: {
		char* user_name = (char*)wp;
		buffer send_data;
		int sizeUser = std::strlen(user_name);
		int sizeFileName = chouse_wnd.full_name.length();
		send_data.write((char*)&sizeUser, sizeof(int));
		send_data.write(user_name, sizeUser);
		send_data.write((char*)&sizeFileName, sizeof(int));
		send_data.write((char*)chouse_wnd.full_name.c_str(), sizeFileName);
		cf::send_data(client, msg_type::del_user, (char*)send_data.data(), send_data.getLength());
	}; break;
	case MSG_SUMBOL: {
		sumbol_info* sumbol = (sumbol_info*)wp;
		cf::sendToWnd(edit, (char*)sumbol, sizeof(sumbol_info), PROC_SUMBOL);
	}; break;
	case WM_COPYDATA: {
		COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lp;
		DWORD size = cds->cbData;
		DWORD action = cds->dwData;
		switch (action)
		{
		case PROC_CLOSE: {
			cf::send_data(client, msg_type::close_file, cur_name_file.c_str());
			cur_name_file = "";
			CloseHandle(editProc);
			editProc = NULL;
			edit = NULL;
		}; break;
		case PROC_SENDSUMBOL: {
			sumbol_info* symbol = (sumbol_info*)cds->lpData;
			char* data = new char[0];
			buffer send_buffer(data, 0);
			size_t size_name = cur_name_file.size();
			send_buffer.write((char*)&size_name, sizeof(size_t));
			send_buffer.write((char*)cur_name_file.c_str(), size_name);
			send_buffer.write((char*)symbol, sizeof(sumbol_info));
			cf::send_data(client, msg_type::update_file, send_buffer.data(), send_buffer.getLength());
		}; break;
		}
	}; break;
	case WM_DESTROY: {
		if(editProc)
			TerminateProcess(editProc, 0);
		cf::send_data(client, msg_type::close_file, cur_name_file.c_str());
		cf::send_data(client, msg_type::disconnect, " ");
		Gdiplus::GdiplusShutdown(token);
		PostQuitMessage(0);
	}; break;
	}
	return DefWindowProc(wnd, msg, wp, lp);
}



int WINAPI WinMain(HINSTANCE app,HINSTANCE a, LPSTR cmd,int show)
{
	cmd_str = cmd;
	if (!cf::initLib())return -1;
	WNDCLASSA wnd_class{ 0 };
	wnd_class.hInstance = app;
	wnd_class.lpfnWndProc = mainProc;
	wnd_class.lpszClassName = "mainWnd";
	wnd_class.hbrBackground = CreateSolidBrush(RGB(200, 200, 200));
	RegisterClassA(&wnd_class);

	WNDCLASSA child_class{ 0 };
	child_class.hInstance = app;
	child_class.lpfnWndProc = secondProc;
	child_class.lpszClassName = "childWnd";
	child_class.hbrBackground = CreateSolidBrush(RGB(200, 200, 200));
	RegisterClassA(&child_class);

	HWND mainWnd = CreateWindowA("mainWnd", "FSacces", WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, NULL, app, NULL);
	ShowWindow(mainWnd, 1);

	MSG msg;
	while (GetMessageA(&msg, 0, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	return 0;
}
