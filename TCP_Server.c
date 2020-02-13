#ifndef _WINSOCK2_H_
#include <winsock2.h>
#endif
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include "TCP_Common.h"
#include "Parameter.h"


//TCP Server Connect 결과를 모니터링 하는 Thread이다.
void TCPServer_AcceptThread( void* p ) 
{
	SOCKET Server_listen_Sock = (SOCKET)p;
	SOCKADDR_IN clientaddr;
	int addrlen = -1, StartTime = -1, EndTime = -1, retry = 0;
	addrlen = sizeof(clientaddr);
	Server_Acceptflag = Waiting;
	StartTime = GetTickCount();
	while ( TRUE ) 
	{
		sock = accept (Server_listen_Sock , (SOCKADDR* ) &clientaddr , &addrlen );
		if( sock == INVALID_SOCKET ) 
		{
			if (Server_Acceptflag == Disconnect) break;
			Server_Acceptflag = Fail;
			EndTime = GetTickCount();
			if ((EndTime - StartTime) >= 3000 * (retry + 1)) 	retry++;
			if (retry >= 3)
			{
				Server_Acceptflag = Timeout;
				break;
			}
		}
		else 
		{
			Server_Acceptflag = Connect;
			break;
		}
		Sleep(1);
	}
	
	

	return ;
}
//TCP Server Connect 해제한다.
void TCPServer_Close () 
{
	WSACleanup();
	closesocket(sock);
}

//Host IP주소와 Port Number를 전달받아 Server를 구성한다.
BOOL TCPServer_Connect (DWORD Host , int PortNo )
{
	int retval;
	WSADATA wsa;
	SOCKET listen_sock;
	SOCKADDR_IN serveraddr;
	SOCKADDR_IN clientaddr;
	int addrlen;
	

	if ( WSAStartup(MAKEWORD(2,2) , &wsa) != 0 ) return 0;

	listen_sock = socket ( AF_INET , SOCK_STREAM , 0 );
	if ( listen_sock == INVALID_SOCKET ) err_quit("socket()");

	
	ZeroMemory( &serveraddr , sizeof( serveraddr ) );

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(Host);
	serveraddr.sin_port = htons(PortNo);
	
	retval = bind( listen_sock , (SOCKADDR* ) &serveraddr, sizeof( serveraddr ) ) ;
	if ( retval == SOCKET_ERROR )
	{
		err_display("Bind()");
		return FALSE;
	}

	retval = listen(listen_sock , SOMAXCONN );
	if ( retval == SOCKET_ERROR )
	{
		err_display("listen()");
		return FALSE;
	}

	
	
	addrlen = sizeof(clientaddr);
	_beginthread(TCPServer_AcceptThread, 0, (void*)listen_sock);
	
	return TRUE;
}