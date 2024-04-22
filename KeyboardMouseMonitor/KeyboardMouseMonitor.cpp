///////////////////////////////////////////////////////////////////////////////////////////////////
// KeyboardMouseMonitor.cpp : Defines the entry point for the application.
//
// Displays decoded mouse and keyboard messages.
//
// Has support for saving and restoring the window
// size and position to and from the registry.
//   (HKCU\Software\Alex Sokolek\Keyboard Mouse Monitor\1.0.0.1\WindowPlacement)
//
// Has support for changing the font and color, as well as saving to the registry.
//
// Microsoft Visual Studio Community Edition 64 bit, version 17.9.6
//
// Alex Sokolek, Version 1.0.0.1, Copyright (c) March 26, 2024
// 
// Version 1.0.0.2, April 14, 2024, Version changed for release.
// 
// Version 1.0.0.3, April 21, 2024, Added mouse capture logic.
// 
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "framework.h"
#include "KeyboardMouseMonitor.h"

#include "ApplicationRegistry.h"                // Application Registry Settings class

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
CHOOSEFONT sChooseFont;                         // The ChooseFont structure for the paint procedure
LOGFONT sLogFont;                               // The LogFont structure for the paint procedure
BOOL bChooseFont = false;                       // The result of calling ChooseFont
HFONT hFont = 0, hOldFont = 0;                  // Old and new fonts for the paint procedure

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
const TCHAR* GetMessageText(UINT);
const TCHAR* GetExtendedStatus(LPARAM);
const TCHAR* MouseButtons(WPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_KEYBOARDMOUSEMONITOR, szWindowClass, MAX_LOADSTRING);

	// Allow only one instance of this application to run at the same time
	// If not, find and activate the other window and then exit
	HANDLE h = CreateMutex(NULL, false, _T("{8F40457C-FAAB-40E6-98CC-346C75335191}"));
	if (h == NULL || h == (HANDLE)ERROR_INVALID_HANDLE || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(NULL, _T("ERROR: Unable to create mutex!\n\nAnother instance is probably running."), szTitle, MB_OK | MB_ICONSTOP);
		HWND hWnd = FindWindow(NULL, szTitle);
		if (hWnd) PostMessage(hWnd, WM_ACTIVATE, WA_ACTIVE, NULL);

		return false;
	}

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KEYBOARDMOUSEMONITOR));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KEYBOARDMOUSEMONITOR));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_KEYBOARDMOUSEMONITOR);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}



//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window. RestoreWindowPlacement is
//        called to restore the window to the size and position it last had.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Initialize font structures for the command and paint procedures
	// (Declared global so already initialized to zeroes)
	sChooseFont.lStructSize = sizeof(sChooseFont);
	sChooseFont.hwndOwner = hWnd;
	sChooseFont.lpLogFont = &sLogFont;
	sChooseFont.Flags = CF_INITTOLOGFONTSTRUCT | CF_FIXEDPITCHONLY | CF_EFFECTS;

	// Instantiate ApplicationRegistry class and load/restore window placement
	ApplicationRegistry ar;
	if (ar.Init(hWnd))
	{
		WINDOWPLACEMENT wp;
		if (ar.LoadMemoryBlock(_T("WindowPlacement"), (LPBYTE)&wp, sizeof(wp)))
		{
			if (wp.flags == 0 && wp.showCmd == SW_MINIMIZE) wp.flags = WPF_SETMINPOSITION;
			SetWindowPlacement(hWnd, &wp);
		}
	}

	// Load/restore Choosefont and LogFont structures from the registry
	if (ar.LoadMemoryBlock(_T("ChooseFont"), (LPBYTE)&sChooseFont, sizeof(sChooseFont)))
	{
		if (ar.LoadMemoryBlock(_T("LogFont"), (LPBYTE)&sLogFont, sizeof(sLogFont)))
		{
			sChooseFont.hwndOwner = hWnd;
			sChooseFont.lpLogFont = &sLogFont;
			bChooseFont = true;
		}
	}
	return TRUE;
}



//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static UINT sequence = 0;

	// Application Registry class for saving and loading memory blocks
	ApplicationRegistry ar;
	ar.Init(hWnd);

	// Boolean flags to control mouse move recording
	static bool LButtonDown = false, RButtonDown = false, MButtonDown = false, XButtonDown = false;

	// Message array
	#define MAX_MESSAGES 50
	typedef struct
	{
		UINT sequence;
		UINT message;
		WPARAM wParam;
		LPARAM lParam;
	} mqstruct;
	static mqstruct mq[MAX_MESSAGES];

	// Process the message
	switch (message)
	{
		// Process menu message
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_EDIT_FONT:
			if (ChooseFont(&sChooseFont))
			{
				if (ar.SaveMemoryBlock(_T("ChooseFont"), (LPBYTE)&sChooseFont, sizeof(sChooseFont)))
				{
					bChooseFont = true;
					ar.SaveMemoryBlock(_T("LogFont"), (LPBYTE)&sLogFont, sizeof(sLogFont));
					InvalidateRect(hWnd, NULL, true);
				}
			}
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;

	// Process paint message
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		if (bChooseFont)
		{
			hOldFont = (HFONT)SelectObject(hdc, CreateFontIndirect(&sLogFont));
			SetTextColor(hdc, sChooseFont.rgbColors);
		}

		// Setup to write text starting near the upper left corner of the window
		TEXTMETRIC tm;
		GetTextMetrics(hdc, &tm);
		int x = 10;
		int y = 10;

		// A buffer to format each line of text
		#define MAX_BUFFER_LEN 125
		TCHAR sz[MAX_BUFFER_LEN];

		// Tabstops for each type of line of text
		const int TabStopsKeyboard[] =
		{                            // "Sequence:  99999999"
			 24 * tm.tmMaxCharWidth, // "\tMessage:  AAAAAAAAAAAAAAAA"
			 52 * tm.tmMaxCharWidth, // "\tExt:  "
			 58 * tm.tmMaxCharWidth, // "\tU"            (Up)
			 60 * tm.tmMaxCharWidth, // "\tR"            (Repeat)
			 62 * tm.tmMaxCharWidth, // "\tA"            (Alt)
			 64 * tm.tmMaxCharWidth, // "\tM"            (Menu)
			 66 * tm.tmMaxCharWidth, // "\tD"            (Dialog)
			 68 * tm.tmMaxCharWidth, // "\tX"            (Extended)
			 74 * tm.tmMaxCharWidth, // "\tSC:  0xFFFF"  (Scan Code)
			 90 * tm.tmMaxCharWidth, // "\tRC:  0xFFFF"  (Repeat Count)
			105 * tm.tmMaxCharWidth, // "\twParam:  0xFFFFFFFFFFFFFFFF"
			150 * tm.tmMaxCharWidth  // "\t "
		};
		const int TabStopsMouseMove[] =
		{	                         // "Sequence:  99999999"
			 24 * tm.tmMaxCharWidth, // "\tMessage:  AAAAAAAAAAAAAAAA"
			 55 * tm.tmMaxCharWidth, // "\tPoint:  (+9999,+9999)"
			 79 * tm.tmMaxCharWidth, // "\tVKeyStatus:"
			 92 * tm.tmMaxCharWidth, // "\t2"
			 94 * tm.tmMaxCharWidth, // "\t1"
			 96 * tm.tmMaxCharWidth, // "\tM"
			 98 * tm.tmMaxCharWidth, // "\tC"
			100 * tm.tmMaxCharWidth, // "\tS"
			102 * tm.tmMaxCharWidth, // "\tR"
			104 * tm.tmMaxCharWidth, // "\tL"
			150 * tm.tmMaxCharWidth, // "\t "
		};
		const int TabStopsMouseWheel[] =
		{                            // "Sequence:  99999999"
			 24 * tm.tmMaxCharWidth, // "\tMessage:  AAAAAAAAAAAAAAAA"
			 55 * tm.tmMaxCharWidth, // "\tPoint:  (+9999,+9999)"
			 79 * tm.tmMaxCharWidth, // "\tVKeyStatus:"
			 92 * tm.tmMaxCharWidth, // "\t2"
			 94 * tm.tmMaxCharWidth, // "\t1"
			 96 * tm.tmMaxCharWidth, // "\tM"
			 98 * tm.tmMaxCharWidth, // "\tC"
			100 * tm.tmMaxCharWidth, // "\tS"
			102 * tm.tmMaxCharWidth, // "\tR"
			104 * tm.tmMaxCharWidth, // "\tL"
			109 * tm.tmMaxCharWidth, // "\tWheel:  +"
			118 * tm.tmMaxCharWidth, // "\t9999"
			150 * tm.tmMaxCharWidth  // "\t "
		};
		const int TabStopsMouseClick[] =
		{                            // "Sequence:  99999999"
			 24 * tm.tmMaxCharWidth, // "\tMessage:  AAAAAAAAAAAAAAAA"
			 55 * tm.tmMaxCharWidth, // "\tPoint:  (+9999,+9999)"
			 79 * tm.tmMaxCharWidth, // "\tVKeyStatus:"
			 92 * tm.tmMaxCharWidth, // "\t2"
			 94 * tm.tmMaxCharWidth, // "\t1"
			 96 * tm.tmMaxCharWidth, // "\tM"
			 98 * tm.tmMaxCharWidth, // "\tC"
			100 * tm.tmMaxCharWidth, // "\tS"
			102 * tm.tmMaxCharWidth, // "\tR"
			104 * tm.tmMaxCharWidth, // "\tL"
			150 * tm.tmMaxCharWidth  // "\t "
		};
		#define SIZEOFINT(p) (sizeof(p) / sizeof(int)) // A macro to return the number of tabs in each tabstop array

		// For each entry in the message array
		int cbsz;
		for (int i = 0; i < MAX_MESSAGES; i++)
		{
			if (mq[i].message >= WM_KEYFIRST && mq[i].message <= WM_KEYLAST)
			{
				// Format and display keyboard message
				StringCchPrintf(sz, MAX_BUFFER_LEN,
					_T("Sequence:  %08d\tMessage:  %s\tExt:  %s\twParam:  0x%016llX\t "),
					mq[i].sequence, GetMessageText(mq[i].message), GetExtendedStatus(mq[i].lParam), mq[i].wParam);
				cbsz = lstrlen(sz); // usage: 113 out of 125
				TabbedTextOut(hdc, x, y, sz, cbsz, SIZEOFINT(TabStopsKeyboard), TabStopsKeyboard, 10);
			}
			if (mq[i].message == WM_MOUSEMOVE)
			{
				// Format and display mouse move message
				StringCchPrintf(sz, MAX_BUFFER_LEN,
					_T("Sequence:  %08d\tMessage:  %s\tPoint:  (%+05d,%+05d)\tVKeyStatus:  %s\t "),
					mq[i].sequence, GetMessageText(mq[i].message),
					GET_X_LPARAM(mq[i].lParam), GET_Y_LPARAM(mq[i].lParam),
					MouseButtons(GET_KEYSTATE_WPARAM(mq[i].wParam)));
				cbsz = lstrlen(sz); // usage: 94 out of 125
				TabbedTextOut(hdc, x, y, sz, cbsz, SIZEOFINT(TabStopsMouseMove), TabStopsMouseMove, 10);
			}
			if (mq[i].message == WM_MOUSEWHEEL)
			{
				// Format and display mouse wheel message
				StringCchPrintf(sz, MAX_BUFFER_LEN,
					_T("Sequence:  %08d\tMessage:  %s\tPoint:  (%+05d,%+05d)\tVKeyStatus:  %s\tWheel:  %+05d\t "),
					mq[i].sequence, GetMessageText(mq[i].message),
					GET_X_LPARAM(mq[i].lParam), GET_Y_LPARAM(mq[i].lParam),
					MouseButtons(GET_KEYSTATE_WPARAM(mq[i].wParam)), GET_WHEEL_DELTA_WPARAM(mq[i].wParam));
				cbsz = lstrlen(sz); // usage: 109 out of 125
				TabbedTextOut(hdc, x, y, sz, cbsz, SIZEOFINT(TabStopsMouseWheel), TabStopsMouseWheel, 10);
			}
			if (mq[i].message > WM_MOUSEFIRST && mq[i].message <= WM_MOUSELAST && mq[i].message != WM_MOUSEWHEEL)
			{
				// Format and display mouse click message
				StringCchPrintf(sz, MAX_BUFFER_LEN,
					_T("Sequence:  %08d\tMessage:  %s\tPoint:  (%+05d,%+05d)\tVKeyStatus:  %s\t "),
					mq[i].sequence, GetMessageText(mq[i].message),
					GET_X_LPARAM(mq[i].lParam), GET_Y_LPARAM(mq[i].lParam), MouseButtons(mq[i].wParam));
				cbsz = lstrlen(sz); // usage: 96 out of 125
				TabbedTextOut(hdc, x, y, sz, cbsz, SIZEOFINT(TabStopsMouseClick), TabStopsMouseClick, 10);
			}
			y += tm.tmHeight;
		}

		if (bChooseFont)
		{
			SelectObject(hdc, hOldFont);
		}

		EndPaint(hWnd, &ps);
	}
	break;

	// Process all mouse messages
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		// Process all keyboard commands
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
	{
		// Process mouse capture logic
		if (message == WM_LBUTTONDOWN && !LButtonDown && !RButtonDown && !MButtonDown && !XButtonDown) SetCapture(hWnd);
		if (message == WM_RBUTTONDOWN && !LButtonDown && !RButtonDown && !MButtonDown && !XButtonDown) SetCapture(hWnd);
		if (message == WM_MBUTTONDOWN && !LButtonDown && !RButtonDown && !MButtonDown && !XButtonDown) SetCapture(hWnd);
		if (message == WM_XBUTTONDOWN && !LButtonDown && !RButtonDown && !MButtonDown && !XButtonDown) SetCapture(hWnd);
		if (message == WM_LBUTTONUP                   && !RButtonDown && !MButtonDown && !XButtonDown) ReleaseCapture();
		if (message == WM_RBUTTONUP   && !LButtonDown                 && !MButtonDown && !XButtonDown) ReleaseCapture();
		if (message == WM_MBUTTONUP   && !LButtonDown && !RButtonDown                 && !XButtonDown) ReleaseCapture();
		if (message == WM_XBUTTONUP   && !LButtonDown && !RButtonDown && !MButtonDown                ) ReleaseCapture();

		// Record the state of the mouse buttons
		if (message == WM_LBUTTONDOWN) LButtonDown = true;
		if (message == WM_RBUTTONDOWN) RButtonDown = true;
		if (message == WM_MBUTTONDOWN) MButtonDown = true;
		if (message == WM_XBUTTONDOWN) XButtonDown = true;
		if (message == WM_LBUTTONUP)   LButtonDown = false;
		if (message == WM_RBUTTONUP)   RButtonDown = false;
		if (message == WM_MBUTTONUP)   MButtonDown = false;
		if (message == WM_XBUTTONUP)   XButtonDown = false;

		// Filter mouse move to only record when at least one of the buttons is down
		if (message == WM_MOUSEMOVE && !LButtonDown && !RButtonDown && !MButtonDown && !XButtonDown) break;

		// Shift message queuee array up by one
		for (int i = MAX_MESSAGES - 1; i > 0; i--)
		{
			mq[i].sequence = mq[i - 1].sequence;
			mq[i].message = mq[i - 1].message;
			mq[i].wParam = mq[i - 1].wParam;
			mq[i].lParam = mq[i - 1].lParam;
		}

		// Add the current message to the top of the array
		mq[0].sequence = ++sequence;
		mq[0].message = message;
		mq[0].lParam = lParam;
		mq[0].wParam = wParam;

		// Repaint client window without erasing it - Forces WM_PAINT message
		InvalidateRect(hWnd, NULL, false);
		UpdateWindow(hWnd);
	}
	break;

	// Process the close message sent by the menu message handler
	case WM_DESTROY:
		// Save window placement to the registry
		if (ar.Init(hWnd))
		{
			WINDOWPLACEMENT wp;
			ZeroMemory(&wp, sizeof(wp));
			wp.length = sizeof(wp);
			GetWindowPlacement(hWnd, &wp);
			ar.SaveMemoryBlock(_T("WindowPlacement"), (LPBYTE)&wp, sizeof(wp));
		}

		PostQuitMessage(0);
		break;

		// Let the system handle all other messages
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}



//
//  FUNCTION: GetMessageText(UINT)
//
//  PURPOSE: Decodes message type - Helper to the paint procedure
//

const TCHAR* GetMessageText(UINT message)
{
	switch (message)
	{
	case WM_KEYDOWN:       /* 0x0100 */ return _T("WM_KEYDOWN");
	case WM_KEYUP:         /* 0x0101 */ return _T("WM_KEYUP");
	case WM_CHAR:          /* 0x0102 */ return _T("WM_CHAR");
	case WM_DEADCHAR:      /* 0x0103 */ return _T("WM_DEADCHAR");
	case WM_SYSKEYDOWN:    /* 0x0104 */ return _T("WM_SYSKEYDOWN");
	case WM_SYSKEYUP:      /* 0x0105 */ return _T("WM_SYSKEYUP");
	case WM_SYSCHAR:       /* 0x0106 */ return _T("WM_SYSCHAR");
	case WM_SYSDEADCHAR:   /* 0x0107 */ return _T("WM_SYSDEADCHAR");

	case WM_MOUSEMOVE:     /* 0x0200 */ return _T("WM_MOUSEMOVE");
	case WM_LBUTTONDOWN:   /* 0x0201 */ return _T("WM_LBUTTONDOWN");
	case WM_LBUTTONUP:     /* 0x0202 */ return _T("WM_LBUTTONUP");
	case WM_LBUTTONDBLCLK: /* 0x0203 */ return _T("WM_LBUTTONDBLCLK");
	case WM_RBUTTONDOWN:   /* 0x0204 */ return _T("WM_RBUTTONDOWN");
	case WM_RBUTTONUP:     /* 0x0205 */ return _T("WM_RBUTTONUP");
	case WM_RBUTTONDBLCLK: /* 0x0206 */ return _T("WM_RBUTTONDBLCLK");
	case WM_MBUTTONDOWN:   /* 0x0207 */ return _T("WM_MBUTTONDOWN");
	case WM_MBUTTONUP:     /* 0x0208 */ return _T("WM_MBUTTONUP");
	case WM_MBUTTONDBLCLK: /* 0x0209 */ return _T("WM_MBUTTONDBLCLK");
	case WM_MOUSEWHEEL:    /* 0x020A */ return _T("WM_MOUSEWHEEL");
	case WM_XBUTTONDOWN:   /* 0x020B */ return _T("WM_XBUTTONDOWN");
	case WM_XBUTTONUP:     /* 0x020C */ return _T("WM_XBUTTONUP");
	case WM_XBUTTONDBLCLK: /* 0x020D */ return _T("WM_XBUTTONDBLCLK");

	default:                            return _T("NOT_FOUND");
	}
}



//
//  FUNCTION: GetExtendedStatus(LPARAM)
//
//  PURPOSE: Decodes lParam in a keyboard message - Helper to the paint procedure
//

const TCHAR* GetExtendedStatus(LPARAM lParam)
{
	#define MAXSZ1 42
	static TCHAR sz1[MAXSZ1];
	StringCchCopy(sz1, MAXSZ1, _T(""));

	StringCchCat(sz1, MAXSZ1, HIWORD(lParam) & KF_UP ? _T("U") : _T("_"));
	StringCchCat(sz1, MAXSZ1, HIWORD(lParam) & KF_REPEAT ? _T("\tR") : _T("\t_"));
	StringCchCat(sz1, MAXSZ1, HIWORD(lParam) & KF_ALTDOWN ? _T("\tA") : _T("\t_"));
	StringCchCat(sz1, MAXSZ1, HIWORD(lParam) & KF_MENUMODE ? _T("\tM") : _T("\t_"));
	StringCchCat(sz1, MAXSZ1, HIWORD(lParam) & KF_DLGMODE ? _T("\tD") : _T("\t_"));
	StringCchCat(sz1, MAXSZ1, HIWORD(lParam) & KF_EXTENDED ? _T("\tX") : _T("\t_"));

	#define MAXSZ2 15
	TCHAR sz2[MAXSZ2];
	WORD scanCode = LOBYTE(HIWORD(lParam));
	if (HIWORD(lParam) & KF_EXTENDED) scanCode = MAKEWORD(scanCode, 0xE0);
	StringCchPrintf(sz2, MAXSZ2, _T("\tSC:  0x%04X"), scanCode);
	StringCchCat(sz1, MAXSZ1, sz2);

	#define MAXSZ3 15
	TCHAR sz3[MAXSZ3];
	StringCchPrintf(sz3, MAXSZ3, _T("\tRC:  0x%04X"), LOWORD(lParam));
	StringCchCat(sz1, MAXSZ1, sz3);

	return sz1;
}



//
//  FUNCTION: MouseButtons(WPARAM)
//
//  PURPOSE: Decodes wParam in a WM_MOUSEMOVE message - Helper to the paint procedure
//

const TCHAR* MouseButtons(WPARAM wParam)
{
	#define MAXSZ4 16
	static TCHAR sz[MAXSZ4];
	StringCchCopy(sz, MAXSZ4, _T(""));

	StringCchCat(sz, MAXSZ4, wParam & MK_XBUTTON2 /* 0x0040 */ ? _T("2") : _T("_"));
	StringCchCat(sz, MAXSZ4, wParam & MK_XBUTTON1 /* 0x0020 */ ? _T("\t1") : _T("\t_"));
	StringCchCat(sz, MAXSZ4, wParam & MK_MBUTTON  /* 0x0010 */ ? _T("\tM") : _T("\t_"));
	StringCchCat(sz, MAXSZ4, wParam & MK_CONTROL  /* 0x0008 */ ? _T("\tC") : _T("\t_"));
	StringCchCat(sz, MAXSZ4, wParam & MK_SHIFT    /* 0x0004 */ ? _T("\tS") : _T("\t_"));
	StringCchCat(sz, MAXSZ4, wParam & MK_RBUTTON  /* 0x0002 */ ? _T("\tR") : _T("\t_"));
	StringCchCat(sz, MAXSZ4, wParam & MK_LBUTTON  /* 0x0001 */ ? _T("\tL") : _T("\t_"));

	return sz;
}



// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
