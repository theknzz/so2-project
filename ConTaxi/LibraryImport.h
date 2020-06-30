#pragma once
#include <windows.h>
#include "structs.h"

# define DLL_IMPORT __declspec(dllimport)  __cdecl

//enum responde_id DLL_EXPORT CallCentral(CDThread cdata, Content content, enum message_id messageId);
enum response_id DLL_IMPORT RegisterInCentral(LR_Container* res, CDThread cdata, TCHAR* licensePlate, Coords location);
enum response_id DLL_IMPORT ReadLoginResponse(LR_Container* res, CDLogin_Response* response, HANDLE new_response);
enum response_id DLL_IMPORT GetCentralResponse(CC_CDResponse* response, CC_CDRequest* request);
void DLL_IMPORT RequestAction(CC_CDRequest* request, CC_CDResponse* response, SHM_CC_REQUEST message);
enum response_id DLL_IMPORT UpdateMyLocation(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, Coords location);
enum response_id DLL_IMPORT GetMap(char** map, CC_CDRequest* request, CC_CDResponse* response);
char** GetMapFromCentral(CC_CDResponse* response, CC_CDRequest* request);
enum response_id DLL_IMPORT RequestPassengerTransport(CC_CDRequest* request, CC_CDResponse* response, Passenger* passenger, TCHAR passengerName[25], TCHAR* license);
enum response_id DLL_IMPORT NotifyVelocityChange(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi);
enum response_id DLL_IMPORT NotifyCentralNQChange(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi);
enum response_id DLL_IMPORT NotifyCentralTaxiLeaving(CC_CDRequest* request, CC_CDResponse* response, Taxi taxi);
enum response_id DLL_IMPORT NotifyPassengerCatch(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi);
enum response_id DLL_IMPORT NotifyPassengerDeliever(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi);
enum response_id DLL_IMPORT EstablishNamedPipeComunication(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, CD_TAXI_Thread* cd);