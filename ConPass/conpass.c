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
#include "structs.h"

TCHAR** ParseCommand(TCHAR* cmd, int *argc) {
    TCHAR command[6][100];
    TCHAR* delimiter = _T(" \0");
    int counter = 0;

    TCHAR* aux = _tcstok(cmd, delimiter);
    CopyMemory(command[counter++], aux, sizeof(TCHAR) * 100);

    while ((aux = _tcstok(NULL, delimiter)) != NULL) {
        CopyMemory(command[counter++], aux, sizeof(TCHAR) * 100);
        if (counter == 6) break;
    }
    while (counter < 6) {
        CopyMemory(command[counter++], _T("NULL"), sizeof(TCHAR) * 100);
    }
    *argc = counter;
    return command;
}

DWORD WINAPI RecebeNotificacao(LPVOID ptr) {
    HANDLE hPipe = (HANDLE)ptr;
    PassMessage message;
    DWORD nr;
    BOOL ret;

    while (1) {
        ret = ReadFile(hPipe, &message, sizeof(PassMessage), &nr, NULL);
        if (message.resp == OK) {
            _tprintf(_T("Passenger added to central of taxis!\n"));
        }
        message.resp = OK;
        if (!WriteFile(hPipe, &message, sizeof(PassMessage), &nr, NULL)) {
            _tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
            exit(-1);
        }
        ZeroMemory(&message, sizeof(PassMessage));
    }
    return 0;
}

int _tmain(int argc, TCHAR* argv[]) {

    HANDLE hRegister, hTalk;
    BOOL centralIsActive = TRUE;
    BOOL ret;
    Passenger me;
    PassMessage message;
    DWORD nr;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
    
    // criar um handle para conseguir ler do pipe
    hRegister = CreateFile(NP_PASS_REGISTER, /*GENERIC_READ*/ PIPE_ACCESS_DUPLEX, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hRegister == NULL) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), NP_PASS_REGISTER);
        exit(-1);
    }
    else {
        _tprintf(_T("NP_REGISTER create file success!\n"));
    }

    // criar um handle para conseguir ler do pipe
    hTalk = CreateFile(NP_PASS_TALK, /*GENERIC_READ*/ PIPE_ACCESS_DUPLEX, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hTalk == NULL) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), NP_PASS_TALK);
        exit(-1);
    }
    else {
        _tprintf(_T("NP_TALK create file success!\n"));
    }

    // esperar que o pipe seja criado
    //if (!WaitNamedPipe(NP_PASS_REGISTER, NMPWAIT_WAIT_FOREVER)) {
    //    _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NP_PASS_REGISTER);
    //    exit(-1);
    //}
    //else {
    //    _tprintf(_T("NP_REGISTER wait success!\n"));
    //}

    //if (!WaitNamedPipe(NP_PASS_TALK, NMPWAIT_WAIT_FOREVER)) {
    //    _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NP_PASS_TALK);
    //    exit(-1);
    //}
    //else {
    //    _tprintf(_T("NP_TALK wait success!\n"));
    //}

    HANDLE cthread = CreateThread(NULL, 0, RecebeNotificacao, hTalk, 0, NULL);
    if (!cthread) {
        _tprintf(_T("Error launching console thread (%d)\n"), GetLastError());
        Sleep(2000);
        exit(-1);
    }

    TCHAR command[100];
    TCHAR cmd[6][100];
    int nrArgs;
    BOOL gate = TRUE;

    while (centralIsActive) {
        while (1) {
            _tscanf(_T(" %[^\n]"), command);
            CopyMemory(cmd, ParseCommand(command, &nrArgs), sizeof(TCHAR) * 6 * 100);

            if (!_tcscmp(PASS_REGISTER, cmd[0]) && nrArgs == 6) {
                CopyMemory(me.nome, cmd[1], _tcslen(cmd[1]) * sizeof(TCHAR));
                me.nome[_tcslen(cmd[1])] = '\0';
                int locX, locY, desX, desY;
                locX = _ttoi(cmd[2]); locY = _ttoi(cmd[3]); desX = _ttoi(cmd[4]); desY = _ttoi(cmd[5]);
                if (locX < 0 || locY > MIN_COL || desX < 0 || desY > MIN_COL) {
                    _tprintf(_T("Inserted coordinates doesn't match the city map.\n x: [0, %d]\n y: [0, %d]"), MIN_COL, MIN_LIN);
                    continue;
                }
                else {
                    me.location.x = locX;
                    me.location.y = locY;
                    me.destination.x = desX;
                    me.destination.y = desY;
                }
                break;
            }
            _tprintf(_T("System doesn't recognize '%s' as a command. You may use %s name locX locY desX desY\n"), cmd[0], PASS_REGISTER);
            ZeroMemory(cmd, sizeof(TCHAR)*6*100);
        }

        CopyMemory(&message.passenger, &me, sizeof(Passenger));
        if (!WriteFile(hRegister, &message, sizeof(PassMessage), &nr, NULL)) {
            _tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
            exit(-1);
        }

        ret = ReadFile(hRegister, &message, sizeof(PassMessage), &nr, NULL);
        if (message.resp == OK) {
            _tprintf(_T("Passenger added to central of taxis!\n"));
        }
        else {
            _tprintf(_T("ERRO\n"));
        }
    }
    CloseHandle(hRegister);
    CloseHandle(hTalk);
    CloseHandle(cthread);
}