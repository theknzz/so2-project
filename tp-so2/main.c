#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#include "rules.h"
#include "structs.h"

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
}

// função-thread responsável por tratar a interação com o admin
DWORD WINAPI TextInterface(LPVOID ptr) {

	TI_Controldata* cdata = (TI_Controldata*)ptr;
	TCHAR command[100];
	
	while (1) {
		ClearScreen();
		PrintMap(cdata->map);
		_tprintf(_T("Command: "));
		_tscanf_s(_T(" %99[^\n]"), command, sizeof(TCHAR)*100);
		_tprintf(_T("Executing '%s'\n"), command);
		FindFeatureAndRun(command, cdata);
	}

	return 0;
}

void RespondToTaxi(CC_CDResponse response, Content* content, enum response_id responseId) {

	WaitForSingleObject(response.mutex, INFINITE);
	
	response.shared->action = responseId;
	CopyMemory(&response.shared->responseContent, content, sizeof(Content));

	ReleaseMutex(response.mutex);

	SetEvent(response.got_response);

	_tprintf(_T("Response sent to taxi.\n"));
}

DWORD WINAPI ListenToTaxis(LPVOID ptr) {
	CDThread* cd = (CDThread*)ptr;
	CC_CDRequest cdata = cd->controlDataTaxi;
	SHM_CC_REQUEST shared;

	while (1) {
		WaitForSingleObject(cdata.write_event, INFINITE);
		_tprintf(_T("Got something written to me\n"));
		
		_tprintf(_T("Waiting for access to the mutex\n"));
		WaitForSingleObject(cdata.mutex, INFINITE);
		_tprintf(_T("Got access to the mutex\n"));

		// Save Request
		CopyMemory(&shared, cdata.shared, sizeof(SHM_CC_REQUEST));

		ReleaseMutex(cdata.mutex);
		int index = *(cd->taxiFreePosition);
		// Process Event
		switch (shared.action) {
			case RegisterTaxiInCentral:
				// insert the taxi on the taxi's array
				CopyMemory(&cd->taxis[index], &shared.messageContent.taxi, sizeof(Taxi));
				_tprintf(_T("YOOOOO %d\n"), (*cd->taxiFreePosition)++);
				// insert the taxi on the map
				Taxi* t = &(cd->map + shared.messageContent.taxi.location.x + (shared.messageContent.taxi.location.y * MIN_COL))->taxi;
				(cd->map + shared.messageContent.taxi.location.x + (shared.messageContent.taxi.location.y * MIN_COL))->display = 't';
				CopyMemory(t, &shared.messageContent.taxi, sizeof(Taxi));
				
				Content content;
				// notifiquei o taxi a dizer que a mensagem foi lida
				//SetEvent(cdata.read_event);
				RespondToTaxi(cd->cdResponse, &content, OK);
				break;
		}
	}
	return 0;
}

int FindFeatureAndRun(TCHAR* command, TI_Controldata* cdata) {
	TCHAR commands[6][100] = {
		_T("\tkick x - kick taxi with x as id.\n"),
		_T("\tclose - close the system.\n"), 
		_T("\tlist - list all the taxis in the system.\n"), 
		_T("\tpause - pauses taxis acceptance in the system.\n"), 
		_T("\tresume - resumes taxis acceptance in the system.\n"), 
		_T("\tinteval x - changes the time central waits for the taxis to ask for work.\n") };
	int argument=-1;
	TCHAR* delimiter = _T(" ");

	TCHAR* cmd = _tcstok(command, delimiter);

	if (cmd != NULL) {
		TCHAR* aux = _tcstok(NULL, delimiter);
		if (aux!=NULL)
			argument = _ttoi(aux);
	}

	// Garantir que cobre todas as situacoes
	cmd = _tcslwr(cmd);

	if (_tcscmp(cmd, ADM_KICK) == 0) {
		if (argument==-1)
			_tprintf(_T("This command requires a id.\n"));
		else {
			_tprintf(_T("I am kicking taxi with id %d\n"), argument);
		}
	}
	else if (_tcscmp(cmd, ADM_CLOSE) == 0) {
		// #TODO notify todas as apps
		_tprintf(_T("System is closing.\n"));
	}
	else if (_tcscmp(cmd, ADM_LIST) == 0) {
		_tprintf(_T("Listing all taxis %d\n"), (*cdata->taxiCount));
		for (int i = 0; i < (*cdata->taxiCount); i++)
			_tprintf(_T("%.2d - %9s at {%.2d;%.2d}\n"), i, cdata->taxis[i].licensePlate, cdata->taxis[i].location.x, cdata->taxis[i].location.y);
	}
	else if (_tcscmp(cmd, ADM_PAUSE) == 0) {
		_tprintf(_T("System pause\n"));
	}
	else if (_tcscmp(cmd, ADM_RESUME) == 0) {
		_tprintf(_T("System resume\n"));
	}
	else if (_tcscmp(cmd, ADM_INTERVAL) == 0) {
		if (argument == -1)
			_tprintf(_T("This command requires a id.\n"));
		else
			_tprintf(_T("System changing the interval to %d\n"), argument);
	}
	else if (_tcscmp(cmd, ADM_HELP) == 0) {
		for (int i = 0; i < 6; i++)
			_tprintf(_T("%s"), commands[i]);
	}
	else {
		_tprintf(_T("System doesn't recognize the command, type 'help' to view all the commands.\n"));
	}
}

void LoadMapa(Cell* map, char* buffer) {
	int aux=0;
	for (int i = 0; i < MIN_LIN; i++) {
		for (int j = 0; j < MIN_COL; j++) {
			aux = (i * MIN_COL) + j;
			char c = buffer[aux];
			if (c == S_CHAR) {
				map[aux].display = '-';
				map[aux].cellType = Street;
				map[aux].x = j;
				map[aux].y = i;
			}
			else if (c == B_CHAR) {
				map[aux].display = '+';
				map[aux].cellType = Building;
				map[aux].x = j;
				map[aux].y = i;
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

	char* mvof = (char*) MapViewOfFile(fmFile, FILE_MAP_READ, 0, 0, 0);
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

// Wait for all the threads to stop
void WaitAllThreads(HANDLE* threads, int nr) {
	for (int i = 0; i < nr; i++) {
		WaitForSingleObject(threads[i], INFINITE);
	}
}

// Close all open handles
void CloseMyHandles(HANDLE* handles, int nr) {
	for (int i = 0; i < nr; i++)
		CloseHandle(handles[i]);
}

// Unmaps all views
void UnmapAllViews(HANDLE* views, int nr) {
	for (int i = 0; i < nr; i++) {
		UnmapViewOfFile(views[i]);
	}
}


int _tmain(int argc, TCHAR* argv[]) {

	Cell map[MIN_LIN * MIN_COL];
	int nrMaxTaxis = MAX_TAXIS; // colocar variavel dentro de uma dll (?)
	int nrMaxPassengers = MAX_PASSENGERS;

	HANDLE views[50];
	HANDLE handles[50];
	HANDLE threads[50];
	int viewCounter = 0, handleCounter = 0, threadCounter = 0;
	int taxiFreePosition = 0;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	// ====================================================================================================
	// #TODO - needs review
	HANDLE taxiCanTalkSemaphore = CreateSemaphore(NULL, 0, MAX_TAXIS, TAXI_CAN_TALK);
	if (taxiCanTalkSemaphore == NULL) {
		_tprintf(_T("Error creating taxiCanTalkSemaphore (%d)\n"), GetLastError());
		_gettch();
		exit(-1);
	}
	// Check if this mechanism is already create
	// if GetLastError() returns ERROR_ALREADY_EXISTS
	// this is the second instance running
	else if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(_T("You can't run multiple instances of this application.\n"));
		_gettch();
		exit(-1);
	}

	handles[handleCounter++] = taxiCanTalkSemaphore;

	// ====================================================================================================

	// #TODO é possivel configurar só um dos argumentos?
	if (argc > 2) {
		nrMaxTaxis = _ttoi(argv[1]);
		nrMaxPassengers = _ttoi(argv[2]);
	}
	_tprintf(_T("Nr max de taxis: %.2d\n"), nrMaxTaxis);
	_tprintf(_T("Nr max de passengers: %.2d\n"), nrMaxPassengers);

	Taxi* taxis = (Taxi*)malloc(nrMaxTaxis * sizeof(Taxi));
	if (taxis == NULL) {
		_tprintf(_T("Error allocating memory (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	_tprintf(_T("Memory allocated successfully.\n"));

	for (int i = 0; i < nrMaxTaxis; i++) {
		taxis[i].location.x = -1;
		taxis[i].location.y = -1;
	}

	Passenger* passengers = (Passenger*)malloc(nrMaxPassengers * sizeof(Passenger));
	if (passengers == NULL) {
		_tprintf(_T("Error allocating memory (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	_tprintf(_T("Memory allocated successfully.\n"));

	TI_Controldata ti_ControlData;
	ti_ControlData.gate = FALSE;
	ti_ControlData.map = map;
	ti_ControlData.taxis = taxis;
	ti_ControlData.taxiCount = &taxiFreePosition;

	HANDLE consoleThread = CreateThread(NULL, 0, TextInterface, &ti_ControlData, 0, NULL);
	if (!consoleThread) {
		_tprintf(_T("Error launching console thread (%d)\n"), GetLastError());
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	threads[threadCounter++] = consoleThread;

	HANDLE fmCenTaxiToConTaxi = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_CC_REQUEST),
		SHM_CC_REQUEST_NAME);
	if (fmCenTaxiToConTaxi == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = fmCenTaxiToConTaxi;

	HANDLE FM_CC_RESPONSE = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_CC_RESPONSE),
		SHM_CC_RESPONSE_NAME);
	if (fmCenTaxiToConTaxi == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = FM_CC_RESPONSE;

	CC_CDRequest controlDataTaxi;
	CC_CDResponse cdResponse;

	controlDataTaxi.shared = (SHM_CC_REQUEST*) MapViewOfFile(fmCenTaxiToConTaxi,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_CC_REQUEST));
	if (controlDataTaxi.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = controlDataTaxi.shared;

	cdResponse.shared = (SHM_CC_REQUEST*)MapViewOfFile(FM_CC_RESPONSE,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_CC_REQUEST));
	if (cdResponse.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = cdResponse.shared;

	controlDataTaxi.mutex = CreateMutex(NULL, FALSE, CENTRAL_MUTEX);
	if (controlDataTaxi.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = controlDataTaxi.mutex;

	cdResponse.mutex = CreateMutex(NULL, FALSE, RESPONSE_MUTEX);
	if (controlDataTaxi.mutex == NULL) {
		_tprintf(TEXT("Error creating cdResponse mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = cdResponse.mutex;

	controlDataTaxi.read_event = CreateEvent(NULL, FALSE, FALSE, EVENT_READ_FROM_TAXIS);
	//controlDataTaxi.read_event = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_READ_FROM_TAXIS);
	if (controlDataTaxi.read_event == NULL) {
		_tprintf(_T("Error creating read event (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	handles[handleCounter++] = controlDataTaxi.read_event;

	cdResponse.got_response = CreateEvent(NULL, FALSE, FALSE, EVENT_GOT_RESPONSE);
	//controlDataTaxi.read_event = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_READ_FROM_TAXIS);
	if (cdResponse.got_response == NULL) {
		_tprintf(_T("Error creating got_response event (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	handles[handleCounter++] = cdResponse.got_response;

	controlDataTaxi.write_event = CreateEvent(NULL, FALSE, FALSE, EVENT_WRITE_FROM_TAXIS);
	//controlDataTaxi.write_event = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_WRITE_FROM_TAXIS);
	if (controlDataTaxi.write_event == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = controlDataTaxi.write_event;

	CDThread cdThread;
	cdThread.taxis = taxis;
	cdThread.controlDataTaxi = controlDataTaxi;
	cdThread.taxiFreePosition = &taxiFreePosition;
	cdThread.map = map;
	cdThread.cdResponse = cdResponse;

	HANDLE listenThread = CreateThread(NULL, 0, ListenToTaxis, &cdThread, 0, NULL);
	if (!listenThread) {
		_tprintf(_T("Error launching console thread (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	threads[threadCounter++] = listenThread;

	char* fileContent = NULL;
	// Le conteudo do ficheiro para array de chars
	if ((fileContent = ReadFileToCharArray(_T("E:\\projects\\so2-project\\maps\\map1.txt"))) == NULL) {
		_tprintf(_T("Error reading the file (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	else {
		_tprintf(_T("Map loaded to memory!\n"));
	}

	// Preenche mapa com o conteudo do ficheiro
	LoadMapa(map, fileContent);

	WaitAllThreads(threads, threadCounter);
	UnmapAllViews(views, viewCounter);
	CloseMyHandles(handles, handleCounter);

	free(taxis);
	free(passengers);
	return 0;
}

