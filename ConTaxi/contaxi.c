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

	HANDLE handles[50];
	HANDLE threads[50];
	HANDLE views[50];
	int handleCounter = 0, threadCounter = 0, viewCounter = 0;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
	
	//HANDLE fmCenTaxiToConTaxi = CreateFileMapping(
	//	INVALID_HANDLE_VALUE,
	//	NULL,
	//	PAGE_READWRITE,
	//	0,
	//	sizeof(SHM_CC_REQUEST),
	//	SHM_CC_REQUEST_NAME);
	//if (fmCenTaxiToConTaxi == NULL) {
	//	_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
	//	exit(-1);
	//}
	//handles[handleCounter++] = fmCenTaxiToConTaxi;

	//HANDLE FM_CC_RESPONSE = CreateFileMapping(
	//	INVALID_HANDLE_VALUE,
	//	NULL,
	//	PAGE_READWRITE,
	//	0,
	//	sizeof(SHM_CC_RESPONSE),
	//	SHM_CC_RESPONSE_NAME);
	//if (FM_CC_RESPONSE == NULL) {
	//	_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
	//	exit(-1);
	//}
	//handles[handleCounter++] = FM_CC_RESPONSE;

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

	//CC_CDRequest controlDataTaxi;
	//CC_CDResponse cdResponse;
	CDLogin_Request cdLogin_Request;
	CDLogin_Response cdLogin_Response;

	//controlDataTaxi.shared = (SHM_CC_REQUEST*)MapViewOfFile(fmCenTaxiToConTaxi,
	//	FILE_MAP_ALL_ACCESS,
	//	0,
	//	0,
	//	sizeof(SHM_CC_REQUEST));
	//if (controlDataTaxi.shared == NULL) {
	//	_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
	//	//WaitAllThreads(threads, threadCounter);
	//	//CloseMyHandles(handles, handleCounter);
	//	exit(-1);
	//}
	//views[viewCounter++] = controlDataTaxi.shared;

	//cdResponse.shared = (SHM_CC_RESPONSE*)MapViewOfFile(FM_CC_RESPONSE,
	//	FILE_MAP_ALL_ACCESS,
	//	0,
	//	0,
	//	sizeof(SHM_CC_RESPONSE));
	//if (cdResponse.shared == NULL) {
	//	_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
	//	//WaitAllThreads(threads, threadCounter);
	//	//CloseMyHandles(handles, handleCounter);
	//	exit(-1);
	//}
	//views[viewCounter++] = cdResponse.shared;

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

	//controlDataTaxi.read_event = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_READ_FROM_TAXIS);
	//if (controlDataTaxi.read_event == NULL) {
	//	_tprintf(_T("Error creating read event (%d)\n"), GetLastError());
	//	//WaitAllThreads(threads, threadCounter);
	//	//UnmapAllViews(views, viewCounter);
	//	//CloseMyHandles(handles, handleCounter);
	//	exit(-1);
	//}
	//handles[handleCounter++] = controlDataTaxi.read_event;

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

	//cdResponse.got_response = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_GOT_RESPONSE);
	//if (cdResponse.got_response == NULL) {
	//	_tprintf(_T("Error creating got_response event (%d)\n"), GetLastError());
	//	//WaitAllThreads(threads, threadCounter);
	//	//UnmapAllViews(views, viewCounter);
	//	//CloseMyHandles(handles, handleCounter);
	//	exit(-1);
	//}
	//handles[handleCounter++] = cdResponse.got_response;

	TCHAR licensePlate[9];/* = (TCHAR*) malloc (sizeof(TCHAR) * 10);*/
	Coords coords;
	
	_tprintf(_T("Insert your license plate: "));
	_tscanf_s(TEXT(" %9s"), licensePlate, 9 * sizeof(TCHAR));
	
	_tprintf(_T("Where are you located?\nx: "));
	_tscanf_s(TEXT(" %d"), &coords.x, sizeof(coords.x));
	_tprintf(_T("y: "));
	_tscanf_s(TEXT(" %d"), &coords.y, sizeof(coords.y));

	CDThread cdThread;
	//cdThread.controlDataTaxi = controlDataTaxi;
	//cdThread.cdResponse = cdResponse;
	cdThread.cdLogin_Request = cdLogin_Request;
	cdThread.cdLogin_Response = cdLogin_Response;

	LR_Container res = RegisterInCentral(cdThread, licensePlate, coords);
	//TCHAR request_shm_name[50];
	//TCHAR request_mutex_name[50];
	//TCHAR request_event_name[50];

	//TCHAR response_shm_name[50];
	//TCHAR response_mutex_name[50];
	//TCHAR response_event_name[50];

	CC_Comm comm;

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
	
	comm.request.shared = (SHM_CC_REQUEST*)MapViewOfFile(FM_REQUEST,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_CC_REQUEST));
	if (comm.request.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = comm.request.shared;

	comm.request.mutex = OpenMutex(SYNCHRONIZE, FALSE, res.request_mutex_name);
	if (comm.request.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = comm.request.mutex;

	comm.request.new_response = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, res.request_event_name);
	if (comm.request.new_response == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = comm.request.new_response;

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

	comm.response.shared = (SHM_CC_RESPONSE*)MapViewOfFile(FM_RESPONSE,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_CC_RESPONSE));
	if (comm.response.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = comm.response.shared;

	comm.response.mutex = OpenMutex(SYNCHRONIZE, FALSE, res.response_mutex_name);
	if (comm.response.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = comm.response.mutex;

	comm.response.new_request = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, res.response_event_name);
	if (comm.response.new_request == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = comm.response.new_request;

	HANDLE thread = CreateThread(NULL, 0, ListenToLoginRequests, &cdThread, 0, NULL);
	if (!thread) {
		_tprintf(_T("Error launching console thread (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	threads[threadCounter++] = thread;

	/*coords.x++;
	coords.y++;
	res = UpdateMyLocation(cdThread, licensePlate, coords);
	if (res == OK)
		_tprintf(_T("My position was updated to {%.2d;%.2d}.\n"), coords.x, coords.y);
	else
		_tprintf(_T("Error got me... lol\n"));*/

	WaitAllThreads(threads, threadCounter);
	UnmapAllViews(views, viewCounter);
	CloseMyHandles(handles, handleCounter);
}