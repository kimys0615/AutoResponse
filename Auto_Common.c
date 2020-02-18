#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <process.h>
#include <commctrl.h>
#include <string.h>
#include "TCP_Client.h"
#include "TCP_Server.h"
#include "TCP_Common.h"
#include "resource.h"
#include "CimRs232.h"
#include "CimSeqnc.h"
#include "Kutlstr.h"

#include "Parameter.h"
#include "parson.h"
#include "Auto_Common.h"

TCHAR ReturnBuf[256];

int giMatch_Serial = -1;
int giTX_History = 0, giRX_History = 0;

BOOL gbEndSignal = FALSE;

typedef struct {
	int Check_Serial;
	TCHAR Check_Result[128];
}Check_Serial;

typedef struct {
	int Result;
	char RcvData[128];
	int DataLen;
}Req_Data;


//문자열을 전달받아 '$'를 의 개수를 리턴해준다.
int Find_Tag_Char(TCHAR* Data)
{
	int len, i, cnt = 0;
	len = lstrlen(Data);
	for (i = 0; i < len; i++)
	{
		if (Data[i] == '$') cnt++;
	}
	return cnt;
}

//문자열을 전달받아 '$'의 위치를 리턴해준다. 
int Find_Tag(TCHAR* pszTag)
{
	int i;

	for (i = 0; i < 128; i++)
	{
		if (pszTag[i] == '$') return i;
	}

	return -1;
}
//Log에 남겨질 Rx/Tx Type과 Logging할 데이터를 전달받아 LogFile을 생성한다.
void Logging(TCHAR* Type, TCHAR* Data) 
{
	SYSTEMTIME st;
	FILE* fp;
	TCHAR temp[512];
	TCHAR Buffer[256];

	memset(temp, 0x00, sizeof(temp) - 1);
	memset(Buffer, 0x00, sizeof(Buffer) - 1);
		
	GetLocalTime(&st);
	_sntprintf(temp, sizeof(temp), TEXT("%4d%02d%02d_AutoResponse.txt"), st.wYear, st.wMonth, st.wDay);
	fp = _tfopen(temp, TEXT("a+"));
	_sntprintf(temp, sizeof(temp), TEXT("[%4d:%02d:%02d %2d:%02d:%02d:%03d] \t %s \t %s"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, Type, Data);
		
	_fputts(temp, fp);
	_fputts(TEXT("\n"), fp);
	fclose(fp);
}

//LogFile 생성
void Log_Create(TCHAR* FileName , TCHAR* Cfg, TCHAR* Data) {
	SYSTEMTIME st;
	FILE* fp; 
	TCHAR temp[512];
	TCHAR Buffer[256];
	TCHAR str[255];
	int i;

	memset(temp, 0x00, sizeof(temp) - 1);
	memset(Buffer, 0x00, sizeof(Buffer) - 1);

	_sntprintf(str, sizeof(str), TEXT("%s.txt"), FileName);
	fp = _tfopen(str, TEXT("a+"));
	GetLocalTime(&st);

	for (i = 0; i < 50; i++)
	{
		temp[i] = Data[i] + '0';
	}

	//_sntprintf(temp, sizeof(temp), TEXT("%4d:%2d:%2d %2d:%2d:%2d:%3d \t %s \t %s"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, Cfg, Data);
	//	while ( TRUE ) {
	//		memset ( Buffer , 0x00 , sizeof(Buffer) -1 );
	//		fgets ( Buffer , sizeof(Buffer) ,  fp );
	//		if( lstrlen ( Buffer ) == 0 ) break;
	//		Sleep ( 10 );
	//	}
	_fputts(TEXT("\n"), fp);
	_fputts(temp, fp);
	fclose(fp);
}
void Data_Logging(int Control, int Logging_Style) {
	SYSTEMTIME st;
	char FileName[256];
	if (Control == FALSE) RS232_Logging_Control(FALSE);
	else {
		GetLocalTime(&st);
		sprintf(FileName, "%04d%02d%02d%02d_RS232.log", st.wYear, st.wMonth, st.wDay, st.wHour);
		RS232_Logging_Style(Logging_Style, FileName);
		RS232_Logging_Control(TRUE);
	}
}
//TCHAR 문자열을 CHAR문자열로 변환해준다.
void TCHAR_TO_CHAR(TCHAR* tchardata, char* chardata, int size) {
	WideCharToMultiByte(CP_ACP, 0, tchardata, size, chardata, size, NULL, NULL);
}
//CHAR 문자열을 TCHAR문자열로 변환해준다.
void CHAR_TO_TCHAR(char* chardata, TCHAR* tchardata, int size) {
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, chardata, size, tchardata, size);
}
void CHAR_TO_TCHAR2(char* chardata, int charsize , TCHAR* tchardata) {
	int charbuf, i;

	if (charsize <= 0) return;

	
	for (i = 0; i < charsize; i++) {
		charbuf = chardata[i];
		tchardata[i] = charbuf;
		
	}
		
}
void CHAR_TO_LOGBUUFER(char* chardata, char* logdata, int size) {
	int i,j;
	for (i = 0,j=0; i < size; i++) {
		if (chardata[i] == '\0') {
			snprintf(logdata+j,size-j, "[NULL]");
			j += 6;
		}
		else {
			logdata[j] = chardata[i];
			j++;
		}
	}
}
//RX통신에 사용된 문자열을 전달받아 RX통신 이력에 저장한다. ( 16개까지 저장 가능 )
void RX_History_Save(TCHAR* RcvData) 
{
	if (lstrlen(RcvData) > 0) 
	{
		_sntprintf(RX_History[giRX_History], sizeof(RX_History[giRX_History]), RcvData);
		if (++giRX_History == 16) giRX_History = 0;
	}
}
//TX통신에 사용된 문자열을 전달받아 TX통신 이력에 저장한다. ( 16개까지 저장 가능 )
void TX_History_Save(TCHAR* RcvData) 
{
	if (lstrlen(RcvData) > 0) 
	{
		_sntprintf(TX_History[giTX_History], sizeof(TX_History[giTX_History]), RcvData);
		if (++giTX_History == 16) giTX_History = 0;
	}
}
//개발 Log를 남길 Type과 Data를 전달받아 Log를 출력해 준다.
void Edit_Print_Log(TCHAR* Type, TCHAR* RcvData) 
{
	TCHAR PrintVal[256];
	SYSTEMTIME st;
	int nLen = -1;

	memset(PrintVal, 0, sizeof(PrintVal));

	GetLocalTime(&st);
	_sntprintf(PrintVal, sizeof(PrintVal), TEXT("%4d:%2d:%2d %2d:%2d:%2d:%3d\t%s\t%s"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, Type, RcvData);

	nLen = GetWindowTextLength(hEdit_Log);
	lstrcat(PrintVal, TEXT("\r\n"));
	SendMessage(hEdit_Log, EM_SETSEL, nLen, nLen);
	SendMessage(hEdit_Log, EM_REPLACESEL, FALSE, (LPARAM)PrintVal);

}
//송수신 결과를 나타낼 RX/TX Type과 송수신에 사용된 문자열을 전달받아 통신로그 창에 출력한다.
void Edit_Print_RX(TCHAR* Type, TCHAR* RcvData)
{
	int nLen = -1, i = -1;
	TCHAR PrintVal[128];
	TCHAR Temp[128];

	memset(PrintVal, 0x00, sizeof(PrintVal) - 1);
	memset(Temp, 0x00, sizeof(Temp) - 1);
	switch (g_RX_PrintType) 
	{
	case 0:
		_sntprintf(PrintVal, sizeof(PrintVal), Type);
		lstrcat(PrintVal, RcvData);
		TX_History_Save(PrintVal);
		nLen = GetWindowTextLength(hEdit_RXTX);
		Logging(Type, PrintVal);
		if (g_RX_CfgNo == 3) {
			if (lstrlen(TermStr[g_TX_CfgNo]) <= 0) lstrcat(PrintVal, TEXT("[NULL]"));
		}
		lstrcat(PrintVal, TEXT("\r\n"));
		SendMessage(hEdit_RXTX, EM_SETSEL, nLen, nLen);
		SendMessage(hEdit_RXTX, EM_REPLACESEL, FALSE, (LPARAM)PrintVal);
		if (_tcscmp(Type, RXTX_TYPE[Tx]) == 0)SetWindowText(hEdit_TX_SEND, TEXT(""));

		break;
	case 1:
		_sntprintf(PrintVal, sizeof(PrintVal), Type);
		for (i = 0; i < 128; i++) 
		{
			_sntprintf(Temp, sizeof(Temp), TEXT("%02x "), RcvData[i]);
			if (RcvData[i] == '\0') break;
			lstrcat(PrintVal, Temp);
		}
		TX_History_Save(PrintVal);
		Logging(Type, PrintVal);
		nLen = GetWindowTextLength(hEdit_RXTX);
		if (g_RX_CfgNo == 3) {
			if (lstrlen(TermStr[g_TX_CfgNo]) <= 0) lstrcat(PrintVal, TEXT("[NULL]"));
		}
		lstrcat(PrintVal, TEXT("\r\n"));
		SendMessage(hEdit_RXTX, EM_SETSEL, nLen, nLen);
		SendMessage(hEdit_RXTX, EM_REPLACESEL, FALSE, (LPARAM)PrintVal);

		if (_tcscmp(Type, RXTX_TYPE[Tx]) == 0)SetWindowText(hEdit_TX_SEND, TEXT(""));
		break;
	}


}

void Edit_Print_TX(TCHAR* Type, TCHAR* RcvData)
{
	int nLen = -1, i = -1;
	TCHAR PrintVal[128];
	TCHAR Temp[128];

	memset(PrintVal, 0x00, sizeof(PrintVal) - 1);
	memset(Temp, 0x00, sizeof(Temp) - 1);
	switch (g_TX_PrintType)
	{
	case 0:
		_sntprintf(PrintVal, sizeof(PrintVal), Type);
		lstrcat(PrintVal, RcvData);
		TX_History_Save(PrintVal);
		nLen = GetWindowTextLength(hEdit_RXTX);
		Logging(Type, PrintVal);
		if (g_TX_CfgNo == 4) {
			if (lstrlen(TermStr[g_TX_CfgNo]) <= 0) lstrcat(PrintVal, TEXT("[NULL]"));
		}
		lstrcat(PrintVal, TEXT("\r\n"));
		SendMessage(hEdit_RXTX, EM_SETSEL, nLen, nLen);
		SendMessage(hEdit_RXTX, EM_REPLACESEL, FALSE, (LPARAM)PrintVal);
		if (_tcscmp(Type, RXTX_TYPE[Tx]) == 0)SetWindowText(hEdit_TX_SEND, TEXT(""));

		break;
	case 1:
		_sntprintf(PrintVal, sizeof(PrintVal), Type);
		for (i = 0; i < 16; i++)
		{
			_sntprintf(Temp, sizeof(Temp), TEXT("%02x "), RcvData[i]);
			if (RcvData[i] == '\0') break;
			lstrcat(PrintVal, Temp);
		}
		TX_History_Save(PrintVal);
		Logging(Type, PrintVal);
		nLen = GetWindowTextLength(hEdit_RXTX);
		if (g_TX_CfgNo == 4) {
			if (lstrlen(TermStr[g_TX_CfgNo]) <= 0) lstrcat(PrintVal, TEXT("[NULL]"));
		}
		lstrcat(PrintVal, TEXT("\r\n"));
		SendMessage(hEdit_RXTX, EM_SETSEL, nLen, nLen);
		SendMessage(hEdit_RXTX, EM_REPLACESEL, FALSE, (LPARAM)PrintVal);

		if (_tcscmp(Type, RXTX_TYPE[Tx]) == 0)SetWindowText(hEdit_TX_SEND, TEXT(""));
		break;
	}


}

//활성화 유/무를 전달받아 RS232 or TCP/IP 통신 연결상태에 따라 Modaless/Main Item을 활성화/비활성화 해준다.
void Disable_Button(BOOL Check) 
{
	EnableWindow(hButton_IDOK, Check);
	EnableWindow(hRadio_TCPClient, Check);
	EnableWindow(hRadio_TCPServer, Check);
	EnableWindow(hRadio_Serial, Check);
	EnableWindow(hCombo_ComPort, Check);
	EnableWindow(hCombo_Baud, Check);
	EnableWindow(hCombo_Data, Check);
	EnableWindow(hCombo_Parity, Check);
	EnableWindow(hCombo_Stop, Check);
	EnableWindow(hEdit_Port, Check);
	EnableWindow(hRadio_Mode_Common, Check);
	EnableWindow(hRadio_Mode_Secs, Check);
	if (giMode == 1) {
		EnableWindow(hRadio_RXCfg_CR, Check);
		EnableWindow(hRadio_RXCfg_LF, Check);
		EnableWindow(hRadio_RXCfg_CRLF, Check);
		EnableWindow(hRadio_RXCfg_etc, Check);
		EnableWindow(hEdit_RXCfg_etc, Check);
		EnableWindow(hButton_RXETC, Check);
		EnableWindow(hRadio_TXCfg_CR, Check);
		EnableWindow(hRadio_TXCfg_LF, Check);
		EnableWindow(hRadio_TXCfg_CRLF, Check);
		EnableWindow(hRadio_TXCfg_etc, Check);
		EnableWindow(hEdit_TXCfg_etc, Check);
		EnableWindow(hButton_TXETC, Check);


	}

}

//BINARY문자열을 전달받아 ASCII로 변환하여준다.
TCHAR* Convert_BINARY_to_ASCII(TCHAR* RcvBuf) 
{
	int i = -1, val = -1, Cnt = 0;
	TCHAR Temp[128], Buf1[3];
	int size = sizeof(RcvBuf);

	memset(Temp, 0, sizeof(Temp));
	memset(Buf1, 0, sizeof(Buf1));
	memset(ReturnBuf, 0, sizeof(ReturnBuf));
	_sntprintf(Temp, sizeof(Temp), RcvBuf);

	for (i = 0; i < lstrlen(Temp); i++) 
	{

		if (Temp[i] == ' ') 
		{
			ReturnBuf[Cnt] = ' ';
			Cnt++;
		}
		else if ((Temp[i] >= 0x30 && Temp[i] <= 0x39) || (Temp[i] >= 0x41 && Temp[i] <= 0x46) || (Temp[i] >= 0x61 && Temp[i] <= 0x66)) 
		{
			_sntprintf(Buf1, sizeof(Buf1), TEXT("%c%c"), Temp[i], Temp[i + 1]);
			val = _tcstol(Buf1, NULL, 16);

			ReturnBuf[Cnt] = val;
			i = i + 1;
			Cnt++;
		}
		else if (Temp[i] == 0x24) 
		{
			lstrcat(ReturnBuf, Temp + i);
			break;
		}
	}
	return ReturnBuf;
}
//문자열을 전달받아 Spaece_Bar를 기준으로 파싱하여 문자열을 2개로 나눠준다.
void STR_SEP(TCHAR* SourceString, TCHAR* ResultString1, TCHAR* ResultString2, int ReadMaxCount) 
{
	int i = -1;
	TCHAR TEMPSTR1[255], TEMPSTR2[255];
	BOOL Do_Flag = FALSE;
	memset(TEMPSTR1, 0, sizeof(TEMPSTR1));
	memset(TEMPSTR2, 0, sizeof(TEMPSTR2));
	for (i = 0; i < ReadMaxCount; i++)
	{
		if (SourceString[i] == 0x20)
		{
			Do_Flag = TRUE;
			_sntprintf(TEMPSTR2, lstrlen(SourceString + i) + 1, SourceString + i + 1);
			_sntprintf(TEMPSTR1, lstrlen(SourceString) + 1, SourceString);
			memset(TEMPSTR1 + i, 0, lstrlen(TEMPSTR1 + i) + 1);
			break;
		}
	}
	if (Do_Flag == TRUE) {
		_sntprintf(ResultString1, ReadMaxCount, TEMPSTR1);
		_sntprintf(ResultString2, ReadMaxCount, TEMPSTR2);
	}
	else {
		_sntprintf(ResultString1, ReadMaxCount, SourceString);
	}
}
//문자열과 Delimiter를 전달받아 Delimiter를 기준으로 파싱하여 문자열을 2개로 나눠준다.
void STR_SEP_CHAR(TCHAR* SourceString, TCHAR delimiterChar, TCHAR* ResultString1, TCHAR* ResultString2, int ReadMaxCount) 
{
	int i = -1;
	TCHAR TEMPSTR1[255],TEMPSTR2[255];
	BOOL Do_Flag = FALSE;
	memset(TEMPSTR1, 0, sizeof(TEMPSTR1));
	memset(TEMPSTR2, 0, sizeof(TEMPSTR2));
	for (i = 0; i < ReadMaxCount; i++)
	{
		if (SourceString[i] == delimiterChar)
		{
			Do_Flag = TRUE;
			_sntprintf(TEMPSTR2, lstrlen(SourceString + i) + 1, SourceString + i+1);
			_sntprintf(TEMPSTR1, lstrlen(SourceString) + 1, SourceString);
			memset(TEMPSTR1 + i, 0, lstrlen(TEMPSTR1 + i) + 1);
			break;
		}
	}
	if (Do_Flag == TRUE) {
		_sntprintf(ResultString1, ReadMaxCount, TEMPSTR1);
		_sntprintf(ResultString2, ReadMaxCount, TEMPSTR2);
	}
	else {
		_sntprintf(ResultString1, ReadMaxCount, SourceString);
	}
	
}
//문자열을 전달받아 '|'의 개수를 리턴해준다.
int NumberOfvBar(TCHAR* Data) 
{
	int i = -1, len = -1, Cnt = 0;

	len = lstrlen(Data);
	for (i = 0; i < len; i++) 
	{
		if (Data[i] == '|')
			Cnt++;
	}
	return Cnt;
}
//ini File을 Load/Save 할때 Initialize 해준다.
BOOL SerialInitialize( TCHAR* FileNameStr) 
{
	TCHAR RecvTag[16];
	BOOL bStepEndFind = FALSE;
	TCHAR FilePath[MAX_PATH];
	FILE* fp = _tfopen(gszFileTitle, TEXT("r+"));
	TCHAR buffer[256];
	TCHAR buffer2[256];
	TCHAR pszbuf[256];
	TCHAR pszSend[256];
	char FileTitle[MAX_PATH];
	char charBuffer[256];
	TCHAR tcharBuffer[256];
	int i = 0, j = 0;
	int NumOfTag = -1, NumOfvBar = -1, DelCnt = -1;

	JSON_Object *cmd;
	JSON_Value *rootValue;
	JSON_Object *rootObject;
	JSON_Array *array1;
	JSON_Array *array2;
	
	memset(FilePath, 0, MAX_PATH);
	memset(pszbuf, 0, sizeof(pszbuf));
	memset(buffer, 0, sizeof(buffer));
	memset(buffer2, 0, sizeof(buffer2));
	memset(FileTitle, 0, sizeof(FileTitle));
	memset(pszSend, 0, sizeof(pszSend));
	memset(RecvTag, 0, sizeof(RecvTag));
	memset(charBuffer, 0, sizeof(charBuffer));
	memset(tcharBuffer, 0, sizeof(tcharBuffer));
	memset(g_Cfg, 0, sizeof(Cfg)*64);
	memset(Tag, 0, sizeof(Tag));
	lstrcpy(FilePath, FileNameStr);

	TCHAR_TO_CHAR(FileNameStr, FileTitle, MAX_PATH);
	rootValue = json_parse_file(FileTitle);
	rootObject = json_value_get_object(rootValue);
	array1 = json_object_get_array(rootObject, "COMMAND");
	/*for (j = 0; j < 128; j++) 
	{
		_sntprintf(Tag[j], 255, TEXT("0"));
	}*/


	for (i = 0; i < ((int) json_array_get_count(array1)) ; i++)     // 배열의 요소 개수만큼 반복
	{
		cmd = json_array_get_object(array1, i);
		if (strlen(json_object_get_string(cmd, "Type")) <= 0)continue;
		_snprintf(charBuffer, sizeof (charBuffer) , json_object_get_string(cmd, "Type"));
		CHAR_TO_TCHAR(charBuffer, tcharBuffer, 256);
		_sntprintf(g_Cfg[i].Type, sizeof(g_Cfg[i].Type), tcharBuffer);

		_snprintf(charBuffer, sizeof(charBuffer), json_object_get_string(cmd, "RECV"));
		CHAR_TO_TCHAR(charBuffer, tcharBuffer, 256);
		_sntprintf(g_Cfg[i].Receive, sizeof(g_Cfg[i].Receive), tcharBuffer);
		if (NumOfTag) STR_SEP_CHAR(g_Cfg[i].Receive, '$', g_Cfg[i].RX_Comp, RecvTag, lstrlen(g_Cfg[i].Receive));
		
		array2 = json_object_get_array(cmd, "SEND");
		for ( j = 0; j < ((int) json_array_get_count(array2)); j++)
		{
			_snprintf(charBuffer, sizeof(charBuffer), json_array_get_string(array2, j));
			CHAR_TO_TCHAR(charBuffer, tcharBuffer, 256);
			STR_SEP_CHAR(tcharBuffer, ':', buffer, buffer2, 256);
			g_Cfg[i].Send[j].index = _tstoi(buffer);
			STR_SEP_CHAR(buffer2, ':', g_Cfg[i].Send[j].Response, buffer, 256);
			g_Cfg[i].Send[j].Delay = _tstoi(buffer);

			g_Cfg[i].SendCnt = j+1 ;
			
		}
		g_Cfg[i].UseYN = TRUE;
	}
	/*
	while (!bStepEndFind) 
	{
		_fgetts(buffer, sizeof(buffer), fp);
		if (lstrlen(buffer) <= 0) return FALSE;
		buffer[lstrlen(buffer) - 1] = '\0';
		STR_SEP_CHAR(buffer, '|', buffer, pszbuf, lstrlen(buffer));

		if (_tcsncmp(buffer, TEXT("ASCII"), strlen("ASCII")) == 0) 
		{
			NumOfvBar = NumberOfvBar(pszbuf);
			if (NumOfvBar < 1) return FALSE;
			_sntprintf(g_Cfg[i].Type, sizeof(g_Cfg[i].Type), buffer);
			STR_SEP_CHAR(pszbuf, '|', pszSend, pszbuf, lstrlen(pszbuf));
			//pszSend = _tcstok(pszbuf, TEXT("|"));
			_sntprintf(g_Cfg[i].Receive, sizeof(g_Cfg[i]), pszSend);
			NumOfTag = Find_Tag_Char(g_Cfg[i].Receive);
			if (NumOfTag) STR_SEP_CHAR(g_Cfg[i].Receive, '$', g_Cfg[i].RX_Comp, RecvTag, lstrlen(g_Cfg[i].Receive));
			DelCnt = NumberOfvBar(pszbuf);
			g_Cfg[i].SendCnt = DelCnt;
			for (j = 0; j <= DelCnt; j++) {
				memset(buffer2, 0, sizeof(buffer2));
				STR_SEP_CHAR(pszbuf, '|', buffer2, pszbuf, lstrlen(pszbuf));
				_sntprintf(g_Cfg[i].Send[j], sizeof(g_Cfg[i].Send[j]), buffer2);
			}
//			_sntprintf(g_Cfg[i].Send, sizeof(g_Cfg[i].Send), pszbuf);//

			i++;
		}
		else if (_tcsncmp(buffer, TEXT("BINARY"), strlen("BINARY")) == 0) 
		{
			NumOfvBar = NumberOfvBar(pszbuf);
			if (NumOfvBar < 1) return FALSE;
			_sntprintf(g_Cfg[i].Type, sizeof(g_Cfg[i]), buffer);
			STR_SEP_CHAR(pszbuf, '|', pszSend, pszbuf, lstrlen(pszbuf));
			//pszSend = _tcstok(pszbuf, TEXT("|"));
			_sntprintf(g_Cfg[i].Receive, sizeof(g_Cfg[i].Receive), pszSend);
			NumOfTag = Find_Tag_Char(g_Cfg[i].Receive);
			if (NumOfTag) STR_SEP_CHAR(Convert_BINARY_to_ASCII(g_Cfg[i].Receive), '$', g_Cfg[i].RX_Comp, RecvTag, lstrlen(g_Cfg[i].Receive));
			DelCnt = NumberOfvBar(pszbuf);
			for (j = 0; j <= DelCnt; j++) {
				memset(buffer2, 0, sizeof(buffer2));
				STR_SEP_CHAR(pszbuf, '|', buffer2, pszbuf, lstrlen(pszbuf));
				_sntprintf(g_Cfg[i].Send[j], sizeof(g_Cfg[i].Send[j]), buffer2);
			}
			i++;
		}
		else if (_tcsnccmp(buffer, TEXT("Setting_End"), lstrlen(TEXT("Setting_End"))) == 0) 
		{
			Stepcnt = i;
			fclose(fp);
			bStepEndFind = TRUE;

		}
		else return FALSE;
		
	}
	*/
	return TRUE;
}
// ini File Save할 때 File을 생성하여 저장한다.
void SaveSerialSetting() 
{
	int i = 0;
	FILE* fp = _tfopen(gszFileTitle, TEXT("w"));
	BOOL bStepEnd = FALSE;
	TCHAR Save_Data[Max_Cfg_No][256];

	memset(Save_Data, 0, sizeof(Save_Data));

	while (!bStepEnd) 
	{
		for (i = 0; i < Max_Cfg_No; i++) 
		{
			if (lstrlen(Save_Serial[i]) <= 0) break;
			lstrcat(Save_Data[i], Save_Serial[i]);
			_fputts(Save_Data[i], fp);
			_fputts(TermStr[1], fp);
		}

		_fputts(TEXT("Setting_End"), fp);
		_fputts(TermStr[2], fp);
		bStepEnd = TRUE;
	}
	Stepcnt = i;

	fclose(fp);
}
//Index를 전달받아 Item Config 화면에서 Index번째 Auto Response항목을 삭제한다.
void SerialDelete(int SendStep ,int idx , int Type) 
{
	int i = -1, j, k;
	Cfg TempBuf[64];

	lstrcpy(Save_Serial[idx], TEXT(""));

	switch (Type) {
		case 1:
			for (i = 0, j = 0; i < Max_Cfg_No; i++)
			{
				if (g_Cfg[i].UseYN == TRUE) {
					lstrcpy(TempBuf[j].Receive, g_Cfg[i].Receive);
					lstrcpy(TempBuf[j].RX_Comp, g_Cfg[i].RX_Comp);
					TempBuf[j].SendCnt = g_Cfg[i].SendCnt;
					TempBuf[j].UseYN = g_Cfg[i].UseYN;
					TempBuf[j].SendIdx = g_Cfg[i].SendIdx;
					for (k = 0; k < 8; k++) {
						TempBuf[j].Send[k].Delay = g_Cfg[i].Send[k].Delay;
						TempBuf[j].Send[k].index = g_Cfg[i].Send[k].index;
						lstrcpy(TempBuf[j].Send[k].Response, g_Cfg[i].Send[k].Response);
					}
					j++;
				}
				
			}
			for (i = 0; i < Max_Cfg_No; i++) {
				if (TempBuf[i].UseYN == TRUE) {
					lstrcpy(g_Cfg[i].Receive, TempBuf[i].Receive);
					lstrcpy(g_Cfg[i].RX_Comp, TempBuf[i].RX_Comp);
					g_Cfg[i].SendCnt = TempBuf[i].SendCnt;
					g_Cfg[i].UseYN = TempBuf[i].UseYN;
					g_Cfg[i].SendIdx = TempBuf[i].SendIdx;
					for (k = 0; k < 8; k++) {
						g_Cfg[i].Send[k].Delay = TempBuf[i].Send[k].Delay;
						g_Cfg[i].Send[k].index = TempBuf[i].Send[k].index;
						lstrcpy(g_Cfg[i].Send[k].Response, TempBuf[i].Send[k].Response);
					}
				}
			}
			break;


				/*
				if (lstrlen(g_Cfg[i].Type) <= 0) continue;
				lstrcpy(g_Cfg[i].Type, g_Cfg[i + 1].Type);
				//lstrcpy(Save_Serial[i], Save_Serial[i + 1]);
				lstrcpy(g_Cfg[i].Receive, g_Cfg[i + 1].Receive);
				g_Cfg[i].SendCnt = g_Cfg[i + 1].SendCnt;
				
				for (j = 0; j < 8; j++) {
					g_Cfg[i].Send[j].index = g_Cfg[i+1].Send[j].index;
					lstrcpy(g_Cfg[i].Send[j].Response, g_Cfg[i+1].Send[j].Response);
					g_Cfg[i].Send[j].Delay = g_Cfg[i+1].Send[j].Delay;
				}*/
			
		case 2:
			for (j = idx; j <7 ; j++) {
				g_Cfg[SendStep].Send[j].index = g_Cfg[SendStep].Send[j+1].index;
				lstrcpy(g_Cfg[SendStep].Send[j].Response, g_Cfg[SendStep].Send[j+1].Response);
				g_Cfg[SendStep].Send[j].Delay = g_Cfg[SendStep].Send[j+1].Delay;
			}
		break;

	}
	//for (i = idx; i < Max_Cfg_No; i++) 
	//{
	//	if ( lstrlen ( g_Cfg[i + 1].Type ) <= 0) break;
	//	lstrcpy(g_Cfg[i].Type, g_Cfg[i + 1].Type);
	//	//lstrcpy(Save_Serial[i], Save_Serial[i + 1]);
	//	lstrcpy(g_Cfg[i].Receive, g_Cfg[i + 1].Receive);
	//	for (j = 0 ; j < g_Cfg[i + 1].SendCnt; j++) {
	//		g_Cfg[i].Send[j].index =  g_Cfg[i + 1].Send[j].index;
	//		lstrcpy(g_Cfg[i].Send[j].Response, g_Cfg[i + 1].Send[j].Response);
	//		g_Cfg[i].Send[j].Delay = g_Cfg[i + 1].Send[j].Delay; 
	//	}
	//}
}

//문자열을 전달받아 '$'태그를 찾아 '$'태그의 번호를 리턴해준다.
int Get_Tag_No(TCHAR* pszStr) 
{
	int i = -1, j = -1, k = 0, len = -1;
	TCHAR temp[32];
	int TagNo = -1;

	memset(temp, 0, sizeof(temp) - 1);
	len = lstrlen(pszStr);
	for (i = 0; i < len; i++) 
	{
		if (pszStr[i] == '$') 
		{
			for (j = i + 1; j < len; j++) 
			{
				if (pszStr[j] >= 0x30 && pszStr[j] <= 0x39) 
				{
					temp[k] = pszStr[j];
					k++;
				}
			}
		}
	}
	TagNo = _tstoi(temp);

	return TagNo;

}
//문자열을 전달받아 '$'이외에 다른 문자가 있는지 확인후 문자의 위치를 리턴한다.
TCHAR Find_CHAR(TCHAR* Data) 
{
	int i = -1;

	for (i = 0; i < lstrlen(Data); i++) 
	{
		if (Data[i] != '$') 
		{
			if (!(Data[i] >= 0x30 && Data[i] <= 0x39)) return Data[i];
		}
	}
	return -1;
}
//문자열과 구분자를  전달받아 구분자의 개수를 리턴한다.
int NumOfCHAR(TCHAR Data, TCHAR* RcvBuf) 
{
	int i,cnt=0;
	
	for (i = 0; i < lstrlen(RcvBuf); i++)  
	{
		if (RcvBuf[i] == Data) cnt++;
	}
	return cnt;
}
//SendData와 결과를 저장할 문자열을 전달받아 SendData의 '$Num' 위치에 '$Num'에 저장된 값으로 셋팅후에 결과를 Check문자열에 저장한다.
void Tagcat(TCHAR* SendData, TCHAR* Check, int NumOfTag)
{
	int i = -1, j = -1, k = -1, len = -1;
	TCHAR Temp[8];
	TCHAR RecvData[64], RecvTag[64], TempBuf[64];
	int Tagno, TagCnt = 0;

	memset(Temp, 0, sizeof(Temp) - 1);
	memset(RecvData, 0, sizeof(RecvData) - 1);
	memset(RecvTag, 0, sizeof(RecvData) - 1);
	memset(TempBuf, 0, sizeof(TempBuf) - 1);
	if (SendData[0] != '$')
	{
		STR_SEP_CHAR(SendData, '$', RecvData, TempBuf, lstrlen(SendData));
		_sntprintf(RecvTag, sizeof(RecvTag), TEXT("$%s"), TempBuf);
	}
	else _sntprintf(RecvTag, sizeof(RecvTag), SendData);
	len = lstrlen(SendData);
	_sntprintf(Check, sizeof(Check) + 1, RecvData);
	//if ( lstrlen ( RecvData ) > 0 ) lstrcat ( Check , TEXT(" ") );
	for (i = 0; i < len; i++) 
	{
		k = 0;
		if (RecvTag[i] == '$') 
		{
			for (j = i + 1; j <= len; j++) 
			{
				if (RecvTag[j] >= 0x30 && RecvTag[j] <= 0x39) 
				{
					Temp[k] = RecvTag[j];
					k++;
				}
				else break;
			}
			Tagno = _tstoi(Temp);
			if (Tagno != 0) 
			{
				lstrcat(Check, Tag[Tagno]);
				Tagno = 0;
				memset(Temp, 0, sizeof(Temp) - 1);
				k = 0;
				TagCnt++;

				if (TagCnt == NumOfTag) 
				{
					/*
					for ( i = 0 ; i < NumOfTag ; i++ ) 
					{
						free ( SaveVal[i] );
					}
					free ( SaveVal );
					*/
					return;
				}

			}
		}
	}
}


//'$Num'와 '$Num'에 저장할 값을 전달받아 '$Num'에 Val값을 저장한다.
void Tag_Set(TCHAR* TagNo, TCHAR* Val, int NumOfTag) 
{
	int len = -1, i = -1, j = -1, k = -1;
	TCHAR Temp[5];
	int Tagno = -1;
	int itemCnt = 0;
	int vallen = -1;
	int ValCnt = 0;

	TCHAR** SaveVal;
	TCHAR Del;

	memset(Temp, 0, sizeof(Temp));
	Del = Find_CHAR(Val);
	SaveVal = (TCHAR**)malloc(sizeof(TCHAR*) * NumOfTag);
	for (i = 0; i < NumOfTag; i++) 
	{
		SaveVal[i] = (TCHAR*)malloc(64);
		memset(SaveVal[i], 0, 64);
	}
	if (NumOfTag == 1) _sntprintf(SaveVal[0], 64, Val);

	for (i = 0; i < NumOfTag - 1; i++) 
	{

		if (i == 0) STR_SEP_CHAR(Val, Del, SaveVal[0], SaveVal[NumOfTag - 1], lstrlen(Val));
		else STR_SEP_CHAR(SaveVal[NumOfTag - 1], Del, SaveVal[i], SaveVal[NumOfTag - 1], lstrlen(SaveVal[NumOfTag - 1]));

	}
	len = lstrlen(TagNo);
	vallen = lstrlen(Val);
	for (i = 0; i <= len; i++) 
	{
		memset(Temp, 0, sizeof(Temp) - 1);
		if (itemCnt == NumOfTag) break;
		k = 0;
		if (TagNo[i] == '$') 
		{
			for (j = i + 1; j < len; j++) 
			{
				if (TagNo[j] >= 0x30 && TagNo[j] <= 0x39) 
				{
					Temp[k] = TagNo[j];
					k++;
				}
				else break;
			}
			Tagno = _tstoi(Temp);
			if (Tagno) 
			{
				k = 0;
				if (lstrlen(SaveVal[ValCnt]) <= 0) _sntprintf(Tag[Tagno], sizeof(Tag[Tagno]), TEXT("0"));
				else _sntprintf(Tag[Tagno], sizeof(Tag[Tagno]), SaveVal[ValCnt]);
				k++;
				ValCnt++;
				if (ValCnt == NumOfTag) 
				{
					for (i = 0; i < NumOfTag; i++) 
					{
						free(SaveVal[i]);
					}
					free(SaveVal);
					return;
				}
				memset(Temp, 0, sizeof(Temp) - 1);
			}
			itemCnt++;
		}

		if (itemCnt == NumOfTag) return;
	}

	for (i = 0; i < NumOfTag; i++) 
	{
		free(SaveVal[i]);
	}
	free(SaveVal);
}

BOOL Rs232_IO_Send(char *Cmnds , TCHAR* TermStr) {
	unsigned int i;
	
	for ( i = 0 ; i < strlen(Cmnds) ; i++ ) {
		if (!RS232_Write_Char(Cmnds[i])) {
			return FALSE;
		}
	}
	
	if (TermStr[0] == 0x00) {
		RS232_Write_Char('\0');
	}
	return TRUE;
}
BOOL Rs232_IO_Send_Secs(char* Cmnds) {
	unsigned int i;

	for (i = 0; i < strlen(Cmnds); i++) {
		if (!RS232_Write_Char(Cmnds[i])) {
			return FALSE;
		}
	}
	return TRUE;
}
BOOL Send_Terminate( char* szData, int iCnt)
{
	int i;
	for (i = 0; i < 128; i++) {
		if ( strcmp( szData+i , TermStr_CH[g_RX_CfgNo]) == 0 ) {
			if (strcmp(TermStr_CH[g_RX_CfgNo], "") == 0) {
				if (strcmp(szData + iCnt, TermStr_CH[g_RX_CfgNo]) == 0) {
					return TRUE;
				}
				return FALSE;
			}
			else {
				*(szData + iCnt) = '\0';
			}
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Rs232_IO_Receive_Secs(char* Response, int* ItemCnt) {
	BOOL bResult = FALSE;
	char ch;
	int iCnt = 0, iLen = -1 ,i;
	bResult = RS232_Read_Char(&ch);
	if (bResult == FALSE)	return FALSE;

	switch (ch) {
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x15:
		Response[iCnt++] = ch;
		*ItemCnt = iCnt;
		return TRUE;
	default:
		Response[iCnt++] = ch;
		iLen = ch;
		break;
	}
	if (iLen <= 0) return FALSE;
	for (i = 0; i < iLen; i++) {
		bResult = RS232_Read_Char(&ch);
		if (bResult == FALSE) return FALSE;
		Response[iCnt++] = ch;
	}
	*ItemCnt = iCnt;
	return TRUE;
}

BOOL Rs232_IO_Receive(char *Response, int* ItemCnt ) {
	BOOL bResult = FALSE;
	char ch;
	int i = 0,j=0,k=0, icnt = 0 ;

	
	bResult = RS232_Read_Char(&ch);
	if (bResult == FALSE)	return FALSE;

	do
	{
		if (bResult)
		{
			Response[icnt] = ch;
			if (Send_Terminate(Response, icnt)) {
				break;
			}
			icnt++;
		}

		bResult = RS232_Read_Char(&ch);
	} while( TRUE );

	
	

	
	//if (bResult) {
	//	*Response = ch;
	//	i++;
	//	//fprintf(fpt, "%c\n", ch);
	//	//fclose(fpt);
	//	while (TRUE) {
	//		bResult = RS232_Read_Char(&ch);
	//		if (bResult) {
	//			//fprintf(fpt, "%c\n", ch);
	//			*(Response + i) = ch;
	//			*ItemCnt = i;
	//			i++;
	//			for (j = 0 ; j < 50; j++) {
	//				if (strcmp(Response + j, TermStr_CH[g_RX_CfgNo]) == 0) {
	//					*(Response + j) = '\0';
	//					//fclose(fpt);
	//					return TRUE;
	//				}
	//			}
	//		}
	//		else break;
	//		Sleep(1);
	//	}
	//	return FALSE;
	//}
	//else {
	//	//fclose(fpt);
	//	return FALSE;
	//}
	*ItemCnt = icnt;
	return TRUE;
}


void Response_Part_Thread(void* p) {
	Check_Serial* Check_Res = (Check_Serial*)p;
	int res;
	int i;
	int Match_Serial = giMatch_Serial;
	char SendData[128];
	TCHAR SendData2[128];
	
	memset(SendData, 0x00, 128);
	memset(SendData2, 0x00, 128);
	for (i = 0; i < g_Cfg[Match_Serial].SendCnt; i++) {
		Sleep(g_Cfg[Match_Serial].Send[i].Delay);
		if (Check_Res->Check_Serial == 1)
		{
			if (_tcsnccmp(g_Cfg[Match_Serial].Type, TEXT("BINARY"), lstrlen(TEXT("BINARY"))) == 0) _sntprintf(SendData2, sizeof(SendData), TEXT("%s"), Convert_BINARY_to_ASCII(g_Cfg[Match_Serial].Send[i].Response));
			else if (_tcsnccmp(g_Cfg[Match_Serial].Type, TEXT("ASCII"), lstrlen(TEXT("ASCII"))) == 0) _sntprintf(SendData2, sizeof(SendData), g_Cfg[Match_Serial].Send[i].Response);
			switch (giMode) {
			case 0:		//Common 
				Edit_Print_TX(RXTX_TYPE[Tx], SendData2);
				lstrcat(SendData2, TermStr[g_TX_CfgNo]);
				WideCharToMultiByte(CP_ACP, 0, SendData2, 128, SendData, 128, NULL, NULL);
				//res = RS232_Write_String(SendData, strlen(SendData));

				res = Rs232_IO_Send(SendData, TermStr[g_TX_CfgNo]);


				/*else {
					res = RS232_Write_String(SendData, strlen(SendData));
				}*/
				if (!res) 	return;
				break;
			case 1:
				Edit_Print_TX(RXTX_TYPE[Tx], SendData2);
				WideCharToMultiByte(CP_ACP, 0, SendData2, 128, SendData, 128, NULL, NULL);
				res = Rs232_IO_Send_Secs(SendData);
				if (!res) 	return;
				break;
			}

		}
		else if (Check_Res->Check_Serial == 2)
		{
			_sntprintf(SendData2, sizeof(SendData2), Check_Res->Check_Result);
			Edit_Print_TX(RXTX_TYPE[Tx], SendData2);
			lstrcat(SendData2, TermStr[g_TX_CfgNo]);
			WideCharToMultiByte(CP_ACP, 0, SendData2, 128, SendData, 128, NULL, NULL);
			//
			res = Rs232_IO_Send(SendData,TermStr[g_TX_CfgNo]);
			//else {
			//	//res = RS232_Write_String(SendData, strlen(SendData));
			//}
			//
			if (!res) return;

		}
	}
	//Sleep(g_Cfg[Match_Serial].Send[g_Cfg[Match_Serial].SendIdx].Delay);
	/*if (Check_Res->Check_Serial == 1)
	{
		if (_tcsnccmp(g_Cfg[Match_Serial].Type, TEXT("BINARY"), lstrlen(TEXT("BINARY"))) == 0) _sntprintf(SendData2, sizeof(SendData), TEXT("%s"), Convert_BINARY_to_ASCII(g_Cfg[Match_Serial].Send[g_Cfg[Match_Serial].SendIdx].Response));
		else if (_tcsnccmp(g_Cfg[Match_Serial].Type, TEXT("ASCII"), lstrlen(TEXT("ASCII"))) == 0) _sntprintf(SendData2, sizeof(SendData), g_Cfg[Match_Serial].Send[g_Cfg[Match_Serial].SendIdx].Response);

		Edit_Print_RXTX(RXTX_TYPE[Tx], SendData2);
		lstrcat(SendData2, TermStr[g_TX_CfgNo]);
		WideCharToMultiByte(CP_ACP, 0, SendData2, 50, SendData, 50, NULL, NULL);
		res = RS232_Write_String(SendData, strlen(SendData));
		if (!res) 	return;
		g_Cfg[Match_Serial].SendIdx++;
		if (g_Cfg[Match_Serial].SendIdx == g_Cfg[Match_Serial].SendCnt) g_Cfg[Match_Serial].SendIdx = 0;
	}
	else if (Check_Res->Check_Serial == 2)
	{
		_sntprintf(SendData2, sizeof(SendData2), Check_Res->Check_Result);
		Edit_Print_RXTX(RXTX_TYPE[Tx], SendData2);
		lstrcat(SendData2, TermStr[g_TX_CfgNo]);
		WideCharToMultiByte(CP_ACP, 0, SendData2, 50, SendData, 50, NULL, NULL);
		res = RS232_Write_String(SendData, strlen(SendData));
		if (!res) return;

	}*/
}


void PRINT_LOG_AFTER_REQUEST(void* p) {
	Req_Data* Temp = (Req_Data*)p;
	Check_Serial* ChkSerial;
	TCHAR RcvData2[128];
	char LogBuffer[128];
	TCHAR LogBuffer2[128];

	
	ChkSerial = (Check_Serial *)malloc(sizeof(Check_Serial));
	CHAR_TO_TCHAR(Temp->RcvData, RcvData2, 128);
	CHAR_TO_LOGBUUFER(Temp->RcvData, LogBuffer, Temp->DataLen);
	CHAR_TO_TCHAR(LogBuffer, LogBuffer2, 128);
	if (!Temp->Result)
	{

		if (strlen(Temp->RcvData) > 0)
		{
			if (strncmp(LogBuffer, "[NULL]", 6)) Edit_Print_RX(RXTX_TYPE[Rx], LogBuffer2);
		}

	}
	if (Temp->Result)
	{
		Edit_Print_RX(RXTX_TYPE[Rx], LogBuffer2);
		//i_CheckSerial = SerialCheck(RcvData2, CheckResult);
		memset(ChkSerial->Check_Result, 0, 128);
		EnterCriticalSection(&CS);
		ChkSerial->Check_Serial = SerialCheck(RcvData2, ChkSerial->Check_Result);
		_beginthread(Response_Part_Thread, 0, (void*)ChkSerial);
		LeaveCriticalSection(&CS);
	}
	free(ChkSerial);
}
void Response_Part_TCP_Thread(void* p) {
	Check_Serial* Check_Res = (Check_Serial*)p;
	int res;
	int i;
	int Match_Serial = giMatch_Serial;
	TCHAR SendData2[128];

	memset(SendData2, 0x00, 128);
	for (i = 0; i < g_Cfg[Match_Serial].SendCnt; i++) {
		Sleep(g_Cfg[Match_Serial].Send[i].Delay);
		if (Check_Res->Check_Serial == 1)
		{
			if (_tcsnccmp(g_Cfg[Match_Serial].Type, TEXT("BINARY"), lstrlen(TEXT("BINARY"))) == 0) _sntprintf(SendData2, sizeof(SendData2), TEXT("%s"), Convert_BINARY_to_ASCII(g_Cfg[Match_Serial].Send[i].Response));
			else if (_tcsnccmp(g_Cfg[Match_Serial].Type, TEXT("ASCII"), lstrlen(TEXT("ASCII"))) == 0) _sntprintf(SendData2, sizeof(SendData2), g_Cfg[Match_Serial].Send[i].Response);

			Edit_Print_RX(RXTX_TYPE[Tx], SendData2);								
			lstrcat(SendData2, TermStr[g_TX_CfgNo]);
			res = TCPIP_Write(SendData2, strlen(SendData2));
			if (!res) 	return;
		}
		else if (Check_Res->Check_Serial == 2)
		{
			_sntprintf(SendData2, sizeof(SendData2), Check_Res->Check_Result);
			Edit_Print_RX(RXTX_TYPE[Tx], SendData2);
			lstrcat(SendData2, TermStr[g_TX_CfgNo]);
			res = TCPIP_Write(SendData2, strlen(SendData2));
			if (!res) return;

		}
	}
}

//문자열을 전달받아 AutoResponse항목에 있는 Case인지 확인하여 Check한 결과를 리턴해준다.
//-1 : AutoResponse 항목이 아니거나 '$Num'에 값을 셋팅한다.
// 1 : SendData에 '$'Tag가 없을 경우이다.
// 2 : SendData에 '$'Tag가 존재할 경우이다.
int SerialCheck(TCHAR* RcvData, TCHAR* CheckResult) 
{
	int i,j, TagPos = -1;
	int NumOfTag = -1;
	TCHAR Temp[32];
	TCHAR Temp2[32], Temp2Buf[32];
	TCHAR ReceiveBuf[128], CompBuf[128];
	TCHAR Test[32];

	memset(Temp, 0, sizeof(Temp));
	memset(Temp2, 0, sizeof(Temp2));
	memset(Temp2Buf, 0, sizeof(Temp2Buf));
	memset(ReceiveBuf, 0, sizeof(ReceiveBuf));
	memset(CompBuf, 0, sizeof(CompBuf));
	memset(Test, 0, sizeof(Test));
	_sntprintf(ReceiveBuf, sizeof(ReceiveBuf), RcvData);

	for (i = 0; i < Max_Cfg_No; i++) 
	{
		if (_tcsnccmp(g_Cfg[i].Type, TEXT("ASCII"), lstrlen(TEXT("ASCII"))) == 0) 
		{
			if (_tcscmp(ReceiveBuf, g_Cfg[i].Receive) == 0)
			{
				if (lstrlen(g_Cfg[i].Receive) <= 0) return FALSE;
				giMatch_Serial = i;
				for (j = 0; j <= g_Cfg[i].SendCnt; j++) {
					if (NumOfTag = Find_Tag_Char(g_Cfg[i].Send[j].Response))
					{
						Tagcat(g_Cfg[i].Send[j].Response, CheckResult, NumOfTag);

						return 2;
					}
				}
				return 1;
			}
		}
		else if (_tcsnccmp(g_Cfg[i].Type, TEXT("BINARY"), lstrlen(TEXT("BINARY"))) == 0) 
		{
			if (_tcscmp(ReceiveBuf, Convert_BINARY_to_ASCII(g_Cfg[i].Receive)) == 0)
			{
				if (lstrlen(Convert_BINARY_to_ASCII(g_Cfg[i].Receive)) <= 0) return FALSE;
				giMatch_Serial = i;
				for (j = 0; j <= g_Cfg[i].SendCnt; j++) {
					if (NumOfTag = Find_Tag_Char(g_Cfg[i].Send[j].Response))
					{
						Tagcat(Convert_BINARY_to_ASCII(g_Cfg[i].Send[j].Response), CheckResult, NumOfTag);

						return 2;
					}
				}
				return 1;

			}
		}
	}
	for (i = 0; i < Max_Cfg_No; i++) 
	{
		if (_tcsnccmp(g_Cfg[i].Type, TEXT("ASCII"), lstrlen(TEXT("ASCII"))) == 0) 
		{
			if (_tcsncmp(ReceiveBuf, g_Cfg[i].RX_Comp, lstrlen(g_Cfg[i].RX_Comp)) == 0)
			{
				if (_tcscmp(ReceiveBuf, g_Cfg[i].Receive) == 0) break;
				if (lstrlen(g_Cfg[i].RX_Comp) <= 0) continue;
				if (ReceiveBuf[Find_Tag(g_Cfg[i].Receive)] < 0x30 || ReceiveBuf[Find_Tag(g_Cfg[i].Receive)] > 0x39) continue;
				giMatch_Serial = i;
				NumOfTag = Find_Tag_Char(g_Cfg[i].Receive);
				if (NumOfTag > 0) 
				{
					TagPos = Find_Tag(g_Cfg[i].Receive);
					_sntprintf(Temp, sizeof(Temp), ReceiveBuf + TagPos);
					STR_SEP_CHAR(g_Cfg[i].Receive, '$', ReceiveBuf, Temp2Buf, lstrlen(g_Cfg[i].Receive));
					_sntprintf(Temp2, sizeof(Temp2), TEXT("$%s"), Temp2Buf);
					Tag_Set(Temp2, Temp, NumOfTag);
				}
				else 
				{
					if (lstrlen(g_Cfg[i].Receive) <= 0) return FALSE;
					for (j = 0; j <= g_Cfg[i].SendCnt; j++) {
						if (NumOfTag = Find_Tag_Char(g_Cfg[i].Send[j].Response))
						{
							Tagcat(g_Cfg[i].Send[j].Response, CheckResult, NumOfTag);
						}
						return 2;
					}
				}

				return 1;
			}
		}
		else if (_tcsnccmp(g_Cfg[i].Type, TEXT("BINARY"), lstrlen(TEXT("BINARY"))) == 0) 
		{
			if (_tcsncmp(ReceiveBuf, g_Cfg[i].RX_Comp, lstrlen(g_Cfg[i].RX_Comp)) == 0)
			{
				if (_tcscmp(ReceiveBuf, Convert_BINARY_to_ASCII(g_Cfg[i].Receive)) == 0) break;
				if (lstrlen(g_Cfg[i].RX_Comp) <= 0) continue;
				if (ReceiveBuf[Find_Tag(Convert_BINARY_to_ASCII(g_Cfg[i].Receive))] < 0x30 || ReceiveBuf[Find_Tag(Convert_BINARY_to_ASCII(g_Cfg[i].Receive))] > 0x39) continue;

				giMatch_Serial = i;
				NumOfTag = Find_Tag_Char(g_Cfg[i].Receive);
				if (NumOfTag > 0) 
				{
					TagPos = Find_Tag(Convert_BINARY_to_ASCII(g_Cfg[i].Receive));
					_sntprintf(Temp, sizeof(Temp), ReceiveBuf + TagPos);
					STR_SEP_CHAR(Convert_BINARY_to_ASCII(g_Cfg[i].Receive), '$', ReceiveBuf, Temp2Buf, lstrlen(g_Cfg[i].Receive));
					_sntprintf(Temp2, sizeof(Temp2), TEXT("$%s"), Temp2Buf);
					Tag_Set(Temp2, Temp, NumOfTag);
				}
				else 
				{
					if (lstrlen(g_Cfg[i].Receive) <= 0) return FALSE;
					for (j = 0; j <= g_Cfg[i].SendCnt; j++) {
						if (NumOfTag = Find_Tag_Char(g_Cfg[i].Send[j].Response))
						{
							Tagcat(Convert_BINARY_to_ASCII(g_Cfg[i].Send[j].Response), CheckResult, NumOfTag);
						}
					}
					return 2;
				}

				return 1;
			}
		}
	}
	return -1;
}

//TCP/IP 통신이 Connect되면 실행되는 Thread이다. Message를 수신하여 AutoResponse항목을 검사한후 상황에따른 Message를 송신해준다.
void Auto_Response_TCP(void* p)
{

	int i_CheckSerial = -1;
	BOOL res = -1;
	TCHAR RcvData[128];
	TCHAR CheckResult[128];
	int ReadCnt = -1;
	Check_Serial *ChkSerial;
	TCHAR SendData[128];
	gbEndSignal = FALSE;

	memset(RcvData, 0, sizeof(RcvData));
	memset(CheckResult, 0, sizeof(CheckResult));
	memset(SendData, 0, sizeof(SendData));

	ChkSerial = (Check_Serial *)malloc(sizeof(Check_Serial));
	while (!gbEndSignal)
	{
		while (!gbEndSignal)
		{
			memset(RcvData, 0x00, sizeof(RcvData) - 1);
			memset(CheckResult, 0, sizeof(CheckResult) - 1);
			i_CheckSerial = 0;

			res = TCPIP_Read(RcvData, TermStr[g_RX_CfgNo], 255, 100, &ReadCnt);
			if (!res)
			{
				if (lstrlen(RcvData) > 0)
				{
					Edit_Print_RX(RXTX_TYPE[Rx], RcvData);
				}
			}
			if (res)
			{
				Edit_Print_RX(RXTX_TYPE[Rx], RcvData);
				i_CheckSerial = SerialCheck(RcvData, CheckResult);
				ChkSerial->Check_Serial = SerialCheck(RcvData, ChkSerial->Check_Result);
				_beginthread(Response_Part_TCP_Thread, 0, (void*)ChkSerial);
				break;
			}

			Sleep(1);
		}
		/*if (i_CheckSerial == 1)
		{
			memset(SendData, 0x00, sizeof(RcvData) - 1);

			if (_tcsnccmp(g_Cfg[giMatch_Serial].Type, TEXT("BINARY"), lstrlen(TEXT("BINARY"))) == 0) _sntprintf(SendData, sizeof(SendData), TEXT("%s"), Convert_BINARY_to_ASCII(g_Cfg[giMatch_Serial].Send[g_Cfg[giMatch_Serial].SendIdx].Response));
			else if (_tcsncmp(g_Cfg[giMatch_Serial].Type, TEXT("ASCII"), lstrlen(TEXT("ASCII"))) == 0) _sntprintf(SendData, sizeof(SendData), g_Cfg[giMatch_Serial].Send[g_Cfg[giMatch_Serial].SendIdx].Response);

			Edit_Print_RXTX(RXTX_TYPE[Tx], SendData);
			lstrcat(SendData, TermStr[g_TX_CfgNo]);
			res = TCPIP_Write(SendData, lstrlen(SendData));
			if (!res) 	return;
			g_Cfg[giMatch_Serial].SendIdx++;
			if (g_Cfg[giMatch_Serial].SendIdx == g_Cfg[giMatch_Serial].SendCnt) g_Cfg[giMatch_Serial].SendIdx = 0;
		}
		else if (i_CheckSerial == 2)
		{
			_sntprintf(SendData, sizeof(SendData), CheckResult);
			Edit_Print_RXTX(RXTX_TYPE[Tx], SendData);
			lstrcat(SendData, TermStr[g_TX_CfgNo]);
			res = TCPIP_Write(SendData, lstrlen(SendData));
			if (!res) return;




		}
		*/
	}
	
	free(ChkSerial);
}



//RS232 통신이 Connect되면 실행되는 Thread이다. Message를 수신하여 AutoResponse항목을 검사한후 상황에따른 Message를 송신해준다.
void Auto_Response(void* p) 
{
	int ItemCnt = 0;
	int i_CheckSerial = -1;
	int ReadCnt = -1;
	char TermString[3];
	char Buf = 0;
	char LogBuffer[128];
	BOOL res = FALSE;
	TCHAR RcvData2[128];
	TCHAR LogBuffer2[128];
	TCHAR CheckResult[128];
	TCHAR tcharBuf[128];
	Check_Serial *ChkSerial;
	Req_Data *RD;
	


	gbEndSignal = FALSE;
	
	//

	memset(tcharBuf, 0, 128);
	memset(TermString, 0, sizeof(TermString));
	
	
	ChkSerial = (Check_Serial *)malloc(sizeof(Check_Serial));
	RD = (Req_Data*)malloc(sizeof(Req_Data));
	

	
	while (!gbEndSignal)
	{
			memset(RD->RcvData, 0x00, 128);
			memset(CheckResult, 0, sizeof(CheckResult));
			memset(LogBuffer, 0, 128);
			memset(LogBuffer2, 0, 128);
			memset(RcvData2, 0, sizeof(RcvData2));
			i_CheckSerial = 0;
			ItemCnt = 0;
			RD->DataLen = 0;
			//	WideCharToMultiByte(CP_ACP, 0, TermStr[g_RX_CfgNo], 3, TermString, 3, NULL, NULL);
			//
			//if (lstrlen(TermStr[g_RX_CfgNo])== 0) {
			//	res = Rs232_IO_Receive(RcvData, 255);
			//	CHAR_TO_TCHAR(RcvData, tcharBuf, 50);
			//	/*if (res)*/ Log_Create(_T("TEst"), _T("charTest"), tcharBuf );
			//			
			//}
			//else {
			//	res = RS232_Read_String(RcvData, TermStr_CH[g_RX_CfgNo], 255, 100, &ReadCnt);
			//	CHAR_TO_TCHAR(RcvData, tcharBuf, strlen(RcvData) + 1);
			//}
			switch (giMode) {
			case 0: // Common
				RD->Result = Rs232_IO_Receive(RD->RcvData, &RD->DataLen);
				break;
			case 1: // Secs
				RD->Result = Rs232_IO_Receive_Secs(RD->RcvData, &RD->DataLen);
				break;
			}
			
			
			
			
			//_beginthread(PRINT_LOG_AFTER_REQUEST, 0, (void*)RD);
			CHAR_TO_TCHAR2(RD->RcvData, RD->DataLen, RcvData2);
			CHAR_TO_LOGBUUFER(RD->RcvData, LogBuffer, RD->DataLen);
			CHAR_TO_TCHAR(LogBuffer, LogBuffer2, 128);
			if (!RD->Result)
			{

				if (strlen(RD->RcvData) > 0)
				{
					if (strncmp(LogBuffer, "[NULL]", 6)) Edit_Print_RX(RXTX_TYPE[Rx], LogBuffer2);
				}

			}
			if (RD->Result)
			{
				RS232_Clear_Buffer();
				Edit_Print_RX(RXTX_TYPE[Rx], LogBuffer2);
				//i_CheckSerial = SerialCheck(RcvData2, CheckResult);
				memset(ChkSerial->Check_Result, 0, 128);
				ChkSerial->Check_Serial = SerialCheck(RcvData2, ChkSerial->Check_Result);
				 _beginthread(Response_Part_Thread, 0, (void*)ChkSerial);
			}
			
			//
			//CHAR_TO_TCHAR(RcvData, RcvData2, 50);
			//CHAR_TO_LOGBUUFER(RcvData, LogBuffer, ItemCnt);
			//CHAR_TO_TCHAR(LogBuffer, LogBuffer2, 50);
			//if (!res)
			//{
			//			
			//	if (strlen(RcvData) > 0)
			//	{
			//		if ( strncmp( LogBuffer , "[NULL]" , 6)) Edit_Print_RXTX(RXTX_TYPE[Rx], LogBuffer2);
			//	}
			//	
			//}
			//if (res)
			//{
			//	Edit_Print_RXTX(RXTX_TYPE[Rx], LogBuffer2);
			//	//i_CheckSerial = SerialCheck(RcvData2, CheckResult);
			//	memset(CS->Check_Result, 0, sizeof(CS->Check_Result));
			//	CS->Check_Serial = SerialCheck(RcvData2, CS->Check_Result);
			//	_beginthread(Response_Part_Thread, 0, (void*)CS);
			//	break;
			//}

			Sleep(1);

		
	
		/*if (i_CheckSerial == 1) 
		{
		
			if (_tcsnccmp(g_Cfg[giMatch_Serial].Type, TEXT("BINARY"), lstrlen(TEXT("BINARY"))) == 0) _sntprintf(SendData2, sizeof(SendData), TEXT("%s"), Convert_BINARY_to_ASCII(g_Cfg[giMatch_Serial].Send[g_Cfg[giMatch_Serial].SendIdx].Response));
			else if (_tcsnccmp(g_Cfg[giMatch_Serial].Type, TEXT("ASCII"), lstrlen(TEXT("ASCII"))) == 0) _sntprintf(SendData2, sizeof(SendData), g_Cfg[giMatch_Serial].Send[g_Cfg[giMatch_Serial].SendIdx].Response);

			Edit_Print_RXTX(RXTX_TYPE[Tx], SendData2);
			lstrcat(SendData2, TermStr[g_TX_CfgNo]);
			WideCharToMultiByte(CP_ACP, 0, SendData2, 50, SendData, 50, NULL, NULL);
			res = RS232_Write_String(SendData, strlen(SendData));
			if (!res) 	return;
			g_Cfg[giMatch_Serial].SendIdx++;
			if (g_Cfg[giMatch_Serial].SendIdx == g_Cfg[giMatch_Serial].SendCnt) g_Cfg[giMatch_Serial].SendIdx = 0;
		}
		else if (i_CheckSerial == 2	) 
		{
			_sntprintf(SendData2, sizeof(SendData), CheckResult);
			Edit_Print_RXTX(RXTX_TYPE[Tx], SendData2);
			lstrcat(SendData2, TermStr[g_TX_CfgNo]);
			WideCharToMultiByte(CP_ACP, 0, SendData2, 50, SendData, 50, NULL, NULL);
			res = RS232_Write_String(SendData, strlen(SendData));
			if (!res) return;
			
		}*/

	}
	free(ChkSerial);
	free(RD);
}
BOOL Save_AutoCfg_File() {
	int ItemCnt = -1;
	int i,j;
	JSON_Value *rootValue;
	JSON_Object *rootObject;
	JSON_Array *cmd;
	JSON_Value *leaf_value;
	JSON_Object *leaf_object;
	JSON_Array *sendstr;
	char charBuffer[128];
	TCHAR tcharBuffer[128];

	memset(charBuffer, 0, 128);
	memset(tcharBuffer, 0, 128);


	rootValue = json_value_init_object();             // JSON_Value 생성 및 초기화
	rootObject = json_value_get_object(rootValue);    // JSON_Value에서 JSON_Object를 얻음
	ItemCnt = ListView_GetItemCount(hList_Req);

	json_object_set_value(rootObject, "COMMAND", json_value_init_array());
	cmd = json_object_get_array(rootObject, "COMMAND");

	
	if (ItemCnt < 0) return FALSE;
	for (i = 0; i < ItemCnt; i++) {
		leaf_value = json_value_init_object();
		leaf_object = json_value_get_object(leaf_value);
		TCHAR_TO_CHAR(g_Cfg[i].Type, charBuffer, 128);
		json_object_set_string(leaf_object, "Type", charBuffer);
		TCHAR_TO_CHAR(g_Cfg[i].Receive, charBuffer, 128);
		json_object_set_string(leaf_object, "RECV", charBuffer);

		json_object_set_value(leaf_object, "SEND", json_value_init_array());
		sendstr = json_object_get_array(leaf_object, "SEND");

		for (j = 0; j < g_Cfg[i].SendCnt; j++) {
			if (g_Cfg[i].Send[j].index == 0) continue;
			_sntprintf(tcharBuffer, sizeof(tcharBuffer), TEXT("%d:%s:%d"), g_Cfg[i].Send[j].index, g_Cfg[i].Send[j].Response, g_Cfg[i].Send[j].Delay);
			TCHAR_TO_CHAR(tcharBuffer, charBuffer, 128);
			json_array_append_string(sendstr, charBuffer);

		}
		json_array_append_value(cmd, leaf_value);
		
		
	}
	TCHAR_TO_CHAR(gszFileTitle, charBuffer, 128);
	json_serialize_to_file_pretty(rootValue, charBuffer);
	json_value_free(rootValue);    // JSON_Value에 할당된 동적 메모리 해제

	return TRUE;
}
//개발로그에 연결상태를 써주는 Thread이다.
void Set_Title_Thread(void* p)
{
	HWND MainHwnd = (HWND)p;
	TCHAR Buf[128], Type[32];
	int Cnt[3] = { 0, };

	memset(Buf, 0, sizeof(Buf));
	memset(Type, 0, sizeof(Type));

	while (TRUE)
	{
		if (Client_Connectflag != Not_Use)
		{
			EnableWindow(h_Button_Close, TRUE);
			_sntprintf(Type, sizeof(Type), TEXT("TCP/IP_Client"));
			if (Client_Connectflag == Fail)
			{
				if (Cnt[TCP_Client] == Not_Use)
				{
					_sntprintf(Buf, sizeof(Buf), TEXT("Connecting..."));
					Edit_Print_Log(Type, Buf);
					Logging(Type, Buf);
					Cnt[TCP_Client] = Fail;
				}
				else if (Cnt[TCP_Client] == Connect || Cnt[TCP_Client] == Timeout)	Cnt[TCP_Client] = Not_Use;

			}
			else if (Client_Connectflag == Connect)
			{
				if (Cnt[TCP_Client] != Connect)
				{
					_sntprintf(Buf, sizeof(Buf), TEXT("Connect!!!"));
					_beginthread(Auto_Response_TCP, 0, (void*)sock);
					Edit_Print_Log(Type, Buf);
					Logging(Type, Buf);
					Cnt[TCP_Client] = Connect;
				}
			}
			else if (Client_Connectflag == Timeout)
			{
				if (Cnt[TCP_Client] != Timeout)
				{
					Disable_Button(TRUE);
					EnableWindow(hIPADDRESS_IPAddr, TRUE);

					Client_Connectflag = Not_Use;
					_sntprintf(Buf, sizeof(Buf), TEXT("TimeOut..."));
					Edit_Print_Log(Type, Buf);
					Logging(Type, Buf);
					MessageBox(MainHwnd, TEXT("Connection TimeOut"), TEXT("Button"), MB_OK);
					b_TCPClientConnect = FALSE;
					Cnt[TCP_Client] = Timeout;
				}
			}
			else if (Client_Connectflag == Disconnect)
			{
				if (Cnt[TCP_Client] != Disconnect)
				{
					Disable_Button(TRUE);
					EnableWindow(hIPADDRESS_IPAddr, TRUE);

					Client_Connectflag = Not_Use;
					while (TRUE)
					{
						if (giClientCancel == Disconnect)
						{
							_sntprintf(Buf, sizeof(Buf), TEXT("Disconnect"));
							break;
						}
						else if (giClientCancel == Cancel)
						{
							_sntprintf(Buf, sizeof(Buf), TEXT("Cancel"));
							break;
						}



					}

					gbEndSignal = TRUE;
					giClientCancel = 0;
					Edit_Print_Log(Type, Buf);
					Cnt[TCP_Client] = Not_Use;
				}
			}

		}
		else if (Server_Acceptflag != Not_Use)
		{
			EnableWindow(h_Button_Close, TRUE);
			_sntprintf(Type, sizeof(Type), TEXT("TCP/IP_Server"));
			if (Server_Acceptflag == Waiting)
			{
				if (Cnt[TCP_Server] == Not_Use)
				{
					_sntprintf(Buf, sizeof(Buf), TEXT("Waiting..."));
					Edit_Print_Log(Type, Buf);
					Logging(Type, Buf);
					Cnt[TCP_Server] = Waiting;
				}
				else if (Cnt[TCP_Server] == Connect || Cnt[TCP_Server] == Timeout)	Cnt[TCP_Server] = Not_Use;

			}
			else if (Server_Acceptflag == Connect)
			{
				if (Cnt[TCP_Server] != Connect)
				{
					_sntprintf(Buf, sizeof(Buf), TEXT("Connect!!!"));
					_beginthread(Auto_Response_TCP, 0, (void*)sock);
					Edit_Print_Log(Type, Buf);
					Logging(Type, Buf);
					Cnt[TCP_Server] = Connect;
				}
			}
			else if (Server_Acceptflag == Timeout)
			{
				if (Cnt[TCP_Server] != Timeout)
				{
					Disable_Button(TRUE);
					EnableWindow(hCombo_LocalIp, TRUE);
					Server_Acceptflag = Not_Use;
					_sntprintf(Buf, sizeof(Buf), TEXT("TimeOut..."));
					Edit_Print_Log(Type, Buf);
					Logging(Type, Buf);
					MessageBox(MainHwnd, TEXT("Connection TimeOut"), TEXT("Button"), MB_OK);
					b_TCPClientConnect = FALSE;
					Cnt[TCP_Server] = Timeout;
				}
			}
			else if (Server_Acceptflag == Disconnect)
			{
				if (Cnt[TCP_Server] != Disconnect)
				{
					Disable_Button(TRUE);
					EnableWindow(hCombo_LocalIp, TRUE);
					Server_Acceptflag = Not_Use;
					while (TRUE)
					{
						if (giServerCancel == Disconnect)
						{
							_sntprintf(Buf, sizeof(Buf), TEXT("Disconnect"));
							break;
						}
						else if (giServerCancel == Cancel)
						{
							_sntprintf(Buf, sizeof(Buf), TEXT("Cancel"));
							break;
						}



					}
					giServerCancel = 0;
					Edit_Print_Log(Type, Buf);
					Logging(Type, Buf);
					gbEndSignal = TRUE;
					Cnt[TCP_Server] = Not_Use;
				}
			}
		}
		else if (RS232_Connectflag != Not_Use)
		{
			EnableWindow(h_Button_Close, TRUE);
			_sntprintf(Type, sizeof(Type), TEXT("RS232"));
			if (RS232_Connectflag == Fail)
			{
				
				if (Cnt[RS232] == Not_Use)
				{
					_sntprintf(Buf, sizeof(Buf), TEXT("Connect Fail..."));
					Edit_Print_Log(Type, Buf);
					Logging(Type, Buf);
					Cnt[RS232] = Fail;
				}
				else if (Cnt[RS232] == Connect)	Cnt[RS232] = Not_Use;
			}
			else if (RS232_Connectflag == Connect)
			{
				if (Cnt[RS232] != Connect)
				{
					_sntprintf(Buf, sizeof(Buf), TEXT("Connect!!!"));
					Edit_Print_Log(Type, Buf);
					//Logging(Type, Buf);
					Cnt[RS232] = Connect;
				}

			}
			else if (RS232_Connectflag == Disconnect)
			{
				if (Cnt[RS232] != Disconnect)
				{
					Disable_Button(TRUE);
					RS232_Connectflag = Not_Use;
					gbEndSignal = TRUE;
					_sntprintf(Buf, sizeof(Buf), TEXT("Disconnect"));
					Edit_Print_Log(Type, Buf);
					Logging(Type, Buf);
					Cnt[RS232] = Not_Use;
				}
			}
		}
		else EnableWindow(h_Button_Close, FALSE);


		Sleep(1);


	}
}
//선택된 항목이나 특정한 속성을 가진 아이템을 알아내는 메시지의 결과를 리턴해준다.
int GetNextItem( HWND hWnd , int nItem,  int nFlags)
{
	return SendMessage(hWnd, LVM_GETNEXTITEM, nItem, MAKELPARAM(nFlags, 0));
}
//ListCtrl에서 선택한 항목들 중 가장 첫번째 항목의 위치를 리턴한다.
POSITION2 GetFirstSelectedItemPosition( HWND hWnd) 
{
	return (POSITION2)(DWORD_PTR)(1 + GetNextItem( hWnd , -1, LVIS_SELECTED));
}
//ListCtrl에서 선택한 항목들 중 다음번 항목의 위치를 리턴한다.
int GetNextSelectedItem( HWND hWnd , POSITION2 *pos) 
{
	
	DWORD_PTR nOldPos = (DWORD_PTR)pos - 1;
	pos = (POSITION2)(DWORD_PTR)(1 + GetNextItem(hWnd, (UINT)nOldPos, LVIS_SELECTED));
	return (UINT)nOldPos;
}
//ListCtrl에서 선택된 아이템들의 개수를 리턴해준다.
int GetSelectedCount( HWND hWnd ) 
{
	return (int)SendMessage(hWnd, LVM_GETSELECTEDCOUNT, 0, 0L);
}

//File Config 창에서 선택된 Item의 개수에 따라 Edit Button의 활성화 유/무를 결정하고, Item개수에 따라 Delete Button의 활성화 유/무를 결정한다.
void File_Cfg_Monitor( void* p )
{	
	int itemCnt = -1, SelectItemCnt = -1;
	int itemCnt_Res = -1, SelectItemCnt_Res = -1;
	while (TRUE) {
		itemCnt = ListView_GetItemCount(hList_Req);
		SelectItemCnt = GetSelectedCount(hList_Req);
		itemCnt_Res = ListView_GetItemCount(hList_Response);
		SelectItemCnt_Res = GetSelectedCount(hList_Response);
		if (itemCnt <= 0)
		{
			EnableWindow(hButton_Item_ReqEdit, FALSE);
			EnableWindow(hButton_Item_ReqDel, FALSE);
			EnableMenuItem(hMenu_Sub, ID_AutoCfg_DEL,  MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu_Sub, ID_AutoCfg_EDIT, MF_BYCOMMAND | MF_GRAYED);
			EnableWindow(hButton_Item_ResAdd, FALSE);
			EnableWindow(hButton_Item_ResDel, FALSE);
			EnableWindow(hButton_Item_ResEdit, FALSE);
			
			
		}
		else
		{
			if (SelectItemCnt == 1)
			{
				EnableWindow(hButton_Item_ReqEdit, TRUE);
				EnableWindow(hButton_Item_ReqDel, TRUE);
				EnableMenuItem(hMenu_Sub, ID_AutoCfg_EDIT, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hMenu_Sub, ID_AutoCfg_DEL,  MF_BYCOMMAND | MF_ENABLED);
				EnableWindow(hButton_Item_ResAdd, TRUE);

				if (SelectItemCnt_Res == 1) {
					EnableWindow(hButton_Item_ResDel, TRUE);
					EnableWindow(hButton_Item_ResEdit, TRUE);
				}
				else if (SelectItemCnt_Res > 1) {
					EnableWindow(hButton_Item_ResEdit, FALSE);
				}
				else {
					EnableWindow(hButton_Item_ResDel, FALSE);
					EnableWindow(hButton_Item_ResEdit, FALSE);
				}
					
			}
			else if ( SelectItemCnt > 1 )
			{
				EnableWindow(hButton_Item_ReqEdit, FALSE);
				EnableWindow(hButton_Item_ReqDel, TRUE);
				EnableWindow(hButton_Item_ResAdd, FALSE);
				EnableWindow(hButton_Item_ResDel, FALSE);
				EnableWindow(hButton_Item_ResEdit, FALSE);
				EnableMenuItem(hMenu_Sub, ID_AutoCfg_EDIT, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu_Sub, ID_AutoCfg_DEL,  MF_BYCOMMAND | MF_ENABLED);
				
			}
			else
			{
				EnableWindow(hButton_Item_ReqEdit, FALSE);
				EnableWindow(hButton_Item_ReqDel, FALSE);
				EnableWindow(hButton_Item_ResAdd, FALSE);
				EnableWindow(hButton_Item_ResDel, FALSE);
				EnableWindow(hButton_Item_ResEdit, FALSE);
				EnableMenuItem(hMenu_Sub, ID_AutoCfg_DEL,  MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu_Sub, ID_AutoCfg_EDIT, MF_BYCOMMAND | MF_GRAYED);
			}
			
			
		}
		

		Sleep(1);
	}
}
