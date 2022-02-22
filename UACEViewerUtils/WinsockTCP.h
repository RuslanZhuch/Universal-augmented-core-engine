#pragma once
#define _WINSOCKAPI_
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <Winsock2.h>
//#include <winsock2.h>
//#include <ws2tcpip.h>

static auto initWinsock()
{

	static WSADATA wsaData;
	static WORD DllVersion = MAKEWORD(2, 2);
	static auto result{ WSAStartup(DllVersion, &wsaData) };
	return result;

}