#include "comm.h"

DWORD WINAPI TalkToTaxi(LPVOID ptr) {
	IndividualCD* ind = (IndividualCD*)ptr;
	CDThread* cd = ind->cd;
	CC_CDRequest* request = &ind->comm.request;
	CC_CDResponse* response = &ind->comm.response;
	SHM_CC_REQUEST shm_request;
	SHM_CC_RESPONSE shm_response;
	BOOL isTimerLaunched = FALSE;
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

		cd->dllMethods->Log(_T("Central got request from %s.\n"), shm_request.messageContent.taxi.licensePlate);

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
		cd->dllMethods->Log(_T("Central sent response to %s.\n"), shm_request.messageContent.taxi.licensePlate);
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

		IndividualCD* indCD = (IndividualCD*)malloc(sizeof(IndividualCD));
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
		cdThread->dllMethods->Log(_T("Central got registration request from %s.\n"), licensePlate);
	}
	SetEvent(request->new_response);
	cdThread->dllMethods->Log(_T("Central sent registration response to %s.\n"), licensePlate);
	_tprintf(_T("[LOG] Sent response to the '%s' registration request.\n"), licensePlate);
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

DWORD WINAPI GetPassengerRegistration(LPVOID ptr) {
	CDThread* cd = (CDThread*)ptr;
	BOOL ret;
	PassRegisterMessage message;
	DWORD nr;

	while (1) {
		ret = ReadFile(cd->hPassPipeRegister, &message, sizeof(PassRegisterMessage), &nr, NULL);
		_tprintf(_T("%s - {%.2d,%.2d} to {%.2d,%.2d}.\n"), message.passenger.nome, message.passenger.location.x, message.passenger.location.y,
			message.passenger.destination.x, message.passenger.destination.y);
		message.resp = OK;
		if (!WriteFile(cd->hPassPipeRegister, &message, sizeof(PassRegisterMessage), &nr, NULL)) {
			_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
			Sleep(2000);
			exit(-1);
		}
		
		Passenger p;
		CopyMemory(&p, &message.passenger, sizeof(Passenger));
		Taxi taxi;
		CopyMemory(taxi.licensePlate, _T("aa-aa-aa\0"), sizeof(TCHAR) * 9);
		SendMessageToPassenger(PASSENGER_GOT_TAXI_ASSIGNED, &p, &taxi, cd);
		
		ZeroMemory(&message, sizeof(PassRegisterMessage));
	}
	return 0;
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

SHM_CC_RESPONSE ParseAndExecuteOperation(CDThread* cd, enum message_id action, Content content) {
	SHM_CC_RESPONSE response;
	response.action = ERRO;
	TCHAR log[300];
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
		_stprintf(log, _T("Taxi '%s' moved to {%.2d, %.2d} and has %s passenger.\n"), cd->taxis[index].licensePlate,
			cd->taxis[index].location.x, cd->taxis[index].location.y, hasPassenger(cd->taxis[index]));
		cd->dllMethods->Log(log);
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

		index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
		_stprintf(log, _T("Taxi '%s' caught '%s'.\n"), cd->taxis[index].licensePlate, cd->taxis[index].client.nome);
		cd->dllMethods->Log(log);
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
		_stprintf(log, _T("Taxi '%s' delivered '%s'.\n"), content.taxi.licensePlate, content.taxi.client.nome);
		cd->dllMethods->Log(log);
		break;
	case GetCityMap:
		CopyMemory(response.map, cd->charMap, sizeof(cd->charMap));
		_tprintf(_T("Got a map request...\n"));
		response.action = OK;
		_stprintf(log, _T("Taxi '%s' requested city map!\n"), content.taxi.licensePlate);
		cd->dllMethods->Log(log);
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
		CopyMemory(response.passenger.nome, cd->passengers[index].nome, sizeof(TCHAR) * 25);
		// atualizar o estado do passageiro na lista de passageiros
		cd->passengers[index].state = Taken;
		response.passenger.location.x = cd->passengers[index].location.x;
		response.passenger.location.y = cd->passengers[index].location.y;
		response.passenger.destination.x = cd->passengers[index].destination.x;
		response.passenger.destination.y = cd->passengers[index].destination.y;
		response.passenger.state = Waiting;
		response.action = OK;
		_stprintf(log, _T("Taxi '%s' requested '%s' as his passenger!\n"), content.taxi.licensePlate, content.taxi.client.nome);
		cd->dllMethods->Log(log);
		break;
	case NotifySpeedChange:
		index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
		if (index == -1) {
			response.action = TAXI_KICKED;
			break;
		}
		CopyMemory(&cd->taxis[index], &content.taxi, sizeof(Taxi));
		response.action = OK;
		_stprintf(log, _T("Taxi '%s' speeded up to %d!\n"), content.taxi.licensePlate, content.taxi.velocity);
		cd->dllMethods->Log(log);
		break;
	case NotifyNQChange:
		index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
		if (index == -1) {
			response.action = TAXI_KICKED;
			break;
		}
		CopyMemory(&cd->taxis[index], &content.taxi, sizeof(Taxi));
		response.action = OK;
		_stprintf(log, _T("Taxi '%s' slowed down to %d!\n"), content.taxi.licensePlate, content.taxi.velocity);
		cd->dllMethods->Log(log);
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
		_stprintf(log, _T("Taxi '%s' leaving the system!\n"), content.taxi.licensePlate);
		cd->dllMethods->Log(log);
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

void SendBroadCastMessage(CC_Broadcast* broadcast, SHM_BROADCAST* message, int nr) {

	WaitForSingleObject(broadcast->mutex, INFINITE);

	CopyMemory(broadcast->shared, message, sizeof(SHM_BROADCAST));

	ReleaseMutex(broadcast->mutex);

	ReleaseSemaphore(broadcast->new_passenger, (LONG)nr, NULL);
}

int FindFeatureAndRun(TCHAR* command, CDThread* cdata) {
	TCHAR log[300];
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
		if (argc < 2)
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
			_stprintf(log, _T("Taxi with '%s' as license has been kicked from the server.\n"), cmd[1]);
			cdata->dllMethods->Log(log);
		}
	}
	else if (_tcscmp(cmd[0], ADM_CLOSE) == 0) {
		_tprintf(_T("System is closing.\n"));
		SHM_BROADCAST b;
		cdata->isSystemClosing = TRUE;
		b.isSystemClosing = TRUE;
		SendBroadCastMessage(cdata->broadcast, &b, NumberOfActiveTaxis(cdata->taxis, cdata->nrMaxTaxis));
		cdata->dllMethods->Log(_T("Central sent a broadcast message!.\n"));
		cdata->dllMethods->Log(_T("Central is shutting down.\n"));
		return -1;
	}
	else if (_tcscmp(cmd[0], ADM_LIST_TAXIS) == 0) {
		for (unsigned int i = 0; i < cdata->nrMaxTaxis; i++) {
			if (cdata->taxis[i].location.x > -1)
				_tprintf(_T("%.2d - %9s at {%.2d;%.2d}\n"), i, cdata->taxis[i].licensePlate, cdata->taxis[i].location.x, cdata->taxis[i].location.y);
		}
	}
	else if (_tcscmp(cmd[0], ADM_LIST_PASSENGERS) == 0) {
		for (unsigned int i = 0; i < cdata->nrMaxPassengers; i++) {
			if (cdata->taxis[i].location.x > -1)
				_tprintf(_T("%.2d - %s at {%.2d;%.2d} with {%.2d;%.2d} as destination is %s!\n"),
					i, cdata->passengers[i].nome, cdata->passengers[i].location.x, cdata->passengers[i].location.y,
					cdata->passengers[i].destination.x, cdata->passengers[i].destination.y, ParseStateToString(cdata->passengers[i].state));
		}
	}
	else if (_tcscmp(cmd[0], ADM_PAUSE) == 0) {
		_tprintf(_T("System pause\n"));
		cdata->areTaxisRequestsPause = TRUE;
		cdata->dllMethods->Log(_T("Central is pausing taxi registration requests.\n"));
	}
	else if (_tcscmp(cmd[0], ADM_RESUME) == 0) {
		_tprintf(_T("System resume\n"));
		cdata->areTaxisRequestsPause = FALSE;
		cdata->dllMethods->Log(_T("Central is resuming taxi registration requests.\n"));
	}
	else if (_tcscmp(cmd[0], ADM_INTERVAL) == 0) {
		if (argc < 2)
			_tprintf(_T("This command requires a id.\n"));
		else {
			_tprintf(_T("System changing the wait time of a taxi request to %d\n"), _ttoi(cmd[1]));
			(*cdata->WaitTimeOnTaxiRequest) = _ttoi(cmd[1]);
			cdata->dllMethods->Log(_T("Central is chaning the wait time interval to %d.\n"), _ttoi(cmd[1]));
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
			cdata->dllMethods->Log(_T("New passenger added to the system: %s at {%.2d,%.2d} with {%.2d,%.2d} as destination!\n"), cmd[1], x, y, dest_x, dest_y);
			cdata->dllMethods->Log(_T("Central sent a broadcast message!.\n"));
		}
	}
	else {
		_tprintf(_T("System doesn't recognize the command, type 'help' to view all the commands.\n"));
	}
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
		_gettchar();
	} while (!cdata->isSystemClosing);
	return 0;
}