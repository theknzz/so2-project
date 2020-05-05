#pragma once

typedef struct _COORDS {
	int x, y;
} Coords;

typedef struct _TAXI {
	TCHAR licensePlate[9];
	Coords location;
} Taxi;

typedef struct _Passenger {
	TCHAR nome[25];
	Coords location;
} Passenger;

typedef struct MapCell {
	int x, y;
	enum type cellType;
	char display;
	Taxi taxi;
	Passenger passenger;
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

typedef struct _MSG_TO_TAXI {
	enum response_id action;
	Content responseContent;
} SHM_CC_RESPONSE;

typedef struct txtInterfaceControlData {
	BOOL gate;
	Cell* map;
	Taxi* taxis;
	int *taxiCount;
	int* WaitTimeOnTaxiRequest;
} TI_Controldata;

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
	HANDLE login_m;
	HANDLE login_write_m;
	HANDLE new_response;
	SHM_LOGIN_REQUEST* request;
} CDLogin_Request;

typedef struct Control_Data_Login_Response {
	HANDLE login_m;
	HANDLE new_request;
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

typedef struct ThreadControlData {
	int *taxiFreePosition;
	Cell* map;
	Taxi* taxis;
	HContainer* hContainer;
	//CC_CDRequest controlDataTaxi;
	//CC_CDResponse cdResponse;
	CDLogin_Request cdLogin_Request;
	CDLogin_Response cdLogin_Response;
} CDThread;

