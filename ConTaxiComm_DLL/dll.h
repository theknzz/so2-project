#pragma once

#include <Windows.h>
#include "structs.h"

# define DLL_EXPORT __declspec(dllexport)  __cdecl
# define DLL_IMPORT __declspec(dllimport)

enum responde_id DLL_EXPORT CallCentral(CDThread cdata, Content content, enum message_id messageId);

LR_Container DLL_EXPORT RegisterInCentral(CDThread cdata, TCHAR* licensePlate, Coords location);

LR_Container DLL_EXPORT ReadLoginResponse(CDLogin_Response response);

enum response_id DLL_EXPORT GetCentralResponse(CC_CDResponse* response, CC_CDRequest* request);

void DLL_EXPORT RequestAction(CC_CDRequest* request, CC_CDResponse* response, enum Content content);

//enum responde_id DLL_EXPORT ReadResponse(CC_CDResponse response);

enum response_id DLL_EXPORT UpdateMyLocation(CC_CDRequest* request, CC_CDResponse* response, TCHAR* licensePlate, Coords location);