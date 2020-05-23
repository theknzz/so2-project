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

void SendBroadCastMessage(CC_Broadcast* broadcast, SHM_BROADCAST* message, int nr) {

	WaitForSingleObject(broadcast->mutex, INFINITE);

	CopyMemory(broadcast->shared, message, sizeof(SHM_BROADCAST));

	ReleaseMutex(broadcast->mutex);

	ReleaseSemaphore(broadcast->new_passenger, (LONG)nr, NULL);
}

// função-thread responsável por tratar a interação com o admin
DWORD WINAPI TextInterface(LPVOID ptr) {
	CDThread* cdata = (CDThread*)ptr;
	TCHAR command[100];
	do {
		PrintMap(cdata->map);
		_tprintf(_T("Command: "));
		_tscanf(_T(" %[^\n]"), command);
		if (FindFeatureAndRun(command, cdata) == -1) break;
	} while (!cdata->isSystemClosing);
	return 0;
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


SHM_CC_RESPONSE ParseAndExecuteOperation(CDThread* cd, enum message_id action, Content content) {
	SHM_CC_RESPONSE response;
	response.action = ERRO;
	int index, x, y;
	enum passanger_state status;
	//_tprintf(_T("Action request: %d\n"), action);
	switch (action) {
			case UpdateTaxiLocation:
				// encontrar o taxi na tabela de taxis ativos
				index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
				// se nao encontrar devolve erro
				if (index == -1) {
					response.action = TAXI_KICKED;
					break;
				}
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
				if (content.taxi.client.location.x >= 0) {
					cd->taxis[index].client.location.x = content.taxi.location.x;
					cd->taxis[index].client.location.y = content.taxi.location.y;
					// update the passenger position inside the map cell
					index = FindTaxiIndex(cd->map[x + y * MIN_LIN].taxis, cd->nrMaxTaxis, content.taxi);
					if (index == -1) break;
					cd->map[x + y * MIN_LIN].taxis[index].client.location.x = x;
					cd->map[x + y * MIN_LIN].taxis[index].client.location.y = y;
					// update the passenger position inside the list of passengers
					index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.taxi.client.nome);
					if (index == -1) break;
					cd->passengers[index].location.x = x;
					cd->passengers[index].location.y = y;
				}
				response.action = OK;
				break;
			case WarnPassengerCatch:
				// change the state of the taxi's client
				index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
				if (index == -1) {
					response.action = TAXI_KICKED;
					break;
				}
				x = (*(cd->taxis + index)).location.x;
				y = (*(cd->taxis + index)).location.y;
				cd->taxis[index].client.state = OnDrive;

				// update state of the passenger in the list of passengers
				index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.taxi.client.nome);
				if (index == -1) break;
				cd->passengers[index].state = OnDrive;

				// update the state of the passenger in the map cell
				index = FindTaxiIndex(cd->map[x + y * MIN_LIN].taxis, cd->nrMaxTaxis, content.taxi);
				if (index == -1) break;
				cd->map[x + y * MIN_LIN].taxis[index].client.state = OnDrive;
				response.action = OK;
				break;
			case WarnPassengerDeliever:
				index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
				if (index == -1) {
					response.action = TAXI_KICKED;
					break;
				}
				x = cd->taxis[index].location.x;
				y = cd->taxis[index].location.y;
				cd->taxis[index].client.state = Done;

				// remove passenger from the taxi
				ZeroMemory(&cd->taxis[index].client, sizeof(Passenger));
				cd->taxis[index].client.location.x = -1;
				cd->taxis[index].client.location.y = -1;

				index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.taxi.client.nome);
				if (index == -1) break;

				// remove passenger from the list of passengers
				ZeroMemory(&cd->passengers[index], sizeof(Passenger));

				// remove passenger from the map cell and taxi ownership
				index = FindTaxiIndex(cd->map[x + y * MIN_LIN].taxis, cd->nrMaxTaxis, content.taxi);
				if (index == -1) break;
				ZeroMemory(&cd->map[x + y * MIN_LIN].taxis[index].client, sizeof(Passenger));
				cd->map[x + y * MIN_LIN].taxis[index].client.location.x = -1;
				cd->map[x + y * MIN_LIN].taxis[index].client.location.y = -1;
				response.action = OK;
				break;
			case GetCityMap:
				CopyMemory(response.map, cd->charMap, sizeof(cd->charMap));
				_tprintf(_T("Got a map request...\n"));
				response.action = OK;
				break;
			case RequestPassenger:
				index = FindTaxiWithLicense(cd->taxis, cd->nrMaxTaxis, content.taxi.licensePlate);
				if (index == -1) {
					response.action = TAXI_KICKED;
					break;
				}
				status = GetPassengerStatus(cd->passengers, cd->nrMaxPassengers, &content.passenger.nome);
				if (status == NotFound) {
					response.action = PASSENGER_DOESNT_EXIST;
					break;
				}
				else if (status != Waiting) {
					response.action = PASSENGER_ALREADY_TAKEN;
					break;
				}
				CopyMemory(&cd->taxis[index].client, &cd->passengers[GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.passenger.nome)], sizeof(Passenger));
				index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.passenger.nome);
				CopyMemory(response.passenger.nome, cd->passengers[index].nome, sizeof(TCHAR)*25);
				// atualizar o estado do passageiro na lista de passageiros
				cd->passengers[index].state = Taken;
				response.passenger.location.x = cd->passengers[index].location.x;
				response.passenger.location.y = cd->passengers[index].location.y;
				response.passenger.destination.x = cd->passengers[index].destination.x;
				response.passenger.destination.y = cd->passengers[index].destination.y;
				response.passenger.state = Waiting;
				response.action = OK;
				break;
			case NotifySpeedChange:
				index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
				if (index == -1) {
					response.action = TAXI_KICKED;
					break;
				}
				CopyMemory(&cd->taxis[index], &content.taxi, sizeof(Taxi));
				response.action = OK;
				break;
			case NotifyNQChange:
				index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
				if (index == -1) {
					response.action = TAXI_KICKED;
					break;
				}
				CopyMemory(&cd->taxis[index], &content.taxi, sizeof(Taxi));
				response.action = OK;
				break;

			case NotifyTaxiLeaving:
				index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
				if (index == -1) {
					response.action = TAXI_KICKED;
					break;
				}
				// taxis has passenger
				if (cd->taxis[index].client.location.x > 0) {
					response.action = CANT_QUIT_WITH_PASSENGER;
					break;
				}
				ZeroMemory(&cd->taxis[index], sizeof(Taxi));
				cd->taxis[index].location.x = -1;
				cd->taxis[index].location.y = -1;
				response.action = OK;
				break;
	}
	return response;
}

DWORD WINAPI timer(LPVOID ptr) {
	int* waitTime = (int*)ptr;
	HANDLE hTimer;

	FILETIME ft, ftUTC;
	LARGE_INTEGER DueTime;
	SYSTEMTIME systime;
	LONG interval = (LONG)*waitTime * 1000;

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
	return 0;
}

DWORD WINAPI TalkToTaxi(LPVOID ptr) {
	IndividualCD* ind= (IndividualCD*)ptr;
	CDThread* cd = ind->cd;
	CC_CDRequest* request = &ind->comm.request;
	CC_CDResponse* response = &ind->comm.response;
	SHM_CC_REQUEST shm_request;
	SHM_CC_RESPONSE shm_response;
	BOOL isTimerLaunched=FALSE;
	SHM_CC_REQUEST* requestPassengerList;
	int index = 0;
	//HANDLE timerHandle = NULL;

	requestPassengerList = (SHM_CC_REQUEST*)malloc(cd->nrMaxTaxis * sizeof(SHM_CC_REQUEST));
	if (requestPassengerList == NULL)
		return -1;

	while (!cd->isSystemClosing) {
		// Receber request
		WaitForSingleObject(response->new_request, INFINITE);
		WaitForSingleObject(request->mutex, INFINITE);
		//_tprintf(_T("Got resquest done!\n"));
		// Guardar o conteudo da mensagem
		CopyMemory(&shm_request.messageContent, &request->shared->messageContent, sizeof(Content));
		shm_request.action = request->shared->action;
		ReleaseMutex(request->mutex);

		//if (shm_request.action == RequestPassenger) {
			/*CopyMemory(&requestPassengerList[index++], &shm_request, sizeof(SHM_CC_REQUEST));
			if (!isTimerLaunched) {
				if ( (timerHandle = CreateThread(NULL, 0, timer, cd->WaitTimeOnTaxiRequest, 0, NULL)) == NULL) {
					_tprintf(_T("Error launching console thread (%d)\n"), GetLastError());
					exit(-1);
				}
				isTimerLaunched = TRUE;
			}
			if (timerHandle != NULL) {
				WaitForSingleObject(timerHandle, INFINITE);
			}
			
			int nr = (rand() % index - 0) + 0;
			isTimerLaunched = FALSE;

			for (unsigned int i = 0; i < cd->nrMaxTaxis; i++) {
				if (i == nr) {
					_tprintf(_T("%s - ok!\n"), request->shared->messageContent.taxi);*/
					//shm_response = ParseAndExecuteOperation(cd, shm_request.action, shm_request.messageContent);
				//}
				//else
				//	shm_response.action = PASSENGER_ALREADY_TAKEN;

				// Enviar resposta
				//WaitForSingleObject(response->mutex, INFINITE);
				//response->shared->action = shm_response.action;
				//CopyMemory(&response->shared->passenger, &shm_response.passenger, sizeof(Passenger));
				//ReleaseMutex(response->mutex);
				//SetEvent(request->new_response);
			//}
		//}
		//else {

			// Tratar mensagem
			shm_response = ParseAndExecuteOperation(cd, shm_request.action, shm_request.messageContent);

			// Enviar resposta
			WaitForSingleObject(response->mutex, INFINITE);
			if (shm_request.action == GetCityMap) {
				CopyMemory(&response->shared->map, &shm_response.map, sizeof(char) * MIN_LIN * MIN_COL);
			}
			else if (shm_request.action == RequestPassenger) {

				CopyMemory(&response->shared->passenger, &shm_response.passenger, sizeof(Passenger));

			}
			response->shared->action = shm_response.action;
			CopyMemory(&response->shared->passenger, &shm_response.passenger, sizeof(Passenger));

			ReleaseMutex(response->mutex);
			SetEvent(request->new_response);
			PrintMap(cd->map);
		}
	//}
	free(ind);

	//CDThread* cd = (CDThread*)ptr;
	//CC_CDRequest* request = cd->comm->request;
	//CC_CDResponse* response = cd->comm->response;
	//SHM_CC_REQUEST shm_request;
	//SHM_CC_RESPONSE shm_response;

	//while (!cd->isSystemClosing) {

	//	WaitForSingleObject(response->new_request, INFINITE);
	//	WaitForSingleObject(request->mutex, INFINITE);

	//	// Guardar o conteudo da mensagem
	//	CopyMemory(&shm_request.messageContent, &request->shared->messageContent, sizeof(Content));
	//	shm_request.action = request->shared->action;
	//	ReleaseMutex(request->mutex);

	//	// Tratar mensagem
	//	shm_response = ParseAndExecuteOperation(cd, shm_request.action, shm_request.messageContent);

	//	WaitForSingleObject(response->mutex, INFINITE);

	//	if (shm_request.action == GetCityMap) {
	//		CopyMemory(&response->shared->map, &shm_response.map, sizeof(char) * MIN_LIN * MIN_COL);
	//	}
	//	else if (shm_request.action == RequestPassenger) {
	//		CopyMemory(&response->shared->passenger, &shm_response.passenger, sizeof(Passenger));
	//	}
	//	response->shared->action = shm_response.action;

	//	ReleaseMutex(response->mutex);
	//	SetEvent(request->new_response);
	//	PrintMap(cd->map);
	//}
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
		cdThread->dllMethods->Register(request_shm_name, FILE_MAPPING);

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
		cdThread->dllMethods->Register(request_shm_name, VIEW);

		cdRequest.mutex = CreateMutex(NULL, FALSE, request_mutex_name);
		if (cdRequest.mutex == NULL) {
			_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdRequest.mutex;
		cdThread->dllMethods->Register(request_mutex_name, MUTEX);

		cdRequest.new_response = CreateEvent(NULL, FALSE, FALSE, response_event_name);
		if (request->new_response == NULL) {
			_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdRequest.new_response;
		cdThread->dllMethods->Register(response_event_name, EVENT);

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
		cdThread->dllMethods->Register(response_shm_name, FILE_MAPPING);

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
		cdThread->dllMethods->Register(response_shm_name, VIEW);

		cdResponse.mutex = CreateMutex(NULL, FALSE, response_mutex_name);
		if (cdResponse.mutex == NULL) {
			_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdResponse.mutex;
		cdThread->dllMethods->Register(response_mutex_name, MUTEX);

		cdResponse.new_request = CreateEvent(NULL, FALSE, FALSE, request_event_name);
		if (cdResponse.new_request == NULL) {
			_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdResponse.new_request;
		cdThread->dllMethods->Register(request_event_name, EVENT);

		IndividualCD* indCD = (IndividualCD*) malloc(sizeof(IndividualCD));
		indCD->cd = cdThread;
		CopyMemory(&indCD->comm.container, &res, sizeof(LR_Container));
		CopyMemory(&indCD->comm.request, &cdRequest, sizeof(CC_CDRequest));
		CopyMemory(&indCD->comm.response, &cdResponse, sizeof(CC_CDResponse));

		if (container->threads[(*container->threadCounter)++] = CreateThread(NULL, 0, TalkToTaxi, indCD, 0, NULL) == NULL) {
			_tprintf(_T("Error launching comm thread (%d)\n"), GetLastError());
			WaitAllThreads(container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
	}
	SetEvent(request->new_response);
	_tprintf(_T("[LOG] Sent response to the '%s' registration request.\n"), licensePlate);
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

DWORD WINAPI ListenToLoginRequests(LPVOID ptr) {
	CDThread* cd = (CDThread*)ptr;
	CDLogin_Request* cdata = cd->cdLogin_Request;
	CDLogin_Response* cdResponse = cd->cdLogin_Response;
	SHM_LOGIN_REQUEST shared;

	while (!cd->isSystemClosing) {
		if (WaitForSingleObject(cdResponse->new_request, 3000) == WAIT_TIMEOUT)
			continue;
		
		WaitForSingleObject(cdata->login_m, INFINITE);

		// Save Request
		CopyMemory(&shared, cdata->request, sizeof(SHM_LOGIN_REQUEST));

		ReleaseMutex(cdata->login_m);
		int index = FindTaxiWithLicense(cd->taxis, cd->nrMaxTaxis, shared.taxi.licensePlate);
		if (index != -1) {
			RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, LICENSE_PLATED_ALREADY_IN_CENTRAL);
			continue;
		}
		index = FindFirstFreeTaxiIndex(cd->taxis, cd->nrMaxTaxis);
		if (index == -1) {
			RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, ERRO);
			continue;
		}
		if (cd->areTaxisRequestsPause) {
			RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, TAXI_REQUEST_PAUSED);
			continue;
		}
		if (shared.action == RegisterTaxiInCentral) {
			_tprintf(_T("Got a registration request from '%s'\n"), shared.taxi.licensePlate);
			if (shared.taxi.location.x > MIN_COL || shared.taxi.location.y > MIN_LIN || shared.taxi.location.x < 0 || shared.taxi.location.y < 0) {
				RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, OUTOFBOUNDS_TAXI_POSITION);
				continue;
			}
			// inserir taxi na sua posição atual 
			Cell* cell = cd->map + (shared.taxi.location.x + shared.taxi.location.y * MIN_COL);
			enum responde_id res;

			if (cell->display == B_DISPLAY) {
				RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, OUTOFBOUNDS_TAXI_POSITION);
				continue;
			}

			if ((res = InsertTaxiIntoMapCell(cell, shared.taxi, cd->nrMaxTaxis)) != OK) {
				RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, res);
				continue;
			}
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

TCHAR** ParseCommand(TCHAR* cmd) {
	TCHAR command[6][100];
	TCHAR* delimiter = _T(" ");
	int counter = 0;

	TCHAR* aux = _tcstok(cmd, delimiter);
	CopyMemory(command[counter++], aux, sizeof(TCHAR) * 100);

	while ((aux = _tcstok(NULL, delimiter)) != NULL) {
		CopyMemory(command[counter++], aux, sizeof(TCHAR) * 100);
		if (counter == 6) break;
	}
	while (counter < 6) {
		CopyMemory(command[counter++], _T("NULL"), sizeof(TCHAR) * 100);
	}
	return command;
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

int FindFeatureAndRun(TCHAR* command, CDThread* cdata) {
	TCHAR commands[7][100] = {
		_T("\tkick x - kick taxi with x as id.\n"),
		_T("\tclose - close the system.\n"), 
		_T("\tlist_taxis - list all the taxis in the system.\n"), 
		_T("\tlist_passengers - list all the passengers in the system.\n"), 
		_T("\tpause - pauses taxis acceptance in the system.\n"), 
		_T("\tresume - resumes taxis acceptance in the system.\n"), 
		_T("\tinteval x - changes the time central waits for the taxis to ask for work.\n") 
	};
	
	TCHAR cmd[6][100];
	CopyMemory(cmd, ParseCommand(command), sizeof(TCHAR) * 6 * 100);
	int argc = 0;

	for (int i = 0; i < 6; i++)
		if (_tcscmp(cmd[i], _T("NULL")) != 0) argc++;

	if (_tcscmp(cmd[0], ADM_KICK) == 0) {
		if (argc<2)
			_tprintf(_T("This command requires a id.\n"));
		else {
			int index = FindTaxiWithLicense(cdata->taxis, cdata->nrMaxTaxis, cmd[1]);
			if (index == -1) {
				_tprintf(_T("Specified taxi license doesn't exist in the system.\n"));
				return;
			}
			RemoveTaxiFromMapCell(&cdata->map[cdata->taxis[index].location.x + cdata->taxis[index].location.y * MIN_COL], cmd[1], cdata->nrMaxTaxis);
			ZeroMemory(&cdata->taxis[index], sizeof(Taxi));
			cdata->taxis[index].location.x = -1;
			cdata->taxis[index].location.y = -1;
			_tprintf(_T("Taxi with '%s' as license has been kicked from the server.\n"), cmd[1]);
		}
	}
	else if (_tcscmp(cmd[0], ADM_CLOSE) == 0) {
		// @TODO notify todas as apps
		_tprintf(_T("System is closing.\n"));
		SHM_BROADCAST b;
		cdata->isSystemClosing = TRUE;
		b.isSystemClosing = TRUE;
		SendBroadCastMessage(cdata->broadcast, &b, NumberOfActiveTaxis(cdata->taxis, cdata->nrMaxTaxis));
		return -1;
	}
	else if (_tcscmp(cmd[0], ADM_LIST_TAXIS) == 0) {
		for (unsigned int i = 0; i < cdata->nrMaxTaxis; i++) {
			if (cdata->taxis[i].location.x > 0)
				_tprintf(_T("%.2d - %9s at {%.2d;%.2d} is \n"), i, cdata->taxis[i].licensePlate, cdata->taxis[i].location.x, cdata->taxis[i].location.y);
		}
	}
	else if (_tcscmp(cmd[0], ADM_LIST_PASSENGERS) == 0) {
		for (unsigned int i = 0; i < cdata->nrMaxPassengers; i++) {
			if (cdata->taxis[i].location.x > 0)
				_tprintf(_T("%.2d - %s at {%.2d;%.2d} with {%.2d;%.2d} as destination is %s!\n"), 
					i, cdata->passengers[i].nome, cdata->passengers[i].location.x, cdata->passengers[i].location.y,
					cdata->passengers[i].destination.x, cdata->passengers[i].destination.y, ParseStateToString(cdata->passengers[i].state));
		}
	}
	else if (_tcscmp(cmd[0], ADM_PAUSE) == 0) {
		_tprintf(_T("System pause\n"));
		cdata->areTaxisRequestsPause = TRUE;
	}
	else if (_tcscmp(cmd[0], ADM_RESUME) == 0) {
		_tprintf(_T("System resume\n"));
		cdata->areTaxisRequestsPause = FALSE;
	}
	else if (_tcscmp(cmd[0], ADM_INTERVAL) == 0) {
		if (argc < 2)
			_tprintf(_T("This command requires a id.\n"));
		else {
			_tprintf(_T("System changing the wait time of a taxi request to %d\n"), _ttoi(cmd[1]));
			(*cdata->WaitTimeOnTaxiRequest) = _ttoi(cmd[1]);
		}
	}
	else if (_tcscmp(cmd[0], ADM_HELP) == 0) {
		cdata->dllMethods->Test();
		for (int i = 0; i < 7; i++)
			_tprintf(_T("%s"), commands[i]);
	}
	else if (_tcscmp(cmd[0], DBG_ADD_PASSENGER) == 0) {
		if (argc < 6)
			_tprintf(_T("To few arguments!\n"));
		else {
			int index = GetLastPassengerIndex(cdata->passengers, cdata->nrMaxPassengers);
			CopyMemory(cdata->passengers[index + 1].nome, cmd[1], sizeof(TCHAR) * 25);
			int x = _ttoi(cmd[2]);
			int y = _ttoi(cmd[3]);
			int dest_x = _ttoi(cmd[4]);
			int dest_y = _ttoi(cmd[5]);
			cdata->passengers[index + 1].location.x = x;
			cdata->passengers[index + 1].location.y = y;
			cdata->passengers[index + 1].destination.x = dest_x;
			cdata->passengers[index + 1].destination.y = dest_y;
			cdata->passengers[index + 1].state = Waiting;
			index = GetLastPassengerIndex(cdata->map[x + y * MIN_COL].passengers, cdata->nrMaxPassengers);
			CopyMemory(cdata->map[x + y * MIN_COL].passengers[index + 1].nome, cmd[1], sizeof(TCHAR) * 25);
			cdata->map[x + y * MIN_COL].passengers[index + 1].location.x = x;
			cdata->map[x + y * MIN_COL].passengers[index + 1].location.y = y;
			cdata->map[x + y * MIN_COL].passengers[index + 1].destination.x = dest_x;
			cdata->map[x + y * MIN_COL].passengers[index + 1].destination.y = dest_y;
			cdata->map[x + y * MIN_COL].passengers[index + 1].state = Waiting;
			SHM_BROADCAST b;
			b.isSystemClosing = FALSE;
			CopyMemory(&b.passenger, &cdata->passengers[index + 1], sizeof(Passenger));
			SendBroadCastMessage(cdata->broadcast, &b, NumberOfActiveTaxis(cdata->taxis, cdata->nrMaxTaxis));
		}
	}
	else {
		_tprintf(_T("System doesn't recognize the command, type 'help' to view all the commands.\n"));
	}
}

void LoadMapa(Cell* map, char* buffer, int nrTaxis, int nrPassangers) {
	int aux=0;
	int ignored = 0;
	int nr = 0;
	for (int i = 0; i < MIN_LIN; i++) {
		for (int j = 0; j < MIN_COL; j++) {
			nr = ((i * MIN_COL) + j) + ignored;
			char c = buffer[nr];
			if (c=='\n'||c=='\r') {
				ignored+=2;
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

	for (unsigned int lin = 0; lin < MIN_LIN; lin++)
		for (unsigned int col = 0; col < MIN_COL; col++){
			char aux = map[col + lin * MIN_COL].display;
			if (aux == B_DISPLAY)
				charMap[col][lin] = B_DISPLAY;
			else if (aux == S_DISPLAY)
				charMap[col][lin] = S_DISPLAY;
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
	if ((fileContent = ReadFileToCharArray(_T(".\\..\\maps\\mapa.txt"))) == NULL) {
		_tprintf(_T("Error reading the file (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	// Preenche mapa com o conteudo do ficheiro
	LoadMapa(map, fileContent, nrMaxTaxis, nrMaxPassengers);

	CDThread cdThread;
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

	// dar free da memoria dos taxis em todas as celulas
	for (unsigned int i = 0; i < MIN_LIN * MIN_COL; i++) {
		free(map[i].taxis);
		free(map[i].passengers);
	}
	free(taxis);
	free(passengers);
	return 0;
}

