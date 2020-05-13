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


// Wait for all the threads to stop
void WaitAllThreads(HANDLE* threads, int *nr) {
	for (int i = 0; i < nr; i++) 
		WaitForSingleObject(threads[i], INFINITE);
	*(nr) = 0;
}

// Close all open handles
void CloseMyHandles(HANDLE* handles, int *nr) {
	for (int i = 0; i < nr; i++)
		CloseHandle(handles[i]);
	*(nr) = 0;
}

// Unmaps all views
void UnmapAllViews(HANDLE* views, int *nr) {
	for (int i = 0; i < nr; i++)
		UnmapViewOfFile(views[i]);
	*(nr) = 0;
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
}

// função-thread responsável por tratar a interação com o admin
DWORD WINAPI TextInterface(LPVOID ptr) {

	TI_Controldata* cdata = (TI_Controldata*)ptr;
	TCHAR command[100];
	
	while (1) {
		//ClearScreen();
		PrintMap(cdata->map);
		_tprintf(_T("Command: "));
		_tscanf_s(_T(" %99[^\n]"), command, sizeof(TCHAR)*100);
		FindFeatureAndRun(command, cdata);
	}

	return 0;
}

// insere taxi na primeira posição livre
enum respond_id InsertTaxiIntoMapCell(Cell* cell, Taxi taxi, int size) {
	for (unsigned int i = 0; i < size; i++) {
		if (cell->taxis[i].location.x == -1) {
			if (cell->cellType == Building) return INVALID_REGISTRATION_TAXI_POSITION;
			cell->display = T_DISPLAY;
			CopyMemory(cell->taxis[i].licensePlate, taxi.licensePlate, sizeof(taxi.licensePlate));
			cell->taxis[i].location.x = taxi.location.x;
			cell->taxis[i].location.y = taxi.location.y;
			cell->taxis[i].autopilot = taxi.autopilot;
			cell->taxis[i].velocity = taxi.velocity;
			return OK;
		}
	}
}

void RemoveTaxiFromMapCell(Cell* cell, TCHAR* lcPlate, int size) {
	Taxi aux;
	for (unsigned int i = 0; i < size; i++) {
		aux = (cell + i)->taxis[i];
		if (_tcscmp(aux.licensePlate, lcPlate) == 0) {
			ZeroMemory(&aux.licensePlate, sizeof(aux.licensePlate));
			aux.location.x = -1;
			aux.location.y = -1;
		}
	}
}
SHM_CC_RESPONSE ParseAndExecuteOperation(CDThread* cd, enum message_id action, Content content) {
	SHM_CC_RESPONSE response;
	response.action = ERRO;
	int index, x, y;
	_tprintf(_T("Action request: %d\n"), action);
	switch (action) {
			case UpdateTaxiLocation:
				// #TODO
				// algoritmo
				// encontrar taxi atual
				// atualizar display da célula atual do taxi para estrada
				// mudar coordenadas do taxi para coordenadas destino
				// mudar display da celula da nova posição do taxi

				// encontrar o taxi na tabela de taxis ativos
				index = FindTaxiIndex(cd->taxis, *(cd->taxiFreePosition), content.taxi);
				// se nao encontrar devolve erro
				if (index == -1) break;
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
				InsertTaxiIntoMapCell((cd->map + (x + y * MIN_LIN)), content.taxi, cd->nrMaxTaxis);
				// atualizar tabela de taxis ativos
				(cd->taxis + index)->location.x = content.taxi.location.x;
				(cd->taxis + index)->location.y = content.taxi.location.y;
				(cd->taxis + index)->autopilot = content.taxi.autopilot;
				(cd->taxis + index)->velocity = content.taxi.velocity;
				_tprintf(_T("Changing taxi position...\n"));
				response.action = OK;
				break;
			case WarnPassengerCatch:
				break;
			case WarnPassengerDeliever:
				break;
			case GetCityMap:
				CopyMemory(response.map, cd->charMap, sizeof(cd->charMap));
				_tprintf(_T("Got a map request...\n"));
				response.action = OK;
				break;
	}
	return response;
}

DWORD WINAPI TalkToTaxi(LPVOID ptr) {
	CDThread* cd= (CDThread*)ptr;
	CC_CDRequest* request = cd->comm->request;
	CC_CDResponse* response = cd->comm->response;
	SHM_CC_REQUEST shm_request;
	SHM_CC_RESPONSE shm_response;
	while (1) {

		WaitForSingleObject(response->new_request, INFINITE);
		WaitForSingleObject(request->mutex, INFINITE);

		// Guardar o conteudo da mensagem
		CopyMemory(&shm_request.messageContent, &request->shared, sizeof(Content));
		shm_request.action = request->shared->action;
		ReleaseMutex(request->mutex);

		// Tratar mensagem
		shm_response = ParseAndExecuteOperation(cd, shm_request.action, shm_request.messageContent);

		WaitForSingleObject(response->mutex, INFINITE);
		CopyMemory(&response->shared->map, &shm_response.map, sizeof(char) * MIN_LIN * MIN_COL);
		response->shared->action = shm_response.action;

		ReleaseMutex(response->mutex);
		SetEvent(request->new_response);
	}
}

void RespondToTaxiLogin(CDThread* cdThread, TCHAR* licensePlate, HContainer* container, enum response_id resp) {
	LR_Container res;
	CDLogin_Request* request = cdThread->cdLogin_Request;
	CDLogin_Response* response = cdThread->cdLogin_Response;
	CC_CDRequest cdRequest;
	CC_CDResponse cdResponse;

	TCHAR request_event_name[50];
	TCHAR request_mutex_name[50];
	TCHAR request_shm_name[50];
	TCHAR response_event_name[50];
	TCHAR response_mutex_name[50];
	TCHAR response_shm_name[50];

	WaitForSingleObject(response->login_m, INFINITE);

	// Build the names
	response->response->action = resp;
	if (resp == OK) {
		_stprintf(request_event_name, EVENT_REQUEST, licensePlate);
		_stprintf(request_mutex_name, REQUEST_MUTEX, licensePlate);
		_stprintf(request_shm_name, SHM_CC_REQUEST_NAME, licensePlate);
		_stprintf(response_event_name, EVENT_RESPONSE, licensePlate);
		_stprintf(response_mutex_name, RESPONSE_MUTEX, licensePlate);
		_stprintf(response_shm_name, SHM_CC_RESPONSE_NAME, licensePlate);

		// Transfer the names to the container
		CopyMemory(res.request_event_name, request_event_name, sizeof(TCHAR) * 50); // 23
		CopyMemory(res.request_mutex_name, request_mutex_name, sizeof(TCHAR) * 50); // 23
		CopyMemory(res.request_shm_name, request_shm_name, sizeof(TCHAR) * 50); // 24

		CopyMemory(res.response_event_name, response_event_name, sizeof(TCHAR) * 50); // 23
		CopyMemory(res.response_mutex_name, response_mutex_name, sizeof(TCHAR) * 50); // 23 
		CopyMemory(res.response_shm_name, response_shm_name, sizeof(TCHAR) * 50); // 24

		CopyMemory(&response->response->container, &res, sizeof(LR_Container));
	}
	ReleaseMutex(response->login_m);

	if (resp == OK) {

		HANDLE FM_REQUEST = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			sizeof(SHM_CC_REQUEST),
			request_shm_name);
		if (FM_REQUEST == NULL) {
			_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = FM_REQUEST;

		cdRequest.shared = (SHM_CC_REQUEST*)MapViewOfFile(FM_REQUEST,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			sizeof(SHM_CC_REQUEST));
		if (cdRequest.shared == NULL) {
			_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->views[(*container->viewCounter)++] = cdRequest.shared;

		cdRequest.mutex = CreateMutex(NULL, FALSE, request_mutex_name);
		if (cdRequest.mutex == NULL) {
			_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdRequest.mutex;

		cdRequest.new_response = CreateEvent(NULL, FALSE, FALSE, response_event_name);
		if (request->new_response == NULL) {
			_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdRequest.new_response;

		HANDLE FM_RESPONSE = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			sizeof(SHM_CC_RESPONSE),
			response_shm_name);
		if (FM_RESPONSE == NULL) {
			_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = FM_RESPONSE;

		cdResponse.shared = (SHM_CC_REQUEST*)MapViewOfFile(FM_RESPONSE,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			sizeof(SHM_CC_RESPONSE));
		if (cdResponse.shared == NULL) {
			_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->views[(*container->viewCounter)++] = cdResponse.shared;

		cdResponse.mutex = CreateMutex(NULL, FALSE, response_mutex_name);
		if (cdResponse.mutex == NULL) {
			_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdResponse.mutex;

		cdResponse.new_request = CreateEvent(NULL, FALSE, FALSE, request_event_name);
		if (cdResponse.new_request == NULL) {
			_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdResponse.new_request;

		CC_Comm comm;
		comm.container = &res;
		comm.request = &cdRequest;
		comm.response = &cdResponse;
		cdThread->comm = &comm;

		if (container->threads[(*container->threadCounter)++] = CreateThread(NULL, 0, TalkToTaxi, cdThread, 0, NULL) == NULL) {
			_tprintf(_T("Error launching comm thread (%d)\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
	}

	SetEvent(request->new_response);

	_tprintf(_T("[LOG] Sent response to the taxi's registration request.\n"));
}

//void RespondToTaxi(CC_CDResponse response, Content* content, enum response_id responseId) {
//
//	WaitForSingleObject(response.mutex, INFINITE);
//	
//	response.shared->action = responseId;
//	CopyMemory(&response.shared->responseContent, content, sizeof(Content));
//
//	ReleaseMutex(response.mutex);
//
//	SetEvent(response.got_response);
//
//}

int FindTaxiIndex(Taxi* taxis, int size, Taxi target) {
	for (unsigned int i = 0; i < size; i++)
		if (_tcscmp((taxis + i)->licensePlate, target.licensePlate) == 0)
			return i;
	return -1;
}

DWORD WINAPI ListenToLoginRequests(LPVOID ptr) {
	CDThread* cd = (CDThread*)ptr;
	CDLogin_Request* cdata = cd->cdLogin_Request;
	CDLogin_Response* cdResponse = cd->cdLogin_Response;
	SHM_LOGIN_REQUEST shared;

	while (1) {
		WaitForSingleObject(cdResponse->new_request, INFINITE);
		//_tprintf(_T("Got something written to me\n"));
		
		//_tprintf(_T("Waiting for access to the mutex\n"));
		WaitForSingleObject(cdata->login_m, INFINITE);
		//_tprintf(_T("Got access to the mutex\n"));

		// Save Request
		CopyMemory(&shared, cdata->request, sizeof(SHM_LOGIN_REQUEST));

		ReleaseMutex(cdata->login_m);
		int index = *(cd->taxiFreePosition);

		// Process Event

		if (shared.action == RegisterTaxiInCentral) {
			_tprintf(_T("Got a registration request from '%s'\n"), shared.taxi.licensePlate);
			if (shared.taxi.location.x > MIN_COL || shared.taxi.location.y > MIN_LIN) {
				RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, OUTOFBOUNDS_TAXI_POSITION);
				continue;
			}
			// inserir taxi na sua posição atual 
			Cell* cell = cd->map + (shared.taxi.location.x + shared.taxi.location.y * MIN_COL);
			enum responde_id res;

			if ((res = InsertTaxiIntoMapCell(cell, shared.taxi, cd->nrMaxTaxis)) != OK)
				RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, res);
			else {
				// inserir o taxi dentro da lista de taxis ativos
				CopyMemory(&cd->taxis[index], &shared.taxi, sizeof(Taxi));

				// notifiquei o taxi a dizer que a mensagem foi lida
				RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, res);
			}
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
		// @TODO notify todas as apps
		_tprintf(_T("System is closing.\n"));
	}
	else if (_tcscmp(cmd, ADM_LIST) == 0) {
		for (unsigned int i = 0; i < cdata->taxiCount; i++) {
			if (cdata->taxis[i].location.x > 0)
				_tprintf(_T("%.2d - %9s at {%.2d;%.2d}\n"), i, cdata->taxis[i].licensePlate, cdata->taxis[i].location.x, cdata->taxis[i].location.y);
		}

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
		else {
			_tprintf(_T("System changing the wait time of a taxi request to %d\n"), argument);
			(*cdata->WaitTimeOnTaxiRequest) = argument;
		}
	}
	else if (_tcscmp(cmd, ADM_HELP) == 0) {
		for (int i = 0; i < 6; i++)
			_tprintf(_T("%s"), commands[i]);
	}
	else {
		_tprintf(_T("System doesn't recognize the command, type 'help' to view all the commands.\n"));
	}
}

void LoadMapa(Cell* map, char* buffer, int nrTaxis, int nrPassangers) {
	int aux=0;
	for (int i = 0; i < MIN_LIN; i++) {
		for (int j = 0; j < MIN_COL; j++) {
			aux = (i * MIN_COL) + j;
			char c = buffer[aux];
			if (c == S_CHAR) {
				map[aux].taxis = malloc(nrTaxis * sizeof(Taxi));
				for (unsigned int i = 0; i < nrTaxis; i++) {
					map[aux].taxis[i].location.x = -1;
					map[aux].taxis[i].location.y = -1;
				}
				map[aux].passengers = malloc(nrPassangers * sizeof(Passenger));
				for (unsigned int i = 0; i < nrPassangers; i++) {
					map[aux].passengers[i].location.x = -1;
					map[aux].passengers[i].location.y = -1;
				}
				map[aux].display = S_DISPLAY;
				map[aux].cellType = Street;
				map[aux].x = j;
				map[aux].y = i;
			}
			else if (c == B_CHAR) {
				map[aux].taxis = malloc(nrTaxis * sizeof(Taxi));
				for (unsigned int i = 0; i < nrTaxis; i++) {
					map[aux].taxis[i].location.x = -1;
					map[aux].taxis[i].location.y = -1;
				}
				map[aux].passengers = malloc(nrPassangers * sizeof(Passenger));
				for (unsigned int i = 0; i < nrPassangers; i++) {
					map[aux].passengers[i].location.x = -1;
					map[aux].passengers[i].location.y = -1;
				}
				map[aux].display = B_DISPLAY;
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

char** ConvertMapIntoCharMap(Cell* map) {
	char charMap[MIN_LIN][MIN_COL];

	for (unsigned int i = 0; i < MIN_LIN; i++)
		for (unsigned int j = 0; j < MIN_COL; j++){
			char aux = (*(map + (j + i * MIN_COL))).display;
			if (aux == B_DISPLAY)
				charMap[i][j] = B_DISPLAY;
			else if (aux == S_DISPLAY)
				charMap[i][j] = S_DISPLAY;
		}
	return charMap;
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
	int WaitTimeOnTaxiRequest = 0;

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
	if (argc >= 2) {

		if (_tcscmp(argv[1], _T("-t"))==0)
			nrMaxTaxis = _ttoi(argv[2]);
		else if (_tcscmp(argv[1], _T("-p"))==0)
			nrMaxPassengers = _ttoi(argv[2]);

		if (_tcscmp(argv[3], _T("-t"))==0)
			nrMaxTaxis = _ttoi(argv[4]);
		else if (_tcscmp(argv[3], _T("-p"))==0)
			nrMaxPassengers = _ttoi(argv[4]);
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

	Passenger* passengers = (Passenger*)malloc(nrMaxPassengers * sizeof(Passenger));
	if (passengers == NULL) {
		_tprintf(_T("Error allocating memory (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

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
	LoadMapa(map, fileContent, nrMaxTaxis, nrMaxPassengers);

	TI_Controldata ti_ControlData;
	ti_ControlData.gate = FALSE;
	ti_ControlData.map = map;
	ti_ControlData.taxis = taxis;
	ti_ControlData.taxiCount = nrMaxTaxis;
	ti_ControlData.WaitTimeOnTaxiRequest = &WaitTimeOnTaxiRequest;

	HANDLE consoleThread = CreateThread(NULL, 0, TextInterface, &ti_ControlData, 0, NULL);
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

	CDLogin_Request.login_m = CreateMutex(NULL, FALSE, LOGIN_REQUEST_MUTEX);
	if (CDLogin_Request.login_m == NULL) {
		_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Request.login_m;

	CDLogin_Response.login_m = CreateMutex(NULL, FALSE, LOGIN_RESPONSE_MUTEX);
	if (CDLogin_Response.login_m == NULL) {
		_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Response.login_m;

	CDLogin_Request.login_write_m = CreateMutex(NULL, FALSE, LOGIN_TAXI_WRITE_MUTEX);
	if (CDLogin_Request.login_write_m == NULL) {
		_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Request.login_write_m;

	CDLogin_Request.new_response = CreateEvent(NULL, FALSE, FALSE, EVENT_WRITE_FROM_TAXIS);
	if (CDLogin_Request.new_response == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Request.new_response;

	CDLogin_Response.new_request = CreateEvent(NULL, FALSE, FALSE, EVENT_READ_FROM_TAXIS);
	if (CDLogin_Response.new_request == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = CDLogin_Response.new_request;


	CDThread cdThread;
	cdThread.taxis = taxis;
	cdThread.taxiFreePosition = &taxiFreePosition;
	cdThread.map = map;
	cdThread.cdLogin_Response = &CDLogin_Response;
	cdThread.cdLogin_Request = &CDLogin_Request;
	cdThread.nrMaxTaxis = nrMaxTaxis;
	CopyMemory(cdThread.charMap, ConvertMapIntoCharMap(map), sizeof(char)*MIN_LIN*MIN_COL);

	HContainer container;
	container.threads = threads;
	container.views = views;
	container.handles = handles;
	container.threadCounter = &threadCounter;
	container.viewCounter = &viewCounter;
	container.handleCounter = &handleCounter;
	cdThread.hContainer = &container;

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

	// dar free da memoria dos taxis em todas as celulas
	for (unsigned int i = 0; i < MIN_LIN * MIN_COL; i++) {
		free(map[i].taxis);
		free(map[i].passengers);
	}
	free(taxis);
	free(passengers);
	return 0;
}

