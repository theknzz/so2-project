#include "utils.h"

// Wait for all the threads to stop
void WaitAllThreads(HANDLE* threads, int nr) {
	for (int i = 0; i < nr; i++)
		WaitForSingleObject(threads[i], INFINITE);
	nr = 0;
}

// Close all open handles
void CloseMyHandles(HANDLE* handles, int nr) {
	for (int i = 0; i < nr; i++)
		CloseHandle(handles[i]);
	nr = 0;
}

// Unmaps all views
void UnmapAllViews(HANDLE* views, int nr) {
	for (int i = 0; i < nr; i++)
		UnmapViewOfFile(views[i]);
	nr = 0;
}

void ClearScreen() {
	Sleep(1000 * 3);
	//_tprintf(_T("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"));
}

void PrintMap(Cell* map) {
	for (int i = 0; i < MIN_LIN; i++) {
		for (int j = 0; j < MIN_COL; j++) {
			Cell cell = *(map + (i * MIN_COL) + j);
			_tprintf(_T("%c"), cell.display);
		}
		_tprintf(_T("\n"));
	}
	_tprintf(_T("\n"));
}


// insere taxi na primeira posição livre
enum respond_id InsertTaxiIntoMapCell(Cell* cell, Taxi taxi, int size) {
	int i = FindFirstFreeTaxiIndex(cell->taxis, size);
	if (i == -1) return ERRO;
	cell->display = T_DISPLAY;
	CopyMemory(cell->taxis[i].licensePlate, taxi.licensePlate, sizeof(taxi.licensePlate));
	cell->taxis[i].location.x = taxi.location.x;
	cell->taxis[i].location.y = taxi.location.y;
	cell->taxis[i].autopilot = taxi.autopilot;
	cell->taxis[i].velocity = taxi.velocity;
	return OK;
}

void RemoveTaxiFromMapCell(Cell* cell, TCHAR* lcPlate, int size) {
	for (unsigned int i = 0; i < size; i++) {
		if (_tcscmp((cell + i)->taxis[i].licensePlate, lcPlate) == 0) {
			ZeroMemory(&(cell + i)->taxis[i].licensePlate, sizeof((cell + i)->taxis[i].licensePlate));
			(cell + i)->taxis[i].location.x = -1;
			(cell + i)->taxis[i].location.y = -1;
		}
	}
}

int GetLastPassengerIndex(Passenger* passengers, int size) {
	int aux = -1;
	for (unsigned int i = 0; i < size; i++) {
		if (passengers[i].location.x == -1) return aux;
		aux = i;
	}
	return aux;
}

int GetIndexFromPassengerWithoutTransport(Passenger* passengers, int size) {
	for (unsigned int i = 0; i < size; i++)
		if (passengers[i].state == Waiting) return i;
	return -1;
}

enum passanger_state GetPassengerStatus(Passenger* passengers, int size, TCHAR* name) {
	for (unsigned int i = 0; i < size; i++) {
		if (_tcscmp(passengers[i].nome, name) == 0)
			return passengers[i].state;
	}
	return NotFound;
}

int GetPassengerIndex(Passenger* passengers, int size, TCHAR* name) {
	for (unsigned int i = 0; i < size; i++) {
		if (_tcscmp(passengers[i].nome, name) == 0)
			return i;
	}
	return -1;
}

TCHAR* hasPassenger(Taxi taxi) {
	return taxi.client.location.x > 0 ? taxi.client.nome : _T("no");
}

int FindTaxiIndex(Taxi* taxis, int size, Taxi target) {
	for (unsigned int i = 0; i < size; i++)
		if (_tcscmp((taxis + i)->licensePlate, target.licensePlate) == 0)
			return i;
	return -1;
}

int FindTaxiWithLicense(Taxi* taxis, int size, TCHAR* lc) {
	for (unsigned int i = 0; i < size; i++) {
		if (_tcscmp(taxis[i].licensePlate, lc) == 0)
			return i;
	}
	return -1;
}

int FindFirstFreeTaxiIndex(Taxi* taxis, int size) {
	for (unsigned int i = 0; i < size; i++) {
		if (taxis[i].location.x == -1)
			return i;
	}
	return -1;
}

TCHAR* ParseStateToString(enum passanger_state state) {
	switch (state) {
	case Waiting:
		return _T("Waiting for a taxi attribution");
	case Taken:
		return _T("Waiting for taxi to arrive");
	case OnDrive:
		return _T("On drive");
	case Done:
		return _T("Arrived destination");
	}
}

int NumberOfActiveTaxis(Taxi* taxis, int size) {
	int count = 0;
	for (unsigned int i = 0; i < size; i++)
		if (taxis[i].location.x != -1)
			count++;
	return count;
}

void LoadMapa(Cell* map, char* buffer, int nrTaxis, int nrPassangers) {
	int aux = 0;
	int ignored = 0;
	int nr = 0;
	for (int i = 0; i < MIN_LIN; i++) {
		for (int j = 0; j < MIN_COL; j++) {
			nr = ((i * MIN_COL) + j) + ignored;
			char c = buffer[nr];
			if (c == '\n' || c == '\r') {
				ignored += 2;
				j--;
				continue;
			}
			aux = ((i * MIN_COL) + j);
			Cell* cell = &map[aux];
			if (c == S_CHAR) {
				cell->taxis = malloc(nrTaxis * sizeof(Taxi));
				for (unsigned int i = 0; i < nrTaxis; i++) {
					ZeroMemory(&map[aux].taxis[i].licensePlate, sizeof(TCHAR) * 9);
					cell->taxis[i].location.x = -1;
					cell->taxis[i].location.y = -1;
				}
				cell->passengers = malloc(nrPassangers * sizeof(Passenger));
				for (unsigned int i = 0; i < nrPassangers; i++) {
					ZeroMemory(&map[aux].passengers[i].nome, sizeof(TCHAR) * 25);
					cell->passengers[i].location.x = -1;
					cell->passengers[i].location.y = -1;
				}
				cell->display = S_DISPLAY;
				cell->cellType = Street;
				cell->x = j;
				cell->y = i;
			}
			else if (c == B_CHAR) {
				cell->taxis = malloc(nrTaxis * sizeof(Taxi));
				for (unsigned int i = 0; i < nrTaxis; i++) {
					ZeroMemory(&map[aux].taxis[i].licensePlate, sizeof(TCHAR) * 9);
					cell->taxis[i].location.x = -1;
					cell->taxis[i].location.y = -1;
				}
				cell->passengers = malloc(nrPassangers * sizeof(Passenger));
				for (unsigned int i = 0; i < nrPassangers; i++) {
					ZeroMemory(&map[aux].passengers[i].nome, sizeof(TCHAR) * 25);
					cell->passengers[i].location.x = -1;
					cell->passengers[i].location.y = -1;
				}
				cell->display = B_DISPLAY;
				cell->cellType = Building;
				cell->x = j;
				cell->y = i;
			}
		}
	}
}

char* ReadFileToCharArray(TCHAR* mapName) {
	HANDLE file = CreateFile(mapName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Erro ao obter handle para o ficheiro (%d).\n"), GetLastError());
		return NULL;
	}

	HANDLE fmFile = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL /*FILE MAPPING NAME*/);
	if (!fmFile) {
		_tprintf(_T("Erro ao mapear o ficheiro (%d).\n"), GetLastError());
		CloseHandle(file);
		return NULL;
	}

	char* mvof = (char*)MapViewOfFile(fmFile, FILE_MAP_READ, 0, 0, 0);
	if (!mvof) {
		_tprintf(_T("Erro - MapViewOfFile (%d).\n"), GetLastError());
		CloseHandle(file);
		CloseHandle(fmFile);
		return NULL;
	}

	CloseHandle(fmFile);
	CloseHandle(file);
	return mvof;
}

char** ConvertMapIntoCharMap(Cell* map) {
	char charMap[MIN_LIN][MIN_COL];

	for (unsigned int lin = 0; lin < MIN_LIN; lin++)
		for (unsigned int col = 0; col < MIN_COL; col++) {
			char aux = map[col + lin * MIN_COL].display;
			if (aux == B_DISPLAY)
				charMap[col][lin] = B_DISPLAY;
			else if (aux == S_DISPLAY)
				charMap[col][lin] = S_DISPLAY;
		}
	return charMap;
}
