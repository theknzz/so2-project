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

BOOL CALLBACK PaintMap(HWND hWnd, MapInfo info, HINSTANCE hInst) {
	PAINTSTRUCT ps;
	HBITMAP StretBitMap, GrassBitMap, TaxiWaitingBitMap, PassengerBitMap;
	HDC hdcStreet, hdcGrass, hdcTaxiWaiting, hdcPassenger;
	HDC hdc = BeginPaint(hWnd, &ps);

	hdcStreet = CreateCompatibleDC(hdc);
	hdcGrass = CreateCompatibleDC(hdc);
	hdcTaxiWaiting = CreateCompatibleDC(hdc);
	hdcPassenger = CreateCompatibleDC(hdc);

	StretBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_STREET));
	SelectObject(hdcStreet, StretBitMap);

	GrassBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GRASS));
	SelectObject(hdcGrass, GrassBitMap);

	TaxiWaitingBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TAXI));
	SelectObject(hdcTaxiWaiting, TaxiWaitingBitMap);

	PassengerBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PASSENGER));
	SelectObject(hdcPassenger, PassengerBitMap);

	int x = 0, y = 0;
	for (unsigned int i = 0; i < MIN_LIN; i++) {
		for (unsigned int j = 0; j < MIN_COL; j++) {
			if (info.map[i][j].cellType == Building) {
				BitBlt(hdc, x, y, 20, 20, hdcGrass, 0, 0, SRCCOPY);
			}
			else if (info.map[i][j].cellType == Street) {
				BitBlt(hdc, x, y, 20, 20, hdcStreet, 0, 0, SRCCOPY);
			}
			for (unsigned int a = 0; a < info.nrTaxis; a++)
				if (info.taxis[a].location.x == j && info.taxis[a].location.y == i) {
					BitBlt(hdc, x, y, 20, 20, hdcTaxiWaiting, 0, 0, SRCCOPY);
				}
			for (unsigned int b = 0; b < info.nrPassengers; b++) {
				if (info.passengers[b].location.x == j && info.passengers[b].location.y == i)
					BitBlt(hdc, x, y, 20, 20, hdcPassenger, 0, 0, SRCCOPY);
			}
			x += 20;
		}
		x = 0;
		y += 20;
	}

	DeleteDC(hdcGrass);
	DeleteDC(hdcTaxiWaiting);
	DeleteDC(hdcStreet);

	EndPaint(hWnd, &ps);
}