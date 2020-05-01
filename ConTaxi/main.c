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


void CallCentral(CDTaxi cdata, Content content, enum message_id messageId) {
	WaitForSingleObject(cdata.mutex, INFINITE);

	cdata.shared->action = messageId;
	cdata.shared->messageContent = content;
	//CopyMemory(&cdata.shared->messageContent, &content, sizeof(Content));

	ReleaseMutex(cdata.mutex);
	SetEvent(cdata.write_event);

	WaitForSingleObject(cdata.read_event, INFINITE);
}


void RegisterInCentral(CDTaxi cdata, TCHAR *licensePlate, Coords location) {
	Content content;
	
	CopyMemory(content.taxi.licensePlate, licensePlate, sizeof(TCHAR) * 9);
	content.taxi.location.x = location.x;
	content.taxi.location.y = location.y;

	CallCentral(cdata, content, RegisterTaxiInCentral);
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
		sizeof(SHM_CEN_CON),
		SHM_CENTAXI_CONTAXI);
	if (fmCenTaxiToConTaxi == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		exit(-1);
	}

	handles[handleCounter++] = fmCenTaxiToConTaxi;

	CDTaxi controlDataTaxi;
	controlDataTaxi.shared = (SHM_CEN_CON*)MapViewOfFile(fmCenTaxiToConTaxi,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_CEN_CON));
	if (controlDataTaxi.shared == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	else _tprintf(TEXT("Vista da Memória partilhada criada.\n"));

	views[viewCounter++] = controlDataTaxi.shared;

	controlDataTaxi.mutex = CreateMutex(NULL, FALSE, CENTRAL_MUTEX);
	if (controlDataTaxi.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	handles[handleCounter++] = controlDataTaxi.mutex;

	controlDataTaxi.read_event = CreateEvent(NULL, FALSE, FALSE, EVENT_READ_FROM_TAXIS);
	controlDataTaxi.read_event = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_READ_FROM_TAXIS);
	if (controlDataTaxi.read_event == NULL) {
		_tprintf(_T("Error creating read event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = controlDataTaxi.read_event;

	controlDataTaxi.write_event = CreateEvent(NULL, FALSE, FALSE, EVENT_WRITE_FROM_TAXIS);
	controlDataTaxi.write_event = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, EVENT_WRITE_FROM_TAXIS);
	if (controlDataTaxi.write_event == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		//WaitAllThreads(threads, threadCounter);
		//UnmapAllViews(views, viewCounter);
		//CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = controlDataTaxi.write_event;

	TCHAR* licensePlate = (TCHAR*) malloc (sizeof(TCHAR) * 9);
	Coords coords;
	
	_tprintf(_T("Insert your license plate: "));
	_tscanf_s(TEXT(" %9s"), licensePlate, 9 * sizeof(TCHAR));
	
	_tprintf(_T("Where are you located?\nx: "));
	_tscanf_s(TEXT(" %d"), &coords.x, sizeof(coords.x));
	_tprintf(_T("y: "));
	_tscanf_s(TEXT(" %d"), &coords.y, sizeof(coords.y));

	RegisterInCentral(controlDataTaxi, licensePlate, coords);

	_gettchar();
	free(licensePlate);
}