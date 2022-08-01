#include <iostream>
#include <Windows.h>
#include <deque>
#include <string>
#include "editor_functions.h"
#include <CommCtrl.h>
#include <Commdlg.h>
#include <tchar.h>
HINSTANCE g_app;
HWND editor;
std::string cur_file_name = "";

UINT findReplaceMsg = 0;
HWND findDlg = NULL;
FINDREPLACE frs;

#define MENU_SAVE 100
#define MENU_OPEN 101
#define TIMER_1 102

void CALLBACK timer_proc(HWND hwnd, UINT msg, UINT id, DWORD dwTime) {
	RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
}

LRESULT CALLBACK proc(HWND hwnd, unsigned int msg, WPARAM wp, LPARAM lp) {
	static std::deque<sumbol_info> cur_data_file;
	static HWND parent = NULL;
	static unsigned long long cur_pos = 0;
	static short line_height = 30;
	static POINT size_wnd;
	static int scroll = 0;
	static POINT caret_pos;
	static std::deque<unsigned int> line_length;
	static int start_highLight_pos = 0;
	static int end_highLight_pos = 0;
	static int last_caret_pos = 0;
	static std::deque<sumbol_info*>buff_highlight_text;
	static HWND fontList = NULL;
	static bool isFocusPage = true;
	static std::deque<std::string> fonts;
	static unsigned int choiceFont = 0;
	static HWND bg_color_button = NULL;
	static HWND text_color_button = NULL;
	static HWND find_button = NULL;
	static HWND spin_size_sumb = NULL;
	static HWND static_size_symb = NULL;
	static HWND static_text_symb = NULL;
	static wchar_t findBuffer[MAX_REASON_NAME_LEN];
	static int headerblockY = 80;
	static int sumbLen = 0;
	static int dpm = 0;
	static int userChoiceSizeSumbol = 3;
	static int find_pos = 0;
	static int find_len = 0;

	static PAINTSTRUCT ps;
	static HDC hdc;
	static HDC backDc;
	static HBITMAP backBMP;
	static HGDIOBJ oldBMP;
	switch (msg)
	{
	case WM_CREATE: {
		parent = FindWindowA("mainWnd", NULL);
		if (parent == NULL && !cur_file_name.empty()) {
			ef::OpenFile(cur_file_name.c_str(), cur_data_file);
		}
		CreateCaret(hwnd, NULL, 1, line_height);
		ShowCaret(hwnd);
		SetCaretPos(0, headerblockY);

		HMENU sub_menu = CreateMenu();
		AppendMenu(sub_menu, MF_ENABLED, MENU_SAVE, L"&Save");
		if(!parent)
			AppendMenu(sub_menu, MF_ENABLED, MENU_OPEN, L"&Open");
		HMENU menu = CreateMenu();
		AppendMenu(menu, MF_POPUP, (UINT_PTR)sub_menu, L"Menu");
		SetMenu(hwnd, menu);

		bg_color_button = CreateWindowA(WC_BUTTONA, "BG COLOR", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
		text_color_button = CreateWindowA(WC_BUTTONA, "TEXT COLOR", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
		find_button = CreateWindowA(WC_BUTTONA, "FIND", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
		spin_size_sumb = CreateWindowA(UPDOWN_CLASSA, "", WS_VISIBLE | WS_CHILD | UDS_ALIGNRIGHT, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
		SendMessageA(spin_size_sumb, UDM_SETRANGE, 0, MAKELPARAM(2,10));
		SendMessageA(spin_size_sumb, UDM_SETPOS, 0, (LPARAM)2);
		static_size_symb = CreateWindowA(WC_STATICA, "", WS_VISIBLE | WS_CHILD | WS_BORDER, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
		SendMessageA(spin_size_sumb, UDM_SETBUDDY, (WPARAM)static_size_symb, NULL);
		static_text_symb = CreateWindowA(WC_STATICA, "symbol size: ", WS_VISIBLE | WS_CHILD , 0, 0, 0, 0, hwnd, NULL, NULL, NULL);

		fontList = CreateWindowA(WC_COMBOBOXA, "", LBS_STANDARD | WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_OVERLAPPED, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
		fonts = ef::getAllFonts();
		fonts.push_front("Times New Roman");
		for (const auto& el : fonts) {
			AddFontResourceA(("C:\\Windows\\Fonts\\" + el + ".ttf").c_str());
			SendMessageA(fontList, CB_ADDSTRING, 0, (LPARAM)el.c_str());
		}
		SendMessageA(fontList, CB_SETCURSEL, 0, 0);
		if (!fonts.empty())
			choiceFont = 0;

		SetTimer(hwnd, TIMER_1, 1000/60, (TIMERPROC)timer_proc);
	}; break;
	case WM_KEYDOWN: {
		GetCaretPos(&caret_pos);
		int y = (caret_pos.y - headerblockY + scroll) / line_height;
		int x = caret_pos.x / sumbLen;
		switch (wp)
		{
		case VK_LEFT: {
			if (x > 0)
				SetCaretPos(caret_pos.x - sumbLen, caret_pos.y);
			else if (y + 1 > 1) {
				SetCaretPos((line_length[y - 1] - 1) * sumbLen, caret_pos.y - line_height);
			}
		}; break;
		case VK_RIGHT: {
			auto temp = ef::getAllLinesLength(cur_data_file, size_wnd, sumbLen);
			if (temp.empty()) break;
			int compire_x = x;
			if (y + 1 < temp.size())
				compire_x = x + 1;
			if (compire_x < temp[y])
				SetCaretPos(caret_pos.x + sumbLen, caret_pos.y);
			else if (temp.size() > y + 1)
				SetCaretPos(0, caret_pos.y + line_height);
		}; break;
		case VK_DOWN: {
			auto temp = ef::getAllLinesLength(cur_data_file, size_wnd, sumbLen);
			int new_x = caret_pos.x;
			int new_y = caret_pos.y;
			if (temp.size() > y + 1)
				new_y = caret_pos.y + line_height;
			if (temp.size() > y + 1 && x > temp[y + 1] && y + 2 >= temp.size())
				new_x = temp[y + 1] * sumbLen;
			if (temp.size() > y + 1 && x > temp[y + 1] && y + 2 < temp.size())
				new_x = (temp[y + 1] - 1) * sumbLen;
			SetCaretPos(new_x, new_y);
		}; break;
		case VK_UP: {
			line_length = ef::getAllLinesLength(cur_data_file, size_wnd, sumbLen);
			int new_x = caret_pos.x;
			int new_y = caret_pos.y;
			if (y > 0)
				new_y = caret_pos.y - line_height;
			if (y > 0 && x >= line_length[y - 1])
				new_x = (line_length[y - 1] - 1) * sumbLen;
			SetCaretPos(new_x, new_y);
		}; break;
		}
		GetCaretPos(&caret_pos);
	}; break;
	case WM_CHAR: {
		if (!isFocusPage)break;

		bool erase_highlight = false;
		wchar_t ch = (wchar_t)wp;

		line_length = ef::getAllLinesLength(cur_data_file, size_wnd, sumbLen);
		GetCaretPos(&caret_pos);
		int y = (caret_pos.y - headerblockY + scroll) / line_height;
		int x = caret_pos.x / sumbLen;
		cur_pos = ef::XYtoPos(line_length, x, y + 1);

		sumbol_info *sumbol = new sumbol_info{ch,cur_pos ,choiceFont};
		if (cur_data_file.size() <= cur_pos + 1)
			cur_data_file.resize(cur_pos + 1);

		if (cur_data_file[cur_pos].sumbol == '\r') {
			cur_data_file.insert(cur_data_file.begin() + cur_pos, *sumbol);
			if (parent != NULL) {
				ef::updatePos(cur_data_file, cur_pos, parent);
			}
		}
		else if (ch == '\b') {
			if (!buff_highlight_text.empty()) {
				cur_data_file.erase(cur_data_file.begin() + buff_highlight_text[0]->pos, cur_data_file.begin() + buff_highlight_text.back()->pos);
				buff_highlight_text.clear();
				erase_highlight = true;
				end_highLight_pos = 0;
				start_highLight_pos = 0;
			}
			else {
				if (cur_pos < 1) break;
				cur_data_file.erase(cur_data_file.begin() + cur_pos - 1);
			}
			if (parent != NULL) {
				ef::updatePos(cur_data_file, cur_pos - 1, parent);
			}
		}
		else {
			cur_data_file[cur_pos] = *sumbol;
			if (parent != NULL)
				ef::sendToWnd(parent, (char*)sumbol, sizeof(*sumbol), PROC_SENDSUMBOL);
		}

		if (caret_pos.x >= size_wnd.x || ch == '\r')
			SetCaretPos(0, caret_pos.y + line_height);
		else if (ch == '\b') {
			if (x > 0 && erase_highlight)
				SetCaretPos(caret_pos.x, caret_pos.y);
			else if(x > 0)
				SetCaretPos(caret_pos.x - sumbLen, caret_pos.y);
			else
				SetCaretPos((line_length[y - 1] - 1) * sumbLen, caret_pos.y - line_height);
			cur_data_file.pop_back();
		}
		else
			SetCaretPos(caret_pos.x + sumbLen, caret_pos.y);

		GetCaretPos(&caret_pos);
	}; break;
	case WM_PAINT: {
		hdc = BeginPaint(hwnd, &ps);
		backDc = CreateCompatibleDC(hdc);
		RECT clientR;
		GetClientRect(hwnd, &clientR);
		backBMP = CreateCompatibleBitmap(hdc, clientR.right, clientR.bottom);
		oldBMP = SelectObject(backDc, backBMP);

		HBRUSH b = CreateSolidBrush(RGB(240, 240, 240));
		FillRect(backDc, &clientR, b);
		DeleteObject(b);

		GetCaretPos(&caret_pos);
		int line = 0;
		int x = 0;
		unsigned int cur_page = 1;
		std::deque<sumbol_info*> temp_highlight;
		for (int i = 0; i < cur_data_file.size(); ++i) {
			wchar_t ch[2]{ cur_data_file[i].sumbol,'\0' };
			if (cur_data_file[i].sumbol == '\r' || x * sumbLen >= size_wnd.x - sumbLen) {
				++line;
				x = 0;
			}
			else {
				bool choise_sumbol = false;
				bool find_sumbol = false;
				if (((i >= start_highLight_pos && i < end_highLight_pos) || (i <= start_highLight_pos && i >= end_highLight_pos)) && end_highLight_pos >= 0 && start_highLight_pos != end_highLight_pos) {
					SetBkColor(backDc, RGB(0, 159, 209));
					temp_highlight.push_back(&(cur_data_file[i]));
					choise_sumbol = true;
				}
				else if (i >= find_pos && i < find_pos + find_len && find_pos > 0) {
					SetBkColor(backDc, RGB(255, 0, 0));
					find_sumbol = true;
				}
				LOGFONTA lf{ 0 };
				lf.lfWeight = userChoiceSizeSumbol - 2;
				lf.lfHeight = line_height - 3*(userChoiceSizeSumbol);
				lf.lfPitchAndFamily = FIXED_PITCH;
				strcpy_s(lf.lfFaceName, fonts[cur_data_file[i].font].c_str());
				HFONT cur_font = CreateFontIndirectA(&lf); SelectObject(backDc, cur_font);
				if (!choise_sumbol && !find_sumbol)
					SetBkColor(backDc, cur_data_file[i].bk_color);
				SetTextColor(backDc, cur_data_file[i].text_color);
				TextOutW(backDc, x * sumbLen, line * line_height + headerblockY - scroll, ch, 2);
				DeleteObject(cur_font);
				++x;
			}
		}
		buff_highlight_text = temp_highlight;
		SetScrollRange(hwnd, SB_VERT, 0, line, true);
		RECT headerR{ 0, 0, size_wnd.x, headerblockY };
		HBRUSH b1 = CreateSolidBrush(RGB(200, 200, 200));
		FillRect(backDc, &headerR, b1);
		DeleteObject(b1);

		BitBlt(hdc, 0, 0, clientR.right, clientR.bottom, backDc, 0, 0, SRCCOPY);

		SelectObject(backDc, oldBMP);
		DeleteObject(backBMP);
		DeleteDC(backDc);
		EndPaint(hwnd, &ps);
	}; break;
	case WM_COMMAND: {
		int command = HIWORD(wp);
		if ((HWND)lp == fontList) {
			switch (command)
			{
			case CBN_SELCHANGE: {
				int el_id = SendMessage(fontList, CB_GETCURSEL, 0, 0);
				if (buff_highlight_text.empty()) {
					choiceFont = el_id;
				}
				else {
					for (auto& el : buff_highlight_text) {
						el->font = el_id;
						ef::sendToWnd(parent, (char*)el, sizeof(sumbol_info), PROC_SENDSUMBOL);
					}
					end_highLight_pos = 0; start_highLight_pos = 0;
				}
			}; break;
			case CBN_SETFOCUS: {
				isFocusPage = false;
			}; break;
			case CBN_KILLFOCUS: {
				isFocusPage = true;
			}; break;
			};
		}
		else if (((HWND)lp == bg_color_button || (HWND)lp == text_color_button) && !buff_highlight_text.empty()) {
			COLORREF color = RGB(240, 240, 240);
			COLORREF colors[16];

			CHOOSECOLOR cc = { 0 };
			cc.Flags = CC_RGBINIT | CC_FULLOPEN;
			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.lCustData = 0;
			cc.rgbResult = RGB(0, 0, 0);
			cc.lpCustColors = colors;
			cc.lpfnHook = NULL;
			if (ChooseColor(&cc))
				color = (COLORREF)cc.rgbResult;
			for (auto& el : buff_highlight_text) {
				if ((HWND)lp == bg_color_button)
					el->bk_color = color;
				else
					el->text_color = color;
				ef::sendToWnd(parent, (char*)el, sizeof(sumbol_info), PROC_SENDSUMBOL);
			}
			end_highLight_pos = 0; start_highLight_pos = 0;
		}
		else if ((HWND)lp == find_button) {
			ZeroMemory(&frs, sizeof(FINDREPLACE));
			frs.lStructSize = sizeof(FINDREPLACE);
			frs.hwndOwner = hwnd;
			frs.lpstrFindWhat = findBuffer;
			frs.wFindWhatLen = MAX_REASON_NAME_LEN;
			frs.Flags = 0;
			findDlg = FindText(&frs);
		}
		else if (LOWORD(wp) == MENU_SAVE) {
			char* path = ef::SaveFileName(hwnd,"*.fs\0");
			if (!path) break;
			ef::SaveFile(path, cur_data_file);
			GetCaretPos(&caret_pos);
			ef::UpdateCaret(hwnd, 0, headerblockY, 1, line_height);
		}
		else if (LOWORD(wp) == MENU_OPEN) {
			char * path = ef::OpenFileName(hwnd, "*.fs\0");
			if (!path) break;
			ef::OpenFile(path, cur_data_file);
			ef::UpdateCaret(hwnd, 0, headerblockY, 1, line_height);
		}
	}; break;
	case WM_NOTIFY: {
		NMHDR* event_inf = (LPNMHDR)lp;
		if (event_inf->hwndFrom == spin_size_sumb) {
			switch (event_inf->code)
			{
			case UDN_DELTAPOS: {
				int delta = ((LPNMUPDOWN)lp)->iDelta;
				int pos = LOWORD(SendMessageA(spin_size_sumb, UDM_GETPOS, 0, 0));
				pos += delta;
				if (pos > 10 || pos < 2) break;
				SetWindowTextA(static_size_symb, std::to_string(pos).c_str());
				userChoiceSizeSumbol = pos;
				SendMessage(hwnd, WM_SIZE, 0, (LPARAM)MAKELPARAM(size_wnd.x,size_wnd.y));
			}; break;
			}
		}
	}; break;
	case WM_LBUTTONUP: {
		POINT mouse;
		mouse.x = LOWORD(lp);
		mouse.y = HIWORD(lp);
		RECT font_wnd_rect;
		GetWindowRect(fontList, &font_wnd_rect);
		if (!ef::MouseCollision(mouse, font_wnd_rect)) {
			SetFocus(hwnd);
		}
		if (!isFocusPage)break;
		int y = (mouse.y - headerblockY + scroll) / line_height;
		int x = mouse.x / sumbLen;
		auto temp = ef::getAllLinesLength(cur_data_file, size_wnd, sumbLen);
		if (y >= temp.size() || y < 0) { end_highLight_pos = 0; start_highLight_pos = 0; break; }
		if (x > temp[y] || x < 0) { end_highLight_pos = 0; start_highLight_pos = 0; break; }
		SetCaretPos(x * sumbLen, y * line_height + headerblockY - scroll);

		end_highLight_pos = ef::XYtoPos(temp, x, y + 1);

		GetCaretPos(&caret_pos);
	}; break;
	case WM_LBUTTONDOWN: {
		if (!isFocusPage)break;
		end_highLight_pos = -1;
		int x = LOWORD(lp);
		int y = HIWORD(lp);
		int new_x = x / sumbLen;
		int new_y = (y - headerblockY + scroll) / line_height;
		auto temp = ef::getAllLinesLength(cur_data_file, size_wnd, sumbLen);
		if (new_y >= temp.size() || new_y < 0) { end_highLight_pos = 0; start_highLight_pos = 0; break; }
		if (new_x > temp[new_y] || new_x < 0) { end_highLight_pos = 0; start_highLight_pos = 0; break; }
		SetCaretPos(x* sumbLen, y* line_height + headerblockY - scroll);

		start_highLight_pos = ef::XYtoPos(temp, new_x, new_y + 1);

		GetCaretPos(&caret_pos);
	}; break;
	case WM_SIZE: {
		MoveWindow(fontList, 0, 0, size_wnd.x / 5, 90, true);
		MoveWindow(bg_color_button, size_wnd.x / 5 + 5, 0, size_wnd.x / 5, 20, true);
		MoveWindow(text_color_button, size_wnd.x / 5 + 10 + size_wnd.x / 5, 0, size_wnd.x / 5, 20, true);
		MoveWindow(find_button, size_wnd.x / 5 + 10 + 2*(size_wnd.x / 5), 0, size_wnd.x / 5, 20, true);

		MoveWindow(static_text_symb, size_wnd.x / 5 + 10 + 2 * (size_wnd.x / 5) + size_wnd.x / 5, 0, (size_wnd.x / 5) / 2 - 30, 20, true);
		MoveWindow(static_size_symb, size_wnd.x / 5 + 10 + 2 * (size_wnd.x / 5) + (size_wnd.x / 5) + (size_wnd.x / 5) / 2 - 30, 0, (size_wnd.x / 5)/2, 20, true);
		MoveWindow(spin_size_sumb, size_wnd.x / 5 + 10 + 3*(size_wnd.x / 5) + (size_wnd.x / 5 - 30), 0, 20, 20, true);
		SetWindowText(static_size_symb, std::to_wstring(userChoiceSizeSumbol).c_str());
		GetCaretPos(&caret_pos);
		int new_x = -1;
		int new_y = -1;
		if (sumbLen) {
			new_x = caret_pos.x / sumbLen;
			new_y = (caret_pos.y - headerblockY) / line_height;
		}
		size_wnd.x = LOWORD(lp);
		size_wnd.y = HIWORD(lp);
		dpm = size_wnd.x / 210;
		sumbLen = dpm * userChoiceSizeSumbol;
		line_height = sumbLen * 2;
		DestroyCaret();
		CreateCaret(hwnd, NULL, 1, line_height);
		ShowCaret(hwnd);
		if (new_x != -1)
			SetCaretPos(new_x * sumbLen, new_y * line_height + headerblockY);
		int pos = GetScrollPos(hwnd, SB_VERT);
		ef::do_scroll(hwnd, caret_pos, scroll, headerblockY, line_height, pos);
	}; break;
	case WM_VSCROLL: {
		HWND sender = (HWND)lp;
		if (sender != NULL) break;
		int event = LOWORD(wp);
		switch (event)
		{
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION: {
			ef::do_scroll(hwnd, caret_pos, scroll, headerblockY, line_height, HIWORD(wp));
		}; break;
		}
		GetCaretPos(&caret_pos);
	}; break;
	case WM_GETMINMAXINFO: {
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lp;
		lpMMI->ptMinTrackSize.x = 1330;
		lpMMI->ptMinTrackSize.y = headerblockY * 2;
	}; break;
	case WM_COPYDATA: {
		COPYDATASTRUCT* cds = (PCOPYDATASTRUCT)lp;
		DWORD action = cds->dwData;
		DWORD size = cds->cbData;
		switch (action)
		{
		case PROC_DATA: {
			char* data = new char[size];
			for (int i = 0; i < size; ++i) data[i] = ((char*)(cds->lpData))[i];
			buffer* buff = new buffer(data, size);
			while (buff->getLength() > buff->getPointR()) {
				sumbol_info* sumbol = (sumbol_info*)buff->read(sizeof(sumbol_info));
				cur_data_file.push_back(*sumbol);
			}
		}; break;
		case PROC_SUMBOL: {
			char* data = new char[size];
			for (int i = 0; i < size; ++i) data[i] = ((char*)(cds->lpData))[i];
			buffer buff(data, size);
			sumbol_info* sumbol = (sumbol_info*)buff.read(sizeof(sumbol_info));
			if (cur_data_file.size() <= sumbol->pos + 1)
				cur_data_file.resize(sumbol->pos + 1);
			cur_data_file[sumbol->pos] = *sumbol;
		}; break;
		};
	}; break;
	case WM_DESTROY: {
		ef::sendToWnd(parent, "", 0, PROC_CLOSE);
		PostQuitMessage(0);
	}; break;
	default: {
		if (msg == findReplaceMsg) {
			line_length = ef::getAllLinesLength(cur_data_file, size_wnd, sumbLen);
			int y = (caret_pos.y - headerblockY + scroll) / line_height;
			int x = caret_pos.x / sumbLen;
			int s_e_pos = ef::XYtoPos(line_length, x, y + 1);

			bool isLower = true;
			LPFINDREPLACE fr_local_s = (LPFINDREPLACE)lp;
			if (fr_local_s->Flags & FR_MATCHCASE) isLower = false;
			if (fr_local_s->Flags & FR_FINDNEXT) {
				int p = -1;
				wchar_t* arr = fr_local_s->lpstrFindWhat;
				if (fr_local_s->Flags & FR_DOWN) {
					p = ef::SearchSubString(std::wstring(arr), cur_data_file, s_e_pos, cur_data_file.size(),isLower);
				}
				else {
					p = ef::SearchSubString(std::wstring(arr), cur_data_file, 0, s_e_pos,isLower);
				}
				find_pos = p;
				find_len = std::wstring(arr).length();

			}
			DestroyWindow(findDlg);
			findDlg = NULL;
			DestroyCaret();
			CreateCaret(hwnd, NULL, 1, line_height);
			ShowCaret(hwnd);
			SetCaretPos(caret_pos.x, caret_pos.y);
		}
	}; break;
	};
	return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE app, HINSTANCE a, LPSTR cmd, int show) {
	g_app = app;
	cur_file_name = cmd;
	findReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);

	WNDCLASS wnd_class{ 0 };
	wnd_class.hInstance = app;
	wnd_class.lpfnWndProc = proc;
	wnd_class.lpszClassName = L"file_editor";
	wnd_class.hbrBackground = CreateSolidBrush(RGB(240, 240, 240));
	RegisterClass(&wnd_class);
	editor = CreateWindow(L"file_editor", L"FSaccesEditor", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, NULL, app, NULL);
	MSG msg;
	while (GetMessage(&msg, 0, NULL, NULL)) {
		if(findDlg)
			IsDialogMessage(findDlg, &msg);
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

