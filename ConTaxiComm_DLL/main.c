#include "pch.h"
#include "dll.h"
#include "structs.h"
#include "rules.h"


enum response_id CallCentral(CDThread cdThread, Content content, enum message_id messageId) {
	CC_CDRequest cdata = cdThread.controlDataTaxi;
	CC_CDResponse cdResponse = cdThread.cdResponse;
	WaitForSingleObject(cdata.mutex, INFINITE);

	cdata.shared->action = messageId;
	cdata.shared->messageContent = content;
	//CopyMemory(&cdata.shared->messageContent, &content, sizeof(Content));

	ReleaseMutex(cdata.mutex);
	SetEvent(cdata.write_event);

	//WaitForSingleObject(cdata.read_event, INFINITE);
	return ReadResponse(cdResponse);
}


enum response_id RegisterInCentral(CDThread cdata, TCHAR* licensePlate, Coords location) {
	Content content;

	CopyMemory(&content.taxi.licensePlate, licensePlate, sizeof(TCHAR) * 10);
	content.taxi.location.x = location.x;
	content.taxi.location.y = location.y;

	return CallCentral(cdata, content, RegisterTaxiInCentral);
}

enum responde_id ReadResponse(CC_CDResponse response) {
	enum responde_id res;

	WaitForSingleObject(response.got_response, INFINITE);
	
	WaitForSingleObject(response.mutex, INFINITE);

	res = response.shared->action;

	ReleaseMutex(response.mutex);

	return res;
}