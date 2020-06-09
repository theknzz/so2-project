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

void WaitAllThreads(CDThread* cd, HANDLE* threads, int nr);
void CloseMyHandles(HANDLE* handles, int nr);
void UnmapAllViews(HANDLE* views, int nr);
void ClearScreen();
void PrintMap(Cell* map);
enum respond_id InsertTaxiIntoMapCell(Cell* cell, Taxi taxi, int size);
void RemoveTaxiFromMapCell(Cell* cell, TCHAR* lcPlate, int size);
int GetLastPassengerIndex(Passenger* passengers, int size);
int GetIndexFromPassengerWithoutTransport(Passenger* passengers, int size);
enum passanger_state GetPassengerStatus(Passenger* passengers, int size, TCHAR* name);
int GetPassengerIndex(Passenger* passengers, int size, TCHAR* name);
TCHAR* hasPassenger(Taxi taxi);
int FindTaxiIndex(Taxi* taxis, int size, Taxi target);
int FindTaxiWithLicense(Taxi* taxis, int size, TCHAR* lc);
int FindFirstFreeTaxiIndex(Taxi* taxis, int size);
TCHAR* ParseStateToString(enum passanger_state state);
int NumberOfActiveTaxis(Taxi* taxis, int size);
void LoadMapa(Cell* map, char* buffer, int nrTaxis, int nrPassangers);
char* ReadFileToCharArray(TCHAR* mapName);
char** ConvertMapIntoCharMap(Cell* map);
void InsertPassengerIntoBuffer(ProdCons* box, Passenger passenger);
void GetPassengerFromBuffer(ProdCons* box, Passenger* passenger);
int PickRandomTransportIndex(int size);
BOOL SendMessageToTaxiViaNamePipe(PassMessage message, Taxi* taxi);
int timer(int waitTime);
int WaitAndPickWinner(CDThread* cd, Passenger passenger);
BOOL SendTransportRequestResponse(HANDLE* requests, Passenger client, int size, int winner);
void AddPassengerToCentral(CDThread* cdata, TCHAR* nome, int locX, int locY, int desX, int desY);
enum response_action UpdateTaxiPosition(CDThread* cd, Content content);
enum response_action CatchPassenger(CDThread* cd, Content content);
enum response_id DeliverPassenger(CDThread* cd, Content content);
enum response_id AssignPassengerToTaxi(CDThread* cd, Content content);
BOOL isInRequestBuffer(Taxi* requests, int size, Taxi taxi);
void RemovePassengerFromCentral(TCHAR* nome, Passenger* passengers, int size);
BOOL isValidCoords(CDThread* cd, Coords c);
int FindTaxiWithNamedPipeHandle(Taxi* taxis, int size, HANDLE handle);
void SendMessageToPassenger(enum response_id resp, Passenger* passenger, Taxi* taxi, CDThread* cd);