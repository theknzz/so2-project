#include "dll.h"
#include "taxi_utils.h"

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

	broadcast.new_passenger = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, EVENT_NEW_PASSENGER);
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
		_tscanf(TEXT(" %9s"), licensePlate);
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
	CD_TAXI_Thread cd;
	cd.isTaxiKicked = FALSE;

	LR_Container res;
	enum responde_id resp = RegisterInCentral(&res, cdThread, licensePlate, coords);
	if (resp != OK)
	{
		CloseMyHandles(handles, handleCounter);
		UnmapAllViews(views, viewCounter);
		PrintError(resp, &cd);
		Sleep(5000);
		exit(-1);
	}
	else {
		_tprintf(_T("Login done.\n"));
	}

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
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = request.shared;

	request.mutex = OpenMutex(SYNCHRONIZE, FALSE, res.request_mutex_name);
	if (request.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = request.mutex;

	request.new_response = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, res.response_event_name);
	if (request.new_response == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
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
		WaitAllThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	views[viewCounter++] = response.shared;

	response.mutex = OpenMutex(SYNCHRONIZE, FALSE, res.response_mutex_name);
	if (response.mutex == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = response.mutex;

	response.new_request = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, res.request_event_name);
	if (response.new_request == NULL) {
		_tprintf(_T("Error creating write event (%d)\n"), GetLastError());
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	handles[handleCounter++] = response.new_request;

	CC_Comm cc_comm;
	cc_comm.request = request;
	cc_comm.response = response;
	cc_comm.container = res;

	Taxi me;
	CopyMemory(&cd.comm, &cc_comm, sizeof(CC_Comm));
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
		PrintError(ret, &cd);
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
	}

	if (EstablishNamedPipeComunication(&request, &response, &me, &cd) != OK) {
		_tprintf(_T("Couldn't establish connection to central by named pipe!\n"));
		WaitAllThreads(threads, threadCounter);
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	else {
		_tprintf(_T("Established connection to the central via named pipe!\n"));
	}

	if ((threads[threadCounter++] = CreateThread(NULL, 0, ListenToCentral, &cd, 0, NULL)) == NULL) {
		_tprintf(_T("Error launching comm thread (%d)\n"), GetLastError());
		UnmapAllViews(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
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

