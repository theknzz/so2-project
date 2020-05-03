#pragma once

#include <Windows.h>
#include "structs.h"

# define DLL_EXPORT __declspec(dllexport)  __cdecl
# define DLL_IMPORT __declspec(dllimport)

void DLL_EXPORT CallCentral(CDTaxi cdata, Content content, enum message_id messageId);

void DLL_EXPORT RegisterInCentral(CDTaxi cdata, TCHAR* licensePlate, Coords location);