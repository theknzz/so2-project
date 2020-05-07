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

LR_Container RegisterInCentral(CDThread cdata, TCHAR* licensePlate, Coords location) {
	CDLogin_Request cdRequest = cdata.cdLogin_Request;
	CDLogin_Response cdResponse = cdata.cdLogin_Response;

	// Preencer o conteudo da mensagem
	Taxi taxi;
	CopyMemory(&taxi.licensePlate, licensePlate, sizeof(TCHAR) * 10);
	taxi.location.x = location.x;
	taxi.location.y = location.y;

	// Preciso de um mutex para bloquear o login multiplo ??
	WaitForSingleObject(cdRequest.login_write_m, INFINITE);
	WaitForSingleObject(cdRequest.login_m, INFINITE);

	cdRequest.request->action = RegisterTaxiInCentral;
	CopyMemory(&cdRequest.request->taxi, &taxi, sizeof(Taxi));

	ReleaseMutex(cdRequest.login_m);

	SetEvent(cdResponse.new_request);
	_tprintf(_T("[LOG] Sent my information to the central.\n"));

	// wait read
	LR_Container res;
	res = ReadLoginResponse(cdResponse, cdRequest.new_response);

	ReleaseMutex(cdRequest.login_write_m);
	_tprintf(_T("[LOG] Central read my information.\nr_event: '%s',\nr_mutex: '%s',\nr_shm: '%s'\nevent: '%s',\nmutex: '%s',\nshm: '%s'\n"),
		res.request_event_name, res.request_mutex_name, res.request_shm_name, res.response_event_name, res.response_mutex_name, res.response_shm_name);
	return res;
}

LR_Container ReadLoginResponse(CDLogin_Response response, HANDLE new_response) {
	LR_Container res;

	WaitForSingleObject(new_response, INFINITE);

	WaitForSingleObject(response.login_m, INFINITE);

	CopyMemory(&res, &response.response->container, sizeof(LR_Container));

	ReleaseMutex(response.login_m);
	return res;
}

void RequestAction(CC_CDRequest* request, CC_CDResponse* response, enum Content content) {

	WaitForSingleObject(request->mutex, INFINITE);

	CopyMemory(request->shared, &content, sizeof(Content));

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

	_tprintf(_T("[LOG] Got response.\n"));
	return res;
}

//enum response_id ReadResponse(CC_CDResponse response) {
//	enum response_id res;
//
//	WaitForSingleObject(response.got_response, INFINITE);
//	
//	WaitForSingleObject(response.mutex, INFINITE);
//
//	res = response.shared->action;
//
//	ReleaseMutex(response.mutex);
//
//	return res;
//}

enum response_id UpdateMyLocation(CC_CDRequest* request, CC_CDResponse* response, TCHAR* licensePlate, Coords location) {
	Content content;

	CopyMemory(&content.taxi.licensePlate, licensePlate, sizeof(TCHAR) * 9);
	content.taxi.location.x = location.x;
	content.taxi.location.y = location.y;

	RequestAction(request, response, UpdateTaxiLocation);

	return GetCentralResponse(response, request);
}