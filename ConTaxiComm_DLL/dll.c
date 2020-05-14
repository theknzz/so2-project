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

//enum response_id CallCentral(CDThread cdThread, Content content, enum message_id messageId) {
//	CC_CDRequest cdata = cdThread.controlDataTaxi;
//	CC_CDResponse cdResponse = cdThread.cdResponse;
//
//	WaitForSingleObject(cdata.mutex, INFINITE);
//
//	cdata.shared->action = messageId;
//	//cdata.shared->messageContent = content;
//	CopyMemory(&cdata.shared->messageContent, &content, sizeof(Content));
//
//	ReleaseMutex(cdata.mutex);
//	SetEvent(cdata.write_event);
//
//	//WaitForSingleObject(cdata.read_event, INFINITE);
//	return ReadResponse(cdResponse);
//}

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
	_tprintf(_T("[LOG] Sent my information to the central.\n"));

	// wait read
	enum response_id r = ReadLoginResponse(res, cdResponse, cdRequest->new_response);

	ReleaseMutex(cdRequest->login_write_m);

	if (r == OK)
		_tprintf(_T("[LOG] Central read my information.\nr_event: '%s',\nr_mutex: '%s',\nr_shm: '%s'\nevent: '%s',\nmutex: '%s',\nshm: '%s'\n"),
			res->request_event_name, res->request_mutex_name, res->request_shm_name, res->response_event_name, res->response_mutex_name, res->response_shm_name);
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
	_tprintf(_T("[LOG] Request done.\n"));
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

	_tprintf(_T("[LOG] Got map.\n"));
	return map;
}

enum response_id UpdateMyLocation(CC_CDRequest* request, CC_CDResponse* response, TCHAR* licensePlate, Coords location) {
	SHM_CC_REQUEST r;
	Content content;

	CopyMemory(&content.taxi.licensePlate, licensePlate, sizeof(TCHAR) * 9);
	content.taxi.location.x = location.x;
	content.taxi.location.y = location.y;
	
	CopyMemory(&r.messageContent, &content, sizeof(Content));
	r.action = UpdateTaxiLocation;

	RequestAction(request, response, r);

	return GetCentralResponse(response, request);
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

	_tprintf(_T("[LOG] Got passenger request.\n"));
	return response->shared->action;
}

enum response_id RequestPassengerTransport(CC_CDRequest* request, CC_CDResponse* response, Passenger* passenger) {
	SHM_CC_REQUEST r;
	r.action = RequestPassenger;

	RequestAction(request, response, r);

	return GetPassengerFromCentral(response, request, passenger);
}