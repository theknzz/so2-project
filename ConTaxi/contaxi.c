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
	}
}

int CalculateDistanceTo(Coords org, Coords dest) {
	int y_dist, x_dist;

	y_dist = dest.y - org.y;
	x_dist = dest.x - org.x;

	return (int)sqrt(pow(y_dist, 2) + pow(x_dist, 2));
}

enum response_id MoveMeToOptimalPosition(CC_CDRequest* request, CC_CDResponse* response, TCHAR* lPlate, Coords org, Coords dest, char map[MIN_LIN][MIN_COL]) {
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
	if (coords[0].y > MIN_LIN || map[coords[0].x][coords[0].y]== B_DISPLAY) {
		positions[0] = INT_MAX;
	}
	else {
		positions[0] = CalculateDistanceTo(coords[0], dest);
	}

	if (coords[1].x < 0 || map[coords[1].x][coords[1].y] == B_DISPLAY) {
		positions[1] = INT_MAX;
	}
	else {
		positions[1] = CalculateDistanceTo(coords[1], dest);
	}

	if (coords[2].y < 0 || map[coords[2].x][coords[2].y] == B_DISPLAY) {
		positions[2] = INT_MAX;
	}
	else {
		positions[2] = CalculateDistanceTo(coords[2], dest);
	}

	if (coords[3].y > MIN_COL || map[coords[3].x][coords[3].y] == B_DISPLAY) {
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

	return UpdateMyLocation(request, response, lPlate, coords[nr]);
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

void FindFeatureAndRun(TCHAR* command, CC_Comm* cdata) {
	Passenger passenger;
	TCHAR commands[9][100] = {
		_T("\ttransport - request a passanger transport.\n"),
		_T("\tspeed - increase the speed in 0.5 cells/s.\n"),
		_T("\tslow - decrease the speed in 0.5 cells/s.\n"),
		_T("\tnq x - assign x to the nq variable.\n"),
		_T("\tautopilot - turn on/off the taxi's autopilot.\n"),
		_T("\tdown - move the taxi one cell down.\n"),
		_T("\tleft - move the taxi one cell left.\n"),
		_T("\tup - move the taxi one cell up.\n"),
		_T("\tright - move the taxi one cell right.\n"),
	};

	TCHAR cmd[4][100];
	CopyMemory(cmd, ParseCommand(command), sizeof(TCHAR) * 4 * 100);
	int argc = 0;
	for (int i = 0; i < 4; i++)
		if (_tcscmp(cmd[i], _T("NULL")) != 0) argc++;

	if (_tcscmp(cmd[0], TXI_TRANSPORT) == 0) {
		_tprintf(_T("transport\n"));
		enum response_id res;
		if ((res = RequestPassengerTransport(cdata->request, cdata->response, &passenger)) != OK)
			PrintError(res);
		//else
		//	_tprintf(_tprintf(_T("Got new passenger!\n")));
	}
	else if (_tcscmp(cmd[0], TXI_SPEED_UP) == 0) {
		_tprintf(_T("speed\n"));
	}
	else if (_tcscmp(cmd[0], TXI_SLOW_DOWN) == 0) {
		_tprintf(_T("slow\n"));
	}
	else if (_tcscmp(cmd[0], TXI_NQ_DEFINE) == 0) {
		_tprintf(_T("nr : %d\n"), _ttoi(cmd[1]));
	}
	else if (_tcscmp(cmd[0], TXI_AUTOPILOT) == 0) {
		_tprintf(_T("autopilot %d\n"), cmd[1]);
	}
	else if (_tcscmp(cmd[0], TXI_UP) == 0) {
		_tprintf(_T("Moving up\n"));
	}
	else if (_tcscmp(cmd[0], TXI_LEFT) == 0) {
		_tprintf(_T("Moving left\n"));
	}
	else if (_tcscmp(cmd[0], TXI_DOWN) == 0) {
		_tprintf(_T("Moving down\n"));
	}
	else if (_tcscmp(cmd[0], TXI_RIGHT) == 0) {
		_tprintf(_T("Moving right\n"));
	}
	else if (_tcscmp(cmd[0], TXI_HELP) == 0) {
		for (int i = 0; i < 9; i++)
			_tprintf(_T("%s"), commands[i]);
	}
	else {
		_tprintf(_T("System doesn't recognize the command, type 'help' to view all the commands.\n"));
	}
}

DWORD WINAPI TextInterface(LPVOID ptr) {
	CC_Comm* cdata = (CC_Comm*)ptr;
	TCHAR command[100];

	while (1) {
		_tprintf(_T("Command: "));
		_tscanf_s(_T(" %99[^\n]"), command, sizeof(TCHAR) * 100);
		FindFeatureAndRun(command, cdata);
	}
	return 0;
}

//Coords MoveMeToOptimalPosition_DEBUG(Coords org, Coords dest, char map[MIN_LIN][MIN_COL]) {
//	// bottom, left, top, right
//	int positions[4];
//	Coords coords[4];
//
//	// bottom
//	coords[0].x = org.x;
//	coords[0].y = org.y + 1;
//
//	// left
//	coords[1].x = org.x - 1;
//	coords[1].y = org.y;
//
//	// top
//	coords[2].x = org.x;
//	coords[2].y = org.y - 1;
//
//	// right
//	coords[3].x = org.x + 1;
//	coords[3].y = org.y;
//
//	// se nesta posição estiver uma celula de edificio ou um taxi, ou desqualificou-a do algoritmo
//	if (coords[0].y > MIN_LIN || map[coords[0].x][coords[0].y] == B_DISPLAY) {
//		positions[0] = INT_MAX;
//	}
//	else {
//		positions[0] = CalculateDistanceTo(coords[0], dest);
//	}
//
//	if (coords[1].x < 0 || map[coords[1].x][coords[1].y] == B_DISPLAY) {
//		positions[1] = INT_MAX;
//	}
//	else {
//		positions[1] = CalculateDistanceTo(coords[1], dest);
//	}
//
//	if (coords[2].y < 0 || map[coords[2].x][coords[2].y] == B_DISPLAY) {
//		positions[2] = INT_MAX;
//	}
//	else {
//		positions[2] = CalculateDistanceTo(coords[2], dest);
//	}
//
//	if (coords[3].y > MIN_COL || map[coords[3].x][coords[3].y] == B_DISPLAY) {
//		positions[3] = INT_MAX;
//	}
//	else {
//		positions[3] = CalculateDistanceTo(coords[3], dest);
//	}
//
//	// select the best position (less distance)
//	int optimal = positions[0];
//	int nr = 0;
//	for (unsigned int i = 1; i < 4; i++) {
//		if (positions[i] < optimal) {
//			optimal = positions[i];
//			nr = i;
//		}
//	}
//	return coords[nr];
//}

int _tmain(int argc, TCHAR* argv[]) {

	HANDLE handles[50];
	HANDLE threads[50];
	HANDLE views[50];
	int handleCounter = 0, threadCounter = 0, viewCounter = 0;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

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

	CloseMyHandles(handles, handleCounter);
	UnmapAllViews(views, viewCounter);

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

	//CD_TAXI_Thread cd;
	//cd.comm = &cc_comm;
	//CopyMemory(cd.taxi->licensePlate, licensePlate, sizeof(TCHAR) * 9);
	//cd.taxi->autopilot = 0;
	//cd.taxi->location.x = coords.x;
	//cd.taxi->location.y = coords.y;
	//cd.taxi->velocity = 1;

	if ((threads[threadCounter++] = CreateThread(NULL, 0, TextInterface, &cc_comm, 0, NULL)) == NULL) {
		_tprintf(_T("Error launching comm thread (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	enum response_id ret;
	char map[MIN_LIN][MIN_COL];
	ret = GetMap(map, &request, &response);
	if (ret != OK) {
		PrintError(ret);
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
	}
	Coords dest;
	dest.x = 2;
	dest.y = 5;

	//ret = MoveMeToOptimalPosition(&request, &response, licensePlate, coords, dest, &map);
	//if (ret != OK) {
	//	PrintError(ret);
	//}
	//else
	//	_tprintf(_T("Taxi moved to optimal position!\n"));

	WaitAllThreads(threads, threadCounter);
	UnmapAllViews(views, viewCounter);
	CloseMyHandles(handles, handleCounter);
}

