#pragma once
#include <Windows.h>

#include "grp.h"

typedef struct {
	WCHAR szOpenFile[MAX_PATH];

	grpconf_entry_t *entries; //GRPConf entries
	int nEntries;             //Number of entries
	int switching;            //Is editor swapping data

	HWND hWnd;
	HWND hWndIdSelect;
	HWND hWndCreateButton;
	HWND hWndDeleteButton;
	HWND hWndCopyButton;
	HWND hWndPasteButton;

	HWND hWndHasModel;
	HWND hWndNearClip;
	HWND hWndFarClip;
	HWND hWndCollisionType;
	HWND hWndWidth;
	HWND hWndHeight;
	HWND hWndDepth;
} GRPEDITORDATA;
