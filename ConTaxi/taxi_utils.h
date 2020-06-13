#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rules.h"
#include "structs.h"

void PrintError(enum response_id resp, CD_TAXI_Thread* cd);
TCHAR* DirectionToString(enum taxi_direction direction);
void PrintPersonalInformation(CD_TAXI_Thread* cd);
int CalculateDistanceTo(Coords org, Coords dest);
enum response_id MoveMeToOptimalPosition(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, Coords dest, char map[MIN_LIN][MIN_COL]);
BOOL CanMoveTo(char map[MIN_LIN][MIN_COL], Coords destination);
void WaitAllThreads(HANDLE* threads, int nr);
void CloseMyHandles(HANDLE* handles, int nr);
void UnmapAllViews(HANDLE* views, int nr);
TCHAR** ParseCommand(TCHAR* cmd);
enum taxi_direction getTaxiDirection(Coords org, Coords target);
enum taxi_direction getOppositeDirection(enum taxi_direction dir);
enum response_id MoveAleatorio(CC_CDRequest* request, CC_CDResponse* response, Taxi* taxi, char map[MIN_LIN][MIN_COL]);
BOOL hasPassenger(Taxi* taxi);
BOOL isPassengerLocation(Taxi* taxi);
BOOL isPassengerDestination(Taxi* taxi);
void moveTaxi(CD_TAXI_Thread* cdata);
DWORD WINAPI TaxiAutopilot(LPVOID ptr);
int FindFeatureAndRun(TCHAR command[100], CD_TAXI_Thread* cdata);
DWORD WINAPI ReceiveBroadcastMessage(LPVOID ptr);
DWORD WINAPI TextInterface(LPVOID ptr);
DWORD WINAPI ListenToCentral(LPVOID* ptr);
