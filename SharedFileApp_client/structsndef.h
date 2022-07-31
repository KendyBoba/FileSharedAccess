#pragma once
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <list>

struct pth {
	HWND wnd;
	SOCKET sock;
};

struct loginWnd
{
	HWND loginLine;
	HWND passwordLine;
	HWND buttonLogin;
	HWND buttonReg;
	HWND staticLog;
	HWND staticPass;
};

struct pageWnd {
	HWND wnd;
	HWND wnd_text;
	std::string full_name = "";
	std::string name = "";
};

struct listWnd {
	std::list<pageWnd> files;
	HMENU menuBar;
	HWND findEdit = NULL;
	HWND findButton = NULL;
	std::string cur_find = "";
};

struct editWnd {
	HWND font;
	HMENU menuBar;
	POINT CaretPos{ 0,0 };
	unsigned int pos = 0; 
	const unsigned short line_height = 30;
};