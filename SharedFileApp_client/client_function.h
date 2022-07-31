#pragma once
#include <winsock.h>
#include <gdiplus.h>
#include <gdiplusheaders.h>
#include "..\sharedFileApp_server\general.h"
#include "defines.h"
#include <iostream>
#include "structsndef.h"


namespace cf {

	DWORD WINAPI read_data(LPVOID param);

	void send_data(SOCKET sock, msg_type type, const char* data,int size = 0);

	void processing(SOCKET sock,HWND wnd, msg_type type, int size);

	bool initLib();

	loginWnd* initLoginWindow(HWND parent,bool show);

	listWnd* initListWindow(HWND parent,const char* name,bool show);

	void drawImageOnWindow(HWND wnd, Gdiplus::Image *img);

	void showFileCreateWnd(HWND parent,bool show,unsigned type);

	HANDLE StartEditorProc();

	void sendToWnd(HWND wnd, const char* data, unsigned long long size, DWORD msg_type);

	std::string cutSuffix(const std::string& str);

	std::string getFullName(const std::string &name,listWnd* listWnd);

	void UpdateSize(HWND hwnd);
};