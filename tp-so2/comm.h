#pragma once
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#include "rules.h"
#include "structs.h"
#include "utils.h"

DWORD WINAPI TalkToTaxi(LPVOID ptr);
void RespondToTaxiLogin(CDThread* cdThread, TCHAR* licensePlate, HContainer* container, enum response_id resp);
DWORD WINAPI ListenToLoginRequests(LPVOID ptr);
void SendBroadCastMessage(CC_Broadcast* broadcast, SHM_BROADCAST* message, int nr);
int FindFeatureAndRun(TCHAR* command, CDThread* cdata);
DWORD WINAPI TextInterface(LPVOID ptr);
TCHAR** ParseCommand(TCHAR* cmd);
DWORD WINAPI GetPassengerRegistration(LPVOID ptr);
DWORD WINAPI WaitTaxiConnect(LPVOID ptr);
SHM_CC_RESPONSE ParseAndExecuteOperation(CDThread* cd, enum message_id action, Content content);
DWORD WINAPI RequestWaitTimeFeature(LPVOID ptr);
void BroadcastViaNamedPipeToTaxi(Taxi* taxis, int size, PassMessage message, HANDLE newMessage);
void UpdateView(CDThread* cd);