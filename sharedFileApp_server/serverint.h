#pragma once
#include "general.h"
class client_info;
class protoServer {
public:
	protoServer(){};
	virtual void read(msg_type* type,int size,SOCKET *sock) = 0;
};