#pragma once
#include "rules.h"

typedef struct _COORDS {
	int x, y;
} Coords;

typedef struct _Passenger {
	enum passanger_state state;
	TCHAR nome[25];
	Coords location;
	Coords destination;
} Passenger;

typedef struct _TAXI {
	TCHAR licensePlate[9];
	Coords location;
	int autopilot; // 0 - false, 1- true
	float velocity;
	Passenger client;
	int nq;
	enum taxi_direction direction;
} Taxi;

typedef struct MapCell {
	int x, y;
	enum type cellType;
	char display;
	Taxi* taxis;
	Passenger* passengers;
} Cell;

typedef struct _MSG_CONTENT {
	Taxi taxi;
	Passenger passenger;
} Content;

typedef struct _LOGIN_RESPONSE_CONTAINER {
	TCHAR request_shm_name[50];
	TCHAR request_mutex_name[50];
	TCHAR request_event_name[50];

	TCHAR response_shm_name[50];
	TCHAR response_mutex_name[50];
	TCHAR response_event_name[50];
} LR_Container;

typedef struct _LOGIN_MSG {
	enum message_id action;
	Taxi taxi;
} SHM_LOGIN_REQUEST;

typedef struct _LOGIN_MSG_R {
	enum message_id action;
	LR_Container container;
} SHM_LOGIN_RESPONSE;

typedef struct _MSG_TO_CENTRAL {
	enum message_id action;
	Content messageContent;
} SHM_CC_REQUEST;

typedef struct _BROADCAST_MSG {
	Passenger passenger;
	BOOL isSystemClosing;
} SHM_BROADCAST;

typedef struct _MSG_TO_TAXI {
	enum response_id action;
	char map[MIN_COL][MIN_LIN];
	Passenger passenger;
	//Content responseContent;
} SHM_CC_RESPONSE;

typedef struct Control_Data_Broadcast {
	HANDLE mutex;
	HANDLE new_passenger;
	SHM_BROADCAST* shared;
} CC_Broadcast;

typedef struct Control_Data_For_Request {
	HANDLE mutex;
	HANDLE new_response;
	SHM_CC_REQUEST* shared;
} CC_CDRequest;

typedef struct Control_Data_For_Response {
	HANDLE mutex;
	HANDLE new_request;
	SHM_CC_RESPONSE* shared;
} CC_CDResponse;

typedef struct Control_Data_Login_Request {
	HANDLE login_m; // mutex_x
	HANDLE login_write_m; // mutex_y
	HANDLE new_response; // event
	SHM_LOGIN_REQUEST* request;
} CDLogin_Request;

typedef struct Control_Data_Login_Response {
	HANDLE login_m; // mutex_x
	HANDLE new_request; // event
	SHM_LOGIN_RESPONSE* response;
} CDLogin_Response;


typedef struct HANDLE_CONTAINER {
	HANDLE* threads;
	HANDLE* handles;
	HANDLE* views;
	int* handleCounter;
	int* viewCounter;
	int* threadCounter;
} HContainer;

typedef struct _CC_COMMUNICATION_CONTAINER {
	CC_CDRequest request;
	CC_CDResponse response;
	LR_Container container;
} CC_Comm;

typedef struct _CONTROL_DATA_TAXI_THREAD{
	CC_Comm comm;
	Taxi* taxi;
	HANDLE taxiGate;
	char charMap[MIN_COL][MIN_LIN];
	CC_Broadcast* broadcast;
	BOOL isTaxiKicked;
} CD_TAXI_Thread;

typedef struct ThreadControlData {
	BOOL* areTaxisRequestsPause;
	int nrMaxTaxis;
	int nrMaxPassengers;
	Cell* map;
	Taxi* taxis;
	BOOL isSystemClosing;
	CC_Broadcast* broadcast;
	int* WaitTimeOnTaxiRequest;
	Passenger* passengers;
	HContainer* hContainer;
	//CC_Comm* comm;
	CDLogin_Request* cdLogin_Request;
	CDLogin_Response* cdLogin_Response;
	char charMap[MIN_COL][MIN_LIN];
	SHM_CC_REQUEST* requests;
} CDThread;

typedef struct IndividualThreadControl {
	CDThread* cd;
	CC_Comm comm;
} IndividualCD;

typedef struct _CMD_STRUCT {
	TCHAR* command;
	TCHAR* license;
	int x;
	int y;
} CMD;
