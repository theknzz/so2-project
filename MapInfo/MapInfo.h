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

#include "resource.h"
#include "rules.h"
#include "structs.h"

DWORD WINAPI TalkToCentral(LPVOID ptr);
BOOL CALLBACK PaintMap(HWND hWnd, MapInfo info, HINSTANCE hInst);