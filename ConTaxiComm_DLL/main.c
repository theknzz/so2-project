#include "pch.h"
#include "dll.h"
#include "structs.h"
#include "rules.h"


void CallCentral(CDTaxi cdata, Content content, enum message_id messageId) {
	WaitForSingleObject(cdata.mutex, INFINITE);

	cdata.shared->action = messageId;
	cdata.shared->messageContent = content;
	//CopyMemory(&cdata.shared->messageContent, &content, sizeof(Content));

	ReleaseMutex(cdata.mutex);
	SetEvent(cdata.write_event);

	WaitForSingleObject(cdata.read_event, INFINITE);
}


void RegisterInCentral(CDTaxi cdata, TCHAR* licensePlate, Coords location) {
	Content content;

	CopyMemory(content.taxi.licensePlate, licensePlate, sizeof(TCHAR) * 9);
	content.taxi.location.x = location.x;
	content.taxi.location.y = location.y;

	CallCentral(cdata, content, RegisterTaxiInCentral);
}