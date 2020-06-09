#include "pch.h"
#include "dll.h"
#include "structs.h"
#include "rules.h"

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

enum response_id RegisterInCentral(LR_Container* res, CDThread cdata, TCHAR* licensePlate, Coords location) {
	CDLogin_Request* cdRequest = cdata.cdLogin_Request;
	CDLogin_Response* cdResponse = cdata.cdLogin_Response;

	// Preencer o conteudo da mensagem
	Taxi taxi;
	CopyMemory(&taxi.licensePlate, licensePlate, sizeof(TCHAR) * 10);
	taxi.location.x = location.x;
	taxi.location.y = location.y;

	// Preciso de um mutex para bloquear o login multiplo ??
	WaitForSingleObject(cdRequest->login_write_m, INFINITE);
	WaitForSingleObject(cdRequest->login_m, INFINITE);

	cdRequest->request->action = RegisterTaxiInCentral;
	CopyMemory(&cdRequest->request->taxi, &taxi, sizeof(Taxi));

	ReleaseMutex(cdRequest->login_m);

	SetEvent(cdResponse->new_request);
	//_tprintf(_T("[LOG] Sent my information to the central.\n"));

	// wait read
	enum response_id r = ReadLoginResponse(res, cdResponse, cdRequest->new_response);

	ReleaseMutex(cdRequest->login_write_m);

	/*if (r == OK)
		_tprintf(_T("[LOG] Central read my information.\nr_event: '%s',\nr_mutex: '%s',\nr_shm: '%s'\nevent: '%s',\nmutex: '%s',\nshm: '%s'\n"),
			res->request_event_name, res->request_mutex_name, res->request_shm_name, res->response_event_name, res->response_mutex_name, res->response_shm_name);*/
	return r;
}

enum response_id ReadLoginResponse(LR_Container* res, CDLogin_Response* response, HANDLE new_response) {
	enum responde_id r;

	WaitForSingleObject(new_response, INFINITE);

	WaitForSingleObject(response->login_m, INFINITE);

	CopyMemory(res, &response->response->container, sizeof(LR_Container));
	r = response->response->action;

	ReleaseMutex(response->login_m);

	return r;
}

void RequestAction(CC_CDRequest* request, CC_CDResponse* response, SHM_CC_REQUEST message) {

	WaitForSingleObject(request->mutex, INFINITE);

	CopyMemory(request->shared, &message, sizeof(SHM_CC_REQUEST));

	ReleaseMutex(request->mutex);

	SetEvent(response->new_request);
}

enum response_id GetCentralResponse(CC_CDResponse* response, CC_CDRequest* request) {
	enum response_id res;

	WaitForSingleObject(request->new_response, INFINITE);

	WaitForSingleObject(response->mutex, INFINITE);
	
	res = response->shared->action;

	ReleaseMutex(response->mutex);

	return res;
}

char** GetMapFromCentral(CC_CDResponse* response, CC_CDRequest* request) {
	char map[MIN_LIN][MIN_COL];

	WaitForSingleObject(request->new_response, INFINITE);

	WaitForSingleObject(response->mutex, INFINITE);

	CopyMemory(map, response->shared->map, sizeof(response->shared->map));

	ReleaseMutex(response->mutex);
	return map;
}

enum response_id UpdateMyLocation(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, Coords location) {
	SHM_CC_REQUEST r;
	enum response_id res;

	CopyMemory(&r.messageContent.taxi, taxi, sizeof(Taxi));
	// change the location of the taxi to the destination
	// this happens here so the real position is just changed when
	// the central gives a response to the taxi's request
	r.messageContent.taxi.location.x = location.x;
	r.messageContent.taxi.location.y = location.y;
	r.action = UpdateTaxiLocation;

	RequestAction(request, response, r);

	if ((res = GetCentralResponse(response, request)) == OK) {
		taxi->location.x = location.x;
		taxi->location.y = location.y;
	}
	return res;
}

enum response_id GetMap(char** map, CC_CDRequest* request, CC_CDResponse* response) {
	SHM_CC_REQUEST r;
	r.action = GetCityMap;

	RequestAction(request, response, r);
	
	CopyMemory(map, GetMapFromCentral(response, request), sizeof(char)*MIN_LIN*MIN_COL);

	if (map == NULL) return ERRO;
	return OK;
}

enum response_id GetPassengerFromCentral(CC_CDResponse* response, CC_CDRequest* request, Passenger* passenger) {
	WaitForSingleObject(request->new_response, INFINITE);

	WaitForSingleObject(response->mutex, INFINITE);

	CopyMemory(passenger, &response->shared->passenger, sizeof(response->shared->passenger));

	ReleaseMutex(response->mutex);
	return response->shared->action;
}

enum response_id RequestPassengerTransport(CC_CDRequest* request, CC_CDResponse* response, Passenger* passenger, TCHAR passengerName[25], TCHAR* license) {
	SHM_CC_REQUEST r;
	r.action = RequestPassenger;

	CopyMemory(r.messageContent.taxi.licensePlate, license, sizeof(Taxi));
	CopyMemory(r.messageContent.passenger.nome, passengerName, sizeof(TCHAR)*25);

	RequestAction(request, response, r);

	return GetPassengerFromCentral(response, request, passenger);
}

enum response_id NotifyVelocityChange(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi) {
	SHM_CC_REQUEST r;
	r.action = NotifySpeedChange; 
	CopyMemory(&r.messageContent.taxi, &taxi, sizeof(Taxi));

	RequestAction(request, response, r);

	return GetCentralResponse(response, request);
}

enum response_id NotifyCentralNQChange(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi) {
	SHM_CC_REQUEST r;
	r.action = NotifyNQChange;
	CopyMemory(&r.messageContent.taxi, &taxi, sizeof(Taxi));

	RequestAction(request, response, r);

	return GetCentralResponse(response, request);
}

enum response_id NotifyCentralTaxiLeaving(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi) {
	SHM_CC_REQUEST r;
	r.action = NotifyTaxiLeaving;
	CopyMemory(&r.messageContent.taxi, &taxi, sizeof(Taxi));

	RequestAction(request, response, r);

	return GetCentralResponse(response, request);
}

enum response_id NotifyPassengerCatch(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi) {
	SHM_CC_REQUEST r;
	enum response_id res;
	r.action = WarnPassengerCatch;
	CopyMemory(&r.messageContent.taxi, taxi, sizeof(Taxi));
	RequestAction(request, response, r);
	res = GetCentralResponse(response, request);
	if (res==OK)
		taxi->client.state = OnDrive;
	return res;
}

enum response_id NotifyPassengerDeliever(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi) {
	SHM_CC_REQUEST r;
	enum response_id res;

	r.action = WarnPassengerDeliever;
	CopyMemory(&r.messageContent.taxi, taxi, sizeof(Taxi));
	RequestAction(request, response, r);

	res = GetCentralResponse(response, request);

	if (res == OK) {
		ZeroMemory(&taxi->client, sizeof(Passenger));
		taxi->client.location.x = -1;
		taxi->client.location.y = -1;
	}

	return res;
}

enum response_id DLL_EXPORT EstablishNamedPipeComunication(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, CD_TAXI_Thread* cd)
{
	SHM_CC_REQUEST r;
	enum response_id res;

	r.action = EstablishNamedPipeComm;
	CopyMemory(&r.messageContent.taxi, taxi, sizeof(Taxi));
	RequestAction(request, response, r);

	res = GetCentralResponse(response, request);

	if (res == OK) {
		// Estabelcer comunicação
		if (!WaitNamedPipe(NP_TAXI_NAME, NMPWAIT_WAIT_FOREVER)) {
			_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe) %d\n"), NP_TAXI_NAME, GetLastError());
			res = ERRO;
			return res;
		}
		Sleep(2000);
		//DWORD dwMode = PIPE_READMODE_MESSAGE;
		if ((cd->hNamedPipeComm = CreateFile(NP_TAXI_NAME, /*GENERIC_READ*/ PIPE_ACCESS_DUPLEX, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))==NULL) {
			_tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), NP_PASS_REGISTER);
			res = ERRO;
			return res;
		}
		if (CreateThread(NULL, 0, ListenToCentral, cd, 0, NULL) == NULL)
			_tprintf(_T("well rip\n"));
		//else if (!SetNamedPipeHandleState(cd->hNamedPipeComm, &dwMode, NULL, NULL)) {
		//	_tprintf(_T("SetNamedPipeHandleState failed! (%d)\n"), GetLastError());
		//	res = ERRO;
		//}
	}
	return res;
}

DWORD WINAPI ListenToCentral(LPVOID* ptr) {
	CD_TAXI_Thread* cd = (CD_TAXI_Thread*)ptr;
	PassMessage message;
	DWORD nr;
	BOOL ret;
	while (!cd->isTaxiKicked) {
		ret = ReadFile(cd->hNamedPipeComm, &message, sizeof(PassMessage), &nr, NULL);
		
		if (message.resp == OK)
			_tprintf(_T("U are working with %s\n"), message.content.passenger.nome);
		else
			_tprintf(_T("better luck next time\n"));
	}
	return 0;
}
