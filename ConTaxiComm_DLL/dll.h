#pragma once

#include <Windows.h>
#include "structs.h"

# define DLL_EXPORT __declspec(dllexport)  __cdecl
# define DLL_IMPORT __declspec(dllimport)

//enum responde_id DLL_EXPORT CallCentral(CDThread cdata, Content content, enum message_id messageId);

enum response_id DLL_EXPORT RegisterInCentral(LR_Container* res, CDThread cdata, TCHAR* licensePlate, Coords location);

enum response_id DLL_EXPORT ReadLoginResponse(LR_Container* res, CDLogin_Response* response, HANDLE new_response);

enum response_id DLL_EXPORT GetCentralResponse(CC_CDResponse* response, CC_CDRequest* request);

void DLL_EXPORT RequestAction(CC_CDRequest* request, CC_CDResponse* response, SHM_CC_REQUEST message);

enum response_id DLL_EXPORT UpdateMyLocation(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, Coords location);

enum response_id DLL_EXPORT GetMap(char* * map, CC_CDRequest* request, CC_CDResponse* response);

char** GetMapFromCentral(CC_CDResponse* response, CC_CDRequest* request);

enum response_id DLL_EXPORT RequestPassengerTransport(CC_CDRequest* request, CC_CDResponse* response, Passenger* passenger, TCHAR passengerName[25]);

enum response_id DLL_EXPORT NotifyVelocityChange(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi);

enum response_id DLL_EXPORT NotifyCentralNQChange(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi);

enum response_id DLL_EXPORT NotifyCentralTaxiLeaving(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi);

enum response_id DLL_EXPORT NotifyPassengerCatch(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi);

enum response_id DLL_EXPORT NotifyPassengerDeliever(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi);