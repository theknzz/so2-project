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
	
	HANDLE fmCenTaxiToConTaxi = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_CC_REQUEST),
		SHM_CC_REQUEST_NAME);
	if (fmCenTaxiToConTaxi == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		exit(-1);
	}

	handles[handleCounter++] = fmCenTaxiToConTaxi;

	HANDLE FM_CC_RESPONSE = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_CC_RESPONSE),
		SHM_CC_RESPONSE_NAME);
	if (fmCenTaxiToConTaxi == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		exit(-1);
	}

	handles[handleCounter++] = FM_CC_RESPONSE;

	CC_CDRequest controlDataTaxi;
	CC_CDResponse cdResponse;

	controlDataTaxi.shared = (SHM_CC_REQUEST*)MapViewOfFile(fmCenTaxiToConTaxi,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_CC_REQUEST));
	if (controlDataTaxi.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = controlDataTaxi.shared;

	cdResponse.shared = (SHM_CC_RESPONSE*)MapViewOfFile(fmCenTaxiToConTaxi,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_CC_RESPONSE));
	if (cdResponse.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	views[viewCounter++] = cdResponse.shared;

	controlDataTaxi.mutex = OpenMutex(SYNCHRONIZE, FALSE, CENTRAL_MUTEX);
	if (controlDataTaxi.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	handles[handleCounter++] = controlDataTaxi.mutex;

	//controlDataTaxi.read_event = CreateEvent(NULL, FALSE, FALSE, EVENT_READ_FROM_TAXIS);
	controlDataTaxi.read_event = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_READ_FROM_TAXIS);
	if (controlDataTaxi.read_event == NULL) {
		_tprintf(_T("Error creating read event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = controlDataTaxi.read_event;

	//controlDataTaxi.write_event = CreateEvent(NULL, FALSE, FALSE, EVENT_WRITE_FROM_TAXIS);
	controlDataTaxi.write_event = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_WRITE_FROM_TAXIS);
	if (controlDataTaxi.write_event == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = controlDataTaxi.write_event;

	cdResponse.got_response = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_GOT_RESPONSE);
	if (cdResponse.got_response == NULL) {
		_tprintf(_T("Error creating got_response event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = cdResponse.got_response;

	TCHAR licensePlate[9];/* = (TCHAR*) malloc (sizeof(TCHAR) * 10);*/
	Coords coords;
	
	_tprintf(_T("Insert your license plate: "));
	_tscanf_s(TEXT(" %9s"), licensePlate, 9 * sizeof(TCHAR));
	
	_tprintf(_T("Where are you located?\nx: "));
	_tscanf_s(TEXT(" %d"), &coords.x, sizeof(coords.x));
	_tprintf(_T("y: "));
	_tscanf_s(TEXT(" %d"), &coords.y, sizeof(coords.y));

	CDThread cdThread;
	cdThread.controlDataTaxi = controlDataTaxi;
	cdThread.cdResponse = cdResponse;

	enum response_id res = RegisterInCentral(cdThread, &licensePlate, coords);
	if (res == OK)
		_tprintf(_T("Central added me!\n"));

	WaitAllThreads(threads, threadCounter);
	UnmapAllViews(views, viewCounter);
	CloseMyHandles(handles, handleCounter);
}