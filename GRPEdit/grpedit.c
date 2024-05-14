#include <Windows.h>
#include <CommCtrl.h>
#include <winternl.h>

#include "grpedit.h"
#include "mobjnames.h"
#include "resource.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int __wgetmainargs(int *_Argc, wchar_t ***_Argv, wchar_t ***_Env, int _DoWildCard, int *_StartInfo);

LPWSTR SaveFileDialog(HWND hWnd, LPCWSTR title, LPCWSTR filter, LPCWSTR extension) {
	OPENFILENAME o = { 0 };
	WCHAR fbuff[MAX_PATH + 1] = { 0 };
	ZeroMemory(&o, sizeof(o));
	o.lStructSize = sizeof(o);
	o.hwndOwner = hWnd;
	o.nMaxFile = MAX_PATH;
	o.lpstrTitle = title;
	o.lpstrFilter = filter;
	o.nMaxCustFilter = 255;
	o.lpstrFile = fbuff;
	o.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	o.lpstrDefExt = extension;

	if (GetSaveFileName(&o)) {

		LPWSTR fname = (LPWSTR) calloc(wcslen(fbuff) + 1, 2);
		memcpy(fname, fbuff, wcslen(fbuff) * 2);
		return fname;
	}
	return NULL;
}

LPWSTR OpenFileDialog(HWND hWnd, LPCWSTR title, LPCWSTR filter, LPCWSTR extension) {
	OPENFILENAME o = { 0 };
	WCHAR fname[MAX_PATH + 1] = { 0 };
	o.lStructSize = sizeof(o);
	o.hwndOwner = hWnd;
	o.nMaxFile = MAX_PATH;
	o.lpstrTitle = title;
	o.lpstrFilter = filter;
	o.nMaxCustFilter = 255;
	o.lpstrFile = fname;
	o.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	o.lpstrDefExt = extension;
	if (GetOpenFileName(&o)) {
		LPWSTR fname2 = (LPWSTR) calloc(wcslen(fname) + 1, 2);
		memcpy(fname2, fname, wcslen(fname) * 2);
		return fname2;
	}
	return NULL;
}

BOOL __stdcall SetFontProc(HWND hWnd, LPARAM lParam) {
	SendMessage(hWnd, WM_SETFONT, (WPARAM) lParam, TRUE);
	return TRUE;
}

LPCWSTR ResolveObjectName(int id) {
	//try resolve by table
	for (int i = 0; i < sizeof(g_mobjNames) / sizeof(g_mobjNames[0]); i++) {
		int mid = g_mobjNames[i].id;
		if (mid != id) continue;
		return g_mobjNames[i].name;
	}

	//TODO: maybe have some sort of secondary resolve method
	return L"<unknown>";
}

void PopulateObjectCombobox(HWND hWndCombo, grpconf_entry_t *entries, int nEntries, int selIndex) {
	WCHAR bf[64];
	int nIntChars = 1;
	LPCWSTR lpszSpaces = L"         ";

	SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);
	for (int i = 0; i < nEntries; i++) {
		int id = entries[i].objectId;

		//format and print
		if (id >= 0 && id < 10) nIntChars = 1;
		else if (id >= 10 && id < 100) nIntChars = 2;
		else if (id >= 100 && id < 1000) nIntChars = 3;
		wsprintf(bf, L"%d%s%s", id, lpszSpaces + (nIntChars - 1) * 2, ResolveObjectName(id));
		SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) bf);
	}
	SendMessage(hWndCombo, CB_SETCURSEL, selIndex, 0);
}

void SetWindowEnabled(HWND hWnd, BOOL enabled) {
	DWORD style = GetWindowLong(hWnd, GWL_STYLE);
	style = enabled ? (style & ~WS_DISABLED) : (style | WS_DISABLED);
	SetWindowLong(hWnd, GWL_STYLE, style);
}

void UpdateEnabledFields(HWND hWnd) {
	GRPEDITORDATA *data = (GRPEDITORDATA *) GetWindowLongPtr(hWnd, 0);

	int sel = SendMessage(data->hWndIdSelect, CB_GETCURSEL, 0, 0);
	grpconf_entry_t *entry = data->entries + sel;
	
	//near/far clip fields
	SetWindowEnabled(data->hWndNearClip, entry->has3DModel != 0);
	SetWindowEnabled(data->hWndFarClip, entry->has3DModel != 0);

	//width/height/depth
	int colType = entry->collisionType;
	SetWindowEnabled(data->hWndWidth, colType != MOBJ_COLLISION_TYPE_NONE);
	SetWindowEnabled(data->hWndHeight, colType != MOBJ_COLLISION_TYPE_NONE 
					 && colType != MOBJ_COLLISION_TYPE_SPHERE);
	SetWindowEnabled(data->hWndDepth, colType != MOBJ_COLLISION_TYPE_NONE 
					 && colType != MOBJ_COLLISION_TYPE_SPHERE 
					 && colType != MOBJ_COLLISION_TYPE_CYLINDER
					 && colType != MOBJ_COLLISION_TYPE_SPHEROID);

	InvalidateRect(hWnd, NULL, FALSE);
}

void PopulateObjectFields(HWND hWnd) {
	GRPEDITORDATA *data = (GRPEDITORDATA *) GetWindowLongPtr(hWnd, 0);

	int sel = SendMessage(data->hWndIdSelect, CB_GETCURSEL, 0, 0);
	grpconf_entry_t *entry = data->entries + sel;
	int switching = data->switching;
	data->switching = 1;

	WCHAR buffer[16];
	SendMessage(data->hWndHasModel, CB_SETCURSEL, entry->has3DModel <= 2 ? entry->has3DModel : 2, 0);
	wsprintfW(buffer, L"%d", entry->nearClip);
	SendMessage(data->hWndNearClip, WM_SETTEXT, wcslen(buffer), (LPARAM) buffer);
	wsprintfW(buffer, L"%d", entry->farClip);
	SendMessage(data->hWndFarClip, WM_SETTEXT, wcslen(buffer), (LPARAM) buffer);
	SendMessage(data->hWndCollisionType, CB_SETCURSEL, entry->collisionType, 0);
	wsprintfW(buffer, L"%d", entry->width);
	SendMessage(data->hWndWidth, WM_SETTEXT, wcslen(buffer), (LPARAM) buffer);
	wsprintfW(buffer, L"%d", entry->height);
	SendMessage(data->hWndHeight, WM_SETTEXT, wcslen(buffer), (LPARAM) buffer);
	wsprintfW(buffer, L"%d", entry->depth);
	SendMessage(data->hWndDepth, WM_SETTEXT, wcslen(buffer), (LPARAM) buffer);

	data->switching = switching;
}

void OpenFileByName(HWND hWnd, LPCWSTR path) {
	GRPEDITORDATA *data = (GRPEDITORDATA *) GetWindowLongPtr(hWnd, 0);
	if (data->entries != NULL) {
		free(data->entries);
		data->entries = NULL;
	}

	DWORD dwSizeLow, dwSizeHigh, dwRead;
	memcpy(data->szOpenFile, path, 2 * (wcslen(path) + 1));
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
	data->nEntries = dwSizeLow / sizeof(grpconf_entry_t);
	data->entries = (grpconf_entry_t *) calloc(data->nEntries, sizeof(grpconf_entry_t));
	ReadFile(hFile, data->entries, data->nEntries * sizeof(grpconf_entry_t), &dwRead, NULL);
	CloseHandle(hFile);

	data->switching = 1;
	PopulateObjectCombobox(data->hWndIdSelect, data->entries, data->nEntries, 0);
	PopulateObjectFields(hWnd);
	UpdateEnabledFields(hWnd);
	data->switching = 0;
}

void MainWmCreate(HWND hWnd) {
	GRPEDITORDATA *data = (GRPEDITORDATA *) calloc(1, sizeof(GRPEDITORDATA));
	SetWindowLongPtr(hWnd, 0, (LONG_PTR) data);
	
	data->hWndIdSelect = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_HASSTRINGS | CBS_DROPDOWNLIST | WS_VSCROLL,
									  10, 10, 200, 500, hWnd, NULL, NULL, NULL);

	int boxWidth = 284, boxHeight = 216;
	int boxX = 10, boxY = 42;

	CreateWindow(L"STATIC", L"Has Model:", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, boxX + 12, boxY + 20, 100, 22, hWnd, NULL, NULL, NULL);
	CreateWindow(L"STATIC", L"Near Clip:", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, boxX + 12, boxY + 20 + 27 * 1, 100, 22, hWnd, NULL, NULL, NULL);
	CreateWindow(L"STATIC", L"Far Clip:", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, boxX + 12, boxY + 20 + 27 * 2, 100, 22, hWnd, NULL, NULL, NULL);
	CreateWindow(L"STATIC", L"Collision Type:", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, boxX + 12, boxY + 20 + 27 * 3, 100, 22, hWnd, NULL, NULL, NULL);
	CreateWindow(L"STATIC", L"Width:", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, boxX + 12, boxY + 20 + 27 * 4, 100, 22, hWnd, NULL, NULL, NULL);
	CreateWindow(L"STATIC", L"Height:", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, boxX + 12, boxY + 20 + 27 * 5, 100, 22, hWnd, NULL, NULL, NULL);
	CreateWindow(L"STATIC", L"Depth:", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, boxX + 12, boxY + 20 + 27 * 6, 100, 22, hWnd, NULL, NULL, NULL);


	data->hWndHasModel = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_HASSTRINGS | CBS_DROPDOWNLIST, 
									  boxX + 122, boxY + 20, 150, 22, hWnd, NULL, NULL, NULL);
	data->hWndNearClip = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 
										boxX + 122, boxY + 20 + 27 * 1, 150, 22, hWnd, NULL, NULL, NULL);
	data->hWndFarClip = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 
									   boxX + 122, boxY + 20 + 27 * 2, 150, 22, hWnd, NULL, NULL, NULL);
	data->hWndCollisionType = CreateWindowEx(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_HASSTRINGS | CBS_DROPDOWNLIST, 
											 boxX + 122, boxY + 20 + 27 * 3, 150, 22, hWnd, NULL, NULL, NULL);
	data->hWndWidth = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 
									 boxX + 122, boxY + 20 + 27 * 4, 150, 22, hWnd, NULL, NULL, NULL);
	data->hWndHeight = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 
									  boxX + 122, boxY + 20 + 27 * 5, 150, 22, hWnd, NULL, NULL, NULL);
	data->hWndDepth = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 
									 boxX + 122, boxY + 20 + 27 * 6, 150, 22, hWnd, NULL, NULL, NULL);
	CreateWindow(L"BUTTON", L"Configuration", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, boxX, boxY, boxWidth, boxHeight, hWnd, NULL, NULL, NULL);

	//set collision types dialog
	LPCWSTR colTypes[] = {
		L"None",
		L"Sphere",
		L"Spheroid",
		L"Cylinder",
		L"Box",
		L"Custom"
	};
	for (int i = 0; i < sizeof(colTypes) / sizeof(*colTypes); i++) {
		SendMessage(data->hWndCollisionType, CB_ADDSTRING, 0, (LPARAM) colTypes[i]);
	}

	//set model types
	LPCWSTR modelTypes[] = {
		L"None",
		L"3D",
		L"2D"
	};
	for (int i = 0; i < sizeof(modelTypes) / sizeof(*modelTypes); i++) {
		SendMessage(data->hWndHasModel, CB_ADDSTRING, 0, (LPARAM) modelTypes[i]);
	}

	RECT rc = { 0 };
	rc.right = boxX + boxWidth + 10;
	rc.bottom = boxY + boxHeight + 10;
	AdjustWindowRect(&rc, GetWindowLong(hWnd, GWL_STYLE), GetMenu(hWnd) != NULL);
	SetWindowPos(hWnd, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE);
	EnumChildWindows(hWnd, SetFontProc, (LPARAM) GetStockObject(DEFAULT_GUI_FONT));

	//open file?
	int argc, startInfo;
	wchar_t **argv, **env;
	__wgetmainargs(&argc, &argv, &env, 0, &startInfo);

	if (argc > 1) {
		LPCWSTR path = argv[1];
		OpenFileByName(hWnd, path);
	}
}

void MainWmCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) {
	GRPEDITORDATA *data = (GRPEDITORDATA *) GetWindowLongPtr(hWnd, 0);
	HWND hWndControl = (HWND) lParam;
	
	if (hWndControl == NULL) { //menu/accelerator
		if (HIWORD(wParam) == 0) { //menu
			switch (LOWORD(wParam)) {
				DWORD dwWritten;
				HANDLE hFile;

				case ID_FILE_OPEN:
				{
					LPWSTR path = OpenFileDialog(hWnd, L"Open File", L"TBL Files (*.tbl)\0*.tbl\0All Files\0*.*\0", L"tbl");
					if (path == NULL) break;
					
					OpenFileByName(hWnd, path);
					free(path);
					break;
				}
				case ID_FILE_SAVE:
					hFile = CreateFile(data->szOpenFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					WriteFile(hFile, data->entries, data->nEntries * sizeof(grpconf_entry_t), &dwWritten, NULL);
					CloseHandle(hFile);
					break;
				case ID_FILE_EXIT:
					DestroyWindow(hWnd);
					break;
				case ID_HELP_ABOUT:
					MessageBox(hWnd, L"GUI editor for grpconf.tbl. Made by Garhoogin with help from SGC and Gericom.",
							   L"About GRPEdit", MB_ICONINFORMATION);
					break;
			}
		} else if(HIWORD(wParam) == 1) { //accelerator
			switch (LOWORD(wParam)) {
				case ID_ACCELERATOR_OPEN:
					PostMessage(hWnd, WM_COMMAND, ID_FILE_OPEN, 0);
					break;
				case ID_ACCELERATOR_SAVE:
					PostMessage(hWnd, WM_COMMAND, ID_FILE_SAVE, 0);
					break;
			}
		}
	} else { //control notification
		if (data == NULL) return;
		WORD notification = HIWORD(wParam);

		WCHAR className[16];
		GetClassName(hWndControl, className, 16);
		if (_wcsicmp(className, L"EDIT") == 0 && notification == EN_CHANGE && !data->switching) { //is a text box being edited?
			WCHAR buffer[32];
			int sel = SendMessage(data->hWndIdSelect, CB_GETCURSEL, 0, 0);
			grpconf_entry_t *entry = data->entries + sel;

			SendMessage(hWndControl, WM_GETTEXT, 16, (LPARAM) buffer);
			int val = _wtol(buffer);
			if (hWndControl == data->hWndNearClip) {
				entry->nearClip = val;
			} else if (hWndControl == data->hWndFarClip) {
				entry->farClip = val;
			} else if (hWndControl == data->hWndWidth) {
				entry->width = val;
			} else if (hWndControl == data->hWndHeight) {
				entry->height = val;
			} if (hWndControl == data->hWndDepth) {
				entry->depth = val;
			}
		} else if (_wcsicmp(className, L"COMBOBOX") == 0 && notification == CBN_SELCHANGE) {
			if (hWndControl == data->hWndIdSelect) {
				PopulateObjectFields(hWnd);
				UpdateEnabledFields(hWnd);
			} else if (hWndControl == data->hWndCollisionType && !data->switching) {
				int type = SendMessage(hWndControl, CB_GETCURSEL, 0, 0);
				int sel = SendMessage(data->hWndIdSelect, CB_GETCURSEL, 0, 0);
				grpconf_entry_t *entry = data->entries + sel;

				entry->collisionType = type;
				UpdateEnabledFields(hWnd);
			} else if (hWndControl == data->hWndHasModel && !data->switching) {
				int type = SendMessage(hWndControl, CB_GETCURSEL, 0, 0);
				int sel = SendMessage(data->hWndIdSelect, CB_GETCURSEL, 0, 0);
				grpconf_entry_t *entry = data->entries + sel;

				entry->has3DModel = type;
				UpdateEnabledFields(hWnd);
			}
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CREATE:
			MainWmCreate(hWnd);
			break;
		case WM_COMMAND:
			MainWmCommand(hWnd, wParam, lParam);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void RegisterClasses(void) {

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(wcex);
	wcex.hbrBackground = (HBRUSH) COLOR_WINDOW;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hIconSm = wcex.hIcon;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpszClassName = L"GRPEditClass";
	wcex.lpfnWndProc = WndProc;
	wcex.cbWndExtra = sizeof(void *);
	RegisterClassEx(&wcex);

	HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR2));

	RegisterClasses();

	HWND hWnd = CreateWindow(L"GRPEditClass", L"GRPEdit", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
							 NULL, NULL, NULL, NULL);
	ShowWindow(hWnd, SW_SHOW);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}

ULONG Entry(PPEB Peb) {
	ExitProcess(WinMain(GetModuleHandle(NULL), NULL, NULL, 0));
	return 0; //STATUS_SUCCESS
}
