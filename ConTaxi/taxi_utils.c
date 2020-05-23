#include "taxi_utils.h"

void PrintError(enum response_id resp, CD_TAXI_Thread* cd) {
	switch (resp) {
	case INVALID_REGISTRATION_TAXI_POSITION:
		_tprintf(_T("Taxi can't be place in a building cell!\n"));
		break;
	case ERRO:
		_tprintf(_T("Some error occured!\n"));
		break;
	case OUTOFBOUNDS_TAXI_POSITION:
		_tprintf(_T("You choose coordinates from another city!\n"));
		break;
	case HAS_NO_AVAILABLE_PASSENGER:
		_tprintf(_T("Central has no available passenger at the moment!\n"));
		break;
	case PASSENGER_DOESNT_EXIST:
		_tprintf(_T("Passenger doesn't exist in central!\n"));
		break;
	case PASSENGER_ALREADY_TAKEN:
		_tprintf(_T("Passenger already has a driver!\n"));
		break;
	case CANT_QUIT_WITH_PASSENGER:
		_tprintf(_T("You need to reach destination before you quit!\n"));
		break;
	case TAXI_REQUEST_PAUSED:
		_tprintf(_T("Login requests are now paused, try later!\n"));
		break;
	case TAXI_KICKED:
		_tprintf(_T("Central kicked you!\n"));
		cd->isTaxiKicked = TRUE;
		break;
	}
}

TCHAR* DirectionToString(enum taxi_direction direction) {
	if (direction == LEFT) return _T("LEFT");
	else if (direction == RIGHT) return _T("RIGHT");
	else if (direction == UP) return _T("UP");
	return _T("DOWN");
}

void PrintPersonalInformation(CD_TAXI_Thread* cd) {
	_tprintf(_T("\n\n============= TAXI INFO =============\n"));
	_tprintf(_T("Taxi '%s' located in {%.2d;%.2d}.\n"), cd->taxi->licensePlate, cd->taxi->location.x, cd->taxi->location.y);
	_tprintf(_T("Velocity: %.2d\n"), cd->taxi->velocity);
	_tprintf(_T("NQ: %.2d\n"), cd->taxi->nq);
	if (cd->taxi->client.location.x > -1)
		_tprintf(_T("Taxi has '%s' as passenger.\n"), cd->taxi->client.nome);
	_tprintf(_T("=======================================\n\n"));
}

int CalculateDistanceTo(Coords org, Coords dest) {
	int y_dist, x_dist;

	y_dist = dest.y - org.y;
	x_dist = dest.x - org.x;

	return (int)sqrt(pow(y_dist, 2) + pow(x_dist, 2));
}

enum response_id MoveMeToOptimalPosition(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, Coords dest, char map[MIN_LIN][MIN_COL]) {
	Coords org = taxi->location;
	// bottom, left, top, right
	int positions[4];
	Coords coords[4];

	// bottom
	coords[0].x = org.x;
	coords[0].y = org.y + 1;

	// left
	coords[1].x = org.x - 1;
	coords[1].y = org.y;

	// top
	coords[2].x = org.x;
	coords[2].y = org.y - 1;

	// right
	coords[3].x = org.x + 1;
	coords[3].y = org.y;

	// se nesta posição estiver uma celula de edificio ou um taxi, ou desqualificou-a do algoritmo
	if (taxi->direction == UP || coords[0].y > MIN_LIN || map[coords[0].x][coords[0].y] == B_DISPLAY) {
		positions[0] = INT_MAX;
	}
	else {
		positions[0] = CalculateDistanceTo(coords[0], dest);
	}

	if (taxi->direction == RIGHT || coords[1].x < 0 || map[coords[1].x][coords[1].y] == B_DISPLAY) {
		positions[1] = INT_MAX;
	}
	else {
		positions[1] = CalculateDistanceTo(coords[1], dest);
	}

	if (taxi->direction == DOWN || coords[2].y < 0 || map[coords[2].x][coords[2].y] == B_DISPLAY) {
		positions[2] = INT_MAX;
	}
	else {
		positions[2] = CalculateDistanceTo(coords[2], dest);
	}

	if (taxi->direction == LEFT || coords[3].y > MIN_COL || map[coords[3].x][coords[3].y] == B_DISPLAY) {
		positions[3] = INT_MAX;
	}
	else {
		positions[3] = CalculateDistanceTo(coords[3], dest);
	}

	// select the best position (less distance)
	int optimal = positions[0];
	int nr = 0;
	for (unsigned int i = 1; i < 4; i++) {
		if (positions[i] < optimal) {
			optimal = positions[i];
			nr = i;
		}
	}

	enum response_id ret;
	ret = UpdateMyLocation(request, response, taxi, coords[nr]);
	if (ret == OK) {
		taxi->location.x = coords[nr].x;
		taxi->location.y = coords[nr].y;
		if (taxi->client.state == OnDrive) {
			taxi->client.location.x = coords[nr].x;
			taxi->client.location.y = coords[nr].y;
		}
		taxi->direction = nr;
	}
	return ret;
}

BOOL CanMoveTo(char map[MIN_LIN][MIN_COL], Coords destination) {
	if (destination.x < 0 || destination.y < 0 || destination.x > MIN_COL - 1 || destination.y > MIN_LIN - 1) return FALSE;
	if (map[destination.x][destination.y] == B_DISPLAY)
		return FALSE;
	return TRUE;
}

// Wait for all the threads to stop
void WaitAllThreads(HANDLE* threads, int nr) {
	for (int i = 0; i < nr; i++) {
		WaitForSingleObject(threads[i], INFINITE);
	}
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
	for (int i = 0; i < nr; i++) {
		UnmapViewOfFile(views[i]);
	}
	nr = 0;
}

TCHAR** ParseCommand(TCHAR* cmd) {
	TCHAR command[4][100];
	TCHAR* delimiter = _T(" ");
	int counter = 0;

	TCHAR* aux = _tcstok(cmd, delimiter);
	CopyMemory(command[counter++], aux, sizeof(TCHAR) * 100);

	while ((aux = _tcstok(NULL, delimiter)) != NULL) {
		CopyMemory(command[counter++], aux, sizeof(TCHAR) * 100);
		if (counter == 4) break;
	}
	while (counter < 4) {
		CopyMemory(command[counter++], _T("NULL"), sizeof(TCHAR) * 100);
	}
	return command;
}

enum taxi_direction getTaxiDirection(Coords org, Coords target) {
	if (org.x - 1 == target.x && org.y == target.y) return LEFT;
	else if (org.x + 1 == target.x && org.y == target.y) return RIGHT;
	else if (org.x == target.x && org.y + 1 == target.y) return DOWN;
	return UP;
}

enum taxi_direction getOppositeDirection(enum taxi_direction dir) {
	switch (dir) {
	case DOWN: return UP;
	case LEFT: return RIGHT;
	case RIGHT: return LEFT;
	case UP: return DOWN;
	}
}

enum response_id MoveAleatorio(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, char map[MIN_LIN][MIN_COL]) {
	enum response_id res;
	Coords positions[4];
	Coords finals[4];
	int nr = 0;
	Coords org = taxi->location;
	positions[DOWN] = taxi->location; positions[DOWN].y += 1;
	positions[LEFT] = taxi->location; positions[LEFT].x -= 1;
	positions[UP] = taxi->location; positions[UP].y -= 1;
	positions[RIGHT] = taxi->location; positions[RIGHT].x += 1;

	// invalidate outofbounds locations
	for (unsigned int i = 0; i < 4; i++) {
		if (CanMoveTo(map, positions[i]) && i != getOppositeDirection(taxi->direction)) {
			finals[nr].x = positions[i].x;
			finals[nr].y = positions[i].y;
			nr++;
		}
	}
	int dir = (rand() % nr - 0) + 0;
	if ((res = UpdateMyLocation(request, response, taxi, finals[dir])) == OK) {
		CopyMemory(&taxi->location, &finals[dir], sizeof(Coords));
		taxi->direction = getTaxiDirection(org, finals[dir]);
	}
	return res;
}

BOOL hasPassenger(Taxi* taxi) {
	return taxi->client.location.x < 0 ? 0 : 1;
}

BOOL isPassengerLocation(Taxi* taxi) {
	return taxi->location.x == taxi->client.location.x && taxi->location.y == taxi->client.location.y ? 1 : 0;
}

BOOL isPassengerDestination(Taxi* taxi) {
	return taxi->location.x == taxi->client.destination.x && taxi->location.y == taxi->client.destination.y ? 1 : 0;
}

void moveTaxi(CD_TAXI_Thread* cdata) {
	enum response_id resp;

	if (hasPassenger(cdata->taxi)) {
		if (isPassengerLocation(cdata->taxi) && cdata->taxi->client.state != OnDrive) {
			if ((resp = NotifyPassengerCatch(&cdata->comm.request, &cdata->comm.response, cdata->taxi)) != OK)
				PrintError(resp, cdata);
			else {
				_tprintf(_T("Passenger caught!\n"));
			}
		}
		else if (isPassengerDestination(cdata->taxi)) {
			if ((resp = NotifyPassengerDeliever(&cdata->comm.request, &cdata->comm.response, cdata->taxi)) != OK)
				PrintError(resp, cdata);
			else {
				_tprintf(_T("Passenger delvired!\n"));
			}
		}

		if (cdata->taxi->client.state == OnDrive) {
			if ((resp = MoveMeToOptimalPosition(&cdata->comm.request, &cdata->comm.response, cdata->taxi, cdata->taxi->client.destination, cdata->charMap)) == OK)
				PrintPersonalInformation(cdata);
			else
				PrintError(resp, cdata);
		}
		else if (cdata->taxi->client.state == Waiting && !(cdata->taxi->client.location.x < 0)) {
			if ((resp = MoveMeToOptimalPosition(&cdata->comm.request, &cdata->comm.response, cdata->taxi, cdata->taxi->client.location, cdata->charMap)) == OK)
				PrintPersonalInformation(cdata);
			else
				PrintError(resp, cdata);
		}
	}
	else {
		if (MoveAleatorio(&cdata->comm.request, &cdata->comm.response, cdata->taxi, cdata->charMap) == OK)
			PrintPersonalInformation(cdata);
		else
			_tprintf(_T("For some reason taxi can't move that way!\n"));
	}
}

DWORD WINAPI TaxiAutopilot(LPVOID ptr) {
	CD_TAXI_Thread* cdata = (CD_TAXI_Thread*)ptr;
	HANDLE hTimer;

	FILETIME ft, ftUTC;
	LARGE_INTEGER DueTime;
	SYSTEMTIME systime;
	LONG interval = (LONG)(1 / cdata->taxi->velocity) * 1000;

	SystemTimeToFileTime(&systime, &ft);
	LocalFileTimeToFileTime(&ft, &ftUTC);
	DueTime.HighPart = ftUTC.dwHighDateTime;
	DueTime.LowPart = ftUTC.dwLowDateTime;
	DueTime.QuadPart = -(LONGLONG)interval * 1000 * 10;

	if ((hTimer = CreateWaitableTimer(NULL, TRUE, NULL)) == NULL) {
		_tprintf(_T("Error (%d) creating the waitable timer.\n"), GetLastError());
		return -1;
	}
	while (cdata->taxi->autopilot) {
		if (!SetWaitableTimer(hTimer, &DueTime, 0, NULL, NULL, FALSE)) {
			_tprintf(_T("Something went wrong! %d"), GetLastError());
		}
		moveTaxi(cdata);
		WaitForSingleObject(hTimer, INFINITE);
	}
	return 0;
}

int FindFeatureAndRun(TCHAR command[100], CD_TAXI_Thread* cdata) {
	TCHAR commands[10][100] = {
		_T("\ttransport - request a passanger transport.\n"),
		_T("\tspeed - increase the speed in 0.5 cells/s.\n"),
		_T("\tslow - decrease the speed in 0.5 cells/s.\n"),
		_T("\tnq x - assign x to the nq variable.\n"),
		_T("\tautopilot - turn on/off the taxi's autopilot.\n"),
		_T("\tdown - move the taxi one cell down.\n"),
		_T("\tleft - move the taxi one cell left.\n"),
		_T("\tup - move the taxi one cell up.\n"),
		_T("\tright - move the taxi one cell right.\n"),
		_T("\tclose - closes taxi's application.\n")
	};

	enum response_id res;
	TCHAR cmd[4][100];
	CopyMemory(cmd, ParseCommand(command), sizeof(TCHAR) * 4 * 100);
	int argc = 0;
	for (int i = 0; i < 4; i++)
		if (_tcscmp(cmd[i], _T("NULL")) != 0) argc++;

	if (_tcscmp(cmd[0], TXI_TRANSPORT) == 0) {
		if (argc < 2) {
			_tprintf(_T("Too few arguments to use this command!\n"));
			return;
		}
		if ((res = RequestPassengerTransport(&cdata->comm.request, &cdata->comm.response, &cdata->taxi->client, cmd[1], cdata->taxi->licensePlate)) != OK)
			PrintError(res, cdata);
		else
			/*_tprintf(_T("Got '%s' as a passenger in {%.2d,%.2d} with the destination {%.2d,%.2d}.\n"), cdata->taxi->client.nome,
				cdata->taxi->client.location.x, cdata->taxi->client.location.y, cdata->taxi->client.destination.x, cdata->taxi->client.destination.y);*/
			PrintPersonalInformation(cdata);
	}
	else if (_tcscmp(cmd[0], TXI_SPEED_UP) == 0) {
		_tprintf(_T("speed\n"));
		cdata->taxi->velocity += 0.5;
		if ((res = NotifyVelocityChange(&cdata->comm.request, &cdata->comm.response, *(cdata->taxi))) != OK) {
			PrintError(res, cdata);
			cdata->taxi->velocity -= 0.5;
		}
		else {
			_tprintf(_T("You speed up!\n"));
		}
	}
	else if (_tcscmp(cmd[0], TXI_SLOW_DOWN) == 0) {
		_tprintf(_T("slow\n"));
		if (cdata->taxi->velocity < 0.5) {
			_tprintf(_T("You can't reverse in this streets!\n"));
			return 0;
		}
		cdata->taxi->velocity -= 0.5;
		if ((res = NotifyVelocityChange(&cdata->comm.request, &cdata->comm.response, *(cdata->taxi))) != OK) {
			PrintError(res, cdata);
			cdata->taxi->velocity += 0.5;
		}
		else {
			_tprintf(_T("You are slowing down!\n"));
		}
	}
	else if (_tcscmp(cmd[0], TXI_NQ_DEFINE) == 0) {
		if (argc < 2) {
			_tprintf(_T("Too few arguments to use this command!\n"));
			return;
		}
		int aux = cdata->taxi->nq;
		cdata->taxi->nq = _ttoi(cmd[1]);
		if ((res = NotifyCentralNQChange(&cdata->comm.request, &cdata->comm.response, *(cdata->taxi))) != OK) {
			PrintError(res, cdata);
			cdata->taxi->nq = aux;
		}
		else {
			_tprintf(_T("NQ constante updated to %d.\n"), _ttoi(cmd[1]));
		}
	}
	else if (_tcscmp(cmd[0], TXI_AUTOPILOT) == 0) {
		if (cdata->taxi->autopilot == 0) {
			_tprintf(_T("Taxi's autopilot activated.\n"));
			cdata->taxi->autopilot = 1;
			CreateThread(NULL, 0, TaxiAutopilot, cdata, 0, NULL);
		}
		else {
			_tprintf(_T("Taxi's autopilot desactivated.\n"));
			cdata->taxi->autopilot = 0;
		}
	}
	else if (_tcscmp(cmd[0], TXI_UP) == 0) {
		Coords dest;
		dest.x = cdata->taxi->location.x;
		dest.y = cdata->taxi->location.y - 1;
		if (!CanMoveTo(cdata->charMap, dest)) {
			_tprintf(_T("The system doesn't let you crash your car!\n"));
			return;
		}
		else {
			if ((res = UpdateMyLocation(&cdata->comm.request, &cdata->comm.response, cdata->taxi, dest)) == OK) {
				_tprintf(_T("Moving up\n"));
				CopyMemory(&cdata->taxi->location, &dest, sizeof(Coords));
				cdata->taxi->direction = UP;
			}
			else
				PrintError(res, cdata);
		}
	}
	else if (_tcscmp(cmd[0], TXI_LEFT) == 0) {
		Coords dest;
		dest.x = cdata->taxi->location.x - 1;
		dest.y = cdata->taxi->location.y;
		if (!CanMoveTo(cdata->charMap, dest)) {
			_tprintf(_T("The system doesn't let you crash your car!"));
			return;
		}
		else {
			if ((res = UpdateMyLocation(&cdata->comm.request, &cdata->comm.response, cdata->taxi, dest)) == OK) {
				_tprintf(_T("Moving left\n"));
				CopyMemory(&cdata->taxi->location, &dest, sizeof(Coords));
				cdata->taxi->direction = LEFT;
			}
			else
				PrintError(res, cdata);
		}
	}
	else if (_tcscmp(cmd[0], TXI_DOWN) == 0) {
		Coords dest;
		dest.x = cdata->taxi->location.x;
		dest.y = cdata->taxi->location.y + 1;
		if (!CanMoveTo(cdata->charMap, dest)) {
			_tprintf(_T("The system doesn't let you crash your car!"));
			return;
		}
		else {
			if ((res = UpdateMyLocation(&cdata->comm.request, &cdata->comm.response, cdata->taxi, dest)) == OK) {
				CopyMemory(&cdata->taxi->location, &dest, sizeof(Coords));
				_tprintf(_T("Moving down\n"));
				cdata->taxi->direction = DOWN;
			}
			else
				PrintError(res, cdata);
		}
	}
	else if (_tcscmp(cmd[0], TXI_RIGHT) == 0) {
		Coords dest;
		dest.x = cdata->taxi->location.x + 1;
		dest.y = cdata->taxi->location.y;
		if (!CanMoveTo(cdata->charMap, dest)) {
			_tprintf(_T("The system doesn't let you crash your car!"));
			return;
		}
		else {
			if ((res = UpdateMyLocation(&cdata->comm.request, &cdata->comm.response, cdata->taxi, dest)) == OK) {
				CopyMemory(&cdata->taxi->location, &dest, sizeof(Coords));
				_tprintf(_T("Moving right\n"));
				cdata->taxi->direction = RIGHT;
			}
			else
				PrintError(res, cdata);
		}
	}
	else if (_tcscmp(cmd[0], TXI_CATCH) == 0) {
		if ((res = NotifyPassengerCatch(&cdata->comm.request, &cdata->comm.response, cdata->taxi)) != OK) {
			PrintError(res, cdata);
		}
		else {
			_tprintf(_T("Passenger caught!\n"));
		}
	}
	else if (_tcscmp(cmd[0], TXI_DELIVER) == 0) {
		if ((res = NotifyPassengerDeliever(&cdata->comm.request, &cdata->comm.response, cdata->taxi)) != OK) {
			PrintError(res, cdata);
		}
		else {
			_tprintf(_T("Passenger delivered!\n"));
		}
	}
	else if (_tcscmp(cmd[0], TXI_HELP) == 0) {
		for (int i = 0; i < 10; i++)
			_tprintf(_T("%s"), commands[i]);
	}
	else if (_tcscmp(cmd[0], TXI_CLOSE) == 0) {
		_tprintf(_T("Closing taxi client\n"));
		if ((res = NotifyCentralTaxiLeaving(&cdata->comm.request, &cdata->comm.response, *(cdata->taxi))) != OK) {
			PrintError(res, cdata);
		}
		else {
			// ## TODO tratar o close properly
			ReleaseSemaphore(cdata->taxiGate, 1, NULL);
			cdata->isTaxiKicked = TRUE;
			return -1;
		}
	}
	else {
		_tprintf(_T("System doesn't recognize the command, type 'help' to view all the commands.\n"));
	}
	return 0;
}


DWORD WINAPI ReceiveBroadcastMessage(LPVOID ptr) {
	CD_TAXI_Thread* cd = (CD_TAXI_Thread*)ptr;
	CC_Broadcast* broadcast = cd->broadcast;
	SHM_BROADCAST message;
	enum response_id resp;

	while (!cd->isTaxiKicked) {
		if (WaitForSingleObject(broadcast->new_passenger, 2000) == WAIT_TIMEOUT)
			continue;

		WaitForSingleObject(broadcast->mutex, INFINITE);

		CopyMemory(&message, broadcast->shared, sizeof(SHM_BROADCAST));

		ReleaseMutex(broadcast->mutex, INFINITE);

		cd->isTaxiKicked = message.isSystemClosing;

		if (!message.isSystemClosing)
			_tprintf(_T("New passenger joined central: '%s' at {%.2d;%.2d}\n"), broadcast->shared->passenger.nome,
				broadcast->shared->passenger.location.x, broadcast->shared->passenger.location.y);
		else
			_tprintf(_T("Central is closing...\n"));

		if (cd->taxi->autopilot) {
			if (!hasPassenger(cd->taxi)) {
				if (CalculateDistanceTo(cd->taxi->location, message.passenger.location) <= cd->taxi->nq) {
					if ((resp = RequestPassengerTransport(&cd->comm.request, &cd->comm.response, &cd->taxi->client, message.passenger.nome, cd->taxi->licensePlate)) != OK)
						PrintError(resp, cd);
					else {
						_tprintf(_T("Got '%s' as a passenger in {%.2d,%.2d} with the destination {%.2d,%.2d}.\n"), message.passenger.nome,
							message.passenger.location.x, message.passenger.location.y, message.passenger.destination.x, message.passenger.destination.y);
					}
				}
			}
		}
	}
}

DWORD WINAPI TextInterface(LPVOID ptr) {
	CD_TAXI_Thread* cdata = (CD_TAXI_Thread*)ptr;
	TCHAR str[100];
	while (!cdata->isTaxiKicked) {
		PrintPersonalInformation(cdata);
		_tprintf(_T("Command: "));
		_tscanf(_T(" %99[^\n]"), str);
		if (cdata->isTaxiKicked) break;
		if (FindFeatureAndRun(str, cdata) == -1) break;
	}
	return 0;
}
