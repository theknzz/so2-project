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
	TCHAR licensePlate[100];
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

typedef struct _MSG {
	enum message_id action;
	Content messageContent;
} SHM_CEN_CON;

typedef struct txtInterfaceControlData {
	BOOL gate;
	Cell* map;
} TI_Controldata;

typedef struct ControDataWithTaxis {
	HANDLE mutex;
	HANDLE read_event;
	HANDLE write_event;
	SHM_CEN_CON* shared;
} CDTaxi;

typedef struct ThreadControlData {
	int taxiFreePosition;
	Taxi* taxis;
	CDTaxi controlDataTaxi;
} CDThread;