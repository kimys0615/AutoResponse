#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <commctrl.h>
#include <stdio.h>
#include <iphlpapi.h>
#include "resource.h"
#include "CimRs232.h"
#include "CimSeqnc.h"
#include "Kutlstr.h"
#include "TCP_Client.h"
#include "TCP_Common.h"
#include "TCP_Server.h"
#include "Auto_Common.h"
#include "Parameter.h"
#include "parson.h"
//#include <afxcmn.h>

//ListCtrl::GetFirstSelectedItemPosition();
//#pragma comment(lib, "legacy_stdio_definitions.lib")
WNDPROC OldEditProc;
HINSTANCE g_hInst;

HWND hWndMain, hTabDlg, hTab;
HWND hCombo_ComPort, hCombo_Baud, hCombo_Parity, hCombo_Stop, hCombo_Data, hButton_Open, hButton_Close, hEdit_RXTX, hEdit_TX_SEND, hRadio_RX, hRadio_TX, hList_Req,hList_Response, hEdit_ID, hEdit_Receive, hEdit_Send, hCombo_Receive, hCombo_Send, hDlg_SerialCom, hDlg_TCPIP, hCombo_LocalIp, hIPADDRESS_IPAddr, hEdit_Port, hEdit_Log, hButton_IDOK, hRadio_TCPClient, hRadio_TCPServer, hRadio_Serial, hButton_Item_ReqDel, hButton_Item_ReqEdit, hEdit_FilePath, hButton_Save, h_Button_Close ,h_Button_File , hDlg_FileConfig , hEdit_RXCfg_etc , hEdit_TXCfg_etc, hButton_RXETC , hButton_TXETC , hEdit_Send_Index , hEdit_Send_Response , hEdit_Send_Delay , hButton_Item_ResAdd , hButton_Item_ResDel, hButton_Item_ResEdit, hRadio_Mode_Common , hRadio_Mode_Secs , hRadio_RXCfg_CR, hRadio_RXCfg_LF, hRadio_RXCfg_CRLF,hRadio_RXCfg_etc , hEdit_RXCfg_etc , hButton_RXETC , hRadio_TXCfg_CR, hRadio_TXCfg_LF, hRadio_TXCfg_CRLF, hRadio_TXCfg_etc, hEdit_TXCfg_etc, hButton_TXETC;
HMENU hMenu_Main, hMenu_Cfg, hMenu_Sub;

TCHAR* RXTX_TYPE[] = { TEXT("RX<< ") , TEXT("TX>> ") };
TCHAR* ComPort_List[] = { TEXT("COM1") , TEXT("COM2") , TEXT("COM3") , TEXT("COM4") , TEXT("COM5") , TEXT("COM6") , TEXT("COM7") , TEXT("COM8") , TEXT("COM9") , TEXT("COM10") , TEXT("COM11") };
TCHAR* BaudRate_List[] = { TEXT("9600") , TEXT("19200") , TEXT("57600") , TEXT("115200") };
TCHAR* Parity_List[] = { TEXT("0") , TEXT("1") , TEXT("2") };
TCHAR* Stop_List[] = { TEXT("1") , TEXT("2") };
TCHAR* Data_List[] = { TEXT("5") , TEXT("6") , TEXT("7") , TEXT("8") };
TCHAR g_ComPortName[50], g_BaudRate[50], g_Parity[50], g_Stop[50], g_Data[50], g_Port[50], g_LocalIP[50], g_IPAddr[50];
TCHAR gszFileTitle[MAX_PATH], glpstrFile[MAX_PATH], gInitDir[MAX_PATH];
TCHAR TempBuffer[256];
TCHAR Save_Serial[Max_Cfg_No][256];
TCHAR TX_History[16][128];
TCHAR RX_History[16][128];
TCHAR Tag[128][256];

CRITICAL_SECTION CS;
SOCKET sock = -1;

TCHAR TermStr[5][16] = { TEXT("\r") , TEXT("\n") , TEXT("\r\n") , TEXT("") };
char TermStr_CH[5][16] = { "\r" , "\n" , "\r\n" , "" };

int Server_Acceptflag = Not_Use;
int Client_Connectflag = Not_Use;
int RS232_Connectflag = Not_Use;
int giMainRadio = 0;
int g_Cfg_Type = 0; // 0 : Serial	1 : Binary
int g_RX_CfgNo = 0, g_TX_CfgNo = 0;		// 	0 : CR		1 : LF		2 : CR+LF		3 : RX_etc		4: TX_etc
int ComPortNum = -1, Baud_Rate = -1, Parity_Bit = -1, Stop_Bit = -1, Data_Bit = -1;
int MaxDataLen = 24;
int Stepcnt = 0, EditStep = 0, EditSendStep = 0;
int g_RX_PrintType, g_TX_PrintType;		// 0 : ASCII		1 : BINARY
int giClientCancel = 0;
int giServerCancel = 0;
int gSendStep = -1;
int giMode = 0;

BOOL gbSaveCheck = FALSE;
BOOL b_RS232Connect = FALSE;
BOOL b_TCPClientConnect = FALSE;
BOOL b_TCPServerConnect = FALSE;
BOOL gbDlgEnd = FALSE;
BOOL gbMenuFlag = FALSE;

Cfg g_Cfg[Max_Cfg_No];

BOOL CALLBACK Dlg_FileConfig(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlg_Main(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
//WinMain 함수이다.
int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance
	  ,LPSTR lpszCmdParam,int nCmdShow)
{

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MainDlg), HWND_DESKTOP, Dlg_Main);

	Data_Logging(FALSE, 1);
	TCPServer_Close();
	TCPClient_Close();
	RS232_Disconnect_Port();

	return 0;
}

void ListView_to_Response_Thread(void* p) {
	int index,Check_Index=-1;
	TCHAR Temp[128];
	LVITEM LI;
	int i;
	LI.mask = LVIF_TEXT;
	while (TRUE) {
		index = GetNextItem(hList_Req, -1, LVNI_ALL | LVNI_SELECTED);
		
		Sleep(1);
		if (index == -1) {
			ListView_DeleteAllItems(hList_Response);
			Check_Index = -1;
			continue;
		}
		gSendStep = index;
		if (Check_Index != index) {
			ListView_DeleteAllItems(hList_Response);

			
			for (i = 0; i < 8; i++) {
				Check_Index = index;
				if (g_Cfg[index].Send[i].index == 0) continue;
				if (lstrlen(g_Cfg[index].Send[i].Response) <= 0) continue;
				_sntprintf(Temp, 128, _T("%d") ,g_Cfg[index].Send[i].index);
				LI.iSubItem = 0;
				LI.iItem = i;
				LI.pszText = Temp;
				SendMessage(hList_Response, LVM_INSERTITEM, 0, (LPARAM)&LI);

				LI.iSubItem = 1;
				LI.iItem = i;
				LI.pszText = g_Cfg[index].Send[i].Response;
				SendMessage(hList_Response, LVM_SETITEM, 0, (LPARAM)&LI);

				_sntprintf(Temp, 128, _T("%d") , g_Cfg[index].Send[i].Delay);
				LI.iSubItem = 2;
				LI.iItem = i;
				LI.pszText = Temp;
				SendMessage(hList_Response, LVM_SETITEM, 0, (LPARAM)&LI);
				
/*
				if (SendCnt > 0) {
					STR_SEP_CHAR(Temp, '|', Temp2, Temp, lstrlen(Temp));
					LI.pszText = Temp2;
					SendMessage(hList_Response, LVM_INSERTITEM, 0, (LPARAM)&LI);
					
				}
				else {
					LI.pszText = Temp;
					SendMessage(hList_Response, LVM_INSERTITEM, 0, (LPARAM)&LI);
				}*/
			}
		}
	}
}

//Edit Control을 Sub Classing하여 사용한다.
LRESULT CALLBACK EditSubProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam)
{
	TCHAR SEND_Edit[50] , SEND_RS232[50];
	char SENDBUF[50];
	int nLen=-1;
	int editLen = -1;

	memset(SEND_Edit, 0, sizeof(SEND_Edit));
	memset(SEND_RS232, 0, sizeof(SEND_RS232));
	memset(SENDBUF, 0, sizeof(SENDBUF));
	switch (iMessage) 
	{
	case WM_KEYDOWN:		if (wParam==VK_RETURN) 
	{
			nLen = GetWindowTextLength ( hEdit_RXTX );
			editLen = GetWindowTextLength(hEdit_TX_SEND);
			if (editLen > 50) {
				MessageBox(hWnd, _T("Maximum TX_Length is 50"), _T("ERROR"), MB_OK);
				return CallWindowProc(OldEditProc, hWnd, iMessage, wParam, lParam);
			}
			GetWindowText ( hEdit_TX_SEND , SEND_Edit , editLen+1);
			_sntprintf ( SEND_RS232 , sizeof(SEND_RS232), SEND_Edit );
			switch ( g_TX_CfgNo ) 
			{
			case 0:
				lstrcat ( SEND_RS232 , TEXT("\r") );
				break;
			case 1:
				lstrcat ( SEND_RS232 , TEXT("\n") );
				break;
			case 2:
				lstrcat ( SEND_RS232 , TEXT("\r\n") );
				break;
			case 4:
				lstrcat(SEND_RS232, TermStr[g_TX_CfgNo]);
				break;
			}
			TCHAR_TO_CHAR(SEND_RS232, SENDBUF, 50);
			//WideCharToMultiByte(CP_ACP, 0, SEND_RS232, 50, SENDBUF, 50, NULL, NULL);

			if ( giMainRadio == 0 )	Rs232_IO_Send(SENDBUF, TermStr[g_TX_CfgNo] );
			else if ( giMainRadio == 1 ) TCPIP_Write (SEND_RS232, lstrlen (SEND_RS232) );
			else if (giMainRadio == 2) TCPIP_Write(SEND_RS232, lstrlen(SEND_RS232));
			
			Edit_Print_TX(RXTX_TYPE[Tx], SEND_Edit);
			SetFocus(hWnd);
			break;
		}
		break;
	}
	return CallWindowProc(OldEditProc,hWnd,iMessage,wParam,lParam);
}

// iniFile Load할 때 ADD/DELETE/EDIT로 인해 추가된 작업이 저장이 되지않았을경우 SAVE / SAVEAS / CANCEL 작업 수행한다.
int CALLBACK Dlg_SaveCheck(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam) 
{		
	
	switch (iMessage) 
	{
	case WM_INITDIALOG:
		hButton_Save = GetDlgItem(hDlg, IDSAVEAS);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		
		case IDSAVE:
			EndDialog(hDlg, IDSAVE);
			break;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			break;
		case IDSAVEAS:
			EndDialog(hDlg, IDSAVEAS);
			break;
		}

		break;
	}

	return FALSE;
}

// AutoResponse 항목을 추가한다.
BOOL CALLBACK Dlg_Item_Req_Add ( HWND hDlg , UINT iMessage , WPARAM wParam , LPARAM lParam ) 
{
	TCHAR Type[128],Receive[128];
	int i=-1;
	

	memset(Type, 0x00, sizeof(Type) - 1);
	memset(Receive, 0x00, sizeof(Receive) - 1);
	
	
	switch ( iMessage ) 
	{
	case WM_INITDIALOG:
		hCombo_Receive = GetDlgItem(hDlg, IDC_COMBO_Receive);
		hCombo_Send = GetDlgItem(hDlg, IDC_COMBO_Send);

		
		for ( i=0 ; i<16 ; i++ ) 
		{
			SendMessage ( hCombo_Receive , CB_ADDSTRING , 0 , (LPARAM) RX_History[i] );
			if ( lstrlen ( RX_History[i] ) == 0 ) break;
		}
		/*for ( i=0 ; i<16 ; i++ ) 
		{
			SendMessage ( hCombo_Send , CB_ADDSTRING , 0 , (LPARAM) TX_History[i] );
			if ( lstrlen ( TX_History[i] ) == 0  ) break;
		}*/

		CheckRadioButton ( hDlg , IDC_RADIO_Serial	, IDC_RADIO_Binary , IDC_RADIO_Serial );
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_RADIO_ASCII:
			g_Cfg_Type = 0;
			break;
		case IDC_RADIO_Binary:
			g_Cfg_Type = 1;
			break;
		case IDC_COMBO_Receive:
			switch (HIWORD(wParam)) 
			{
			case CBN_SELCHANGE:
				i = SendMessage(hCombo_Receive, CB_GETCURSEL, 0, 0);
				SendMessage(hCombo_Receive, CB_GETLBTEXT, i, (LPARAM)Receive);
				break;
			case CBN_EDITCHANGE:
				GetWindowText(hCombo_Receive, Receive, sizeof(Receive));
				break;
			}
			break;
		/*case IDC_COMBO_Send:
			switch (HIWORD(wParam)) 
			{
			case CBN_SELCHANGE:
				i = SendMessage(hCombo_Send, CB_GETCURSEL, 0, 0);
				SendMessage(hCombo_Send, CB_GETLBTEXT, i, (LPARAM)Send);
				break;
			case CBN_EDITCHANGE:
				GetWindowText(hCombo_Send, Send, sizeof(Send));
				break;
			}
			break;*/
		case IDOK:
			GetWindowText(hCombo_Receive, Receive, sizeof(Receive) - 1);
			if (lstrlen(Receive) <= 0 ) return FALSE;
			if (g_Cfg_Type == 0) _sntprintf(Type, sizeof(Type), TEXT("ASCII"));
			else if (g_Cfg_Type == 1) _sntprintf(Type, sizeof(Type), TEXT("BINARY"));

			
			_sntprintf(TempBuffer, sizeof(Save_Serial[Stepcnt]), TEXT("%s|%s"), Type, Receive);

			if (NumberOfvBar(TempBuffer) != 1)
			{
				memset(Save_Serial[Stepcnt], 0, sizeof(TempBuffer));
				MessageBox(hDlg, TEXT("	Too many '|' cannot be added"), TEXT("ADD Fail"), MB_OK);
				return FALSE;
			}
			EndDialog(hDlg,IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog( hDlg , IDCANCEL );
			return TRUE;
		}
		break;
	}

	return FALSE;
}
BOOL CALLBACK Dlg_ItemSendAdd(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	TCHAR Index[8], Response[128], Delay[16];

	memset(Index, 0, sizeof(Index));
	memset(Response, 0, sizeof(Response));
	memset(Delay, 0, sizeof(Delay));

	switch (iMessage) {
	case WM_INITDIALOG:
		hEdit_Send_Index = GetDlgItem(hDlg, IDC_EDIT_Send_Index);
		hEdit_Send_Response = GetDlgItem(hDlg, IDC_EDIT_Send_Response);
		hEdit_Send_Delay = GetDlgItem(hDlg, IDC_EDIT_Send_Delay);

		SetWindowText(hEdit_Send_Delay, 0);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		/*case IDC_EDIT_Send_Index:
			switch (HIWORD(wParam)) {
			case EN_CHANGE:
				GetWindowText(hEdit_Send_Index, str, 128);
				SetWindowText(hWnd, str);
			}
			break;
		case IDC_EDIT_Send_Response:
			break;
		case IDC_EDIT_Send_Delay:
			break;*/

		case IDOK:
			memset(TempBuffer, 0, sizeof(TempBuffer));
			GetWindowText(hEdit_Send_Index, Index, sizeof(Index) - 1);
			GetWindowText(hEdit_Send_Response, Response, sizeof(Response) - 1);
			GetWindowText(hEdit_Send_Delay, Delay, sizeof(Delay) - 1);

			if (lstrlen(Index) <= 0 || lstrlen(Response) <= 0 || lstrlen(Delay) <= 0) {
				MessageBox(hDlg, _T("Please Enter All Data"), _T("Error"), MB_OK);
				return FALSE;
			}

			_sntprintf(TempBuffer, sizeof(TempBuffer), _T("%s|%s|%s"), Index, Response, Delay);
			EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	}

	return FALSE;
}


// Auto Response 항목을 수정한다.
BOOL CALLBACK Dlg_Item_Req_Edit ( HWND hDlg , UINT iMessage , WPARAM wParam , LPARAM lParam  ) 
{
	TCHAR Type[128], Receive[128];
	TCHAR* pszData = NULL;
	TCHAR Temp[50];
	int i = -1, Cfg_Type = -1;
		
	memset ( Type		, 0x00 , sizeof(Type) -1 );
	memset ( Receive	, 0x00 , sizeof(Receive)-1 );
	memset ( Temp		, 0x00 , sizeof(Temp)-1 );
	
	switch ( iMessage ) 
	{
	case WM_INITDIALOG:
		hCombo_Receive = GetDlgItem(hDlg, IDC_COMBO_Receive);
		hCombo_Send = GetDlgItem(hDlg, IDC_COMBO_Send);

		/*_sntprintf(Temp, sizeof(Temp), Save_Serial[EditStep]);
		pszData = _tcstok(Temp, TEXT("|"));
		if (lstrlen(pszData) > 0) _sntprintf(Type, sizeof(Type), pszData);
		pszData = _tcstok ( NULL , TEXT("|") );
		if (lstrlen(pszData) > 0) _sntprintf(Receive, sizeof(Receive), pszData);
		pszData = _tcstok ( NULL , TEXT("|") );
		if (lstrlen(pszData) > 0) _sntprintf(Send, sizeof(Send), pszData);*/
		_sntprintf(Type, sizeof(Type), g_Cfg[EditStep].Type);
		_sntprintf(Receive, sizeof(Receive), g_Cfg[EditStep].Receive);
		for ( i=0 ; i<16 ; i++ ) 
		{
			SendMessage(hCombo_Receive, CB_ADDSTRING, 0, (LPARAM)RX_History[i]);
			if ( lstrlen ( RX_History[i] ) == 0 ) break;
			//SendMessage(hCombo_Send, CB_ADDSTRING, 0, (LPARAM)TX_History[i]);
			//if (lstrlen(TX_History[i]) == 0) break;
		}
		if (_tcscmp(Type, TEXT("ASCII")) == 0)
		{
			CheckRadioButton(hDlg, IDC_RADIO_ASCII, IDC_RADIO_Binary, IDC_RADIO_ASCII);
			g_Cfg_Type = 0;
		}
		else if ( _tcscmp( Type , TEXT("BINARY") ) == 0 ) 
		{
			CheckRadioButton(hDlg, IDC_RADIO_ASCII, IDC_RADIO_Binary, IDC_RADIO_Binary);
			g_Cfg_Type = 1;
		}
		SetWindowText(hCombo_Receive, Receive);
		return TRUE;
	case WM_COMMAND:
		switch ( LOWORD (wParam ) ) 
		{
		case IDC_RADIO_ASCII:
			g_Cfg_Type = 0;
			break;
		case IDC_RADIO_Binary:
			g_Cfg_Type = 1;
			break;
		case IDC_COMBO_Receive:
			switch ( HIWORD ( wParam )) 
			{
			case CBN_SELCHANGE:
				i = SendMessage(hCombo_Receive, CB_GETCURSEL, 0, 0);
				SendMessage(hCombo_Receive, CB_GETLBTEXT, i, (LPARAM)Receive);
				break;
			case CBN_EDITCHANGE:
				GetWindowText(hCombo_Receive, Receive, sizeof(Receive));
				break;
			}
			break;
		/*case IDC_COMBO_Send:
			switch ( HIWORD ( wParam )) 
			{
			case CBN_SELCHANGE:
				i = SendMessage(hCombo_Send, CB_GETCURSEL, 0, 0);
				SendMessage(hCombo_Send, CB_GETLBTEXT, i, (LPARAM)Send);
				break;
			case CBN_EDITCHANGE:
				GetWindowText(hCombo_Send, Send, sizeof(Send));
				break;
			}
			break;*/
		case IDOK:
			memset(TempBuffer, 0, 256);
			if (g_Cfg_Type == 0) _sntprintf(Type, sizeof(Type), TEXT("ASCII"));
			else if (g_Cfg_Type == 1) _sntprintf(Type, sizeof(Type), TEXT("BINARY"));

			GetWindowText(hCombo_Receive, Receive, sizeof(Receive) - 1);
			_sntprintf(TempBuffer, sizeof(TempBuffer), TEXT("%s|%s"), Type, Receive);
			if (NumberOfvBar(TempBuffer) != 1)
			{
				memset(TempBuffer, 0, sizeof(TempBuffer));
				MessageBox(hDlg, TEXT("	Too many '|' cannot be edited"), TEXT("Edit Fail"), MB_OK);
				return FALSE;
			}
			EndDialog(hDlg,IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog( hDlg , IDCANCEL );
			return TRUE;
		}
		break;
	}

	return FALSE;
}

BOOL CALLBACK Dlg_ItemSendEDIT(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	TCHAR Index[8], Response[128], Delay[16];
	TCHAR TextBuffer[128];

	memset(TextBuffer, 0, 128);
	memset(Index, 0, sizeof(Index));
	memset(Response, 0, sizeof(Response));
	memset(Delay, 0, sizeof(Delay));

	switch (iMessage) {
	case WM_INITDIALOG:
		hEdit_Send_Index = GetDlgItem(hDlg, IDC_EDIT_Send_Index);
		hEdit_Send_Response = GetDlgItem(hDlg, IDC_EDIT_Send_Response);
		hEdit_Send_Delay = GetDlgItem(hDlg, IDC_EDIT_Send_Delay);

		_sntprintf(TextBuffer, 128, _T("%d"), g_Cfg[gSendStep].Send[EditSendStep].index);
		SetWindowText(hEdit_Send_Index, TextBuffer);
		SetWindowText(hEdit_Send_Response, g_Cfg[gSendStep].Send[EditSendStep].Response);
		_sntprintf(TextBuffer, 128, _T("%d"), g_Cfg[gSendStep].Send[EditSendStep].Delay);
		SetWindowText(hEdit_Send_Delay, TextBuffer);
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			/*case IDC_EDIT_Send_Index:
				switch (HIWORD(wParam)) {
				case EN_CHANGE:
					GetWindowText(hEdit_Send_Index, str, 128);
					SetWindowText(hWnd, str);
				}
				break;
			case IDC_EDIT_Send_Response:
				break;
			case IDC_EDIT_Send_Delay:
				break;*/


		case IDOK:
			memset(TempBuffer, 0, sizeof(TempBuffer));
			GetWindowText(hEdit_Send_Index, Index, sizeof(Index) - 1);
			GetWindowText(hEdit_Send_Response, Response, sizeof(Response) - 1);
			GetWindowText(hEdit_Send_Delay, Delay, sizeof(Delay) - 1);

			if (lstrlen(Index) <= 0 || lstrlen(Response) <= 0 || lstrlen(Delay) <= 0) {
				MessageBox(hDlg, _T("Please Enter All Data"), _T("Error"), MB_OK);
				return FALSE;
			}

			_sntprintf(TempBuffer, sizeof(TempBuffer), _T("%s|%s|%s"), Index, Response, Delay);
			EndDialog(hDlg, IDOK);
			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	
	}

	return FALSE;
}

//Modaless 형태의 TCPIP 설정 화면을 구성한다.
BOOL CALLBACK Dlg_TCPIP ( HWND hDlg , UINT iMessage , WPARAM wParam , LPARAM lParam ) 
{
	int sel = -1;
	TCHAR HostIP[50], Buf[50], TempBuf[256];
	DWORD size = 0, Test = -1, Test1 = -1, Test2 = -1;
	int ret = -1;
	IP_ADAPTER_INFO *pAdapterInfo = NULL;
	IP_ADAPTER_INFO *pAdapt = NULL;
	PIP_ADDR_STRING pAddrStr;
	TCHAR str[255], str2[256];

	memset(str, 0, sizeof(str));
	memset(str2, 0, sizeof(str2));
	memset(HostIP, 0, sizeof(HostIP));
	memset(Buf, 0, sizeof(Buf));
										
	switch ( iMessage ) 
	{
	case WM_INITDIALOG:
		hCombo_LocalIp = GetDlgItem(hDlg, IDC_COMBO_LocalIP);
		hIPADDRESS_IPAddr = GetDlgItem(hDlg, IDC_IPADDRESS_IPAddr);
		hEdit_Port = GetDlgItem(hDlg, IDC_EDIT_PORT);

		ret = GetAdaptersInfo(NULL, &size);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(size);
		SendDlgItemMessageW(hDlg, IDC_COMBO_LocalIP, CB_INSERTSTRING, 0, (LPARAM)TEXT("0.0.0.0 (Any)"));			// INADDR_ANY
		SendDlgItemMessage(hDlg, IDC_COMBO_LocalIP, CB_INSERTSTRING, 1, (LPARAM)TEXT("127.0.0.1 (Loopback)"));	// INADDR_LOOPBACK
		sel = 2;
		ret = GetAdaptersInfo(pAdapterInfo, &size);
		if (ret) 
		{
			free(pAdapterInfo);
			return FALSE;
		}
		/*for (IP_ADAPTER_INFO *pAdapt = pAdapterInfo; pAdapt; pAdapt = pAdapt->Next) 
		{
			DWORD dwGwIp = inet_addr(pAdapt->GatewayList.IpAddress.String);
			
			for (PIP_ADDR_STRING pAddrStr = &(pAdapt->IpAddressList); pAddrStr; pAddrStr = pAddrStr->Next) 
			{
				Test = inet_addr(pAddrStr->IpAddress.String);
				if (Test != Any && Test != LoopBack) 
				{
					memset(TempBuf, 0, sizeof(TempBuf));
					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pAddrStr->IpAddress.String, strlen(pAddrStr->IpAddress.String), TempBuf, 256);
					SendDlgItemMessage(hDlg, IDC_COMBO_LocalIP, CB_INSERTSTRING, sel, (LPARAM)TempBuf);
				}
				sel++;
			}
			Log_Create
		}*/
		pAdapt = pAdapterInfo;

		while (pAdapt)
		{
			pAddrStr = &(pAdapt->IpAddressList);
			//Log_Create
			while (pAddrStr)
			{
				Test = inet_addr(pAddrStr->IpAddress.String);
				
					
				CHAR_TO_TCHAR(pAddrStr->IpAddress.String, str2, 256);
					//MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pAddrStr->IpAddress.String, strlen(pAddrStr->IpAddress.String), str2, 256);

					_sntprintf(str, sizeof(str), TEXT("Ipaddress.String -> [ %s ] , inet_addr Value -> [ %u ]"), str2, Test);
					
					//Log_Create(TEXT("IPLOG"),str2, str);
				

				if (Test != Any && Test != LoopBack) {
					memset(TempBuf, 0, sizeof(TempBuf));
					CHAR_TO_TCHAR(pAddrStr->IpAddress.String, TempBuf, 256);
					//MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pAddrStr->IpAddress.String, strlen(pAddrStr->IpAddress.String), TempBuf, 256);
					SendDlgItemMessage(hDlg, IDC_COMBO_LocalIP, CB_INSERTSTRING, sel, (LPARAM)TempBuf);	// 
					sel++;
				}


				pAddrStr = pAddrStr->Next;
			}
			pAdapt = pAdapt->Next;
		}
		SendMessage(hCombo_LocalIp, CB_SETCURSEL, 1, 0);
		SendMessage ( hCombo_LocalIp, CB_GETLBTEXT, 1, (LPARAM)g_IPAddr );
		STR_SEP(g_IPAddr, g_IPAddr, Buf, lstrlen(g_IPAddr));
		SetWindowText ( hEdit_Port , TEXT("9000") );
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
			case IDC_EDIT_PORT:
			switch ( HIWORD ( wParam )) 
			{
			case EN_CHANGE:
				GetWindowText(hEdit_Port, g_Port, 128);
				break;
			}
			break;
			case IDC_COMBO_LocalIP:
				switch ( HIWORD ( wParam )) 
				{
				case CBN_SELCHANGE:
					sel = SendMessage(hCombo_LocalIp, CB_GETCURSEL, 0, 0); 
					SendMessage(hCombo_LocalIp, CB_GETLBTEXT, sel, (LPARAM)g_IPAddr);
					STR_SEP(g_IPAddr, g_IPAddr, Buf, lstrlen(g_IPAddr));
					break;
				}			
				break;
					
		}
		break;

	}
	
	return FALSE;
}
// Modaless 형태의 RS_232 설정 화면을 구성한다.
BOOL CALLBACK Dlg_SerialCom ( HWND hDlg , UINT iMessage , WPARAM wParam , LPARAM lParam ) 
{
	int i = -1;

	switch ( iMessage ) 
	{
	case WM_INITDIALOG:
		hCombo_ComPort = GetDlgItem(hDlg, IDC_COMBO_ComPort);
		hCombo_Baud = GetDlgItem(hDlg, IDC_COMBO_Baud);
		hCombo_Parity = GetDlgItem(hDlg, IDC_COMBO_Parity);
		hCombo_Stop = GetDlgItem(hDlg, IDC_COMBO_Stop);
		hCombo_Data = GetDlgItem(hDlg, IDC_COMBO_Data);
 		for ( i=0 ; i<4 ; i++ ) 
		{
			
			SendMessage(hCombo_Baud, CB_ADDSTRING, 0, (LPARAM)BaudRate_List[i]);
			SendMessage(hCombo_Data, CB_ADDSTRING, 0, (LPARAM)Data_List[i]);
		}
		for ( i=0 ; i<3 ; i++ ) 
		{
			SendMessage(hCombo_Parity, CB_ADDSTRING, 0, (LPARAM)Parity_List[i]);
		}
		for ( i=0 ; i<2 ; i++ ) 
		{
			SendMessage(hCombo_Stop, CB_ADDSTRING, 0, (LPARAM)Stop_List[i]);
		}
		for (i = 0; i < 11; i++)
		{
			SendMessage(hCombo_ComPort, CB_ADDSTRING, 0, (LPARAM)ComPort_List[i]);
		}
		SetWindowText ( hCombo_ComPort , TEXT("COM2") );
		ComPortNum = 2;
		SetWindowText ( hCombo_Baud , TEXT("9600") );
		Baud_Rate = 9600 ;
		SetWindowText ( hCombo_Parity , TEXT("0") );
		Parity_Bit = 0 ;
		SetWindowText ( hCombo_Stop , TEXT("1") );
		Stop_Bit = 1 ;
		SetWindowText ( hCombo_Data , TEXT("8") );
		Data_Bit = 8 ;

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_COMBO_ComPort:
			switch ( HIWORD ( wParam )) 
			{
			case CBN_SELCHANGE:
				i = SendMessage ( hCombo_ComPort , CB_GETCURSEL , 0 , 0 );
				SendMessage ( hCombo_ComPort , CB_GETLBTEXT , i , (LPARAM) g_ComPortName );
				_stscanf( g_ComPortName , TEXT("COM%d") , &ComPortNum );
				break;
			case CBN_EDITCHANGE:
				GetWindowText ( hCombo_ComPort , g_ComPortName , sizeof(g_ComPortName) );
				_stscanf( g_ComPortName , TEXT("COM%d"),&ComPortNum );
				break;
			}
			break;
		case IDC_COMBO_Baud:
			switch ( HIWORD ( wParam )) 
			{
			case CBN_SELCHANGE:
				i = SendMessage ( hCombo_Baud , CB_GETCURSEL , 0 , 0 );
				SendMessage ( hCombo_Baud , CB_GETLBTEXT , i , (LPARAM) g_BaudRate );
				Baud_Rate = _tstoi(g_BaudRate);
				break;
			case CBN_EDITCHANGE:
				GetWindowText ( hCombo_Baud , g_BaudRate , sizeof(g_BaudRate) );
				Baud_Rate = _tstoi(g_BaudRate);
				break;
			}
			break;
		case IDC_COMBO_Parity:
			switch ( HIWORD ( wParam )) 
			{
			case CBN_SELCHANGE:
				i = SendMessage ( hCombo_Parity , CB_GETCURSEL , 0 , 0 );
				SendMessage ( hCombo_Parity , CB_GETLBTEXT , i , (LPARAM) g_Parity );
				Parity_Bit = _tstoi(g_Parity);
				break;
			case CBN_EDITCHANGE:
				GetWindowText ( hCombo_Parity , g_Parity , sizeof(g_Parity) );
				Parity_Bit = _tstoi(g_Parity);
				break;
			}
			break;
		case IDC_COMBO_Stop:
			switch ( HIWORD ( wParam )) 
			{
			case CBN_SELCHANGE:
				i = SendMessage ( hCombo_Stop , CB_GETCURSEL , 0 , 0 );
				SendMessage ( hCombo_Stop , CB_GETLBTEXT , i , (LPARAM) g_Stop );
				Stop_Bit = _tstoi(g_Stop);
				break;
			case CBN_EDITCHANGE:
				GetWindowText ( hCombo_Stop , g_Stop , sizeof(g_Stop) );
				Stop_Bit = _tstoi(g_Stop);
				break;
			}
			break;
		case IDC_COMBO_Data:
			switch ( HIWORD ( wParam )) 
			{
			case CBN_SELCHANGE:
				i = SendMessage ( hCombo_Data , CB_GETCURSEL , 0 , 0 );
				SendMessage ( hCombo_Data , CB_GETLBTEXT , i , (LPARAM) g_Data );
				Data_Bit = _tstoi(g_Data);
				break;
			case CBN_EDITCHANGE:
				GetWindowText ( hCombo_Data , g_Data , sizeof(g_Data) );
				Data_Bit = _tstoi(g_Data);
				break;
			}
			break;

		}
		break;
	}
	return FALSE;
}
//Main Dialog 화면을 구성한다.
BOOL CALLBACK Dlg_Main ( HWND hDlg , UINT iMessage , WPARAM wParam , LPARAM lParam ) 
{
	TCHAR ETCTEMP[50],ETCTEMP2[50],ETCTEMP3[16];
	DWORD address=-1;
	RECT prt;
	HWND ParentHwnd = GetParent(hDlg);
	int i,j;
	memset(ETCTEMP, 0, sizeof(ETCTEMP));
	memset(ETCTEMP2, 0, sizeof(ETCTEMP2));
	memset(ETCTEMP3, 0, sizeof(ETCTEMP3));
	switch (iMessage) 
	{

	case WM_INITDIALOG:
		InitializeCriticalSection(&CS);
		hButton_IDOK = GetDlgItem(hDlg, ID_OPEN);
		hEdit_RXTX = GetDlgItem(hDlg, IDC_EDIT_RXTX);
		hEdit_TX_SEND = GetDlgItem(hDlg, IDC_EDIT_TX_SEND);
		hEdit_Log = GetDlgItem(hDlg, IDC_EDIT_Log);
		hRadio_Serial = GetDlgItem(hDlg, IDC_RADIO_SerialCom);
		hRadio_TCPClient = GetDlgItem(hDlg, IDC_RADIO_TCP_Client);
		hRadio_TCPServer = GetDlgItem(hDlg, IDC_RADIO_TCP_Server);
		h_Button_Close = GetDlgItem(hDlg, ID_CLOSE);
		h_Button_File = GetDlgItem(hDlg, IDC_BUTTON_File);
		hMenu_Main = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU_MAIN));
		hDlg_SerialCom = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_SerialCom), hDlg, Dlg_SerialCom);
		hDlg_TCPIP = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_TCPIP), hDlg, Dlg_TCPIP);
		hDlg_FileConfig = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_AuToCfg), hDlg, Dlg_FileConfig);
		hEdit_RXCfg_etc = GetDlgItem(hDlg, IDC_EDIT_RXCfg_etc);
		hEdit_TXCfg_etc = GetDlgItem(hDlg, IDC_EDIT_TXCfg_etc);
		hButton_RXETC = GetDlgItem(hDlg, IDC_BUTTON_RXETC);
		hButton_TXETC = GetDlgItem(hDlg, IDC_BUTTON_TXETC);
		hRadio_Mode_Common = GetDlgItem(hDlg, IDC_RADIO_MODE_COMMON);
		hRadio_Mode_Secs = GetDlgItem(hDlg, IDC_RADIO_MODE_SECS);
		hRadio_RXCfg_CR = GetDlgItem(hDlg, IDC_RADIO_RXCfg_CR);
		hRadio_RXCfg_LF = GetDlgItem(hDlg, IDC_RADIO_RXCfg_LF);
		hRadio_RXCfg_CRLF = GetDlgItem(hDlg, IDC_RADIO_RXCfg_CRLF);
		hRadio_RXCfg_etc = GetDlgItem(hDlg, IDC_RADIO_RXCfg_etc);
		hEdit_RXCfg_etc = GetDlgItem(hDlg, IDC_EDIT_RXCfg_etc);
		hButton_RXETC = GetDlgItem(hDlg, IDC_BUTTON_RXETC);
		hRadio_TXCfg_CR = GetDlgItem(hDlg, IDC_RADIO_TXCfg_CR);
		hRadio_TXCfg_LF = GetDlgItem(hDlg, IDC_RADIO_TXCfg_LF);
		hRadio_TXCfg_CRLF = GetDlgItem(hDlg, IDC_RADIO_TXCfg_CRLF);
		hRadio_TXCfg_etc = GetDlgItem(hDlg, IDC_RADIO_TXCfg_etc);
		hEdit_TXCfg_etc	 = GetDlgItem(hDlg, IDC_EDIT_TXCfg_etc);
		hButton_TXETC = GetDlgItem(hDlg, IDC_BUTTON_TXETC);
		GetWindowRect(hTab, &prt);
		TabCtrl_AdjustRect(hTab, FALSE, &prt);
		ScreenToClient(hWndMain, (LPPOINT)&prt);
		//SetWindowPos(hDlg, HWND_TOP, prt.left, prt.top, 0, 0, SWP_NOSIZE);
		SetWindowPos(hDlg, HWND_TOP, 10, 10, 0, 0, SWP_NOSIZE);
		SendMessage(ParentHwnd, PSM_CANCELTOCLOSE, 0, 0);

		memset(Save_Serial, 0, sizeof(Save_Serial));
		memset(Tag, 0, sizeof(Tag));
		
		SetMenu(hDlg, hMenu_Main);

		CheckDlgButton(hDlg, IDC_RADIO_SerialCom, giMainRadio == 0 ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_TCP_Client, giMainRadio == 1 ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_TCP_Server, giMainRadio == 2 ? BST_CHECKED : BST_UNCHECKED);
		CheckRadioButton(hDlg, IDC_RADIO_RX_ASCII, IDC_RADIO_RX_BINARY, IDC_RADIO_RX_ASCII);
		CheckRadioButton(hDlg, IDC_RADIO_TX_ASCII, IDC_RADIO_TX_BINARY, IDC_RADIO_TX_ASCII);
		CheckRadioButton(hDlg, IDC_RADIO_RXCfg_CR, IDC_RADIO_RXCfg_CRLF, IDC_RADIO_RXCfg_CR);
		CheckRadioButton(hDlg, IDC_RADIO_TXCfg_CR, IDC_RADIO_TXCfg_CRLF, IDC_RADIO_TXCfg_CR);
		CheckRadioButton(hDlg, IDC_RADIO_MODE_COMMON, IDC_RADIO_MODE_SECS, IDC_RADIO_MODE_COMMON);
		SetFocus(hEdit_TX_SEND);
		OldEditProc = (WNDPROC)SetWindowLongPtr(hEdit_TX_SEND, GWLP_WNDPROC, (LONG_PTR)EditSubProc);

		SetWindowPos(hDlg_SerialCom, NULL, 30, 60, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		SetWindowPos(hDlg_TCPIP, NULL, 30, 60, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		ShowWindow(hDlg_TCPIP, SW_HIDE);
		ShowWindow(hDlg_SerialCom, SW_SHOW);
		_beginthread(Set_Title_Thread, 0, (void*)ParentHwnd);
		_beginthread(ListView_to_Response_Thread, 0, NULL);
		return TRUE;
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_CLOSE:
			//------------------------------------------------------
			EndDialog(hDlg, 0);
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_TEST_AUTORESPONSECONFIG:
		case IDC_BUTTON_File:


			GetWindowRect(hDlg, &prt);
			SetWindowPos(hDlg_FileConfig, HWND_TOP, prt.left, prt.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

			ShowWindow(hDlg_FileConfig, SW_SHOW);

			//
			//
			break;

		case IDC_BUTTON_RXETC:
			GetWindowText(hEdit_RXCfg_etc, ETCTEMP, 50);
			for (i = 0; i < lstrlen(ETCTEMP); i++) {
				if (ETCTEMP[i] == ',') continue;
				if ((ETCTEMP[i] < 47) || (ETCTEMP[i] > 57)) {
					MessageBox(hDlg, _T("Please Input Integer\nex)13,10"), _T("ERROR"), MB_OK);
					return FALSE;
				}

			}
			if (lstrlen(ETCTEMP) > 0)
			{
				int charcnt = NumOfCHAR(',', ETCTEMP);
				if (charcnt > 0) {
					for (i = 0; i < charcnt + 1; i++) {
						STR_SEP_CHAR(ETCTEMP, ',', ETCTEMP2, ETCTEMP, 50);
						j = _tstoi(ETCTEMP2);
						_sntprintf(ETCTEMP2, sizeof(ETCTEMP2), _T("%c"), j);
						lstrcat(ETCTEMP3, ETCTEMP2);
					}
				}
				else
				{
					j = _tstoi(ETCTEMP);
					_sntprintf(ETCTEMP2, sizeof(ETCTEMP2), _T("%c"), j);
					lstrcat(ETCTEMP3, ETCTEMP2);
				}

				lstrcpy(TermStr[3], ETCTEMP3);
				TCHAR_TO_CHAR(TermStr[3], TermStr_CH[3], 16);
			}
			break;
		case IDC_BUTTON_TXETC:
			GetWindowText(hEdit_TXCfg_etc, ETCTEMP, 50);
			for (i = 0; i < lstrlen(ETCTEMP); i++) {
				if (ETCTEMP[i] == ',') continue;
				if ((ETCTEMP[i] < 47) || (ETCTEMP[i] > 57)) {
					MessageBox(hDlg, _T("Please Input Integer\nex)13,10"), _T("ERROR"), MB_OK);
					return FALSE;
				}
			}
		
			if (lstrlen(ETCTEMP) > 0)
			{
				int charcnt = NumOfCHAR(',', ETCTEMP);
				if (charcnt > 0) {
					for (i = 0; i < charcnt+1 ; i++) {
						STR_SEP_CHAR(ETCTEMP, ',', ETCTEMP2, ETCTEMP, 50);
						j = _tstoi(ETCTEMP2);
						_sntprintf(ETCTEMP2, sizeof(ETCTEMP2), _T("%c"), j);
						lstrcat(ETCTEMP3, ETCTEMP2);
					}
				}
				else
				{
					j = _tstoi(ETCTEMP);
					_sntprintf(ETCTEMP2, sizeof(ETCTEMP2), _T("%c"), j);
					lstrcat(ETCTEMP3, ETCTEMP2);
				}

				lstrcpy(TermStr[4], ETCTEMP3);
				TCHAR_TO_CHAR(TermStr[4], TermStr_CH[4], 16);
			}
			break;
		case IDC_RADIO_MODE_COMMON:
			giMode = 0;
			break;
		case IDC_RADIO_MODE_SECS:
			giMode = 1;
			break;
		case IDC_RADIO_SerialCom:
			giMainRadio = 0;
			ShowWindow(hDlg_TCPIP, SW_HIDE);
			ShowWindow(hDlg_SerialCom, SW_SHOW);
			break;
		case IDC_RADIO_TCP_Client:
			giMainRadio = 1;
			EnableWindow(hCombo_LocalIp, giMainRadio == 2 ? TRUE : FALSE);
			EnableWindow(hIPADDRESS_IPAddr, giMainRadio == 1 ? TRUE : FALSE);

			ShowWindow(hDlg_SerialCom, SW_HIDE);
			ShowWindow(hDlg_TCPIP, SW_SHOW);
			break;
		case IDC_RADIO_TCP_Server:
			giMainRadio = 2;
			EnableWindow(hCombo_LocalIp, giMainRadio == 2 ? TRUE : FALSE);
			EnableWindow(hIPADDRESS_IPAddr, giMainRadio == 1 ? TRUE : FALSE);

			ShowWindow(hDlg_SerialCom, SW_HIDE);
			ShowWindow(hDlg_TCPIP, SW_SHOW);
			break;
		case IDC_RADIO_RX_ASCII:
			g_RX_PrintType = 0;
			break;
		case IDC_RADIO_RX_BINARY:
			g_RX_PrintType = 1;
			break;
		case IDC_RADIO_TX_ASCII:
			g_TX_PrintType = 0;
			break;
		case IDC_RADIO_TX_BINARY:
			g_TX_PrintType = 1;
			break;
		case ID_BUTTON_CLEAR:
			SetWindowText(hEdit_RXTX, TEXT(""));

			break;
		case IDC_RADIO_RXCfg_CR:
			g_RX_CfgNo = 0;
			EnableWindow(hEdit_RXCfg_etc, FALSE);
			EnableWindow(hButton_RXETC, FALSE);
			break;
		case IDC_RADIO_RXCfg_LF:
			g_RX_CfgNo = 1;
			EnableWindow(hEdit_RXCfg_etc, FALSE);
			EnableWindow(hButton_RXETC, FALSE);
			break;
		case IDC_RADIO_RXCfg_CRLF:
			g_RX_CfgNo = 2;
			EnableWindow(hEdit_RXCfg_etc, FALSE);
			EnableWindow(hButton_RXETC, FALSE);
			break;
		case IDC_RADIO_RXCfg_etc:
			g_RX_CfgNo = 3;
			EnableWindow(hEdit_RXCfg_etc, TRUE);
			EnableWindow(hButton_RXETC, TRUE);
			break;
		case IDC_RADIO_TXCfg_CR:
			g_TX_CfgNo = 0;
			EnableWindow(hEdit_TXCfg_etc, FALSE);
			EnableWindow(hButton_TXETC, FALSE);
			break;
		case IDC_RADIO_TXCfg_LF:
			g_TX_CfgNo = 1;
			EnableWindow(hEdit_TXCfg_etc, FALSE);
			EnableWindow(hButton_TXETC, FALSE);
			break;
		case IDC_RADIO_TXCfg_CRLF:
			g_TX_CfgNo = 2;
			EnableWindow(hEdit_TXCfg_etc, FALSE);
			EnableWindow(hButton_TXETC, FALSE);
			break;
		case IDC_RADIO_TXCfg_etc:
			g_TX_CfgNo = 4;
			EnableWindow(hEdit_TXCfg_etc, TRUE);
			EnableWindow(hButton_TXETC, TRUE);
			break;
		
		case ID_OPEN:
			switch (giMainRadio) 
			{
			case 0:
				if (b_RS232Connect == FALSE) 
				{
					b_RS232Connect = RS232_Connect_Port(ComPortNum, Baud_Rate, Data_Bit, Stop_Bit, Parity_Bit, 0, 0, 0);
					if (b_RS232Connect == TRUE) 
					{
						RS232_Connectflag = Connect;
						_beginthread(Auto_Response, 0, NULL);
						Data_Logging(TRUE, 1);
						Disable_Button(FALSE);
					}
					else RS232_Connectflag = Fail;
				}
				else MessageBox(hDlg, TEXT("Already connected"), TEXT("Button"), MB_OK);
				break;
			case 1:
				if (b_TCPClientConnect == FALSE) 
				{

					SendMessage(hIPADDRESS_IPAddr, IPM_GETADDRESS, 0, (LPARAM)&address);					
					b_TCPClientConnect = TCPClient_Connect(address, _tstoi(g_Port));					
					if (b_TCPClientConnect == TRUE) 
					{
						Disable_Button(FALSE);
						EnableWindow(hIPADDRESS_IPAddr, FALSE);
					}
					else if (b_TCPClientConnect == FALSE) 	TCPClient_Close();

				}
				break;
			case 2:
				if (b_TCPServerConnect == FALSE) 
				{

					address = htonl(_tinet_addr(g_IPAddr));
					b_TCPServerConnect = TCPServer_Connect(address, _tstoi(g_Port));
					if (b_TCPServerConnect == TRUE) 
					{
						Disable_Button(FALSE);
						EnableWindow(hCombo_LocalIp, FALSE);
					}
					else if (b_TCPServerConnect == FALSE) TCPServer_Close();

				}
				break;
			}
			break;
		case ID_CLOSE:
			switch (giMainRadio) 
			{

			case 0:
				if (MessageBox(NULL, TEXT("Do you want to disconnect RS232?"), TEXT("RS232"), MB_OKCANCEL) != IDOK) break;
				RS232_Connectflag = Disconnect;
				Data_Logging(FALSE, 1);
				RS232_Disconnect_Port();
				b_RS232Connect = FALSE;
				break;
			case 1:
				if (Client_Connectflag == Fail) 
				{
					if (MessageBox(hDlg, TEXT("Do you want to cancel the server connection?"), TEXT("TCP/IP Client"), MB_OKCANCEL) != IDOK) break;
					b_TCPClientConnect = FALSE;
					TCPClient_Close();
					Client_Connectflag = Disconnect;
					giClientCancel = Cancel;
				}
				else if (Client_Connectflag == Connect) 
				{
					if (MessageBox(hDlg, TEXT("Do you want to disconnect the server connection?"), TEXT("TCP/IP Client"), MB_OKCANCEL) != IDOK) break;
					b_TCPClientConnect = FALSE;
					TCPClient_Close();
					Client_Connectflag = Disconnect;
					giClientCancel = Disconnect;
				}
				break;
			case 2:
				if (Server_Acceptflag == Waiting) 
				{
					if (MessageBox(hDlg, TEXT("Do you want to cancel the server open?"), TEXT("TCP/IP Server"), MB_OKCANCEL) != IDOK) break;
					Server_Acceptflag = Disconnect;
					b_TCPServerConnect = FALSE;
					TCPServer_Close();
					giServerCancel = Cancel;
					break;
				}
				else if (Server_Acceptflag == Connect) 
				{
					if (MessageBox(hDlg, TEXT("Do you want to close the server?"), TEXT("TCP/IP Server"), MB_OKCANCEL) != IDOK) break;
					Server_Acceptflag = Disconnect;
					b_TCPServerConnect = FALSE;
					TCPServer_Close();
					giServerCancel = Disconnect;
					break;
				}
				break;
			default: break;
			}
			break;
		
		}
		
		break;
	
	}

	
	return FALSE;
}

//File Config 화면을 구성한다.
BOOL CALLBACK Dlg_FileConfig(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	RECT prt;
	LVCOLUMN COL;
	LVITEM LI;
	int i = -1, idx = -1, itemCnt = -1, Dlgres = -1 , Test1 = -1 , j ;
	POSITION2  Test2;
	int cursel[64] = { 0 };
	OPENFILENAME OFN;
	TCHAR* pszData=NULL;
	TCHAR Temp[256], Cfg_Type[128];
	TCHAR Buffer1[128], Buffer2[128];
	char charBuffer[128];
	int Sendidx = -1;
	LPNMHDR hdr;
	LPNMLISTVIEW nlv;


	memset(Temp, 0, sizeof(Temp));
	memset(Cfg_Type, 0, sizeof(Cfg_Type));
	memset(Buffer1, 0, 128);
	memset(Buffer2, 0, 128);
	memset(charBuffer, 0, 128);

	switch (iMessage)
	{
	case WM_NOTIFY:
		hdr = (LPNMHDR)lParam;
		nlv = (LPNMLISTVIEW)lParam;

		if (hdr->hwndFrom == hList_Req || hdr->hwndFrom == hList_Response) {
			switch (hdr->code) {
			case LVN_MARQUEEBEGIN:
				return TRUE;
			}
		}
		break;
	case WM_INITDIALOG:
		hList_Req = GetDlgItem(hDlg, IDC_LIST_Req_Config);
		hList_Response = GetDlgItem(hDlg, IDC_LIST_Res_Config);
		hEdit_FilePath = GetDlgItem(hDlg, IDC_EDIT_FilePath);
		hButton_Item_ReqEdit = GetDlgItem(hDlg, IDC_BUTTON_EDIT);
		hButton_Item_ReqDel = GetDlgItem(hDlg, IDC_BUTTON_DELETE);
		hButton_Item_ResAdd = GetDlgItem(hDlg, IDC_BUTTON_RES_ADD);
		hButton_Item_ResDel = GetDlgItem(hDlg, IDC_BUTTON_RES_DELETE);
		hButton_Item_ResEdit = GetDlgItem(hDlg, IDC_BUTTON_RES_EDIT);

		hMenu_Cfg = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU_AutoCfg));
		SetMenu(hDlg, hMenu_Cfg);
				
		GetWindowRect(hTab, &prt);
		TabCtrl_AdjustRect(hTab, FALSE, &prt);
		ScreenToClient(hWndMain, (LPPOINT)&prt);
		SetWindowPos(hDlg, HWND_TOP, prt.left, prt.top, 0, 0, SWP_NOSIZE);
		SendMessage(GetParent(hDlg), PSM_CANCELTOCLOSE, 0, 0);
		SendMessage(hList_Req, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );
		SendMessage(hList_Response, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
		
		COL.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		COL.fmt = LVCFMT_LEFT;
		COL.cx = 80;
		COL.pszText = TEXT("Type");				// 첫 번째 헤더
		COL.iSubItem = 0;
		SendMessage(hList_Req, LVM_INSERTCOLUMN, 0, (LPARAM)&COL);

		COL.cx = 160;
		COL.pszText = TEXT("Receive");			// 두 번째 헤더
		COL.iSubItem = 1;
		SendMessage(hList_Req, LVM_INSERTCOLUMN, 1, (LPARAM)&COL);

		//COL.cx = 160;
		//COL.pszText = TEXT("Send");			// 두 번째 헤더
		//COL.iSubItem = 2;
		//SendMessage(hList_Req, LVM_INSERTCOLUMN, 2, (LPARAM)&COL);

		COL.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		COL.fmt = LVCFMT_LEFT;
		COL.cx = 60;
		COL.pszText = TEXT("Index");				// 첫 번째 헤더
		COL.iSubItem = 0;
		SendMessage(hList_Response, LVM_INSERTCOLUMN, 0, (LPARAM)&COL);

		COL.cx = 120;
		COL.pszText = TEXT("Send");			// 두 번째 헤더
		COL.iSubItem = 1;
		SendMessage(hList_Response, LVM_INSERTCOLUMN, 1, (LPARAM)&COL);

		COL.cx = 60;
		COL.pszText = TEXT("Delay");			// 세 번째 헤더
		COL.iSubItem = 2;
		SendMessage(hList_Response, LVM_INSERTCOLUMN, 2, (LPARAM)&COL);

		_beginthread(File_Cfg_Monitor, 0, NULL);
		
		return TRUE;
	case WM_INITMENU:
		hMenu_Sub = (HMENU)wParam;
	
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_AutoCfg_SAVE:
		case IDC_BUTTON_SAVE:
			memset(&OFN, 0, sizeof(OPENFILENAME));
			OFN.lStructSize = sizeof(OPENFILENAME);
			OFN.hwndOwner = hDlg;

			OFN.lpstrFilter = TEXT("ini 파일\0*.ini\0");
			OFN.lpstrFile = glpstrFile;
			OFN.nMaxFile = MAX_PATH;
			OFN.lpstrTitle = TEXT("파일을 선택해 주세요");
			OFN.lpstrFileTitle = gszFileTitle;
			OFN.nMaxFileTitle = MAX_PATH;
			OFN.lpstrDefExt = TEXT("abc");
			OFN.Flags = OFN_OVERWRITEPROMPT;

			GetModuleFileName(NULL, gInitDir, MAX_PATH);
			OFN.lpstrInitialDir = gInitDir;

			if (GetSaveFileName(&OFN) != 0)
			{
				//SaveSerialSetting();
				Save_AutoCfg_File();
				if (!SerialInitialize( glpstrFile))
				{
					MessageBox(hDlg, TEXT("Save Error"), TEXT("Button"), MB_OK);
					return FALSE;
				}
				SendMessage(hEdit_FilePath, EM_REPLACESEL, FALSE, (LPARAM)OFN.lpstrFile);
				LI.mask = LVIF_TEXT;
				for (i = 0; i < Max_Cfg_No; i++)
				{
					LI.iItem = i;
					LI.iSubItem = 2;

				}
			}
			gbSaveCheck = FALSE;
			break;
		case ID_AutoCfg_LOAD:
		case IDC_BUTTON_LOAD:
			if (gbSaveCheck == TRUE)
			{
				if (lstrlen(gszFileTitle) <= 0) EnableWindow(hButton_Save, FALSE);
				else EnableWindow(hButton_Save, TRUE);
				if ((Dlgres = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_SaveCheck), hDlg, Dlg_SaveCheck)) == IDSAVE)
				{
					SaveSerialSetting();
					gbSaveCheck = FALSE;
					return TRUE;
				}
				else if (Dlgres == IDSAVEAS)
				{
					memset(&OFN, 0, sizeof(OPENFILENAME));
					OFN.lStructSize = sizeof(OPENFILENAME);
					OFN.hwndOwner = hDlg;
					//OFN.lpstrFilter = TEXT( "모든 파일(*.*)\0*.*\0" );
					OFN.lpstrFilter = TEXT("ini 파일\0*.ini\0");
					OFN.lpstrFile = glpstrFile;
					OFN.nMaxFile = MAX_PATH;
					OFN.lpstrTitle = TEXT("파일을 선택해 주세요");
					OFN.lpstrFileTitle = gszFileTitle;
					OFN.nMaxFileTitle = MAX_PATH;
					OFN.lpstrDefExt = TEXT("abc");
					OFN.Flags = OFN_OVERWRITEPROMPT;
					GetModuleFileName(NULL, gInitDir, MAX_PATH);
					OFN.lpstrInitialDir = gInitDir;

					if (GetSaveFileName(&OFN) != 0)
					{
						SaveSerialSetting();
						if (!SerialInitialize( glpstrFile))
						{
							MessageBox(hDlg, TEXT("Save Error"), TEXT("Button"), MB_OK);
							return FALSE;
						}
						SendMessage(hEdit_FilePath, EM_REPLACESEL, FALSE, (LPARAM)OFN.lpstrFile);
						LI.mask = LVIF_TEXT;
						for (i = 0; i < Max_Cfg_No; i++)
						{
							LI.iItem = i;
							LI.iSubItem = 2;
						}
					}
					gbSaveCheck = FALSE;
					return TRUE;
				}
			}
			memset(glpstrFile, 0, MAX_PATH);
			memset(gszFileTitle, 0, MAX_PATH);
			memset(&OFN, 0, sizeof(OPENFILENAME));
			OFN.lStructSize = sizeof(OPENFILENAME);
			OFN.hwndOwner = hDlg;
			OFN.lpstrFilter = TEXT("ini 파일\0*.ini\0");
			OFN.lpstrFile = glpstrFile;
			OFN.nMaxFile = MAX_PATH;
			OFN.lpstrTitle = TEXT("파일을 선택해 주세요");
			OFN.lpstrFileTitle = gszFileTitle;
			OFN.nMaxFileTitle = MAX_PATH;
			OFN.lpstrDefExt = TEXT("abc");
			OFN.Flags = OFN_OVERWRITEPROMPT;
			GetModuleFileName(NULL, gInitDir, MAX_PATH);
			OFN.lpstrInitialDir = gInitDir;
			if (GetOpenFileName(&OFN) != 0)
			{
				SetWindowText(hEdit_FilePath, _T(""));
				SendMessage(hEdit_FilePath, EM_REPLACESEL, FALSE, (LPARAM)OFN.lpstrFile);
			
				ListView_DeleteAllItems(hList_Req);
				memset(Save_Serial, 0, sizeof(Save_Serial));

				if (!SerialInitialize( glpstrFile))
				{
					MessageBox(hDlg, TEXT("Load Error"), TEXT("Button"), MB_OK);
					return FALSE;
				}
				LI.mask = LVIF_TEXT;
				for (i = 0; i < Max_Cfg_No; i++)
				{
					if (lstrlen(g_Cfg[i].Type) <= 0) break;
					LI.iSubItem = 0;
					LI.iItem = i;
					LI.pszText = g_Cfg[i].Type;
					lstrcpy(Save_Serial[i], LI.pszText);
					lstrcat(Save_Serial[i], TEXT("|"));
					SendMessage(hList_Req, LVM_INSERTITEM, 0, (LPARAM)&LI);
					if (lstrlen(g_Cfg[i].Receive) <= 0) break;
					LI.iSubItem = 1;
					LI.iItem = i;
					LI.pszText = g_Cfg[i].Receive;			
					lstrcat(Save_Serial[i], LI.pszText);
					lstrcat(Save_Serial[i], TEXT("|"));
					SendMessage(hList_Req, LVM_SETITEM, 0, (LPARAM)&LI);


					
					for (j = 0; j <= g_Cfg[i].SendCnt; j++) {
						LI.pszText = g_Cfg[i].Send[j].Response;
						lstrcat(Save_Serial[i], LI.pszText);
						if (j == g_Cfg[i].SendCnt) break;
						lstrcat(Save_Serial[i], TEXT("|"));
						//SendMessage(hList_Req, LVM_SETITEM, 0, (LPARAM)&LI);
					}


				}

				gbSaveCheck = FALSE;
			}
			return TRUE;
			break;
		case ID_AutoCfg_ADD:
		case IDC_BUTTON_REQ_ADD:
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_ItemCfg), hDlg, Dlg_Item_Req_Add) != IDOK)  return FALSE;
			if (lstrlen(TempBuffer) <= 0) {
				MessageBox(hDlg, TEXT("Add Data Error"), TEXT("Error"), MB_OK);
			}
			_sntprintf(Temp, sizeof(Temp), TempBuffer);
			if (lstrlen(Temp) <= 0) return FALSE;

			LI.mask = LVIF_TEXT;
			idx = ListView_GetItemCount(hList_Req);
			Stepcnt = idx;

			LI.iItem = idx;
			LI.iSubItem = 0;
			STR_SEP_CHAR(Temp, '|', Buffer1, Buffer2, 128);
			LI.pszText = Buffer1;
			_sntprintf(g_Cfg[idx].Type, sizeof(g_Cfg[idx].Type) , Buffer1);
			ListView_InsertItem(hList_Req, &LI);

			pszData = Buffer2;
			_sntprintf(g_Cfg[idx].Receive, sizeof(g_Cfg[idx].Receive), Buffer2);
			ListView_SetItemText(hList_Req, idx, 1, pszData);

			g_Cfg[idx].UseYN = TRUE;
	
			Stepcnt++;

			itemCnt = ListView_GetItemCount(hList_Req);
		
			gbSaveCheck = TRUE;
			break;

		case IDC_BUTTON_RES_ADD:
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Send), hDlg, Dlg_ItemSendAdd) != IDOK) return FALSE;
			if (lstrlen(TempBuffer) <= 0) {
				MessageBox(hDlg, TEXT("Add Data Error"), TEXT("Error"), MB_OK);
			}
			
			_sntprintf(Temp, sizeof(Temp), TempBuffer);
			if (lstrlen(Temp) <= 0) return FALSE;

			LI.mask = LVIF_TEXT;
			idx = ListView_GetItemCount(hList_Response);
			g_Cfg[gSendStep].SendCnt = idx+1;
			LI.iItem = idx;
			LI.iSubItem = 0;
			STR_SEP_CHAR(Temp, '|', Buffer1, Buffer2, 128);
			LI.pszText = Buffer1;
			g_Cfg[gSendStep].Send[idx].index = _tstoi(Buffer1);
			ListView_InsertItem(hList_Response, &LI);

			STR_SEP_CHAR(Buffer2, '|', Buffer1, Buffer2, 128);
			LI.iItem = idx;
			LI.iSubItem = 1;
			pszData = Buffer1;
			_sntprintf(g_Cfg[gSendStep].Send[idx].Response, sizeof(g_Cfg[gSendStep].Send[idx].Response), Buffer1);
			ListView_SetItemText(hList_Response, idx, 1, pszData);

			LI.iItem = idx;
			LI.iSubItem = 2;
			pszData = Buffer2;
			g_Cfg[gSendStep].Send[idx].Delay = _tstoi(Buffer2);
			ListView_SetItemText(hList_Response, idx, 2, pszData);
			

			gbSaveCheck = TRUE;
			break;

		case ID_AutoCfg_EDIT:
		case IDC_BUTTON_REQ_EDIT:
			idx = ListView_GetNextItem(hList_Req, -1, LVNI_ALL | LVNI_SELECTED);
			EditStep = idx;
			switch (EditStep) 
			{
			case -1:
				MessageBox(hDlg, TEXT("Please Select Item"), TEXT("Button"), MB_OK);
				break;
			default:
				if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_ItemCfg), hDlg, Dlg_Item_Req_Edit) != IDOK) return FALSE;

				_sntprintf(Temp, sizeof(Temp), TempBuffer);

				LI.mask = LVIF_TEXT;
				LI.iItem = idx;
				LI.iSubItem = 0;
				ListView_SetItem(hDlg, &LI);


				STR_SEP_CHAR(Temp, '|', Buffer1, Buffer2, 128);
				_sntprintf(g_Cfg[idx].Type, sizeof(g_Cfg[idx].Type), Buffer1);
				pszData = g_Cfg[EditStep].Type;
				ListView_SetItemText(hList_Req, idx, 0, pszData);

				_sntprintf(g_Cfg[idx].Receive, sizeof(g_Cfg[idx].Receive), Buffer2);
				pszData = g_Cfg[EditStep].Receive;
				ListView_SetItemText(hList_Req, idx, 1, pszData);

				/*pszData = _tcstok(NULL, TEXT("|"));
				ListView_SetItemText(hList_Req, idx, 2, pszData);*/
				Edit_Print_Log(TEXT("EDIT"), TEXT("EDIT"));
				gbSaveCheck = TRUE;
				break;
			}
			
			break;
		case IDC_BUTTON_RES_EDIT:
			idx = ListView_GetNextItem(hList_Response, -1, LVNI_ALL | LVNI_SELECTED);
			EditSendStep = idx;
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_Send), hDlg, Dlg_ItemSendEDIT) != IDOK) return FALSE;
			if (lstrlen(TempBuffer) <= 0) {
				MessageBox(hDlg, TEXT("Add Data Error"), TEXT("Error"), MB_OK);
				return FALSE;
			}
			
			_sntprintf(Temp, sizeof(Temp), TempBuffer);
			

			LI.mask = LVIF_TEXT;
			LI.iItem = idx;
			LI.iSubItem = 0;
			ListView_SetItem(hDlg, &LI);

			STR_SEP_CHAR(Temp, '|', Buffer1, Buffer2, 128);
			pszData = Buffer1;
			g_Cfg[gSendStep].Send[idx].index = _tstoi(Buffer1);
			ListView_SetItemText(hList_Response, idx, 0, pszData);

			STR_SEP_CHAR(Buffer2, '|', Buffer1, Buffer2, 128);
			pszData = Buffer1;
			_sntprintf(g_Cfg[gSendStep].Send[idx].Response, sizeof(g_Cfg[gSendStep].Send[idx].Response), Buffer1);
			ListView_SetItemText(hList_Response, idx, 1, pszData);

			pszData = Buffer2;
			g_Cfg[gSendStep].Send[idx].Delay = _tstoi(Buffer2);
			ListView_SetItemText(hList_Response, idx, 2, pszData);


			gbSaveCheck = TRUE;
			break;
			
		case ID_AutoCfg_DEL:
		case IDC_BUTTON_REQ_DELETE:
			if (MessageBox(hDlg, TEXT("Do you want to delete the item??"), TEXT("Delete"), MB_OKCANCEL) != IDOK) return FALSE;
			Test2 = GetFirstSelectedItemPosition(hList_Req);
			if (Test2 == NULL)return TRUE;;
			
			while (Test2) {
				idx = GetNextSelectedItem(hList_Req,Test2);
				ListView_DeleteItem(hList_Req, idx);
				g_Cfg[idx].UseYN = FALSE;
				Edit_Print_Log(TEXT("DELETE"), TEXT("DELETE ITEM"));
				SerialDelete(0 ,idx , 1);
				Test2 = GetFirstSelectedItemPosition(hList_Req);

			}
			itemCnt = ListView_GetItemCount(hList_Req);
			
			gbSaveCheck = TRUE;
			break;
		case IDC_BUTTON_RES_DELETE:
			if (MessageBox(hDlg, TEXT("Do you want to delete the item??"), TEXT("Delete"), MB_OKCANCEL) != IDOK) return FALSE;
			Test2 = GetFirstSelectedItemPosition(hList_Response);
			if (Test2 == NULL)return TRUE;;

			while (Test2) {
				idx = GetNextSelectedItem(hList_Response, Test2);
				ListView_DeleteItem(hList_Response, idx);
				Edit_Print_Log(TEXT("DELETE"), TEXT("DELETE ITEM"));
				SerialDelete(gSendStep ,idx , 2);
				Test2 = GetFirstSelectedItemPosition(hList_Response);

			}
			itemCnt = ListView_GetItemCount(hList_Response);
			break;
		case ID_AutoCfg_CLOSE:
		case IDCANCEL:
			ShowWindow(hDlg_FileConfig, SW_HIDE);
			break;
		}
	

	
		
		break;
	}
	
	return FALSE;
}
