#pragma once
#include "..\Buffer\Buffer.h"
#include <vector>
#include <list>
#include <string>
#include <sstream>

enum class msg_type {
	connect,
	disconnect,
	user_log,
	user_reg,
	delete_acc,
	access,
	n_access,
	create_file,
	open_file,
	file_name,
	close_file,
	update_file,
	delete_file,
	message,
	get_all_filenames,
	add_user,
	del_user,
};

struct user {
	std::string login = "";
	std::string password = "";
};

struct sumbol_info {
	wchar_t sumbol;
	std::uint64_t pos;
	unsigned int font = 0;
	COLORREF bk_color = RGB(240, 240, 240);
	COLORREF text_color = RGB(0, 0, 0);
};
