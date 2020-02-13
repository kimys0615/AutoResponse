void err_quit ( char *msg );
void err_display ( char* msg );
DWORD _tinet_addr(const TCHAR *cp);
BOOL TCPIP_Read(TCHAR* RcvData, TCHAR* TermStr, int Max_Read_Count, int TimeOut, int* ReadCount);
BOOL TCPIP_Write(TCHAR *Write_Data_Buffer, int Write_Count);
