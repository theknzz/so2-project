#pragma once
#include "rules.h"

typedef struct _CC_MAPINFO CC_INFO;
typedef struct _COORDS Coords;
typedef struct _Passenger Passenger;
typedef struct _TAXI Taxi;
typedef struct MapCell Cell;
typedef struct _MSG_CONTENT Content;
typedef struct _LOGIN_RESPONSE_CONTAINER LR_Container;
typedef struct _LOGIN_MSG SHM_LOGIN_REQUEST;
typedef struct _LOGIN_MSG_R SHM_LOGIN_RESPONSE;
typedef struct _MSG_TO_CENTRAL SHM_CC_REQUEST;
typedef struct _BROADCAST_MSG SHM_BROADCAST;
typedef struct _MSG_TO_TAXI SHM_CC_RESPONSE;
typedef struct Control_Data_Broadcast CC_Broadcast;
typedef struct Control_Data_For_Request CC_CDRequest;
typedef struct Control_Data_For_Response CC_CDResponse;
typedef struct Control_Data_Login_Request CDLogin_Request;
typedef struct Control_Data_Login_Response CDLogin_Response;
typedef struct HANDLE_CONTAINER HContainer;
typedef struct _CC_COMMUNICATION_CONTAINER CC_Comm;
typedef struct _CONTROL_DATA_TAXI_THREAD CD_TAXI_Thread;
typedef struct _DDL_METHODS DLLMethods;
typedef struct Control_Data_PRODUTOR_CONSUMIDOR ProdCons;
typedef struct ThreadControlData CDThread;
typedef struct _SHM_CENTAXI_TO_MAPINFO SHM_MAPINFO;
typedef struct IndividualThreadControl IndividualCD;
typedef struct NP_MESSAGE_REGISTER_PASSENGERS PassRegisterMessage;
typedef struct NP_MESSAGE_TALK_PASSENGERS PassMessage;
typedef struct THREAD_ESTABLISH_NAMEDPIPE_CONNECTION_TAXI TENC;
typedef struct MAPINFO_STRUCT MapInfo;

struct _COORDS {
	int x, y; // map positions
};

struct _Passenger {
	enum passanger_state state; // passenger state (Waiting, ...)
	TCHAR nome[25]; // passenger name
	Coords location; // passenger location
	Coords destination; // passenger destination
	HANDLE* requests;  // buffer de requests de transports
	int* requestsCounter;
};

struct _TAXI {
	TCHAR licensePlate[9]; // taxi's license plate
	Coords location; // taxi actual position
	int autopilot; // 0 - false, 1- true
	float velocity; // taxi's velocity
	Passenger client; // taxis' passenger
	int nq; // taxis's fov
	enum taxi_direction direction; // taxis's direction
	HANDLE hNamedPipe;
};

struct MapCell {
	int x, y; // cell position 
	enum type cellType; // cell type
	char display; // cell display on the screen
	Taxi* taxis; // taxis inside the cell
	Passenger* passengers; // passengers inside the cell
};

struct _MSG_CONTENT {
	Taxi taxi;
	Passenger passenger;
};

struct _LOGIN_RESPONSE_CONTAINER {
	TCHAR request_shm_name[50];
	TCHAR request_mutex_name[50];
	TCHAR request_event_name[50];

	TCHAR response_shm_name[50];
	TCHAR response_mutex_name[50];
	TCHAR response_event_name[50];
};

struct _LOGIN_MSG {
	enum message_id action;  // request action
	Taxi taxi; // content for the request
};

struct _LOGIN_MSG_R {
	enum message_id action; // response action
	LR_Container container; // content for the response
};

struct _MSG_TO_CENTRAL {
	enum message_id action; // request action
	Content messageContent; // content for the request
};

struct _BROADCAST_MSG {
	Passenger passenger;	// content for the broadcast
	BOOL isSystemClosing;	// flag saying if central is shutting down or not
};

struct _MSG_TO_TAXI {
	enum response_id action;  // response action
	char map[MIN_COL][MIN_LIN]; // content for the response
	Passenger passenger;		// content for the response
};

struct Control_Data_Broadcast {
	HANDLE mutex;			// mutex for access control
	HANDLE new_passenger;	// event to notify a new broadcast message
	SHM_BROADCAST* shared;	// broadcast content
};

struct Control_Data_For_Request {
	HANDLE mutex;			// mutex for access control
	HANDLE new_response;	// event to notify a new response
	SHM_CC_REQUEST* shared;	// request content
};

struct Control_Data_For_Response {
	HANDLE mutex;				// mutex for access control
	HANDLE new_request;			// event to notify a new request
	SHM_CC_RESPONSE* shared;	// response content
};

struct Control_Data_Login_Request {
	HANDLE login_m;				// mutex to control all taxis access 
	HANDLE login_write_m;		// mutex to control access to the shm
	HANDLE new_response;		// event to notify a new response
	SHM_LOGIN_REQUEST* request;	// login request content
};

struct Control_Data_Login_Response {
	HANDLE login_m;					// mutex to control access to the shm
	HANDLE new_request;				// event to notify a new request
	SHM_LOGIN_RESPONSE* response;	// login response content
};

struct HANDLE_CONTAINER {
	HANDLE* threads;			// array of thread handles
	HANDLE* handles;			// array of handles
	HANDLE* views;				// array of views
	int* handleCounter;			// handle counter
	int* viewCounter;			// view counter
	int* threadCounter;			// thread counter
};

struct _CC_COMMUNICATION_CONTAINER {
	CC_CDRequest request;		// Communication request container
	CC_CDResponse response;		// Communication response container
	LR_Container container;		// Container for mechanisms names
};

struct _CONTROL_DATA_TAXI_THREAD{
	HANDLE hNamedPipeComm, eventNewCMessage;
	CC_Comm comm;						// Communication container
	Taxi* taxi;							// Information about the taxi himself
	HANDLE taxiGate;					// Flag that controls the access to the system
	char charMap[MIN_COL][MIN_LIN];		// char map to do validations on moves, ...
	CC_Broadcast* broadcast;			// Communication broadcast container
	BOOL isTaxiKicked;					// Flag to exit cycles
	HANDLE connectEvent;				// Event handle
};

struct _DDL_METHODS {
	void (*Register)(TCHAR*, int);		 
	void (*Log)(TCHAR*);				 
	void (*Test)(void);					
};

struct Control_Data_PRODUTOR_CONSUMIDOR {
	HANDLE canCreate, canConsume, mutex;
	int *posW, *posR;
	Passenger* buffer;
};

struct ThreadControlData {
	HANDLE mtx_access_control;
	ProdCons* prod_cons;
	HANDLE hNamedPipe;
	HANDLE hPassPipeRegister, hPassPipeTalk;
	HANDLE eventNewPMessage, eventNewCMessage, eventNewConnection;
	BOOL* areTaxisRequestsPause;		// Flag to control if the system is paused or not 
	int nrMaxTaxis;						// Number of taxis in the system
	int nrMaxPassengers;				// Number of passengers in the system
	Cell* map;							// Cell 
	Taxi* taxis;						// Array of taxis in the system
	BOOL isSystemClosing;				// Flag to control if the system is closing (exit cycles, ...)
	BOOL isTimerLaunched;
	HANDLE timerThread;
	CC_Broadcast* broadcast;			// Communication broadcast container
	int* WaitTimeOnTaxiRequest;			// Time to wait in passenger requests
	Passenger* passengers;				// Array of passengers in the system
	HContainer* hContainer;				// Container of mechanisms names
	CDLogin_Request* cdLogin_Request;	// Communication login request container
	CDLogin_Response* cdLogin_Response;	// Communication login response container
	char charMap[MIN_LIN][MIN_COL];		// Array of chars to pass to the taxi
	DLLMethods* dllMethods;
	CC_INFO* mapinfo;
};

struct _SHM_CENTAXI_TO_MAPINFO {
	Taxi taxis[TAXI_MAX_BUFFER];
	int nrTaxis;
	Passenger passengers[PASSENGER_MAX_BUFFER];
	int nrPassengers;
	Cell map[MIN_LIN][MIN_COL];
};

struct _CC_MAPINFO {
	HANDLE mutex;
	HANDLE new_info;
	SHM_MAPINFO* message;
};

struct IndividualThreadControl {
	CDThread* cd;		// Control data of CenTaxi 
	CC_Comm comm;		// Communication container for each one of the taxis
};

struct NP_MESSAGE_REGISTER_PASSENGERS{
	enum response_id resp;
	Passenger passenger;
};

struct NP_MESSAGE_TALK_PASSENGERS {
	enum response_id resp;
	Content content;
	BOOL isSystemClosing;
};

struct THREAD_ESTABLISH_NAMEDPIPE_CONNECTION_TAXI {
	int index;
	CDThread* cd;
	TCHAR target[9];
};

struct MAPINFO_STRUCT {
	Taxi taxis[TAXI_MAX_BUFFER];
	int nrTaxis;
	Passenger passengers[PASSENGER_MAX_BUFFER];
	int nrPassengers;
	Cell map[MIN_LIN][MIN_COL];
	HWND window;
	HBITMAP StretBitMap, GrassBitMap, TaxiWaitingBitMap, PassengerWaitingWithoutTaxiBitMap, TaxiBusyBitMap, PassengerWaitingWithTaxiBitMap;
};

