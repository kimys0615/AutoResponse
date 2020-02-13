#ifndef _WINSOCK2_H_
#include <winsock2.h>
#endif
#include <tchar.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Parameter.h"
#include "TCP_Common.h"
//소켓 함수 오류를 출력 후 종료한다. 
void err_quit ( char *msg ) 
{

	LPVOID lpMsgBuf;

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL , WSAGetLastError() , MAKELANGID ( LANG_NEUTRAL , SUBLANG_DEFAULT ) , 
		(LPTSTR)&lpMsgBuf , 0 , NULL );

	MessageBox ( NULL , (LPCTSTR)lpMsgBuf , (LPCTSTR)msg , MB_ICONERROR );

	LocalFree ( lpMsgBuf );

	exit(1);
}
//소켓 함수 오류를 출력한다.
void err_display ( char* msg )
{

	LPVOID lpMsgBuf;

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM , 
		NULL , WSAGetLastError() , MAKELANGID ( LANG_NEUTRAL , SUBLANG_DEFAULT ) ,
		(LPTSTR)&lpMsgBuf , 0 , NULL );

	MessageBox(NULL, (LPCTSTR)lpMsgBuf, (LPCTSTR)msg, MB_ICONERROR);

	LocalFree(lpMsgBuf);
}
//TCHAR 문자열일때 inet_addr을 사용할경우 사용한다.
DWORD _tinet_addr(const TCHAR * cp)
{
#ifdef UNICODE
	char IP[16];
	int Ret = 0;
	Ret = WideCharToMultiByte(CP_ACP, 0, cp, lstrlen(cp), IP, 15, NULL, NULL);
	IP[Ret] = 0;
	return inet_addr(IP);
#endif
#ifndef UNICODE
	return init_addr(cp);
#endif
}
//TCPIP통신에서 Read기능을한다.
BOOL TCPIP_Read(TCHAR* RcvData, TCHAR* TermStr, int Max_Read_Count, int TimeOut, int* ReadCount)
{
	int retval, Rcvstrlen;
	char RcvBuf[50];
	TCHAR RcvBuf2[50];
	int TermStrlen;
	TCHAR *ptr;
	int i = -1;

	memset(RcvBuf, 0, sizeof(RcvBuf));
	memset(RcvBuf2, 0, sizeof(RcvBuf));

	TermStrlen = lstrlen(TermStr);
	retval = recv(sock, RcvBuf, Max_Read_Count, 0);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, RcvBuf, 50, RcvBuf2, 50);

	Rcvstrlen = strlen(RcvBuf);

	if (retval == SOCKET_ERROR)
	{
		return FALSE;
	}

	ptr = _tcsstr(RcvBuf2, TermStr);
	if (ptr == NULL) return FALSE;
	if (lstrlen(ptr) <= 0) return FALSE;

	for (i = 0; i < lstrlen(RcvBuf2); i++)
	{
		if (TermStrlen == 1)
		{
			if (RcvBuf2[i] == TermStr[0]) RcvBuf2[i] = '\0';
		}
		else
		{
			if (_tcscmp(RcvBuf2 + i, TermStr) == 0) RcvBuf2[i] = '\0';
		}

	}

	_sntprintf(RcvData, 50, RcvBuf2);
	return TRUE;
}
//TCPIP통신에서 Write기능을 한다.
BOOL TCPIP_Write(TCHAR *Write_Data_Buffer, int Write_Count)
{
	int retval;
	char WriteBuf[50];

	memset(WriteBuf, 0, sizeof(WriteBuf));
	WideCharToMultiByte(CP_ACP, 0, Write_Data_Buffer, 50, WriteBuf, 50, NULL, NULL);

	retval = send(sock, WriteBuf, Write_Count, 0);
	if (retval == SOCKET_ERROR) return FALSE;

	return TRUE;
}