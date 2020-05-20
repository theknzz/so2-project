#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rules.h"
#include "structs.h"
#include "dll.h"

void PrintError(enum response_id resp) {
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
	}
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
	if (taxi->direction == UP || coords[0].y > MIN_LIN || map[coords[0].x][coords[0].y]== B_DISPLAY) {
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
	int nr=1;
	for (unsigned int i = 1; i < 4; i++) {
		if (positions[i] < optimal) {
			optimal = positions[i];
			nr = i;
		}
	}

	enum response_id ret;
	if ((ret = UpdateMyLocation(request, response, taxi, coords[nr]) == OK)) {
		taxi->location.x = coords[nr].x;
		taxi->location.y = coords[nr].y;
		taxi->client.location.x = coords[nr].x;
		taxi->client.location.y = coords[nr].y;
		taxi->direction = nr;
	}
	return ret;
}

BOOL CanMoveTo(char map[MIN_LIN][MIN_COL], Coords destination) {
	if (destination.x < 0 || destination.y < 0 || destination.x > MIN_COL || destination.y > MIN_LIN) return FALSE;
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

enum response_id MoveAleatorio(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, char map[MIN_LIN][MIN_COL]) {
	Coords positions[4];
	Coords finals[4];
	int nr = 0;

	positions[DOWN] = taxi->location; positions[DOWN].y += 1;
	positions[LEFT] = taxi->location; positions[LEFT].x -= 1;
	positions[UP] = taxi->location; positions[UP].y -= 1;
	positions[RIGHT] = taxi->location; positions[RIGHT].x += 1;

	// invalidate outofbounds locations
	for (unsigned int i = 0; i < 4; i++) {
		if (CanMoveTo(map, positions[i]) && i!=taxi->direction) {
			finals[nr].x = positions[i].x;
			finals[nr].y = positions[i].y;
			nr++;
		}
	}
	int dir = (rand() % nr - 0 + 1) + 0;
	return UpdateMyLocation(request, response, taxi, finals[dir]);
}

BOOL hasPassenger(Taxi* taxi) {
	return taxi->client.location.x < 0? 0 : 1;
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

		if (isPassengerLocation(cdata->taxi)) {
			if ((resp = NotifyPassengerCatch(cdata->comm->request, cdata->comm->response, *(cdata->taxi))) != OK)
				PrintError(resp);
			else {
				cdata->taxi->client.state = OnDrive;
				_tprintf(_T("Passenger caught!\n"));
			}
		}
		else if (isPassengerDestination(cdata->taxi)) {
			if ((resp = NotifyPassengerDeliever(cdata->comm->request, cdata->comm->response, *(cdata->taxi))) != OK)
				PrintError(resp);
			else {
				_tprintf(_T("Passenger delvired!\n"));
			}
		}

		if (cdata->taxi->client.state == OnDrive) {
			if (MoveMeToOptimalPosition(cdata->comm->request, cdata->comm->response, cdata->taxi, cdata->taxi->client.destination, cdata->charMap) == OK)
				_tprintf(_T("Taxi location updated!\n"));
			else
				_tprintf(_T("For some reason taxi can't move that way!\n"));
		}
		else if (cdata->taxi->client.state == Waiting) {
			if (MoveMeToOptimalPosition(cdata->comm->request, cdata->comm->response, cdata->taxi, cdata->taxi->client.location, cdata->charMap) == OK)
				_tprintf(_T("Taxi location updated!\n"));
			else
				_tprintf(_T("For some reason taxi can't move that way!\n"));
		}
	}
	else {
		if (MoveAleatorio(cdata->comm->request, cdata->comm->response, cdata->taxi, cdata->charMap)==OK)
			_tprintf(_T("Taxi location updated!\n"));
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
	DueTime.QuadPart = - (LONGLONG) interval * 1000 * 10;

	//if ((hTimer = CreateWaitableTimer(NULL, FALSE, NULL)) == NULL) {
	//	_tprintf(_T("Error (%d) creating the waitable timer.\n"), GetLastError());
	//	return -1;
	//}
	/*while (1) {
		if (!SetWaitableTimer(hTimer, &DueTime, interval, NULL, NULL, FALSE)) {
			_tprintf(_T("Something went wrong! %d"), GetLastError());
		}*/
		moveTaxi(cdata);
	//}
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
		if ((res = RequestPassengerTransport(cdata->comm->request, cdata->comm->response, &cdata->taxi->client, cmd[1])) != OK)
			PrintError(res);
		else
			_tprintf(_T("Got '%s' as a passenger in {%.2d,%.2d} with the destination {%.2d,%.2d}.\n"), cdata->taxi->client.nome,
				cdata->taxi->client.location.x, cdata->taxi->client.location.y, cdata->taxi->client.destination.x, cdata->taxi->client.destination.y);
	}
	else if (_tcscmp(cmd[0], TXI_SPEED_UP) == 0) {
		_tprintf(_T("speed\n"));
		cdata->taxi->velocity += 0.5;
		if ((res = NotifyVelocityChange(cdata->comm->request, cdata->comm->response, *(cdata->taxi))) != OK) {
			PrintError(res);
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
		if ((res = NotifyVelocityChange(cdata->comm->request, cdata->comm->response, *(cdata->taxi))) != OK) {
			PrintError(res);
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
		if ((res = NotifyCentralNQChange(cdata->comm->request, cdata->comm->response, *(cdata->taxi))) != OK) {
			PrintError(res);
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
			//CreateThread(NULL, 0, TaxiAutopilot, cdata, 0, NULL);
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
			if (UpdateMyLocation(cdata->comm->request, cdata->comm->response, cdata->taxi, dest) == OK) {
				_tprintf(_T("Moving up\n"));
				CopyMemory(&cdata->taxi->location, &dest, sizeof(Coords));
				cdata->taxi->direction = UP;
			}
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
			if (UpdateMyLocation(cdata->comm->request, cdata->comm->response, cdata->taxi, dest) == OK) {
				_tprintf(_T("Moving left\n"));
				CopyMemory(&cdata->taxi->location, &dest, sizeof(Coords));
				cdata->taxi->direction = LEFT;
			}
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
			if (UpdateMyLocation(cdata->comm->request, cdata->comm->response, cdata->taxi, dest) == OK) {
				CopyMemory(&cdata->taxi->location, &dest, sizeof(Coords));
				_tprintf(_T("Moving down\n"));
				cdata->taxi->direction = DOWN;
			}
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
			if (UpdateMyLocation(cdata->comm->request, cdata->comm->response, cdata->taxi, dest) == OK) {
				CopyMemory(&cdata->taxi->location, &dest, sizeof(Coords));
				_tprintf(_T("Moving right\n"));
				cdata->taxi->direction = RIGHT;
			}
		}
	}
	else if (_tcscmp(cmd[0], TXI_CATCH) == 0) {
		if ((res = NotifyPassengerCatch(cdata->comm->request, cdata->comm->response, *cdata->taxi)) != OK) {
			PrintError(res);
		}
		else {
			cdata->taxi->client.state = OnDrive;
			_tprintf(_T("Passenger caught!\n"));
		}
	}
	else if (_tcscmp(cmd[0], TXI_DELIVER) == 0) {
		if ((res = NotifyPassengerDeliever(cdata->comm->request, cdata->comm->response, *cdata->taxi)) != OK) {
			PrintError(res);
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
		NotifyCentralTaxiLeaving(cdata->comm->request, cdata->comm->response, *(cdata->taxi));
		// ## TODO tratar o close properly
		ReleaseSemaphore(cdata->taxiGate, 1, NULL);
		return -1;
	}
	else {
		_tprintf(_T("System doesn't recognize the command, type 'help' to view all the commands.\n"));
	}
	return 0;
}


DWORD WINAPI ReceiveBroadcastMessage(LPVOID ptr) {
	CD_TAXI_Thread* cd = (CD_TAXI_Thread*)ptr;
	CC_Broadcast* broadcast = cd->broadcast;
	Passenger newPassenger;
	enum response_id resp;

	while (1) {
		WaitForSingleObject(broadcast->new_passenger, INFINITE);

		WaitForSingleObject(broadcast->mutex, INFINITE);
		
		_tprintf(_T("New passenger joined central: '%s' at {%.2d;%.2d}\n"), broadcast->shared->passenger.nome,
			broadcast->shared->passenger.location.x, broadcast->shared->passenger.location.y);

		CopyMemory(&newPassenger, &broadcast->shared->passenger, sizeof(Passenger));

		ReleaseMutex(broadcast->mutex, INFINITE);

		if (cd->taxi->autopilot) {
			if (!hasPassenger(cd->taxi)) {
				if (CalculateDistanceTo(cd->taxi->location, newPassenger.location) <= cd->taxi->nq) {
					if ((resp = RequestPassengerTransport(cd->comm->request, cd->comm->response, &cd->taxi->client, newPassenger.nome)) != OK)
						PrintError(resp);
					else {
						_tprintf(_T("Got '%s' as a passenger in {%.2d,%.2d} with the destination {%.2d,%.2d}.\n"), newPassenger.nome,
							newPassenger.location.x, newPassenger.location.y, newPassenger.destination.x, newPassenger.destination.y);
					}
				}
			}
		}
		// O evento desbloqueia todos os taxis ao mesmo tempo?
		// SetEvent(broadcast->new_passenger);
	}
}

DWORD WINAPI TextInterface(LPVOID ptr) {
	CD_TAXI_Thread* cdata = (CD_TAXI_Thread*)ptr;
	TCHAR command[100];

	while (1) {
		_tprintf(_T("Command: "));
		_tscanf_s(_T(" %99[^\n]"), command, sizeof(TCHAR) * 100);
		if (FindFeatureAndRun(command, cdata) == -1) break;
	}
	_tprintf(_T("text interface closing...\n"));
	return 0;
}

int _tmain(int argc, TCHAR* argv[]) {

	HANDLE handles[50];
	HANDLE threads[50];
	HANDLE views[50];
	int handleCounter = 0, threadCounter = 0, viewCounter = 0;
	// for the random numbers
	srand(time(0));
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	HANDLE taxiGate;
	if ((taxiGate = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, TAXI_GATEWAY)) == NULL) {
		_tprintf(_T("Error %d opening the gateway semaphore!\n"));
		Sleep(1000 * 2);
		exit(-1);
	}
	_tprintf(_T("Waiting on the taxi gate!\n"));
	WaitForSingleObject(taxiGate, INFINITE);
	_tprintf(_T("Got throw the taxi gate!\n"));

	handles[handleCounter++] = taxiGate;

	HANDLE FM_BROADCAST = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_BROADCAST),
		SHM_BROADCAST_PASSENGER_ARRIVE);
	if (FM_BROADCAST == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		exit(-1);
	}
	handles[handleCounter++] = FM_BROADCAST;

	CC_Broadcast broadcast;
	broadcast.shared = (SHM_BROADCAST*)MapViewOfFile(FM_BROADCAST,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_BROADCAST));
	if (broadcast.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = broadcast.shared;

	broadcast.mutex = OpenMutex(SYNCHRONIZE, FALSE, BROADCAST_MUTEX);
	if (broadcast.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = broadcast.mutex;

	broadcast.new_passenger = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_NEW_PASSENGER);
	if (broadcast.new_passenger  == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = broadcast.new_passenger;


	HANDLE FM_LOGIN_REQUEST = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_LOGIN_REQUEST),
		SHM_LOGIN_REQUEST_NAME);
	if (FM_LOGIN_REQUEST == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		exit(-1);
	}
	handles[handleCounter++] = FM_LOGIN_REQUEST;

	HANDLE FM_LOGIN_RESPONSE = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_LOGIN_RESPONSE),
		SHM_LOGIN_RESPONSE_NAME);
	if (FM_LOGIN_RESPONSE == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		exit(-1);
	}
	handles[handleCounter++] = FM_LOGIN_RESPONSE;

	CDLogin_Request cdLogin_Request;
	CDLogin_Response cdLogin_Response;

	cdLogin_Request.request = (SHM_LOGIN_REQUEST*)MapViewOfFile(FM_LOGIN_REQUEST,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_LOGIN_REQUEST));
	if (cdLogin_Request.request == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = cdLogin_Request.request;

	cdLogin_Response.response = (SHM_LOGIN_RESPONSE*)MapViewOfFile(FM_LOGIN_RESPONSE,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_LOGIN_RESPONSE));
	if (cdLogin_Response.response == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = cdLogin_Response.response;

	cdLogin_Request.login_m = OpenMutex(SYNCHRONIZE, FALSE, LOGIN_REQUEST_MUTEX);
	if (cdLogin_Request.login_m == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = cdLogin_Request.login_m;

	cdLogin_Response.login_m = OpenMutex(SYNCHRONIZE, FALSE, LOGIN_RESPONSE_MUTEX);
	if (cdLogin_Response.login_m == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = cdLogin_Response.login_m;

	cdLogin_Request.new_response = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_WRITE_FROM_TAXIS);
	if (cdLogin_Request.new_response == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = cdLogin_Request.new_response;

	cdLogin_Response.new_request = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_READ_FROM_TAXIS);
	if (cdLogin_Response.new_request == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = cdLogin_Response.new_request;

	TCHAR licensePlate[9];
	Coords coords;

	while (1) {
		_tprintf(_T("Insert your license plate: "));
		_tscanf_s(TEXT(" %9s"), licensePlate, 9 * sizeof(TCHAR));
		if (_tcslen(licensePlate) < 8 || _tcslen(licensePlate) > 8)
		{
			_tprintf(_T("Insert a valid license plate. Example: xx-xx-xx\n"));
			continue;
		}
		_tprintf(_T("Where are you located?\nx: "));
		_tscanf_s(TEXT(" %d"), &coords.x, sizeof(coords.x));
		if (coords.x < 0 && coords.x > MIN_COL) {
			_tprintf(_T("Insert valid x location\n"));
			continue;
		}
		_tprintf(_T("y: "));
		_tscanf_s(TEXT(" %d"), &coords.y, sizeof(coords.y));
		if (coords.y > MIN_LIN && coords.y < 0) {
			_tprintf(_T("Insert valid y location\n"));
			continue;
		}
		break;
	}
	CDThread cdThread;
	cdThread.cdLogin_Request = &cdLogin_Request;
	cdThread.cdLogin_Response = &cdLogin_Response;

	LR_Container res;
	enum responde_id resp = RegisterInCentral(&res, cdThread, licensePlate, coords);
	if (resp != OK)
	{
		CloseMyHandles(handles, handleCounter);
		UnmapAllViews(views, viewCounter);
		PrintError(resp);
		Sleep(5000);
		exit(-1);
	}

	//CloseMyHandles(handles, handleCounter);
	//UnmapAllViews(views, viewCounter);

	CC_CDRequest request;
	CC_CDResponse response;

	HANDLE FM_REQUEST = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_CC_REQUEST),
		res.request_shm_name);
	if (FM_REQUEST == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		exit(-1);
	}
	handles[handleCounter++] = FM_REQUEST;

	request.shared = (SHM_CC_REQUEST*)MapViewOfFile(FM_REQUEST,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_CC_REQUEST));
	if (request.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = request.shared;

	request.mutex = OpenMutex(SYNCHRONIZE, FALSE, res.request_mutex_name);
	if (request.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = request.mutex;

	request.new_response = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, res.response_event_name);
	if (request.new_response == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = request.new_response;

	HANDLE FM_RESPONSE = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_CC_REQUEST),
		res.response_shm_name);
	if (FM_RESPONSE == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		exit(-1);
	}
	handles[handleCounter++] = FM_RESPONSE;

	response.shared = (SHM_CC_RESPONSE*)MapViewOfFile(FM_RESPONSE,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_CC_RESPONSE));
	if (response.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = response.shared;

	response.mutex = OpenMutex(SYNCHRONIZE, FALSE, res.response_mutex_name);
	if (response.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = response.mutex;

	response.new_request = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, res.request_event_name);
	if (response.new_request == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = response.new_request;

	CC_Comm cc_comm;
	cc_comm.request = &request;
	cc_comm.response = &response;
	cc_comm.container = &res;

	CD_TAXI_Thread cd;
	Taxi me;
	cd.comm = &cc_comm;
	CopyMemory(me.licensePlate, licensePlate, sizeof(TCHAR) * 9);
	me.autopilot = 0;
	me.location.x = coords.x;
	me.location.y = coords.y;
	me.velocity = 1;
	me.nq = NQ;
	cd.taxi = &me;
	cd.taxiGate = taxiGate;
	cd.broadcast = &broadcast;

	enum response_id ret;
	ret = GetMap(cd.charMap, &request, &response);
	if (ret != OK) {
		PrintError(ret);
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
	}

	for (unsigned int i = 0; i < MIN_LIN; i++) {
		for (unsigned int j = 0; j < MIN_COL; j++)
			_tprintf(_T("%c"), cd.charMap[j][i]);
		_tprintf(_T("\n"));
	}

	if ((threads[threadCounter++] = CreateThread(NULL, 0, ReceiveBroadcastMessage, &cd, 0, NULL)) == NULL) {
		_tprintf(_T("Error launching comm thread (%d)\n"), GetLastError());
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	if ((threads[threadCounter++] = CreateThread(NULL, 0, TextInterface, &cd, 0, NULL)) == NULL) {
		_tprintf(_T("Error launching comm thread (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}


	WaitAllThreads(threads, threadCounter);
	UnmapAllViews(views, viewCounter);
	CloseMyHandles(handles, handleCounter);
}

