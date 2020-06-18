#include "utils.h"

// Wait for all the threads to stop
void WaitAllThreads(CDThread* cd, HANDLE* threads, int nr) {
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
	//for (int i = 0; i < MIN_LIN; i++) {
	//	for (int j = 0; j < MIN_COL; j++) {
	//		Cell cell = *(map + (i * MIN_COL) + j);
	//		_tprintf(_T("%c"), cell.display);
	//	}
	//	_tprintf(_T("\n"));
	//}
	//_tprintf(_T("\n"));
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
		if (passengers[i].location.x < 0) return aux;
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

void InsertPassengerIntoBuffer(ProdCons* box, Passenger passenger) {
	WaitForSingleObject(box->canCreate, INFINITE);
	WaitForSingleObject(box->mutex, INFINITE);
	passenger.state = Waiting;
	CopyMemory(&box->buffer[*box->posW], &passenger, sizeof(Passenger));
	*box->posW = ((*box->posW) + 1) % CIRCULAR_BUFFER_SIZE;

	ReleaseMutex(box->mutex);
	ReleaseSemaphore(box->canConsume, 1, 0);
}

void GetPassengerFromBuffer(ProdCons* box, Passenger *passenger) {
	WaitForSingleObject(box->canConsume, INFINITE);
	WaitForSingleObject(box->mutex, INFINITE);

	CopyMemory(passenger, &box->buffer[*box->posR], sizeof(Passenger));
	*box->posR = ((*box->posR) + 1) % CIRCULAR_BUFFER_SIZE;

	ReleaseMutex(box->mutex);
	ReleaseSemaphore(box->canCreate, 1, 0);
}

int PickRandomTransportIndex(int size) {
	srand(time(0));
	if (size == 0) return NULL;
	return (rand() % size) ;
}

BOOL SendMessageToTaxiViaNamePipe(PassMessage message, Taxi *taxi) {
	DWORD nr;
	return WriteFile(taxi->hNamedPipe, &message, sizeof(PassMessage), &nr, NULL);
}

int timer(int waitTime) {
	HANDLE hTimer;

	FILETIME ft, ftUTC;
	LARGE_INTEGER DueTime;
	SYSTEMTIME systime;
	LONG interval = (LONG)waitTime * 1000;

	SystemTimeToFileTime(&systime, &ft);
	LocalFileTimeToFileTime(&ft, &ftUTC);
	DueTime.HighPart = ftUTC.dwHighDateTime;
	DueTime.LowPart = ftUTC.dwLowDateTime;
	DueTime.QuadPart = -(LONGLONG)interval * 1000 * 10;

	if ((hTimer = CreateWaitableTimer(NULL, TRUE, NULL)) == NULL) {
		_tprintf(_T("Error (%d) creating the waitable timer.\n"), GetLastError());
		return -1;
	}

	if (!SetWaitableTimer(hTimer, &DueTime, 0, NULL, NULL, FALSE)) {
		_tprintf(_T("Something went wrong! %d"), GetLastError());
	}
	WaitForSingleObject(hTimer, INFINITE);
	CloseHandle(hTimer);
	return 1;
}

int WaitAndPickWinner(CDThread* cd, Passenger passenger) {
	int index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, passenger.nome);
	Passenger aux = cd->passengers[index];
	return PickRandomTransportIndex(*aux.requestsCounter);
}

BOOL SendTransportRequestResponse(HANDLE event, HANDLE* requests, Passenger client, int size, int winner) {
	PassMessage message, aux;
	DWORD nr;
	if (size == 0) return FALSE;
	client.state = Taken;
	CopyMemory(&message.content.passenger, &client, sizeof(Passenger));
	for (unsigned int i = 0; i < size; i++) {
		if (i == winner) {
			message.resp = OK;
			WriteFile(requests[i], &message, sizeof(PassMessage), &nr, NULL);
		}
		else {
			message.resp = ERRO;
			WriteFile(requests[i], &message, sizeof(PassMessage), &nr, NULL);
		}
		SetEvent(event);
	}
	return TRUE;
}

enum response_action UpdateTaxiPosition(CDThread* cd, Content content) {
	int index, x, y;
	// encontrar o taxi na tabela de taxis ativos
	index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
	// se nao encontrar devolve erro
	if (index == -1)
		return TAXI_KICKED;
	// guarda a posição atual no mapa em variaveis auxiliares
	x = (*(cd->taxis + index)).location.x;
	y = (*(cd->taxis + index)).location.y;
	// atualizar o display da célula atual
	(*(cd->map + (x + y * MIN_COL))).display = S_DISPLAY;
	// remover o taxi desta célula
	RemoveTaxiFromMapCell((cd->map + (x + y * MIN_COL)), content.taxi.licensePlate, cd->nrMaxTaxis);
	// atualizar variaveis auxiliares com as coordenadas destino do taxi
	x = content.taxi.location.x;
	y = content.taxi.location.y;
	// alterar o display da célula nova para taxi
	(*(cd->map + (x + y * MIN_LIN))).display = T_DISPLAY;
	// inserir o taxi na célula nova
	Cell* cell = &cd->map[x + y * MIN_COL];
	InsertTaxiIntoMapCell(cell, content.taxi, cd->nrMaxTaxis);
	// atualizar tabela de taxis ativos
	(cd->taxis + index)->location.x = content.taxi.location.x;
	(cd->taxis + index)->location.y = content.taxi.location.y;
	(cd->taxis + index)->autopilot = content.taxi.autopilot;
	(cd->taxis + index)->velocity = content.taxi.velocity;
	// update the passengers position
	if (content.taxi.client.location.x >= 0 && content.taxi.client.state == OnDrive) {
		cd->taxis[index].client.location.x = content.taxi.location.x;
		cd->taxis[index].client.location.y = content.taxi.location.y;
		// update the passenger position inside the map cell
		index = FindTaxiIndex(cd->map[x + y * MIN_LIN].taxis, cd->nrMaxTaxis, content.taxi);
		if (index == -1) return ERRO;
		cd->map[x + y * MIN_LIN].taxis[index].client.location.x = x;
		cd->map[x + y * MIN_LIN].taxis[index].client.location.y = y;
		// update the passenger position inside the list of passengers
		index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.taxi.client.nome);
		if (index == -1) return ERRO;
		cd->passengers[index].location.x = x;
		cd->passengers[index].location.y = y;
	}
	return OK;
}

void SendMessageToPassenger(enum response_id resp, Passenger* passenger, Taxi* taxi, CDThread* cd) {
	PassMessage message;
	DWORD nr;
	BOOL ret;

	CopyMemory(&message.content.passenger, passenger, sizeof(Passenger));
	message.resp = resp;

	if (taxi != NULL) {
		CopyMemory(&message.content.taxi, taxi, sizeof(Taxi));
	}

	if (!WriteFile(cd->hPassPipeTalk, &message, sizeof(PassMessage), &nr, NULL)) {
		_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
		Sleep(2000);
		exit(-1);
	}

	ret = ReadFile(cd->hPassPipeTalk, &message, sizeof(PassMessage), &nr, NULL);
}

int CalculateDistanceTo(Coords org, Coords dest) {
	int y_dist, x_dist;

	y_dist = dest.y - org.y;
	x_dist = dest.x - org.x;

	return (int)sqrt(pow(y_dist, 2) + pow(x_dist, 2));
}

double GetEstimatedTime(CDThread* cd, Coords target)
{
	double time = 0.0;
	int nr_taxis = NumberOfActiveTaxis(cd->taxis, cd->nrMaxTaxis);

	for (unsigned int i = 0; i < nr_taxis; i++) {
		time += (double)(CalculateDistanceTo(cd->taxis[i].location, target) / cd->taxis[i].velocity);
	}
	time = (double) time / nr_taxis;

	return time;
}

enum response_action CatchPassenger(CDThread* cd, Content content) {
	int index, x, y;
	// change the state of the taxi's client
	index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
	if (index == -1)
		return TAXI_KICKED;
	x = (*(cd->taxis + index)).location.x;
	y = (*(cd->taxis + index)).location.y;
	cd->taxis[index].client.state = OnDrive;

	// update state of the passenger in the list of passengers
	index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.taxi.client.nome);
	if (index == -1) return ERRO;
	cd->passengers[index].state = OnDrive;

	// update the state of the passenger in the map cell
	index = FindTaxiIndex(cd->map[x + y * MIN_LIN].taxis, cd->nrMaxTaxis, content.taxi);
	if (index == -1) return ERRO;
	cd->map[x + y * MIN_LIN].taxis[index].client.state = OnDrive;

	SendMessageToPassenger(PASSENGER_CAUGHT, &content.taxi.client, &content.taxi, cd);
	return OK;
}

enum response_id DeliverPassenger(CDThread* cd, Content content) {
	int index, x, y;
	index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
	if (index == -1)
		return TAXI_KICKED;
	x = cd->taxis[index].location.x;
	y = cd->taxis[index].location.y;
	cd->taxis[index].client.state = Done;

	// remove passenger from the taxi
	ZeroMemory(&cd->taxis[index].client, sizeof(Passenger));
	cd->taxis[index].client.location.x = -1;
	cd->taxis[index].client.location.y = -1;

	index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.taxi.client.nome);
	if (index == -1) return ERRO;

	// remove passenger from the list of passengers
	ZeroMemory(&cd->passengers[index], sizeof(Passenger));
	cd->passengers[index].location.x = -1;
	cd->passengers[index].location.y = -1;


	// remove passenger from the map cell and taxi ownership
	index = FindTaxiIndex(cd->map[x + y * MIN_LIN].taxis, cd->nrMaxTaxis, content.taxi);
	if (index == -1) return ERRO;
	ZeroMemory(&cd->map[x + y * MIN_LIN].taxis[index].client, sizeof(Passenger));
	cd->map[x + y * MIN_LIN].taxis[index].client.location.x = -1;
	cd->map[x + y * MIN_LIN].taxis[index].client.location.y = -1;
	SendMessageToPassenger(PASSENGER_DELIVERED, &content.taxi.client, &content.taxi, cd);
	return OK;
}

enum response_id AssignPassengerToTaxi(CDThread* cd, Content content) {
	WaitForSingleObject(cd->mtx_access_control, INFINITE);
	int index, x, y;
	enum passanger_state status;
	index = FindTaxiWithLicense(cd->taxis, cd->nrMaxTaxis, content.taxi.licensePlate);
	if (index == -1)
		return TAXI_KICKED;
	status = GetPassengerStatus(cd->passengers, cd->nrMaxPassengers, &content.passenger.nome);
	if (status == NotFound)
		return PASSENGER_DOESNT_EXIST;
	else if (status != Waiting)
		return PASSENGER_ALREADY_TAKEN;
	content.passenger.state = Taken;
	CopyMemory(&cd->taxis[index].client, &cd->passengers[GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.passenger.nome)], sizeof(Passenger));
	// Update passenger info
	index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.passenger.nome);
	cd->passengers[index].state = Taken;
	ReleaseMutex(cd->mtx_access_control);
	return OK;
}

void AddPassengerToCentral(CDThread *cdata, TCHAR* nome, int x, int y, int dest_x, int dest_y) {
	int index = GetLastPassengerIndex(cdata->passengers, cdata->nrMaxPassengers);
	CopyMemory(cdata->passengers[index + 1].nome, nome, sizeof(TCHAR) * 25);
	if (cdata->map[x + y * MIN_COL].cellType == Building || cdata->map[dest_x + dest_y * MIN_COL].cellType == Building) {
		_tprintf(_T("Passenger can't use the system inside buildings!\n"));
		return;
	}
	cdata->passengers[index + 1].location.x = x;
	cdata->passengers[index + 1].location.y = y;
	cdata->passengers[index + 1].destination.x = dest_x;
	cdata->passengers[index + 1].destination.y = dest_y;
	cdata->passengers[index + 1].state = Waiting;
	index = GetLastPassengerIndex(cdata->map[x + y * MIN_COL].passengers, cdata->nrMaxPassengers);
	CopyMemory(cdata->map[x + y * MIN_COL].passengers[index + 1].nome, nome, sizeof(TCHAR) * 25);
	cdata->map[x + y * MIN_COL].passengers[index + 1].location.x = x;
	cdata->map[x + y * MIN_COL].passengers[index + 1].location.y = y;
	cdata->map[x + y * MIN_COL].passengers[index + 1].destination.x = dest_x;
	cdata->map[x + y * MIN_COL].passengers[index + 1].destination.y = dest_y;
	cdata->map[x + y * MIN_COL].passengers[index + 1].state = Waiting;
}

BOOL isInRequestBuffer(Taxi* requests, int size, Taxi taxi) {
	for (unsigned int i = 0; i < size; i++)
		if (_tcscmp(requests[i].licensePlate, taxi.licensePlate) == 0) return TRUE;
	return FALSE;
}

void RemovePassengerFromCentral(TCHAR* nome, Passenger* passengers, int size) {
	for (unsigned int i = 0; i < size; i++) {
		if (_tcscmp(passengers[i].nome, nome) == 0) {
			ZeroMemory(&passengers[i].nome, _tcslen(passengers[i].nome)*sizeof(TCHAR));
			passengers[i].state = Waiting;
			passengers[i].location.x = -1;
			passengers[i].location.y = -1;
			passengers[i].destination.x = -1;
			passengers[i].destination.y = -1;
		}
	}
}

BOOL isValidCoords(CDThread* cd, Coords c) {
	return cd->charMap[c.x][c.y] == S_DISPLAY ? TRUE : FALSE;
}

int FindTaxiWithNamedPipeHandle(Taxi* taxis, int size, HANDLE handle) {
	for (unsigned int i = 0; i < size; i++) {
		if (taxis[i].hNamedPipe == handle)
			return i;
	}
	return -1;
}

int NumberOfActivePassengers(Passenger* passengers, int size) {
	int count = 0;
	for (unsigned int i = 0; i < size; i++) {
		if (passengers[i].location.x >= 0)
			count++;
	}
	return count;
}