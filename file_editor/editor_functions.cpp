#define _CRT_SECURE_NO_DEPRECATE
#include "editor_functions.h"
#include <math.h>
#include <cctype>


void ef::sendToWnd(HWND wnd,const char* data, unsigned long long size, DWORD msg_type)
{
	COPYDATASTRUCT cds;
	cds.lpData = (PVOID)data;
	cds.cbData = size;
	cds.dwData = msg_type;
	SendMessageA(wnd, WM_COPYDATA, 0, (LPARAM)&cds);
}

unsigned int ef::sum(const std::deque<unsigned int>& arr, int len)
{
	if (len > arr.size()) {
		return 0;
	}
	unsigned int sum = 0;
	for (unsigned int i = 0; i < len; ++i) {
		sum += arr[i];
	}
	return sum;
}

unsigned int ef::sum(const std::deque<unsigned int>& arr)
{
	unsigned int sum = 0;
	for (const auto& el : arr) {
		sum += el;
	}
	return sum;
}

unsigned int ef::XYtoPos(const std::deque<unsigned int>& arr,int x, int y)
{
	int res = ef::sum(arr, y - 1) + x;
	if (res <= 0) return 0;
	return res;
}

std::deque<unsigned int> ef::getAllLinesLength(const std::deque<sumbol_info>& arr,POINT &size_wnd,const int &sumbLen)
{
	std::deque<unsigned int > res;
	unsigned int line_len = 0;
	int x = 0;
	for (int i = 0; i < arr.size(); ++i) {
		wchar_t ch[2]{ arr[i].sumbol,'\0' };
		if (arr[i].sumbol == '\r' || x *  sumbLen >= size_wnd.x) {
			x = 0;
			++line_len;
			res.push_back(line_len);
			line_len = 0;
		}else if(i == arr.size() - 1) {
			++line_len;
			res.push_back(line_len);
		}
		else {
			++x;
			++line_len;
		}
	}
	return res;
}

void ef::updatePos(std::deque<sumbol_info>& arr, unsigned int start_pos,HWND parent)
{
	for (int i = start_pos; i < arr.size(); ++i) {
		arr[i].pos = i;
		ef::sendToWnd(parent, (char*)&(arr[i]), sizeof(sumbol_info), PROC_SENDSUMBOL);
	}
}

bool ef::MouseCollision(POINT mouse, RECT obj)
{
	if (mouse.x < (obj.left + obj.right) && mouse.x > obj.left && mouse.y < (obj.top + obj.bottom) && mouse.y > obj.top) {
		return true;
	}
	return false;
}

std::deque<std::string>& ef::getAllFonts()
{
	std::deque<std::string> *res = new std::deque<std::string>;

	std::string full_path = "C:\\Windows\\Fonts";
	WIN32_FIND_DATAA find_file_data;
	HANDLE firstFile = FindFirstFileA((full_path + "\\*").c_str(), &find_file_data);
	if (firstFile == INVALID_HANDLE_VALUE) {
		return *res;
	}
	do {
		std::string file_name = find_file_data.cFileName;
		std::string sufix = file_name.substr(file_name.find_last_of('.'),file_name.length());
		if (file_name == "." || file_name == ".." || sufix != ".ttf") continue;
		file_name.erase(file_name.begin() + file_name.find_last_of('.'), file_name.begin() + file_name.length());
		res->push_back(file_name);
	} while (FindNextFileA(firstFile,&find_file_data));
	return *res;
}

void ef::do_scroll(HWND hwnd, POINT& caret_pos, int& scroll, int headerBlockY, short line_height, int scroll_pos)
{
	static int last_pos = 0;
	int pos = scroll_pos;
	SetScrollPos(hwnd, SB_VERT, pos, true);
	scroll = pos * line_height;
	GetCaretPos(&caret_pos);
	int new_y = (caret_pos.y - headerBlockY);
	if(new_y)
	new_y /= line_height;
	new_y -= (pos - last_pos);
	SetCaretPos(caret_pos.x, new_y * line_height + headerBlockY);
	last_pos = pos;
}

int ef::SearchSubString(std::wstring sub_string,const std::deque<sumbol_info>& text, int start_pos, int end_pos, bool isLower)
{
	if (start_pos == end_pos) return -1;
	wchar_t* (*lower)(wchar_t*) = nullptr;
	if (isLower)
		lower = _wcslwr;
	else
		lower = emptyf;

	int* arr = new int[sub_string.length() + 1];
	arr[sub_string.length()] = sub_string.length();
	int count = 1;
	for (int i = sub_string.length() - 2; i >= 0; --i) {
		bool compare = false;
		for (int j = i+1; j < sub_string.length() - 1; ++j) {
			if (sub_string[j] == sub_string[i]) {
				arr[i] = arr[j];
				compare = true;
				break;
			}
		}
		if(!compare)
			arr[i] = count;
		++count;
	}
	bool last_sumb_comparable = false;
	for (int i = 0; i < sub_string.length(); ++i) {
		if (i == (sub_string.length() - 1)) continue;
		if (sub_string[i] == sub_string[sub_string.length() - 1]) {
			arr[sub_string.length() - 1] = arr[i];
			last_sumb_comparable = true;
			break;
		}
	}
	if (!last_sumb_comparable)
		arr[sub_string.length() - 1] = sub_string.length();

	int pos = -1;
	for (int i = start_pos + sub_string.length()-1; i < end_pos; ++i) {
		int len_res = 0;
		for (int j = sub_string.length()-1; j >= 0; --j) {
			wchar_t sumb = text[i].sumbol;
			if (*lower(&sub_string[j]) == *lower(&sumb)) {
				++len_res;
				--i;
			}
			else {
				int temp_i = i;
				for (int k = j; k >= 0; --k) { 
					if (text[i].sumbol == sub_string[k]) {
						i += arr[k];
						break;
					}
				}
				if (!(temp_i - i)) i += arr[sub_string.length()];
				break;
			}
		}
		if (len_res == sub_string.length()) { pos = i; break; };
		--i;
	}
	if (pos < 0) return pos;
	return pos+1;
}

wchar_t* ef::emptyf(wchar_t *ch)
{
	return ch;
}

char* ef::OpenFileName(HWND hwnd, const char* filter)
{
	char* res = new char[MAX_PATH];
	OPENFILENAMEA ofn = { 0 };
	ofn.hwndOwner = hwnd;
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.lpstrFile = res;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = filter;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (!GetOpenFileNameA(&ofn)) {
		return nullptr;
	}
	return res;
}

char* ef::SaveFileName(HWND hwnd, const char* filter)
{
	char* res = new char[MAX_PATH];
	OPENFILENAMEA ofn = { 0 };
	ofn.hwndOwner = hwnd;
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.lpstrFile = res;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = filter; 
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	if (!GetSaveFileNameA(&ofn)) {
		return nullptr;
	}
	return res;
}

bool ef::SaveFile(char* path, std::deque<sumbol_info>& destination)
{
	HANDLE file = CreateFileA(path, GENERIC_ALL, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) return false;
	char* data = new char[2];
	buffer buff(data, 2);
	for (const auto& el : destination) {
		buff.write((char*)&el, sizeof(sumbol_info));
	}
	DWORD writed = 0;
	while(writed < buff.getLength())
		WriteFile(file, buff.data(), buff.getLength() - writed, &writed, NULL);
	CloseHandle(file);
	return true;
}

bool ef::OpenFile(const char* path, std::deque<sumbol_info>& destination)
{
	destination.clear();
	HANDLE file = CreateFileA(path, GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) return false;

	DWORD fileSize = GetFileSize(file, NULL);
	char* data_file = new char[fileSize];
	if (!ReadFile(file, data_file, fileSize, NULL, NULL)) {
		return false;
	}
	buffer buff(data_file, fileSize);
	while (buff.getPointR() < buff.getLength()) {
		sumbol_info *sumb = (sumbol_info*)buff.read(sizeof(sumbol_info));
		destination.push_back(*sumb);
	}
	CloseHandle(file);
	return true;
}

void ef::UpdateCaret(HWND hwnd, int x, int y, int width, int height)
{
	DestroyCaret();
	CreateCaret(hwnd,NULL, width, height);
	ShowCaret(hwnd);
	SetCaretPos(x, y);
}

