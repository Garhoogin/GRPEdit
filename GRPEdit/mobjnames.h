#pragma once
#include <Windows.h>

typedef struct MOBJ_NAME_ENTRY_ {
	int id;
	LPCWSTR name;
} MOBJ_NAME_ENTRY;

extern MOBJ_NAME_ENTRY g_mobjNames[96];
