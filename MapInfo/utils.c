#include <Windows.h>
#include "MapInfo.h"


DWORD WINAPI TalkToCentral(LPVOID ptr) {
	MapInfo* info = (MapInfo*)ptr;
	SHM_MAPINFO shm;
	HANDLE mutex, new_info;
	SHM_MAPINFO* message;

	if ((mutex = OpenMutex(SYNCHRONIZE, FALSE, MAPINFO_MUTEX))==NULL) {
		return 0;
	}

	if ((new_info = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_NEW_INFO)) == NULL) {
		return 0;
	}

	HANDLE fm = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(SHM_MAPINFO),SHM_MAP_INFO_NAME);
	if (fm == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		return 0;
	}

	if ((message = (SHM_MAPINFO*)MapViewOfFile(fm, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SHM_MAPINFO))) == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		return 0;
	}

	while (1) {
		WaitForSingleObject(new_info, INFINITE);
		WaitForSingleObject(mutex, INFINITE);

		CopyMemory(&shm, message, sizeof(SHM_MAPINFO));

		ReleaseMutex(mutex);

		info->nrTaxis = shm.nrTaxis;
		info->nrPassengers = shm.nrPassengers;
		CopyMemory(info->taxis, shm.taxis, sizeof(Taxi) * info->nrTaxis);
		CopyMemory(info->passengers, shm.passengers, sizeof(Passenger) * info->nrPassengers);
		CopyMemory(info->map, shm.map, sizeof(Cell) * MIN_COL * MIN_LIN);
		InvalidateRect(info->window, NULL, TRUE);
	}
	return 0; 
}

BOOL CALLBACK PaintMap(HWND hWnd, MapInfo* info, HINSTANCE hInst) {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);

	HDC hdcBitMap = CreateCompatibleDC(hdc);
	HBITMAP hBitMap;

	if (info->map[0][0].cellType != Building && info->map[0][0].cellType != Street)
	{
		DeleteDC(hdcBitMap);
		EndPaint(hWnd, &ps);
		return;
	}

	int x =0, y=0;
	for (unsigned int i = 0; i < MIN_LIN; i++) {
		for (unsigned int j = 0; j < MIN_COL; j++) {
			if (info->map[i][j].cellType == Building) {
				hBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GRASS));
				SelectObject(hdcBitMap, hBitMap);
			}
			else {
				hBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_STREET));
				SelectObject(hdcBitMap, hBitMap);
			}
			BitBlt(hdc, x, y, 20, 20, hdcBitMap, 0, 0, SRCCOPY);
			x += 20;
		}
		x = 0;
		y += 20;
	}
	DeleteDC(hdcBitMap);
	EndPaint(hWnd, &ps);
}
