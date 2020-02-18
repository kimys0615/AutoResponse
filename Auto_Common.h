#include <windows.h>
#include <stdio.h>
#include "Parameter.h"


BOOL SerialInitialize( TCHAR* FileNameStr);
int NumberOfvBar(TCHAR* Data);

void STR_SEP(TCHAR* SourceString, TCHAR* ResultString1, TCHAR* ResultString2, int ReadMaxCount);
void STR_SEP_CHAR(TCHAR* SourceString, TCHAR delimiterChar, TCHAR* ResultString1, TCHAR* ResultString2, int ReadMaxCount);
void SaveSerialSetting();
void SerialDelete(int SendStep , int idx , int Type);
void Auto_Response_TCP(void* p);
void Auto_Response(void* p);
void Set_Title_Thread(void* p);
void Edit_Print_Log(TCHAR* Type, TCHAR* RcvData);
void Edit_Print_RX(TCHAR* Type, TCHAR* RcvData);
void Edit_Print_TX(TCHAR* Type, TCHAR* RcvData);

void Disable_Button(BOOL Check);
int GetNextSelectedItem(HWND hWnd, POSITION2 *pos);
POSITION2 GetFirstSelectedItemPosition(HWND hWnd);
int GetNextItem(HWND hWnd, int nItem, int nFlags);
int GetSelectedCount(HWND hWnd);
void File_Cfg_Monitor( void *p);

void Log_Create(TCHAR* FileName, TCHAR* Cfg, TCHAR* Data);
int NumOfCHAR(TCHAR Data, TCHAR* RcvBuf);
TCHAR Find_CHAR(TCHAR* Data);
void TCHAR_TO_CHAR(TCHAR* tchardata, char* chardata, int size);
void CHAR_TO_TCHAR(char* chardata, TCHAR* tchardata, int size);

BOOL Save_AutoCfg_File();
int SerialCheck(TCHAR* RcvData, TCHAR* CheckResult);

BOOL Rs232_IO_Send(char *Cmnds, TCHAR* TermStr);
void Data_Logging(int Control, int Logging_Style);