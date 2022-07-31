#include "server.h"

server* fsa_server = new server;

bool WINAPI ConsosleEvent(DWORD eventCode) {
	switch (eventCode)
	{
	case CTRL_CLOSE_EVENT:
		delete fsa_server;
		break;
	}
	return true;
}

int main(int argc, char* argv[])
{
	std::string ip = "";
	if (argc > 1)
		ip += argv[1];
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsosleEvent, true);
	fsa_server->start(ip);
	return 0;
}
