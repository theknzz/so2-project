#pragma once

typedef struct MapCell {
	int x, y;
	enum type cellType;
	// + Taxi
	// + Passageiro
} Cell;

typedef struct MessageToCenTaxi {
	enum intention action;			// razao da messagem
	// Taxi taxi;				
} CentralMessage;

typedef struct MessageToConTaxi {
	enum intention action;			// razao da messagem
	// Passenger passenger;
} TaxiMessage;

typedef struct _MSG {
	int read_index, write_index;
	CentralMessage messages[COMM_BUF_SIZE];  // buffer circular
} SHM_Cen_To_Con;

typedef struct _MSG_1 {
	TaxiMessage messages[MAX_TAXIS];		// buffer de mensagens (acesso individual por taxi)
} SHM_Con_To_Cen;

typedef struct _COORDS {
	int x, y;
} Coords;

typedef struct _TAXI {
	TCHAR* licensePlate;
	Coords destination;
} TaxiWorker;