#include <Windows.h>
#include "MapInfo.h"


DWORD WINAPI TalkToCentral(LPVOID ptr) {
	MapInfo* info = (MapInfo*)ptr;
	SHM_MAPINFO shm;
	HANDLE mutex, new_info;
	SHM_MAPINFO* message;

	if ((mutex = OpenMutex(SYNCHRONIZE, FALSE, MAPINFO_MUTEX))==NULL) {
		MessageBox(info->window, _T("CenTaxi isn't running..."), _T("MapInfo - Warning"), MB_OK);
		return 0;
	}

	if ((new_info = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_NEW_INFO)) == NULL) {
		MessageBox(info->window, _T("CenTaxi isn't running..."), _T("MapInfo - Warning"), MB_OK);
		return 0;
	}

	HANDLE fm = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(SHM_MAPINFO),SHM_MAP_INFO_NAME);
	if (fm == NULL) {
		MessageBox(info->window, _T("CenTaxi isn't running..."), _T("MapInfo - Warning"), MB_OK);
		return 0;
	}

	if ((message = (SHM_MAPINFO*)MapViewOfFile(fm, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SHM_MAPINFO))) == NULL) {
		MessageBox(info->window, _T("CenTaxi isn't running..."), _T("MapInfo - Warning"), MB_OK);
		return 0;
	}

	info->nrTaxis = message->nrTaxis;
	info->nrPassengers = message->nrPassengers;
	info->SystemIsClosing = message->isSystemClosing;
	CopyMemory(info->taxis, message->taxis, sizeof(Taxi) * info->nrTaxis);
	CopyMemory(info->passengers, message->passengers, sizeof(Passenger) * info->nrPassengers);
	CopyMemory(info->map, message->map, sizeof(Cell) * MIN_COL * MIN_LIN);
	InvalidateRect(info->window, NULL, FALSE);

	do {
		WaitForSingleObject(new_info, INFINITE);
		WaitForSingleObject(mutex, INFINITE);

		CopyMemory(&shm, message, sizeof(SHM_MAPINFO));

		ReleaseMutex(mutex);
		ResetEvent(new_info);

		info->nrTaxis = shm.nrTaxis;
		info->nrPassengers = shm.nrPassengers;
		info->SystemIsClosing = shm.isSystemClosing;
		CopyMemory(info->taxis, shm.taxis, sizeof(Taxi) * info->nrTaxis);
		CopyMemory(info->passengers, shm.passengers, sizeof(Passenger) * info->nrPassengers);
		CopyMemory(info->map, shm.map, sizeof(Cell) * MIN_COL * MIN_LIN);
		InvalidateRect(info->window, NULL, FALSE);
	} while (!info->SystemIsClosing);
	if (MessageBox(info->window, _T("CenTaxi was closed!\nTry to access the system later."), _T("MapInfo - Warning"), MB_OK) == IDOK) {
		ZeroMemory(info, sizeof(MapInfo));
		InvalidateRect(info->window, NULL, FALSE);
	}
	return 0; 
}

BOOL CALLBACK PaintMap(HDC hdc, MapInfo *info, HINSTANCE hInst) {
	HDC hdcAux;
	TCHAR *str = _T("MAP_INFO");

	hdcAux = CreateCompatibleDC(hdc);
	int x = 0, y = 0;
	for (unsigned int i = 0; i < MIN_LIN; i++) {
		for (unsigned int j = 0; j < MIN_COL; j++) {
			if (info->map[i][j].cellType == Building) {
				SelectObject(hdcAux, info->GrassBitMap);
			}
			else if (info->map[i][j].cellType == Street) {
				SelectObject(hdcAux, info->StretBitMap);
			}

			for (unsigned int b = 0; b < info->nrPassengers; b++) {
				if (info->passengers[b].location.x == j && info->passengers[b].location.y == i) {
					if (info->passengers[b].state == Waiting)
						SelectObject(hdcAux, info->PassengerWaitingWithoutTaxiBitMap);
					else if (info->passengers[b].state == OnDrive || info->passengers[b].state == Taken)
						SelectObject(hdcAux, info->PassengerWaitingWithTaxiBitMap);
				}
			}

			for (unsigned int a = 0; a < info->nrTaxis; a++)
				if (info->taxis[a].location.x == j && info->taxis[a].location.y == i) {
					if (info->taxis[a].client.location.x > -1 && info->taxis[a].client.location.y > -1)
						SelectObject(hdcAux, info->TaxiBusyBitMap);
					else
						SelectObject(hdcAux, info->TaxiWaitingBitMap);
				}

			BitBlt(hdc, x, y, BITMAP_SIZE, BITMAP_SIZE, hdcAux, 0, 0, SRCCOPY);

			x += BITMAP_SIZE;
		}
		x = 0;
		y += BITMAP_SIZE;
	}
	
	TextOut(hdc, 1060, 10, str, _tcslen(str));
	DeleteDC(hdcAux);
	return TRUE;
}

BOOL CALLBACK TrataClick(HDC hdc, MapInfo *info, HINSTANCE hInstance, int x, int y) {
	int offset_y = 200;
	TCHAR str[100];

	for (unsigned int i = 0; i < info->nrPassengers; i++) {
		if ((info->passengers[i].location.x + 1) * BITMAP_SIZE >= x && info->passengers[i].location.x * BITMAP_SIZE <= x && (info->passengers[i].location.y + 1) * BITMAP_SIZE >= y && info->passengers[i].location.y * BITMAP_SIZE <= y) {
			_stprintf(str, _T("Name:"));
			TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 1, str, _tcslen(str));
			TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 2, info->passengers[i].nome, _tcslen(info->passengers[i].nome));
			_stprintf(str, _T("Destination:"));
			TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 3, str, _tcslen(str));
			_stprintf(str, _T("{%.2d;%.2d}"), info->passengers[i].destination.x, info->passengers[i].destination.y);
			TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 4, str, _tcslen(str));
			for (unsigned int j = 0; j < info->nrTaxis; j++) {
				if (info->taxis[j].client.location.x == info->passengers[i].location.x && info->taxis[j].client.location.y == info->passengers[i].location.y) {
					_stprintf(str, _T("Taxi:"));
					TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 5, str, _tcslen(str));
					_stprintf(str, _T("%s"), info->taxis[j].licensePlate);
					TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 6, str, _tcslen(str));
				}
			}
		}
	}
	InvalidateRect(info->window, NULL, TRUE);
}

BOOL CALLBACK TrataHover(HDC hdc, MapInfo *info, HINSTANCE hInstance, LPARAM lParam) {
	int x, y, offset_y = 500;
	TCHAR str[100];

	x = (int)GET_X_LPARAM(lParam);
	y = (int)GET_Y_LPARAM(lParam);

	for (unsigned int i = 0; i < info->nrTaxis; i++) {
		if ((info->taxis[i].location.x + 1) * BITMAP_SIZE >= x && info->taxis[i].location.x * BITMAP_SIZE <= x 
			&& (info->taxis[i].location.y + 1) * BITMAP_SIZE >= y && info->taxis[i].location.y * BITMAP_SIZE <= y) {
			_stprintf(str, _T("License Plate:"));
			TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 2, str, _tcslen(str));
			_stprintf(str, _T("%s"), info->taxis[i].licensePlate);
			TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 3, str, _tcslen(str));
			if (info->taxis[i].client.location.x > -1 && info->taxis[i].client.location.y > -1) {
				_stprintf(str, _T("Destination:"));
				TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 4, str, _tcslen(str));
				_stprintf(str, _T("{%.2d;%.2d}"), info->taxis[i].client.destination.x, info->taxis[i].client.destination.y);
				TextOut(hdc, 1060, offset_y + BITMAP_SIZE * 5, str, _tcslen(str));
			}
		}
	}
	InvalidateRect(info->window, NULL, TRUE);
}

BOOL CALLBACK LoadBitMaps(HINSTANCE hInst, MapInfo *info) {
	HKEY key;
	TCHAR key_name[100], value[100];
	DWORD result, version, size;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, KEY_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &result) != ERROR_SUCCESS) {
		return FALSE;
	}

	info->StretBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_STREET));
	info->GrassBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GRASS));

	RegQueryValueEx(key, FREE_TAXI, NULL, NULL, (LPBYTE)value, &size);
	value[size / sizeof(TCHAR)] = '\0';
	info->TaxiWaitingBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(_ttoi(value)/*IDB_FREE_TAXI*/));

	RegQueryValueEx(key, BUSY_TAXI, NULL, NULL, (LPBYTE)value, &size);
	value[size / sizeof(TCHAR)] = '\0';
	info->TaxiBusyBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(_ttoi(value)/*IDB_BUSY_TAXI*/));

	RegQueryValueEx(key, WAITING_PASSENGER_WITHOUT_TAXI, NULL, NULL, (LPBYTE)value, &size);
	value[size / sizeof(TCHAR)] = '\0';
	info->PassengerWaitingWithoutTaxiBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(_ttoi(value)/*IDB_PASSENGER_WITHOUT_TAXI*/));

	RegQueryValueEx(key, WAITING_PASSENGER_WITH_TAXI, NULL, NULL, (LPBYTE)value, &size);
	value[size / sizeof(TCHAR)] = '\0';
	info->PassengerWaitingWithTaxiBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(_ttoi(value)/*IDB_PASSENGER_WITH_TAXI*/));

	InvalidateRect(info->window, NULL, TRUE);
}

BOOL CALLBACK CreateRegistryForBitMaps(int freeTaxi, int busyTaxi, int passengerWoPassenger, int passengerWPassenger, BOOL isOverwrite) {
	HKEY key;
	DWORD result;
	TCHAR str[100];

	if (RegCreateKeyEx(HKEY_CURRENT_USER, KEY_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &result) != ERROR_SUCCESS) {
		return FALSE;
	}
	else if (isOverwrite) {
		if (freeTaxi == -1)
			freeTaxi = IDB_FREE_TAXI;
		_stprintf(str, _T("%d"), freeTaxi);
		RegSetValueEx(key, FREE_TAXI, 0, REG_SZ, (LPBYTE)str, _tcslen(str) * sizeof(TCHAR));

		if (busyTaxi == -1)
			busyTaxi = IDB_BUSY_TAXI;
		_stprintf(str, _T("%d"), busyTaxi);
		RegSetValueEx(key, BUSY_TAXI, 0, REG_SZ, (LPBYTE)str, _tcslen(str) * sizeof(TCHAR));

		if (passengerWoPassenger == -1)
			passengerWoPassenger = IDB_PASSENGER_WITHOUT_TAXI;
		_stprintf(str, _T("%d"), passengerWoPassenger);
		RegSetValueEx(key, WAITING_PASSENGER_WITHOUT_TAXI, 0, REG_SZ, (LPBYTE)str, _tcslen(str) * sizeof(TCHAR));

		if (passengerWPassenger == -1)
			passengerWPassenger = IDB_PASSENGER_WITH_TAXI;
		_stprintf(str, _T("%d"), passengerWPassenger);
		RegSetValueEx(key, WAITING_PASSENGER_WITH_TAXI, 0, REG_SZ, (LPBYTE)str, _tcslen(str) * sizeof(TCHAR));
	}
}
