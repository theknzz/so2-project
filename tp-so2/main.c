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
#define MAX_PASSENGERS 5

// Map characters
#define S_CHAR 'R' // Street char
#define B_CHAR 'E' // Building char

// Actions
enum type { Street, Building/*, Taxi, Passenger */};

// Message intentions
enum intention { WannaWork, WannaJoin };

// Shared memory
#define SHM_CENTAXI_CONTAXI _T("SHM_CenTaxi_ConTaxi")

// Control mechanisms
#define TAXI_CAN_TALK _T("taxi_can_talk")
#define CENTRAL_CAN_READ _T("central_can_read")
#define CENTRAL_MUTEX _T("central_mutex")

// Admin Commands
#define ADM_KICK _T("kick")
#define ADM_CLOSE _T("close")
#define ADM_LIST _T("list")
#define ADM_PAUSE _T("pause")
#define ADM_RESUME _T("resume")
#define ADM_INTERVAL _T("interval")
#define ADM_HELP _T("help")

typedef struct _COORDS {
	int x, y;
} Coords;

typedef struct _TAXI {
	TCHAR* licensePlate;
	Coords destination;
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

typedef struct MessageToCenTaxi {
	enum intention action;			// razao da messagem
	Taxi taxi;
} CentralMessage;

typedef struct MessageToConTaxi {
	enum intention action;			// razao da messagem
	Passenger passenger;
} TaxiMessage;

typedef struct _MSG {
	int read_index, write_index;
	CentralMessage messages[COMM_BUF_SIZE];  // buffer circular
} SHM_Cen_To_Con;

typedef struct _MSG_1 {
	TaxiMessage messages[MAX_TAXIS];		// buffer de mensagens (acesso individual por taxi)
} SHM_Con_To_Cen;


void ClearScreen() {
	Sleep(1000 * 3);
	//_tprintf(_T("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"));
}

void PrintMap(Cell* map) {
	for (int i = 0; i < MIN_LIN; i++) {
		for (int j = 0; j < MIN_COL; j++) {
			char c = (map + (i*50)+j)->display;
			_tprintf(_T("%c"), c);
		}
		_tprintf(_T("\n"));
	}
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

void LoadMapa(Cell* map, char* buffer) {
	int aux=0;
	for (int i = 0; i < MIN_LIN; i++) {
		for (int j = 0; j < MIN_COL; j++) {
			aux = (i * MIN_COL) + j;
			char c = buffer[aux];
			if (c == S_CHAR) {
				map[aux].display = '-';
				map[aux].cellType = Street;
				map[aux].x = j;
				map[aux].y = i;
			}
			else if (c == B_CHAR) {
				map[aux].display = '+';
				map[aux].cellType = Building;
				map[aux].x = j;
				map[aux].y = i;
			}
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
		CloseHandle(file);
		return NULL;
	}

	char* mvof = (char*) MapViewOfFile(fmFile, FILE_MAP_READ, 0, 0, 0);
	if (!mvof) {
		_tprintf(_T("Erro - MapViewOfFile (%d).\n"), GetLastError());
		CloseHandle(file);
		CloseHandle(fmFile);
		return NULL;
	}

	CloseHandle(fmFile);
	CloseHandle(file);
	return mvof;
}

void WaitAndReleaseThreads(HANDLE* threads, int nr) {
	for (int i = 0; i < nr; i++) {
		WaitForSingleObject(threads + i, INFINITE);
		CloseHandle(threads + i);
	}
}

void CloseMyHandles(HANDLE* handles, int nr) {
	for (int i = 0; i < nr; i++)
		CloseHandle(handles + i);
}

void UnmapAndClose(HANDLE* views, int nr) {
	for (int i = 0; i < nr; i++) {
		UnmapViewOfFile(views + i);
		CloseHandle(views + i);
	}
}

int _tmain(int argc, TCHAR* argv[]) {

	Cell map[MIN_LIN * MIN_COL];
	int nrMaxTaxis = MAX_TAXIS; // colocar variavel dentro de uma dll (?)
	int nrMaxPassengers = MAX_PASSENGERS;
	BOOL gate = FALSE;

	HANDLE views[50];
	HANDLE handles[50];
	HANDLE threads[50];
	int viewCounter = 0, handleCounter = 0, threadCounter = 0;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	// ====================================================================================================

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

	handles[handleCounter++] = taxiCanTalkSemaphore;

	// ====================================================================================================

	HANDLE consoleThread = CreateThread(NULL, 0, TextInterface, &gate, 0, NULL);
	if (!consoleThread) {
		_tprintf(_T("Error launching console thread (%d)\n"), GetLastError());
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	threads[threadCounter++] = consoleThread;

	HANDLE fmCenTaxiToConTaxi = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SHM_Cen_To_Con),
		SHM_CENTAXI_CONTAXI);
	if (fmCenTaxiToConTaxi == NULL) {
		_tprintf(TEXT("Error mapping the shared memory (%d).\n"), GetLastError());
		WaitAndReleaseThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	handles[handleCounter++] = fmCenTaxiToConTaxi;

	HANDLE mfCenTaxiToConTaxi = (SHM_Cen_To_Con*) MapViewOfFile(fmCenTaxiToConTaxi,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(SHM_Cen_To_Con));
	if (mfCenTaxiToConTaxi == NULL) {
		_tprintf(TEXT("Error mapping a view to the shared memory (%d).\n"), GetLastError());
		WaitAndReleaseThreads(threads, threadCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	else _tprintf(TEXT("Vista da Memória partilhada criada.\n"));

	views[viewCounter++] = mfCenTaxiToConTaxi;

	HANDLE mtxCenTaxiToConTaxi = CreateMutex(NULL, FALSE, CENTRAL_MUTEX);
	if (mtxCenTaxiToConTaxi == NULL) {
		_tprintf(TEXT("Error creating mtxCenTaxiToConTaxi mutex (%d).\n"), GetLastError());
		WaitAndReleaseThreads(threads, threadCounter);
		UnmapAndClose(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}

	handles[handleCounter++] = mtxCenTaxiToConTaxi;

	// #TODO é possivel configurar só um dos argumentos?
	if (argc > 2) {
		nrMaxTaxis = _ttoi(argv[1]);
		nrMaxPassengers = _ttoi(argv[2]);
	}
	_tprintf(_T("Nr max de taxis: %.2d\n"), nrMaxTaxis);
	_tprintf(_T("Nr max de passengers: %.2d\n"), nrMaxPassengers);

	Taxi* taxis = (Taxi*)malloc(nrMaxTaxis * sizeof(Taxi));
	if (taxis == NULL) {
		_tprintf(_T("Error allocating memory (%d)\n"), GetLastError());
		WaitAndReleaseThreads(threads, threadCounter);
		UnmapAndClose(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	_tprintf(_T("Memory allocated successfully.\n"));

	Passenger* passengers = (Passenger*)malloc(nrMaxPassengers * sizeof(Passenger));
	if (passengers == NULL) {
		_tprintf(_T("Error allocating memory (%d)\n"), GetLastError());
		WaitAndReleaseThreads(threads, threadCounter);
		UnmapAndClose(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	_tprintf(_T("Memory allocated successfully.\n"));

	char* fileContent = NULL;
	// Le conteudo do ficheiro para array de chars
	if ((fileContent = ReadFileToCharArray(_T("E:\\projects\\so2-project\\maps\\map1.txt"))) == NULL) {
		_tprintf(_T("Error reading the file (%d)\n"), GetLastError());
		WaitAndReleaseThreads(threads, threadCounter);
		UnmapAndClose(views, viewCounter);
		CloseMyHandles(handles, handleCounter);
		exit(-1);
	}
	else {
		_tprintf(_T("Map loaded to memory!\n"));
	}

	// Preenche mapa com o conteudo do ficheiro
	LoadMapa(map, fileContent);
	PrintMap(map);

	WaitAndReleaseThreads(threads, threadCounter);
	UnmapAndClose(views, viewCounter);
	CloseMyHandles(handles, handleCounter);

	free(taxis);
	free(passengers);
	return 0;
}

