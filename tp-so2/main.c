#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#include "rules.h"

// rules
#define MIN_LIN 50
#define MIN_COL 50
#define COMM_BUF_SIZE 5
#define MAX_TAXIS 5

// Map characters
#define S_CHAR 'R' // Street char
#define B_CHAR 'E' // Building char

// Actions
enum type { Street, Building, Taxi, Passenger };

// Message intentions
enum intention { WannaWork, WannaJoin };

// Shared memory
#define SHM_CENTAXI_CONTAXI _T("SHM_CenTaxi_ConTaxi")

// Control mechanisms
#define TAXI_CAN_TALK _T("taxi_can_talk")
#define CENTRAL_CAN_READ _T("central_can_read")

// Admin Commands
#define ADM_KICK _T("kick")
#define ADM_CLOSE _T("close")
#define ADM_LIST _T("list")
#define ADM_PAUSE _T("pause")
#define ADM_RESUME _T("resume")
#define ADM_INTERVAL _T("interval")
#define ADM_HELP _T("help")

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

void ClearScreen() {
	Sleep(1000 * 3);
	_tprintf(_T("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"));
}

// função-thread responsável por tratar a interação com o admin
DWORD WINAPI TextInterface(LPVOID ptr) {

	BOOL* gate = (BOOL*)ptr;
	TCHAR command[100];
	
	while (1) {
		_tprintf(_T("Command: "));
		_tscanf_s(_T(" %99[^\n]"), command, sizeof(TCHAR)*100);
		_tprintf(_T("Executing '%s'\n"), command);
		FindFeatureAndRun(command);
		ClearScreen();
	}

	return 0;
}

int FindFeatureAndRun(TCHAR* command) {
	TCHAR commands[6][100] = {
		_T("\tkick x - kick taxi with x as id.\n"),
		_T("\tclose - close the system.\n"), 
		_T("\tlist - list all the taxis in the system.\n"), 
		_T("\tpause - pauses taxis acceptance in the system.\n"), 
		_T("\tresume - resumes taxis acceptance in the system.\n"), 
		_T("\tinteval x - changes the time central waits for the taxis to ask for work.\n") };
	int argument=-1;
	TCHAR* delimiter = _T(" ");

	TCHAR* cmd = _tcstok(command, delimiter);

	if (cmd != NULL) {
		TCHAR* aux = _tcstok(NULL, delimiter);
		if (aux!=NULL)
			argument = _ttoi(aux);
	}

	// Garantir que cobre todas as situacoes
	cmd = _tcslwr(cmd);

	if (_tcscmp(cmd, ADM_KICK) == 0) {
		if (argument==-1)
			_tprintf(_T("This command requires a id.\n"));
		else
			_tprintf(_T("I am kicking taxi with id %d\n"), argument);
	}
	else if (_tcscmp(cmd, ADM_CLOSE) == 0) {
		// #TODO notify todas as apps
		_tprintf(_T("System is closing.\n"));
	}
	else if (_tcscmp(cmd, ADM_LIST) == 0) {
		_tprintf(_T("List all taxis\n"));
	}
	else if (_tcscmp(cmd, ADM_PAUSE) == 0) {
		_tprintf(_T("System pause\n"));
	}
	else if (_tcscmp(cmd, ADM_RESUME) == 0) {
		_tprintf(_T("System resume\n"));
	}
	else if (_tcscmp(cmd, ADM_INTERVAL) == 0) {
		if (argument == -1)
			_tprintf(_T("This command requires a id.\n"));
		else
			_tprintf(_T("System changing the interval to %d\n"), argument);
	}
	else if (_tcscmp(cmd, ADM_HELP) == 0) {
		for (int i = 0; i < 6; i++)
			_tprintf(_T("%s"), commands[i]);
	}
	else {
		_tprintf(_T("System doesn't recognize the command, type 'help' to view all the commands.\n"));
	}
	return 0;
}


void LoadMapa(Cell* mapa, char* buffer) {
	for (unsigned int y = 0; y < MIN_LIN - 1; y++) {
		for (unsigned int x = 0; x < MIN_COL - 1; x++) {
			int i = x + y;
			switch (buffer[i]) {
			case S_CHAR:
				mapa[i].cellType = Street;
				break;
			case B_CHAR:
				mapa[i].cellType = Building;
				break;
			//case 'T':
			//	mapa[i].bloco = Taxi;
			//	break;
			//case 'P':
			//	mapa[i].bloco = Passageiro;
			//	break;
			}
			mapa[i].x = x;
			mapa[i].y = y;
		}
	}
}

char* ReadFileToCharArray(TCHAR* mapName) {

	HANDLE file = CreateFile(mapName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Erro ao obter handle para o ficheiro (%d).\n"), GetLastError());
		return NULL;
	}

	HANDLE fmFile = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL /*FILE MAPPING NAME*/);
	if (!fmFile) {
		_tprintf(_T("Erro ao mapear o ficheiro (%d).\n"), GetLastError());
		return NULL;
	}

	char* mvof = MapViewOfFile(fmFile, FILE_MAP_READ, 0, 0, 0);
	if (!mvof) {
		_tprintf(_T("Erro - MapViewOfFile (%d).\n"), GetLastError());
		return NULL;
	}

	UnmapViewOfFile(mvof);
	CloseHandle(fmFile);
	CloseHandle(file);

	return mvof;
}

int _tmain(int argc, TCHAR* argv[]) {

	Cell mapa[MIN_LIN * MIN_COL];
	int nrMaxTaxis = MAX_TAXIS; // colocar variavel dentro de uma dll (?)
	BOOL gate = FALSE;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif


	HANDLE taxiCanTalkSemaphore = CreateSemaphore(NULL, 0, MAX_TAXIS, TAXI_CAN_TALK);
	if (taxiCanTalkSemaphore == NULL) {
		_tprintf(_T("Error creating taxiCanTalkSemaphore (%d)\n"), GetLastError());
		_gettch();
		exit(-1);
	}
	// Check if this mechanism is already create
	// if GetLastError() returns ERROR_ALREADY_EXISTS
	// this is the second instance running
	else if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(_T("You can't run multiple instances of this application.\n"));
		_gettch();
		exit(-1);
	}

	HANDLE consoleThread = CreateThread(NULL, 0, TextInterface, &gate, 0, NULL);
	if (!consoleThread) {
		_tprintf(_T("Erro ao lançar thread %d\n"), GetLastError());
		_gettch();
		exit(-1);
	}
	_tprintf(_T("Thread launched!\n"));
	WaitForSingleObject(consoleThread, INFINITE);
	if (argc == 2) {
		nrMaxTaxis = _ttoi(argv[1]);
	}
	_tprintf(_T("Nr max de taxis: %.2d\n"), nrMaxTaxis);

	TaxiWorker* taxis = (TaxiWorker*)malloc(nrMaxTaxis * sizeof(TaxiWorker));
	if (taxis == NULL) {
		_tprintf(_T("Error allocating memory (%d)\n"), GetLastError());
		_gettch();
		exit(-1);
	}
	_tprintf(_T("Memory allocated successfully.\n"));


	// register(TAXI_CAN_TALK, 3);
	
	/*HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(Shared_Msg),
		SHM_CENTAXI_CONTAXI);
	if (hMapFile == NULL) {
		_tprintf(TEXT("Erro ao mapear memória partilhada (%d).\n"),
			GetLastError());
		return FALSE;
	}
	else _tprintf(TEXT("Memória partilhada mapeada.\n"));

	HANDLE shared = (Shared_Msg*) MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(Shared_Msg));
	if (shared == NULL) {
		_tprintf(TEXT("Erro ao criar vista da memória partilhada (%d).\n"),
			GetLastError());
		_gettchar();
		return FALSE;
	}
	else _tprintf(TEXT("Vista da Memória partilhada criada.\n"));*/
	
	char* fileContent = NULL;
	// Le conteudo do ficheiro para array de chars
	if ((fileContent = ReadFileToCharArray(_T("..\\..\\maps\\map.txt"))) == NULL) {
		_tprintf(_T("Error reading the file (%d)\n"), GetLastError());
		exit(-1);
	}
	else {
		_tprintf(_T("Map loaded to memory!\n"));
	}

	// Preenche mapa com o conteudo do ficheiro
	LoadMapa(mapa, fileContent);


	WaitForSingleObject(consoleThread, INFINITE);
	free(taxis);
	return 0;
}

