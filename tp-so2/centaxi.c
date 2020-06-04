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
#include "comm.h"

int _tmain(int argc, TCHAR* argv[]) {
	Cell map[MIN_LIN * MIN_COL];
	int nrMaxTaxis = MAX_TAXIS; // colocar variavel dentro de uma dll (?)
	int nrMaxPassengers = MAX_PASSENGERS;

	HANDLE views[50];
	HANDLE handles[50];
	HANDLE threads[50];
	int viewCounter = 0, handleCounter = 0, threadCounter = 0;
	int taxiFreePosition = 0;
	int WaitTimeOnTaxiRequest = WAIT_TIME;
	DLLMethods dllMethods;
	HINSTANCE hDll;
	srand(time(0));

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
	if (argc >= 2) {
		if (_tcscmp(argv[1], _T("-t")) == 0)
			nrMaxTaxis = _ttoi(argv[2]);
		else if (_tcscmp(argv[1], _T("-p")) == 0)
			nrMaxPassengers = _ttoi(argv[2]);
		if (argc > 3) {
			if (_tcscmp(argv[3], _T("-t")) == 0)
				nrMaxTaxis = _ttoi(argv[4]);
			else if (_tcscmp(argv[3], _T("-p")) == 0)
				nrMaxPassengers = _ttoi(argv[4]);
		}
	}
	_tprintf(_T("Nr max de taxis: %.2d\n"), nrMaxTaxis);
	_tprintf(_T("Nr max de passengers: %.2d\n"), nrMaxPassengers);

	// ====================================================================================================
	HANDLE taxiGate = CreateSemaphore(NULL, nrMaxTaxis, nrMaxTaxis, TAXI_GATEWAY);
	if (taxiGate == NULL) {
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
	handles[handleCounter++] = taxiGate;

	// ====================================================================================================

	hDll = LoadLibraryA(".\\SO2_TP_DLL_64.dll");
	if (hDll == NULL) {
		_tprintf(_T("Error loading dll!\n"));
		CloseMyHandles(handles, &handleCounter);
		exit(-1);
	}

	dllMethods.Register = (void (*)(TCHAR*, int)) GetProcAddress(hDll, "dll_register");
	if (dllMethods.Register == NULL) {
		_tprintf(_T("Error getting pointer to dll function.\n"));
		CloseMyHandles(handles, &handleCounter);
		exit(-1);
	}

	dllMethods.Log = (void (*)(TCHAR*)) GetProcAddress(hDll, "dll_log");
	if (dllMethods.Log == NULL) {
		_tprintf(_T("Error getting pointer to dll function.\n"));
		CloseMyHandles(handles, &handleCounter);
		exit(-1);
	}

	dllMethods.Test = (void (*)(void)) GetProcAddress(hDll, "dll_test");
	if (dllMethods.Test == NULL) {
		_tprintf(_T("Error getting pointer to dll function.\n"));
		CloseMyHandles(handles, &handleCounter);
		exit(-1);
	}
	// ====================================================================================================

	Taxi* taxis = (Taxi*)malloc(nrMaxTaxis * sizeof(Taxi));
	if (taxis == NULL) {
		_tprintf(_T("Error allocating memory (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	for (unsigned int i = 0; i < nrMaxTaxis; i++) {
		ZeroMemory(taxis[i].licensePlate, sizeof(taxis[i].licensePlate));
		taxis[i].location.x = -1;
		taxis[i].location.y = -1;
		taxis[i].velocity = -1;
		taxis[i].autopilot = FALSE;
	}

	Passenger* passengers = (Passenger*)malloc(nrMaxPassengers * sizeof(Passenger));
	if (passengers == NULL) {
		_tprintf(_T("Error allocating memory (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	for (unsigned int i = 0; i < nrMaxPassengers; i++) {
		ZeroMemory(passengers[i].nome, sizeof(passengers[i].nome));
		passengers[i].location.x = -1;
		passengers[i].location.y = -1;
	}
	// ====================================================================================================

	CDThread cdThread;

	cdThread.hPassPipeRegister = CreateNamedPipe(NP_PASS_REGISTER, PIPE_ACCESS_OUTBOUND | PIPE_ACCESS_DUPLEX, PIPE_WAIT |
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, nrMaxPassengers, sizeof(PassMessage), sizeof(PassMessage), 1000, NULL);

	if (cdThread.hPassPipeRegister == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe) %d"), GetLastError());
		CloseMyHandles(handles, handleCounter);
		Sleep(2000);
		return -1;
	}

	cdThread.hPassPipeTalk = CreateNamedPipe(NP_PASS_TALK, PIPE_ACCESS_OUTBOUND | PIPE_ACCESS_DUPLEX, PIPE_WAIT |
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, nrMaxPassengers, sizeof(PassMessage), sizeof(PassMessage), 1000, NULL);

	if (cdThread.hPassPipeTalk == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)"));
		CloseMyHandles(handles, handleCounter);
		return -1;
	}

	// operação bloqueante (fica à espera da ligação de um cliente)
	if (!ConnectNamedPipe(cdThread.hPassPipeRegister, NULL)) {
		_tprintf(TEXT("[ERRO] Ligação ao leitor 1! (ConnectNamedPipe %d).\n"), GetLastError());
		Sleep(20000);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	if (!ConnectNamedPipe(cdThread.hPassPipeTalk, NULL)) {
		_tprintf(TEXT("[ERRO] Ligação ao leitor 2! (ConnectNamedPipe %d).\n"), GetLastError());
		Sleep(20000);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	
	HANDLE FM_BROADCAST = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_BROADCAST),
		SHM_BROADCAST_PASSENGER_ARRIVE);
	if (FM_BROADCAST == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = FM_BROADCAST;
	dllMethods.Register(SHM_BROADCAST_PASSENGER_ARRIVE, FILE_MAPPING);

	CC_Broadcast broadcast;

	broadcast.shared = (SHM_BROADCAST*)MapViewOfFile(FM_BROADCAST,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_BROADCAST));
	if (broadcast.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = broadcast.shared;
	dllMethods.Register(SHM_BROADCAST_PASSENGER_ARRIVE, VIEW);

	broadcast.mutex = CreateMutex(NULL, FALSE, BROADCAST_MUTEX);
	if (broadcast.mutex == NULL) {
		_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = broadcast.mutex;
	dllMethods.Register(BROADCAST_MUTEX, MUTEX);

	broadcast.new_passenger = CreateSemaphore(NULL, 0, nrMaxTaxis, EVENT_NEW_PASSENGER);
	if (broadcast.new_passenger  == NULL) {
		_tprintf(_T("Error creating new passenger event (%d)\n"), GetLastError());
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = broadcast.new_passenger;
	dllMethods.Register(EVENT_NEW_PASSENGER, SEMAPHORE);

	char* fileContent = NULL;
	// Le conteudo do ficheiro para array de chars
	if ((fileContent = ReadFileToCharArray(_T(/*".\\..\\..\\maps\\mapa.txt"*/"E:\\projects\\so2-project\\maps\\mapa.txt"))) == NULL) {
		_tprintf(_T("Error reading the file (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	// Preenche mapa com o conteudo do ficheiro
	LoadMapa(map, fileContent, nrMaxTaxis, nrMaxPassengers);

	cdThread.dllMethods = &dllMethods;
	cdThread.taxis = taxis;
	cdThread.map = map;
	cdThread.nrMaxTaxis = nrMaxTaxis;
	cdThread.nrMaxPassengers = nrMaxPassengers;
	cdThread.passengers = passengers;
	cdThread.WaitTimeOnTaxiRequest = &WaitTimeOnTaxiRequest;
	cdThread.broadcast = &broadcast;
	cdThread.areTaxisRequestsPause = FALSE;
	cdThread.isSystemClosing = FALSE;

	if ((cdThread.requests = (SHM_CC_REQUEST*)malloc(nrMaxTaxis * sizeof(SHM_CC_REQUEST)) == NULL)) {
		_tprintf(_T("Error allocating memory for requests's array.\n"));
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
	}

	HANDLE cthread = CreateThread(NULL, 0, GetPassengerRegistration, &cdThread, 0, NULL);
	if (!cthread) {
		_tprintf(_T("Error launching console thread (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	threads[threadCounter++] = cthread;

	HANDLE consoleThread = CreateThread(NULL, 0, TextInterface, &cdThread, 0, NULL);
	if (!consoleThread) {
		_tprintf(_T("Error launching console thread (%d)\n"), GetLastError());
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	threads[threadCounter++] = consoleThread;

	HANDLE FM_CC_LOGIN_REQUEST = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_LOGIN_REQUEST),
		SHM_LOGIN_REQUEST_NAME);
	if (FM_CC_LOGIN_REQUEST == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = FM_CC_LOGIN_REQUEST;
	dllMethods.Register(SHM_LOGIN_REQUEST_NAME, FILE_MAPPING);

	HANDLE FM_CC_LOGIN_RESPONSE = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_LOGIN_RESPONSE),
		SHM_LOGIN_RESPONSE_NAME);
	if (FM_CC_LOGIN_RESPONSE == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = FM_CC_LOGIN_RESPONSE;
	dllMethods.Register(SHM_LOGIN_RESPONSE_NAME, FILE_MAPPING);

	CDLogin_Request CDLogin_Request;
	CDLogin_Response CDLogin_Response;

	CDLogin_Request.request = (SHM_LOGIN_REQUEST*)MapViewOfFile(FM_CC_LOGIN_REQUEST,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_LOGIN_REQUEST));
	if (CDLogin_Request.request == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = CDLogin_Request.request;
	dllMethods.Register(SHM_LOGIN_REQUEST_NAME, VIEW);

	CDLogin_Response.response = (SHM_LOGIN_RESPONSE*)MapViewOfFile(FM_CC_LOGIN_RESPONSE,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_LOGIN_RESPONSE));
	if (CDLogin_Response.response == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = CDLogin_Response.response;
	dllMethods.Register(SHM_LOGIN_RESPONSE_NAME, VIEW);

	CDLogin_Request.login_m = CreateMutex(NULL, FALSE, LOGIN_REQUEST_MUTEX);
	if (CDLogin_Request.login_m == NULL) {
		_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Request.login_m;
	dllMethods.Register(LOGIN_REQUEST_MUTEX, MUTEX);

	CDLogin_Response.login_m = CreateMutex(NULL, FALSE, LOGIN_RESPONSE_MUTEX);
	if (CDLogin_Response.login_m == NULL) {
		_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Response.login_m;
	dllMethods.Register(LOGIN_RESPONSE_MUTEX, MUTEX);

	CDLogin_Request.login_write_m = CreateMutex(NULL, FALSE, LOGIN_TAXI_WRITE_MUTEX);
	if (CDLogin_Request.login_write_m == NULL) {
		_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Request.login_write_m;
	dllMethods.Register(LOGIN_TAXI_WRITE_MUTEX, MUTEX);

	CDLogin_Request.new_response = CreateEvent(NULL, FALSE, FALSE, EVENT_WRITE_FROM_TAXIS);
	if (CDLogin_Request.new_response == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Request.new_response;
	dllMethods.Register(EVENT_WRITE_FROM_TAXIS, EVENT);

	CDLogin_Response.new_request = CreateEvent(NULL, FALSE, FALSE, EVENT_READ_FROM_TAXIS);
	if (CDLogin_Response.new_request == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Response.new_request;
	dllMethods.Register(EVENT_READ_FROM_TAXIS, EVENT);

	cdThread.cdLogin_Response = &CDLogin_Response;
	cdThread.cdLogin_Request = &CDLogin_Request;
	CopyMemory(cdThread.charMap, ConvertMapIntoCharMap(map), sizeof(char)*MIN_LIN*MIN_COL);

	HContainer container;
	container.threads = threads;
	container.views = views;
	container.handles = handles;
	container.threadCounter = &threadCounter;
	container.viewCounter = &viewCounter;
	container.handleCounter = &handleCounter;
	cdThread.hContainer = &container;
	cdThread.dllMethods = &dllMethods;

	HANDLE listenThread = CreateThread(NULL, 0, ListenToLoginRequests, &cdThread, 0, NULL);
	if (!listenThread) {
		_tprintf(_T("Error launching console thread (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	threads[threadCounter++] = listenThread;

	WaitAllThreads(threads, threadCounter);
	UnmapAllViews(views, viewCounter);
	CloseMyHandles(handles, handleCounter);

	cdThread.dllMethods->Test();

	// dar free da memoria dos taxis em todas as celulas
	for (unsigned int i = 0; i < MIN_LIN * MIN_COL; i++) {
		free(map[i].taxis);
		free(map[i].passengers);
	}
	free(taxis);
	free(passengers);
	return 0;
}

