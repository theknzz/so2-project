#include "comm.h"

DWORD WINAPI TalkToTaxi(LPVOID ptr) {
	IndividualCD* ind = (IndividualCD*)ptr;
	CDThread* cd = ind->cd;
	CC_CDRequest* request = &ind->comm.request;
	CC_CDResponse* response = &ind->comm.response;
	SHM_CC_REQUEST shm_request;
	SHM_CC_RESPONSE shm_response;

	while (!cd->isSystemClosing) {
		// Receber request
		if (WaitForSingleObject(response->new_request, 2000) == WAIT_TIMEOUT) continue;
		WaitForSingleObject(request->mutex, INFINITE);

		// Guardar o conteudo da mensagem
		CopyMemory(&shm_request.messageContent, &request->shared->messageContent, sizeof(Content));
		shm_request.action = request->shared->action;
		ReleaseMutex(request->mutex);

		cd->dllMethods->Log(_T("Central got request from %s.\n"), shm_request.messageContent.taxi.licensePlate);

		if (shm_request.action != RequestPassenger) {

			WaitForSingleObject(cd->mtx_access_control, INFINITE);
			// Tratar mensagem
			shm_response = ParseAndExecuteOperation(cd, shm_request.action, shm_request.messageContent);
			ReleaseMutex(cd->mtx_access_control);

			// Enviar resposta
			WaitForSingleObject(response->mutex, INFINITE);
			if (shm_request.action == GetCityMap) {
				CopyMemory(&response->shared->map, &shm_response.map, sizeof(char) * MIN_LIN * MIN_COL);
			}

			response->shared->action = shm_response.action;
			CopyMemory(&response->shared->passenger, &shm_response.passenger, sizeof(Passenger));

			ReleaseMutex(response->mutex);
			SetEvent(request->new_response);
		}
		else {
			_tprintf(_T("\npassenger request by %s\n"), shm_request.messageContent.taxi.licensePlate);
			int passenger_index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, shm_request.messageContent.passenger.nome);
			int taxi_index = FindTaxiWithLicense(cd->taxis, cd->nrMaxTaxis, shm_request.messageContent.taxi.licensePlate);
			WaitForSingleObject(cd->mtx_access_control, INFINITE);
			// Inserir o request no buffer de requests
			if (!isInRequestBuffer(cd->passengers[passenger_index].requests, *cd->passengers[passenger_index].requestsCounter, cd->taxis[taxi_index]))
				cd->passengers[passenger_index].requests[(*cd->passengers[passenger_index].requestsCounter)++] = cd->taxis[taxi_index].hNamedPipe;
			ReleaseMutex(cd->mtx_access_control);
		}
		cd->dllMethods->Log(_T("Central sent response to %s.\n"), shm_request.messageContent.taxi.licensePlate);
		PrintMap(cd->map);
	}
	free(ind);
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
			WaitAllThreads(cdThread, container->threads, (*container->threadCounter));
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
			WaitAllThreads(cdThread, container->threads, (*container->threadCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->views[(*container->viewCounter)++] = cdRequest.shared;
		cdThread->dllMethods->Register(request_shm_name, VIEW);

		cdRequest.mutex = CreateMutex(NULL, FALSE, request_mutex_name);
		if (cdRequest.mutex == NULL) {
			_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
			WaitAllThreads(cdThread, container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdRequest.mutex;
		cdThread->dllMethods->Register(request_mutex_name, MUTEX);

		cdRequest.new_response = CreateEvent(NULL, FALSE, FALSE, response_event_name);
		if (request->new_response == NULL) {
			_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
			WaitAllThreads(cdThread, container->threads, (*container->threadCounter));
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
			WaitAllThreads(cdThread, container->threads, (*container->threadCounter));
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
			WaitAllThreads(cdThread, container->threads, (*container->threadCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->views[(*container->viewCounter)++] = cdResponse.shared;
		cdThread->dllMethods->Register(response_shm_name, VIEW);

		cdResponse.mutex = CreateMutex(NULL, FALSE, response_mutex_name);
		if (cdResponse.mutex == NULL) {
			_tprintf(TEXT("Error creating cdLogin mutex mutex (%d).\n"), GetLastError());
			WaitAllThreads(cdThread, container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		container->handles[(*container->handleCounter)++] = cdResponse.mutex;
		cdThread->dllMethods->Register(response_mutex_name, MUTEX);

		cdResponse.new_request = CreateEvent(NULL, FALSE, FALSE, request_event_name);
		if (cdResponse.new_request == NULL) {
			_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
			WaitAllThreads(cdThread, container->threads, (*container->threadCounter));
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
			WaitAllThreads(cdThread, container->threads, (*container->threadCounter));
			UnmapAllViews(container->views, (*container->viewCounter));
			CloseMyHandles(container->handles, (*container->handleCounter));
			exit(-1);
		}
		cdThread->dllMethods->Log(_T("Central got registration request from %s.\n"), licensePlate);
	}
	SetEvent(request->new_response);
	cdThread->dllMethods->Log(_T("Central sent registration response to %s.\n"), licensePlate);
	//_tprintf(_T("[LOG] Sent response to the '%s' registration request.\n"), licensePlate);
}


DWORD WINAPI ListenToLoginRequests(LPVOID ptr) {
	CDThread* cd = (CDThread*)ptr;
	CDLogin_Request* cdata = cd->cdLogin_Request;
	CDLogin_Response* cdResponse = cd->cdLogin_Response;
	SHM_LOGIN_REQUEST shared;

	while (!cd->isSystemClosing) {
		if (WaitForSingleObject(cdResponse->new_request, 2000) == WAIT_TIMEOUT)
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
			_tprintf(_T("\nGot a registration request from '%s'\n"), shared.taxi.licensePlate);
			if (shared.taxi.location.x > MIN_COL || shared.taxi.location.y > MIN_LIN || shared.taxi.location.x < 0 || shared.taxi.location.y < 0) {
				RespondToTaxiLogin(cd, shared.taxi.licensePlate, cd->hContainer, OUTOFBOUNDS_TAXI_POSITION);
				continue;
			}
			// inserir taxi na sua posição atual 
			Cell* cell = (Cell*)(cd->map + (shared.taxi.location.x + (shared.taxi.location.y * MIN_COL)));
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
				UpdateView(cd);
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
	SHM_BROADCAST broadcast;
	BOOL WrongCase = FALSE;

	// operação bloqueante (fica à espera da ligação de um cliente)
	if (!ConnectNamedPipe(cd->hPassPipeRegister, NULL)) {
		return 0;
	}

	if (!ConnectNamedPipe(cd->hPassPipeTalk, NULL)) {
		return 0;
	}

	while (!cd->isSystemClosing) {
		if (WaitForSingleObject(cd->eventNewPMessage, 2000) == WAIT_TIMEOUT)
			continue;

		ret = ReadFile(cd->hPassPipeRegister, &message, sizeof(PassRegisterMessage), &nr, NULL);

		if (message.resp == CENTRAL_GOING_OFFLINE) continue;
		
		if (isValidCoords(cd, message.passenger.location) && isValidCoords(cd, message.passenger.destination)) {

			InsertPassengerIntoBuffer(cd->prod_cons, message.passenger);

			AddPassengerToCentral(cd, message.passenger.nome, message.passenger.location.x, message.passenger.location.y, message.passenger.destination.x, message.passenger.destination.y);

			_tprintf(_T("%s - {%.2d,%.2d} to {%.2d,%.2d}.\n"), message.passenger.nome, message.passenger.location.x, message.passenger.location.y,
				message.passenger.destination.x, message.passenger.destination.y);
			message.resp = ESTIMATED_TIME;
			message.estimatedWaitTime = GetEstimatedTime(cd, message.passenger.location);
			UpdateView(cd);
		}
		else {
			message.resp = COORDINATES_FROM_OTHER_CITY;
			WrongCase = TRUE;
		}

		if (!WriteFile(cd->hPassPipeRegister, &message, sizeof(PassRegisterMessage), &nr, NULL)) {
			_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
			Sleep(2000);
			exit(-1);
		}
		if (WrongCase) {
			WrongCase = FALSE;
			continue;
		}

		CopyMemory(&broadcast.passenger, &message.passenger, sizeof(Passenger));
		broadcast.isSystemClosing = FALSE;
		SendBroadCastMessage(cd->broadcast, &broadcast, NumberOfActiveTaxis(cd->taxis, cd->nrMaxTaxis));

		HANDLE cthread = CreateThread(NULL, 0, RequestWaitTimeFeature, cd, 0, NULL);

		ZeroMemory(&message, sizeof(PassRegisterMessage));
		ZeroMemory(&broadcast, sizeof(SHM_BROADCAST));
	}
	return 0;
}

DWORD WINAPI WaitTaxiConnect(LPVOID ptr) {
	TENC* box = (TENC*)ptr;
	SetEvent(box->cd->eventNewConnection);
	if (!ConnectNamedPipe(box->cd->hNamedPipe, NULL)) {
		_tprintf(TEXT("[ERRO] Ligação ao named pipe do taxi! (ConnectNamedPipe %d).\n"), GetLastError());
		return -1;
	}
	box->index = FindTaxiWithLicense(box->cd->taxis, box->cd->nrMaxTaxis, box->target);
	if (box->index == -1) {
		return -1;
	}
	box->cd->taxis[box->index].hNamedPipe = box->cd->hNamedPipe;
	free(box);
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
		cdata->isSystemClosing = TRUE;
		/*SHM_BROADCAST b;
		b.isSystemClosing = TRUE;
		SendBroadCastMessage(cdata->broadcast, &b, NumberOfActiveTaxis(cdata->taxis, cdata->nrMaxTaxis));*/
		PassMessage message;
		message.isSystemClosing = TRUE;
		message.resp = CENTRAL_GOING_OFFLINE;
		BroadcastViaNamedPipeToTaxi(cdata->taxis, cdata->nrMaxTaxis, message, cdata->eventNewCMessage);
		UpdateView(cdata);
		CreateFile(NP_PASS_REGISTER, /*GENERIC_READ*/ PIPE_ACCESS_DUPLEX, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		CreateFile(NP_PASS_TALK, /*GENERIC_READ*/ PIPE_ACCESS_DUPLEX, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		
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
			if (_ttoi(cmd[1] < 0)) {
				_tprintf(_T("Command %s can't handle %d as argument."), cmd[0], _ttoi(cmd[1]));
				return;
			}
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
	/*else if (_tcscmp(cmd[0], DBG_ADD_PASSENGER) == 0) {
		if (argc < 6)
			_tprintf(_T("To few arguments!\n"));
		else {
			int x = _ttoi(cmd[2]);
			int y = _ttoi(cmd[3]);
			int dest_x = _ttoi(cmd[4]);
			int dest_y = _ttoi(cmd[5]);
			AddPassengerToCentral(cdata, cmd[1], x, y, dest_x, dest_y);
			SHM_BROADCAST b;
			b.isSystemClosing = FALSE;
			int index = GetPassengerIndex(cdata->passengers, cdata->nrMaxPassengers, cmd[1]);
			CopyMemory(&b.passenger, &cdata->passengers[index], sizeof(Passenger));
			SendBroadCastMessage(cdata->broadcast, &b, NumberOfActiveTaxis(cdata->taxis, cdata->nrMaxTaxis));
			cdata->dllMethods->Log(_T("New passenger added to the system: %s at {%.2d,%.2d} with {%.2d,%.2d} as destination!\n"), cmd[1], x, y, dest_x, dest_y);
			cdata->dllMethods->Log(_T("Central sent a broadcast message!.\n"));
		}
	}*/
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
	} while (!cdata->isSystemClosing);
	return 0;
}

SHM_CC_RESPONSE ParseAndExecuteOperation(CDThread* cd, enum message_id action, Content content) {
	SHM_CC_RESPONSE response;
	response.action = ERRO;
	TCHAR log[300];
	int index, x, y;
	TENC* box = (TENC*)malloc(sizeof(TENC));
	enum passanger_state status;
	switch (action) {
	case UpdateTaxiLocation:
		response.action = UpdateTaxiPosition(cd, content);
		index = FindTaxiWithLicense(cd->taxis, cd->nrMaxTaxis, content.taxi.licensePlate);
		_stprintf(log, _T("Taxi '%s' moved to {%.2d, %.2d} and has %s passenger.\n"), cd->taxis[index].licensePlate,
			cd->taxis[index].location.x, cd->taxis[index].location.y, hasPassenger(cd->taxis[index]));
		cd->dllMethods->Log(log);
		UpdateView(cd);
		break;
	case WarnPassengerCatch:
		response.action = CatchPassenger(cd, content);
		index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
		_stprintf(log, _T("Taxi '%s' caught '%s'.\n"), cd->taxis[index].licensePlate, cd->taxis[index].client.nome);
		cd->dllMethods->Log(log);
		UpdateView(cd);
		break;
	case WarnPassengerDeliever:
		response.action = DeliverPassenger(cd, content);
		_stprintf(log, _T("Taxi '%s' delivered '%s'.\n"), content.taxi.licensePlate, content.taxi.client.nome);
		cd->dllMethods->Log(log);
		UpdateView(cd);
		break;
	case GetCityMap:
		CopyMemory(response.map, cd->charMap, sizeof(cd->charMap));
		response.action = OK;
		_stprintf(log, _T("Taxi '%s' requested city map!\n"), content.taxi.licensePlate);
		cd->dllMethods->Log(log);
		break;
	case RequestPassenger:
		response.action = AssignPassengerToTaxi(cd, content);
		index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.passenger.nome);
		CopyMemory(response.passenger.nome, cd->passengers[index].nome, sizeof(TCHAR) * 25);
		// atualizar o estado do passageiro na lista de passageiros
		cd->passengers[index].state = Taken;
		response.passenger.location.x = cd->passengers[index].location.x;
		response.passenger.location.y = cd->passengers[index].location.y;
		response.passenger.destination.x = cd->passengers[index].destination.x;
		response.passenger.destination.y = cd->passengers[index].destination.y;
		response.passenger.state = Waiting;
		_stprintf(log, _T("Taxi '%s' requested '%s' as his passenger!\n"), content.taxi.licensePlate, content.taxi.client.nome);
		cd->dllMethods->Log(log);
		UpdateView(cd);
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
		UpdateView(cd);
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
		UpdateView(cd);
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
		UpdateView(cd);
		break;
	case EstablishNamedPipeComm:
		index = FindTaxiIndex(cd->taxis, cd->nrMaxTaxis, content.taxi);
		if (index == -1) {
			response.action = TAXI_KICKED;
			break;
		}
		box->cd = cd;
		CopyMemory(box->target, content.taxi.licensePlate, 9 * sizeof(TCHAR));
		cd->hNamedPipe = CreateNamedPipe(NP_TAXI_NAME, PIPE_ACCESS_DUPLEX, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, cd->nrMaxTaxis, sizeof(PassMessage), sizeof(PassMessage), 1000, NULL);
		if (cd->hNamedPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe) %d"), GetLastError());
			response.action = ERRO;
			break;
		}
		box->cd->dllMethods->Register(NP_TAXI_NAME, NAMED_PIPE);

		if (!CreateThread(NULL, 0, WaitTaxiConnect, box, 0, NULL)) {
			_tprintf(_T("Error launching connection thread (%d)\n"), GetLastError());
			response.action = ERRO; break;
		}
		response.action = OK;

		break;
	}
	return response;
}

DWORD WINAPI RequestWaitTimeFeature(LPVOID ptr) {
	CDThread* cd = (CDThread*)ptr;
	Content content;

	timer(*cd->WaitTimeOnTaxiRequest);

	GetPassengerFromBuffer(cd->prod_cons, &content.passenger);

	int winner = WaitAndPickWinner(cd, content.passenger);
	int passenger_index = GetPassengerIndex(cd->passengers, cd->nrMaxPassengers, content.passenger.nome);

	// if passenger has zero transport requests the passenger must be notified
	if (!SendTransportRequestResponse(cd->eventNewCMessage, cd->passengers[passenger_index].requests, content.passenger, *cd->passengers[passenger_index].requestsCounter, winner)) {
		SendMessageToPassenger(NO_TRANSPORT_AVAILABLE, &content.passenger, NULL, cd);
		RemovePassengerFromCentral(content.passenger.nome, cd->passengers, cd->nrMaxPassengers);
	}
	else {
		CopyMemory(&content.taxi, &cd->taxis[FindTaxiWithNamedPipeHandle(cd->taxis, cd->nrMaxTaxis, cd->passengers[passenger_index].requests[winner])], sizeof(Taxi));
		AssignPassengerToTaxi(cd, content);
		// notify conpass
		SendMessageToPassenger(PASSENGER_GOT_TAXI_ASSIGNED, &content.passenger, &content.taxi, cd);
		// limpar o buffer
		for (unsigned int i = 0; i < *cd->passengers[passenger_index].requestsCounter; i++)
			ZeroMemory(&cd->passengers[passenger_index].requests[i], sizeof(Taxi));
		*cd->passengers[passenger_index].requestsCounter = 0;
	}
	UpdateView(cd);
}

void BroadcastViaNamedPipeToTaxi(Taxi* taxis, int size, PassMessage message, HANDLE newMessage) {
	DWORD nr;
	for (unsigned int i = 0; i < size; i++) {
		WriteFile(taxis[i].hNamedPipe, &message, sizeof(PassMessage), &nr, NULL);
		SetEvent(newMessage);
	}
}

void UpdateView(CDThread* cd) {
	SHM_MAPINFO shm;
	shm.nrTaxis = NumberOfActiveTaxis(cd->taxis, cd->nrMaxTaxis);
	CopyMemory(&shm.taxis, cd->taxis, sizeof(Taxi) * shm.nrTaxis);
	shm.nrPassengers = NumberOfActivePassengers(cd->passengers, cd->nrMaxPassengers);
	CopyMemory(&shm.passengers, cd->passengers, sizeof(Passenger) * shm.nrPassengers);
	CopyMemory(&shm.map, cd->map, sizeof(Cell) * MIN_LIN * MIN_COL);
	shm.isSystemClosing = cd->isSystemClosing;
	WaitForSingleObject(cd->mapinfo->mutex, INFINITE);
	CopyMemory(cd->mapinfo->message, &shm, sizeof(SHM_MAPINFO));
	ReleaseMutex(cd->mapinfo->mutex);
	SetEvent(cd->mapinfo->new_info);
}