#pragma once

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <windowsx.h>

#include "resource.h"
#include "rules.h"
#include "structs.h"

DWORD WINAPI TalkToCentral(LPVOID ptr);
BOOL CALLBACK PaintMap(HDC hdc, MapInfo *info, HINSTANCE hInst);
BOOL CALLBACK TrataClick(HDC hdc, MapInfo *info, HINSTANCE hInstance, int x, int y);
BOOL CALLBACK LoadBitMaps(HINSTANCE hInst, MapInfo *info);
BOOL CALLBACK CreateRegistryForBitMaps(int freeTaxi, int busyTaxi, int passengerWoPassenger, int passengerWPassenger, BOOL isOverwrite);
BOOL CALLBACK TrataHover(HDC hdc, MapInfo *info, HINSTANCE hInstance, LPARAM lParam);