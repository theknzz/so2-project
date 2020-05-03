#pragma once

#include <Windows.h>
#include "structs.h"

# define DLL_EXPORT __declspec(dllexport)  __cdecl
# define DLL_IMPORT __declspec(dllimport)

enum responde_id DLL_EXPORT CallCentral(CDThread cdata, Content content, enum message_id messageId);

enum responde_id DLL_EXPORT RegisterInCentral(CDThread cdata, TCHAR* licensePlate, Coords location);

enum responde_id DLL_EXPORT ReadResponse(CC_CDResponse response);