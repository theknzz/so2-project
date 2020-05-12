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

int CalculateDistanceTo(Coords org, Coords dest) {
	int y_dist, x_dist;

	y_dist = dest.y - org.y;
	x_dist = dest.x - org.x;

	return (int)sqrt(pow(y_dist, 2) + pow(x_dist, 2));
}

void MoveMeToOptimalPosition(Coords org, Coords dest, Cell* map) {
	// bottom, left, top, right
	int positions[4];
	Coords coords[4];

	// bottom
	coords[0].x = org.x;
	coords[0].y = org.y + 1;
	// se nesta posição estiver uma celula de edificio ou um taxi, ou desqualificou-a do algoritmo
	if ((*(map + ((coords[0].x) + coords[0].y * MIN_LIN))).cellType == B_CHAR ||
		(*(map + ((coords[0].x) + coords[0].y * MIN_LIN))).cellType == T_CHAR) {
		positions[0] = INT_MAX;
	}
	else {
		positions[0] = CalculateDistanceTo(coords[0], dest);
	}

	// left
	coords[1].x = org.x - 1;
	coords[1].y = org.y;
	if ((*(map + ((coords[1].x) + coords[1].y * MIN_LIN))).cellType == B_CHAR ||
		(*(map + ((coords[1].x) + coords[1].y * MIN_LIN))).cellType == T_CHAR) {
		positions[1] = INT_MAX;
	}
	else {
		positions[1] = CalculateDistanceTo(coords[1], dest);
	}

	// top
	coords[2].x = org.x;
	coords[2].y = org.y - 1;
	if ((*(map + ((coords[2].x) + coords[2].y * MIN_LIN))).cellType == B_CHAR ||
		(*(map + ((coords[2].x) + coords[2].y * MIN_LIN))).cellType == T_CHAR) {
		positions[2] = INT_MAX;
	}
	else {
		positions[2] = CalculateDistanceTo(coords[2], dest);
	}

	// right
	coords[3].x = org.x + 1;
	coords[3].y = org.y;
	if ((*(map + ((coords[2].x) + coords[2].y * MIN_LIN))).cellType == B_CHAR ||
		(*(map + ((coords[2].x) + coords[2].y * MIN_LIN))).cellType == T_CHAR) {
		positions[3] = INT_MAX;
	}
	else {
		positions[3] = CalculateDistanceTo(coords[3], dest);
	}

	// select the best position (less distance)
	int optimal = positions[0];
	int i;
	for (i = 1; i < 4; i++) {
		if (positions[i] < optimal)
			optimal = positions[i];
	}

	// atualizar a nova celula

	//CopyMemory(&(*(map + ((coords[i].x + coords[i].y * MIN_LIN)))).taxi, &(*(map + ((org.x + org.y * MIN_LIN)))).taxi, sizeof(Taxi));
	//(*(map + ((coords[i].x + coords[i].y * MIN_LIN)))).cellType = T_CHAR;

	// atualizar a celula antiga

	//ZeroMemory(&(*(map + ((org.x + org.y * MIN_LIN)))).taxi, sizeof(Taxi));
	//(*(map + ((org.x + org.y * MIN_LIN)))).cellType = S_CHAR;

	_tprintf(_T("My new positions is: {%.2d; %.2d}\n"), coords[i].x, coords[i].y);
}

// Wait for all the threads to stop
void WaitAllThreads(HANDLE* threads, int* nr) {
	for (int i = 0; i < *(nr); i++) {
		WaitForSingleObject(threads[i], INFINITE);
	}
	*(nr) = 0;
}

// Close all open handles
void CloseMyHandles(HANDLE* handles, int* nr) {
	for (int i = 0; i < *(nr); i++)
		CloseHandle(handles[i]);
	*(nr) = 0;
}

// Unmaps all views
void UnmapAllViews(HANDLE* views, int* nr) {
	for (int i = 0; i < *(nr); i++) {
		UnmapViewOfFile(views[i]);
	}
	*(nr) = 0;
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

void FindFeatureAndRun(TCHAR* command, TI_Controldata* cdata) {
	TCHAR commands[6][100] = {
		_T("\tkick x - kick taxi with x as id.\n"),
		_T("\tclose - close the system.\n"),
		_T("\tlist - list all the taxis in the system.\n"),
		_T("\tpause - pauses taxis acceptance in the system.\n"),
		_T("\tresume - resumes taxis acceptance in the system.\n"),
		_T("\tinteval x - changes the time central waits for the taxis to ask for work.\n") };

	TCHAR cmd[4][100];
	CopyMemory(cmd, ParseCommand(command), sizeof(TCHAR) * 4 * 100);
	int argc = 0;
	for (int i = 0; i < 4; i++)
		if (_tcscmp(cmd[i], _T("NULL")) != 0) argc++;

	if (_tcscmp(cmd[0], TXI_TRANSPORT) == 0) {
	}
	else if (_tcscmp(cmd[0], TXI_SPEED_UP) == 0) {
	}
	else if (_tcscmp(cmd[0], TXI_SLOW_DOWN) == 0) {
		_tprintf(_T("System pause\n"));
	}
	else if (_tcscmp(cmd[0], TXI_NQ_DEFINE) == 0) {
		_tprintf(_T("System resume\n"));
	}
	else if (_tcscmp(cmd[0], TXI_AUTOPILOT) == 0) {
	}
	else if (_tcscmp(cmd[0], TXI_HELP) == 0) {
		for (int i = 0; i < 6; i++)
			_tprintf(_T("%s"), commands[i]);
	}
	else {
		_tprintf(_T("System doesn't recognize the command, type 'help' to view all the commands.\n"));
	}
}

DWORD WINAPI TextInterface(LPVOID ptr) {
	TI_Controldata* cdata = (TI_Controldata*)ptr;
	TCHAR command[100];

	while (1) {
		//ClearScreen();
		//PrintMap(cdata->map);
		_tprintf(_T("Command: "));
		_tscanf_s(_T(" %99[^\n]"), command, sizeof(TCHAR) * 100);
		FindFeatureAndRun(command, cdata);
	}
	return 0;
}

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

	//TextInterface(NULL);

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

	LR_Container res = RegisterInCentral(cdThread, licensePlate, coords);

	CloseMyHandles(handles, &handleCounter);
	UnmapAllViews(views, &viewCounter);

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

	enum response_id ret;
	char map[MIN_LIN][MIN_COL];
	ret = GetMap(map, &request, &response);
	if (ret == OK) {
		_tprintf(_T("\n"));
		for (unsigned int i = 0; i < MIN_LIN; i++) {
			for (unsigned int j = 0; j < MIN_COL; j++)
				_tprintf(_T("%c"), map[i][j]);
			_tprintf(_T("\n"));
		}
	}
	else
		_tprintf(_T("rip.\n"));


	coords.x++;
	coords.y++;
	ret = UpdateMyLocation(&request, &response, licensePlate, coords);
	if (ret == OK)
		_tprintf(_T("My position was updated to {%.2d;%.2d}.\n"), coords.x, coords.y);
	else
		_tprintf(_T("Error got me... lol\n"));

	WaitAllThreads(threads, threadCounter);
	UnmapAllViews(views, viewCounter);
	CloseMyHandles(handles, handleCounter);
}