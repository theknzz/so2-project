#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>

int _tmain(int argc, TCHAR* argv[]) {

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

	char* smth = MapViewOfFile(fmFile, FILE_MAP_READ, 0, 0, 0);
	if (!smth) {
		_tprintf(_T("Erro - MapViewOfFile (%d).\n"), GetLastError());
		exit(-1);
	}

	for (unsigned int i = 0; i < 50*50; i++) {
		_tprintf(_T("%c"), smth[i]);
	}

	UnmapViewOfFile(smth);
	CloseHandle(fmFile);
	CloseHandle(file);
	return 0;
}

