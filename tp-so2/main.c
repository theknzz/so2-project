#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>

#define MIN_LINHAS 50
#define MIN_COLUNAS 50

enum tipo {Rua, Edificio, Taxi, Passageiro};

typedef struct CelulaMapa {
	int x, y;
	enum tipo bloco;
} Bloco;

/*
*/
void LoadMapa(Bloco* mapa, char* buffer) {
	for (unsigned int y = 0; y < MIN_LINHAS - 1; y++) {
		for (unsigned int x = 0; x < MIN_COLUNAS - 1; x++) {
			int i = x + y;
			switch (buffer[i]) {
			case 'R':
				mapa[i].bloco = Rua;
				break;
			case 'E':
				mapa[i].bloco = Edificio;
				break;
			case 'T':
				mapa[i].bloco = Taxi;
				break;
			case 'P':
				mapa[i].bloco = Passageiro;
				break;
			}
			mapa[i].x = x;
			mapa[i].y = y;
			//_tprintf(_T("li cell {%.2d, %.2d} = %d \n"), mapa[i].x, mapa[i].y, mapa[i].bloco);
		}
	}
}

int _tmain(int argc, TCHAR* argv[]) {

	Bloco mapa[MIN_LINHAS * MIN_COLUNAS];

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	HANDLE file = CreateFile(_T("..\\maps\\map.txt"), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Erro ao obter handle para o ficheiro (%d).\n"), GetLastError());
		exit(-1);
	}

	HANDLE fmFile = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL /*FILE MAPPING NAME*/);
	if (!fmFile) {
		_tprintf(_T("Erro ao mapear o ficheiro (%d).\n"), GetLastError());
		exit(-1);
	}

	char* mvof = MapViewOfFile(fmFile, FILE_MAP_READ, 0, 0, 0);
	if (!mvof) {
		_tprintf(_T("Erro - MapViewOfFile (%d).\n"), GetLastError());
		exit(-1);
	}

	LoadMapa(mapa, mvof);
	
	UnmapViewOfFile(mvof);
	CloseHandle(fmFile);
	CloseHandle(file);
	return 0;
}

