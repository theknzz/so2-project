#pragma once
#include "rules.h"

typedef struct _COORDS {
	int x, y; // map positions
} Coords;

typedef struct _Passenger {
	enum passanger_state state; // passenger state (Waiting, ...)
	TCHAR nome[25]; // passenger name
	Coords location; // passenger location
	Coords destination; // passenger destination
} Passenger;

typedef struct _TAXI {
	TCHAR licensePlate[9]; // taxi's license plate
	Coords location; // taxi actual position
	int autopilot; // 0 - false, 1- true
	float velocity; // taxi's velocity
	Passenger client; // taxis' passenger
	int nq; // taxis's fov
	enum taxi_direction direction; // taxis's direction
} Taxi;

typedef struct MapCell {
	int x, y; // cell position 
	enum type cellType; // cell type
	char display; // cell display on the screen
	Taxi* taxis; // taxis inside the cell
	Passenger* passengers; // passengers inside the cell
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
	enum message_id action;  // request action
	Taxi taxi; // content for the request
} SHM_LOGIN_REQUEST;

typedef struct _LOGIN_MSG_R {
	enum message_id action; // response action
	LR_Container container; // content for the response
} SHM_LOGIN_RESPONSE;

typedef struct _MSG_TO_CENTRAL {
	enum message_id action; // request action
	Content messageContent; // content for the request
} SHM_CC_REQUEST;

typedef struct _BROADCAST_MSG {
	Passenger passenger;	// content for the broadcast
	BOOL isSystemClosing;	// flag saying if central is shutting down or not
} SHM_BROADCAST;

typedef struct _MSG_TO_TAXI {
	enum response_id action;  // response action
	char map[MIN_COL][MIN_LIN]; // content for the response
	Passenger passenger;		// content for the response
} SHM_CC_RESPONSE;

typedef struct Control_Data_Broadcast {
	HANDLE mutex;			// mutex for access control
	HANDLE new_passenger;	// event to notify a new broadcast message
	SHM_BROADCAST* shared;	// broadcast content
} CC_Broadcast;

typedef struct Control_Data_For_Request {
	HANDLE mutex;			// mutex for access control
	HANDLE new_response;	// event to notify a new response
	SHM_CC_REQUEST* shared;	// request content
} CC_CDRequest;

typedef struct Control_Data_For_Response {
	HANDLE mutex;				// mutex for access control
	HANDLE new_request;			// event to notify a new request
	SHM_CC_RESPONSE* shared;	// response content
} CC_CDResponse;

typedef struct Control_Data_Login_Request {
	HANDLE login_m;				// mutex to control all taxis access 
	HANDLE login_write_m;		// mutex to control access to the shm
	HANDLE new_response;		// event to notify a new response
	SHM_LOGIN_REQUEST* request;	// login request content
} CDLogin_Request;

typedef struct Control_Data_Login_Response {
	HANDLE login_m;					// mutex to control access to the shm
	HANDLE new_request;				// event to notify a new request
	SHM_LOGIN_RESPONSE* response;	// login response content
} CDLogin_Response;

typedef struct HANDLE_CONTAINER {
	HANDLE* threads;			// array of thread handles
	HANDLE* handles;			// array of handles
	HANDLE* views;				// array of views
	int* handleCounter;			// handle counter
	int* viewCounter;			// view counter
	int* threadCounter;			// thread counter
} HContainer;

typedef struct _CC_COMMUNICATION_CONTAINER {
	CC_CDRequest request;		// Communication request container
	CC_CDResponse response;		// Communication response container
	LR_Container container;		// Container for mechanisms names
} CC_Comm;

typedef struct _CONTROL_DATA_TAXI_THREAD{
	CC_Comm comm;						// Communication container
	Taxi* taxi;							// Information about the taxi himself
	HANDLE taxiGate;					// Flag that controls the access to the system
	char charMap[MIN_COL][MIN_LIN];		// char map to do validations on moves, ...
	CC_Broadcast* broadcast;			// Communication broadcast container
	BOOL isTaxiKicked;					// Flag to exit cycles
} CD_TAXI_Thread;

typedef struct _DDL_METHODS {
	void (*Register)(TCHAR*, int);		 
	void (*Log)(TCHAR*);				 
	void (*Test)(void);					
} DLLMethods;

typedef struct ThreadControlData {
	BOOL* areTaxisRequestsPause;		// Flag to control if the system is paused or not 
	int nrMaxTaxis;						// Number of taxis in the system
	int nrMaxPassengers;				// Number of passengers in the system
	Cell* map;							// Cell 
	Taxi* taxis;						// Array of taxis in the system
	BOOL isSystemClosing;				// Flag to control if the system is closing (exit cycles, ...)
	CC_Broadcast* broadcast;			// Communication broadcast container
	int* WaitTimeOnTaxiRequest;			// Time to wait in passenger requests
	Passenger* passengers;				// Array of passengers in the system
	HContainer* hContainer;				// Container of mechanisms names
	CDLogin_Request* cdLogin_Request;	// Communication login request container
	CDLogin_Response* cdLogin_Response;	// Communication login response container
	char charMap[MIN_COL][MIN_LIN];		// Array of chars to pass to the taxi
	SHM_CC_REQUEST* requests;			// Array of requests (to future select the taxi that will transport the new passenger)
	DLLMethods* dllMethods;
} CDThread;

typedef struct IndividualThreadControl {
	CDThread* cd;		// Control data of CenTaxi 
	CC_Comm comm;		// Communication container for each one of the taxis
} IndividualCD;

