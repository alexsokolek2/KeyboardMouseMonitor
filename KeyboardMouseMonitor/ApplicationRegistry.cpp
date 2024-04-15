////////////////////////////////////////////////////////////////////////////////////////////////////
// ApplicationRegistry.cpp : Provides class for saving and restoring arbitrary blocks
//                           of memory to and from the Registry HKCU\\Software branch.
//
//                           This supports saving and restoring things like
//                           the WindowPlacement and LogFont structures.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "framework.h"
#include "ApplicationRegistry.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////////////////////////////////
ApplicationRegistry::ApplicationRegistry()
{
	_hWnd = 0;
	StringCchCopy(_szRegistrySubKey, MAX_KEYLEN, _T(""));
	_isOK = true;
	_LastAPICallLine = 0;
	_LastErrorNumber = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Initializer
// Saves the window handle and builds a registry subkey pointing to the application settings
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ApplicationRegistry::Init(HWND hWnd)
{
	// Save the window handle for later use
	_hWnd = hWnd;

	//------------------------------------------------------------------------------------------------------------
	// Get the fully qualified path name of the currently executing module
	_LastAPICallLine = __LINE__+1;
	TCHAR* pszModuleFileName = new TCHAR[MAX_PATH];
	if (pszModuleFileName == NULL)
	{
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}
	_LastAPICallLine = __LINE__+1;
	BOOL b2 = GetModuleFileName(NULL, pszModuleFileName, MAX_PATH);
	if (!b2)
	{
		delete[] pszModuleFileName;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}
	//------------------------------------------------------------------------------------------------------------
	// Retrieve the size of the VS_VERSION resource in the module
	DWORD dwIgnored;
	_LastAPICallLine = __LINE__+1;
	DWORD cbFileVersionInfo = GetFileVersionInfoSize(pszModuleFileName, &dwIgnored);
	if (cbFileVersionInfo == 0)
	{
		delete[] pszModuleFileName;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}

	// Retrieve the VS_VERSION resource from the module
	BYTE* pFileVersionInfo = new BYTE[cbFileVersionInfo];
	_LastAPICallLine = __LINE__+1;
	BOOL bGetFileVersionInfo = GetFileVersionInfo(pszModuleFileName, 0, cbFileVersionInfo, pFileVersionInfo);
	delete[] pszModuleFileName;
	if (!bGetFileVersionInfo)
	{
		delete[] pFileVersionInfo;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}
	//------------------------------------------------------------------------------------------------------------
	// Find the list of one (or more) languages and code pages
	// I only process the first (and only) one found.
	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;
	UINT cbTranslate;
	_LastAPICallLine = __LINE__+1;
	BOOL bVerQueryValue1 = VerQueryValue
		(pFileVersionInfo, _T("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate);
	if (!bVerQueryValue1)
	{
		delete[] pFileVersionInfo;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}
	//------------------------------------------------------------------------------------------------------------
	// Build the query for the company name
	TCHAR* pszQueryCompanyName = new TCHAR[MAX_QUERY_COMPANYNAME_LEN];
	_LastAPICallLine = __LINE__+1;
	HRESULT hrStringCchPrintf1 = StringCchPrintf
		(pszQueryCompanyName, MAX_QUERY_COMPANYNAME_LEN, _T("\\StringFileInfo\\%04x%04x\\CompanyName"),
		 lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
	if (hrStringCchPrintf1 != S_OK)
	{
		delete[] pFileVersionInfo;
		delete[] pszQueryCompanyName;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}

	// Find the company name
	TCHAR* pszCompanyName;
	UINT cbCompanyName;
	_LastAPICallLine = __LINE__+1;
	BOOL bVerQueryValue2 = VerQueryValue
		(pFileVersionInfo, pszQueryCompanyName, (LPVOID*)&pszCompanyName, &cbCompanyName);
	if (!bVerQueryValue2)
	{
		delete[] pFileVersionInfo;
		delete[] pszQueryCompanyName;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}
	//------------------------------------------------------------------------------------------------------------
	// Build the query for the product pame
	TCHAR* pszQueryProductName = new TCHAR[MAX_QUERY_PRODUCTNAME_LEN];
	_LastAPICallLine = __LINE__+1;
	HRESULT hrStringCchPrintf2 = StringCchPrintf
		(pszQueryProductName, MAX_QUERY_PRODUCTNAME_LEN, _T("\\StringFileInfo\\%04x%04x\\ProductName"),
		 lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
	if (hrStringCchPrintf2 != S_OK)
	{
		delete[] pFileVersionInfo;
		delete[] pszQueryCompanyName;
		delete[] pszQueryProductName;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}

	// Find the product name
	TCHAR* pszProductName;
	UINT cbProductName;
	_LastAPICallLine = __LINE__+1;
	BOOL bVerQueryValue3 = VerQueryValue
		(pFileVersionInfo, pszQueryProductName, (LPVOID*)&pszProductName, &cbProductName);
	if (!bVerQueryValue3)
	{
		delete[] pFileVersionInfo;
		delete[] pszQueryCompanyName;
		delete[] pszQueryProductName;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}
	//------------------------------------------------------------------------------------------------------------
	// Build the query for the product version
	TCHAR* pszQueryProductVersion = new TCHAR[MAX_QUERY_PRODUCTVERSION_LEN];
	_LastAPICallLine = __LINE__+1;
	HRESULT hrStringCchPrintf3 = StringCchPrintf
		(pszQueryProductVersion, MAX_QUERY_PRODUCTVERSION_LEN, _T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
		 lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
	if (hrStringCchPrintf3 != S_OK)
	{
		delete[] pFileVersionInfo;
		delete[] pszQueryCompanyName;
		delete[] pszQueryProductName;
		delete[] pszQueryProductVersion;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}

	// Find the product version
	TCHAR* pszProductVersion;
	UINT cbProductVersion;
	_LastAPICallLine = __LINE__+1;
	BOOL bVerQueryValue4 = VerQueryValue
		(pFileVersionInfo, pszQueryProductVersion, (LPVOID*)&pszProductVersion, &cbProductVersion);
	if (!bVerQueryValue4)
	{
		delete[] pFileVersionInfo;
		delete[] pszQueryCompanyName;
		delete[] pszQueryProductName;
		delete[] pszQueryProductVersion;
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}
	//------------------------------------------------------------------------------------------------------------
	// Build the registry subkey for the memory block entry
	_LastAPICallLine = __LINE__+1;
	HRESULT hrStringCchPrintf4 = StringCchPrintf
		(_szRegistrySubKey, MAX_KEYLEN, _T("Software\\%s\\%s\\%s"),
		 pszCompanyName, pszProductName, pszProductVersion);
	delete[] pFileVersionInfo;
	delete[] pszQueryCompanyName;
	delete[] pszQueryProductName;
	delete[] pszQueryProductVersion;
	if (hrStringCchPrintf4 != S_OK)
	{
		_isOK = false;
		_LastErrorNumber = GetLastError();
		DisplayAPIError();
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Load memory block from the registry
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ApplicationRegistry::LoadMemoryBlock(const TCHAR *pszEntry, BYTE *lpMemoryBlock, DWORD cbMemoryBlock)
{
	// Verify that Init() has been called
	_LastAPICallLine = __LINE__+1;
	if (_hWnd == 0)
	{
		_LastErrorNumber = ERROR_APP_INIT_FAILURE;
		_isOK = false;
		return false;
	}

	// Retrieve the memory block stored size from the
	// registry and verify that the stored size is correct
	HKEY hKey;
	_LastAPICallLine = __LINE__+1;
	LSTATUS ls1 = RegOpenKeyEx(HKEY_CURRENT_USER, _szRegistrySubKey, 0, KEY_READ, &hKey);
	if (ls1 != ERROR_SUCCESS) return false; // No error handling - See next comment
	DWORD cbStoredMemoryBlock = cbMemoryBlock;
	_LastAPICallLine = __LINE__+1;
	LSTATUS ls2 = RegQueryValueEx(hKey, pszEntry, 0, NULL, NULL, &cbStoredMemoryBlock);
	if (ls2 != ERROR_SUCCESS || cbStoredMemoryBlock != cbMemoryBlock)
	{
		// No error handling, as a wrong size of a value results in default behavior
		// This condition will be fixed in the subsequent call to SaveMemoryBlock()
		RegCloseKey(hKey);
		return false;
	}

	// Retrieve the memory block from the registry
	_LastAPICallLine = __LINE__+1;
	LSTATUS ls3 = RegQueryValueEx(hKey, pszEntry, 0, NULL, lpMemoryBlock, &cbMemoryBlock);
	if (ls3 != ERROR_SUCCESS)
	{
		// No error handling, as lack of a value results in default behavior
		// This condition will be fixed in the subsequent call to SaveMemoryBlock()
		RegCloseKey(hKey);
		return false;
	}
	RegCloseKey(hKey);

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Save memory block to the registry
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ApplicationRegistry::SaveMemoryBlock(const TCHAR* pszEntry, const BYTE* lpMemoryBlock, DWORD cbMemoryBlock)
{
	// Verify that Init() has been called
	_LastAPICallLine = __LINE__ + 1;
	if (_hWnd == 0)
	{
		_LastErrorNumber = ERROR_APP_INIT_FAILURE;
		_isOK = false;
		return false;
	}

	// Save the memory block to the registry
	HKEY hKey;
	_LastAPICallLine = __LINE__ + 1;
	LSTATUS ls1 = RegCreateKeyEx(HKEY_CURRENT_USER, _szRegistrySubKey, 0,
		NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
	if (ls1 != ERROR_SUCCESS)
	{
		_isOK = false;
		_LastErrorNumber = ls1;
		DisplayAPIError();
		return false;
	}
	_LastAPICallLine = __LINE__ + 1;
	LSTATUS ls2 = RegSetValueEx(hKey, pszEntry, 0, REG_BINARY, lpMemoryBlock, cbMemoryBlock);
	RegCloseKey(hKey);
	if (ls2 != ERROR_SUCCESS)
	{
		_isOK = false;
		_LastErrorNumber = ls2;
		DisplayAPIError();
		return false;
	}
	else return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Display API Error information
///////////////////////////////////////////////////////////////////////////////////////////////////
void ApplicationRegistry::DisplayAPIError()
{
	TCHAR sz[MAX_ERROR_MESSAGE_LEN];
	StringCchPrintf(sz, MAX_ERROR_MESSAGE_LEN,
		_T("API Error occurred at line %ld error code %ld"), _LastAPICallLine, _LastErrorNumber);
	MessageBox(_hWnd, sz, _T("ApplicationRegistry.cpp"), MB_OK + MB_ICONSTOP);
}
