#pragma once
#include "framework.h"

#define MAX_KEYLEN 100
#define MAX_QUERY_COMPANYNAME_LEN 50
#define MAX_QUERY_PRODUCTNAME_LEN 50
#define MAX_QUERY_PRODUCTVERSION_LEN 50
#define MAX_ERROR_MESSAGE_LEN 100

class ApplicationRegistry
{
private:
	HWND  _hWnd;
	TCHAR _szRegistrySubKey[MAX_KEYLEN];
	BOOL  _isOK;
	UINT  _LastAPICallLine;
	DWORD _LastErrorNumber;
public:
	ApplicationRegistry();
	// There are no memory allocations that are
	// not freed, so no destructor is needed
	BOOL Init(HWND hWnd);
	BOOL LoadMemoryBlock(const TCHAR* pszEntry,       BYTE *lpMemoryBlock, DWORD cbMemoryBlock);
	BOOL SaveMemoryBlock(const TCHAR *pszEntry, const BYTE *lpMemoryBlock, DWORD cbMemoryBlock);
	BOOL isOK() { return _isOK; }
	void DisplayAPIError();
};
