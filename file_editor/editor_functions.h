#pragma once
#include <Windows.h>
#include <deque>
#include "..\sharedFileApp_server\general.h"
#include "..\SharedFileApp_client\defines.h"
namespace ef {
	void sendToWnd(HWND wnd, const char* data, unsigned long long size, DWORD msg_type);
	unsigned int sum(const std::deque<unsigned int>& arr, int len);
	unsigned int sum(const std::deque<unsigned int>& arr);
	unsigned int XYtoPos(const std::deque<unsigned int>& arr,int x,int y);
	std::deque<unsigned int> getAllLinesLength(const std::deque<sumbol_info>& arr, POINT& size_wnd,const int & sumb_len);
	void updatePos(std::deque<sumbol_info>& arr,unsigned int start_pos,HWND parent);
	bool MouseCollision(POINT mouse, RECT obj);
	std::deque<std::string>& getAllFonts();
	void do_scroll(HWND hwnd, POINT& caret_pos, int& scroll,int headerBlockY, short line_height, int scroll_pos);
	int SearchSubString(std::wstring sub_string, const std::deque<sumbol_info>& text,int start_pos,int end_pos,bool isLower);
	wchar_t* emptyf(wchar_t *ch);
	char* OpenFileName(HWND hwnd, const char* filter);
	char* SaveFileName(HWND hwnd, const char* filter);
	bool SaveFile(char* path, std::deque<sumbol_info>& destination);
	bool OpenFile(const char* path, std::deque<sumbol_info>& destination);
	void UpdateCaret(HWND hwnd, int x, int y, int width, int height);
};