#pragma once
//
//typedef struct MapCell {
//	int x, y;
//	enum type cellType;
//	// + Taxi
//	// + Passageiro
//} Cell;
//
//typedef struct MessageToCenTaxi {
//	enum intention action;			// razao da messagem
//	// Taxi taxi;				
//} CentralMessage;
//
//typedef struct MessageToConTaxi {
//	enum intention action;			// razao da messagem
//	// Passenger passenger;
//} TaxiMessage;
//
//typedef struct _MSG {
//	int read_index, write_index;
//	CentralMessage messages[COMM_BUF_SIZE];  // buffer circular
//} SHM_Cen_To_Con;
//
//typedef struct _MSG_1 {
//	TaxiMessage messages[MAX_TAXIS];		// buffer de mensagens (acesso individual por taxi)
//} SHM_Con_To_Cen;
//
//typedef struct _COORDS {
//	int x, y;
//} Coords;
//
//typedef struct _TAXI {
//	TCHAR* licensePlate;
//	Coords destination;
//} TaxiWorker;


typedef struct _COORDS {
	int x, y;
} Coords;

typedef struct _TAXI {
	TCHAR licensePlate[10];
	Coords location;
} Taxi;

typedef struct _Passenger {
	TCHAR* nome;
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
} TI_Controldata;

typedef struct Control_Data_For_Request {
	HANDLE mutex;
	HANDLE read_event;
	HANDLE write_event;
	SHM_CC_REQUEST* shared;
} CC_CDRequest;

typedef struct Control_Data_For_Response {
	HANDLE mutex;
	HANDLE got_response;
	SHM_CC_RESPONSE* shared;
} CC_CDResponse;

typedef struct ThreadControlData {
	int *taxiFreePosition;
	Cell* map;
	Taxi* taxis;
	CC_CDRequest controlDataTaxi;
	CC_CDResponse cdResponse;
} CDThread;

