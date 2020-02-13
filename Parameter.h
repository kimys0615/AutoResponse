#pragma once

#define Max_Cfg_No 64

enum { Not_Use, Fail, Connect, Timeout, Disconnect, Cancel, Waiting };
enum { RS232, TCP_Client, TCP_Server };
enum { Rx, Tx };
enum { Any, LoopBack = 16777343 };

typedef void* POSITION2;
typedef struct  {
	int index;
	TCHAR Response[128];
	int Delay;
}SendFormat;

typedef struct 
{
	TCHAR Type[128];		//	0 : ASCII		1 : BINARY
	TCHAR Receive[128];
	SendFormat Send[8];
	int SendCnt;
	int SendIdx;
	TCHAR RX_Comp[128];

	BOOL UseYN;
}Cfg;

extern Cfg g_Cfg[Max_Cfg_No];

extern HINSTANCE g_hInst;
extern HWND hWndMain, hTabDlg, hTab;
extern HWND hCombo_ComPort, hCombo_Baud, hCombo_Parity, hCombo_Stop, hCombo_Data, hButton_Open, hButton_Close, hEdit_RXTX, hEdit_TX_SEND, hRadio_RX, hRadio_TX, hList_Req, hList_Response, hEdit_ID, hEdit_Receive, hEdit_Send, hCombo_Receive, hCombo_Send, hDlg_SerialCom, hDlg_TCPIP, hCombo_LocalIp, hIPADDRESS_IPAddr, hEdit_Port, hEdit_Log, hButton_IDOK, hRadio_TCPClient, hRadio_TCPServer, hRadio_Serial, hButton_Item_ReqDel, hButton_Item_ReqEdit, hEdit_FilePath, hButton_Save, h_Button_Close, h_Button_File, hDlg_FileConfig, hEdit_RXCfg_etc, hEdit_TXCfg_etc, hButton_RXETC, hButton_TXETC, hEdit_Send_Index, hEdit_Send_Response, hEdit_Send_Delay , hButton_Item_ResAdd , hButton_Item_ResDel , hButton_Item_ResEdit;
extern HMENU hMenu_Main, hMenu_Cfg, hMenu_Sub;
extern TCHAR Save_Serial[Max_Cfg_No][256];
extern TCHAR Tag[128][256];
extern TCHAR TX_History[16][128];
extern TCHAR RX_History[16][128];
extern TCHAR gszFileTitle[MAX_PATH], glpstrFile[MAX_PATH], gInitDir[MAX_PATH];
extern TCHAR* RXTX_TYPE[];

extern SOCKET sock;

extern int Server_Acceptflag ;
extern int Client_Connectflag ;
extern int RS232_Connectflag ;
extern int MaxDataLen ;
extern int g_RX_CfgNo, g_TX_CfgNo;		// 	0 : CR		1 : LF		2 : CR+LF
extern int Stepcnt , EditStep ;
extern int g_RX_PrintType, g_TX_PrintType;		// 0 : ASCII		1 : BINARY
extern int giClientCancel;
extern int giServerCancel;

extern BOOL b_RS232Connect ;
extern BOOL b_TCPClientConnect ;
extern BOOL b_TCPServerConnect ;
extern BOOL gbDlgEnd;
extern BOOL gbMenuFlag;

extern TCHAR TermStr[5][16];
extern char TermStr_CH[5][16];

extern CRITICAL_SECTION	CS;