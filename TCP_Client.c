#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>
#include <process.h>
#include <string.h>
#include "TCP_Common.h"
#include "Parameter.h"
#include "Auto_Common.h"

DWORD HostAddr = -1;

int HostPort = -1;

//TCP Client Connect 결과를 모니터링 하는 Thread이다.
void TCPClient_ConnectThread( void *p ) 
{
	
	int retval,retry=0,StartTime,EndTime;
	SOCKADDR_IN serveraddr;

	ZeroMemory(&serveraddr, sizeof(HostAddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(HostAddr);
	serveraddr.sin_port = htons(HostPort);

	Client_Connectflag = Not_Use;
	StartTime = GetTickCount();
	while (TRUE) 
	{
		Client_Connectflag = Fail;
		retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) 
		{
			if (Client_Connectflag == Disconnect) break;
			EndTime = GetTickCount();
			if ((EndTime - StartTime) >= 10000)
			{
				Client_Connectflag = Timeout;
				break;
			}
		}
		else
		{
			Client_Connectflag = Connect;
			break;
		}
		Sleep(1);
	}
	return ;
}
//TCP Client Connect 해제한다.
void TCPClient_Close () 
{
	WSACleanup();
	closesocket(sock);
}
//Host IP주소와 Port Number를 전달받아 Server에 Connect한다.
BOOL TCPClient_Connect (DWORD Host , int PortNo )
{
	WSADATA wsa;

	HostAddr = Host;
	HostPort = PortNo;

	if ( WSAStartup(MAKEWORD(2,2) , &wsa) != 0 ) return 1;

	sock = socket ( AF_INET , SOCK_STREAM , 0 );

	if (sock == INVALID_SOCKET) 
	{
		err_quit("socket()");
		return FALSE;
	}
	
	_beginthread(TCPClient_ConnectThread, 0, NULL);
	
	return TRUE;
}

