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

void GetOutput(enum response_id resp, Passenger* passenger, Taxi* taxi) {
    switch (resp) {
    case PASSENGER_GOT_TAXI_ASSIGNED:
        _tprintf(_T("%s was assigned to %s.\n"), passenger->nome, taxi->licensePlate);
        break;
    case PASSENGER_CAUGHT:
        _tprintf(_T("%s was caught by %s.\n"), passenger->nome, taxi->licensePlate);
        break;
    case NO_TRANSPORT_AVAILABLE:
        _tprintf(_T("Central has no taxi available for '%s' at the moment.\n"), passenger->nome);
        break;
    case COORDINATES_FROM_OTHER_CITY:
        _tprintf(_T("You can't choose other city's coordinates.\n"));
        break;
    case PASSENGER_DELIVERED:
        _tprintf(_T("%s as arrived destination. Remember to tip %s"), passenger->nome, taxi->licensePlate);
        break;
    }
}

DWORD WINAPI RecebeNotificacao(LPVOID ptr) {
    HANDLE hPipe = (HANDLE)ptr;
    PassMessage message;
    DWORD nr;
    BOOL ret;

    while (1) {
        ret = ReadFile(hPipe, &message, sizeof(PassMessage), &nr, NULL);

        GetOutput(message.resp, &message.content.passenger, &message.content.taxi);

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
    PassRegisterMessage resgMessage;
    PassMessage message;
    DWORD nr;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

    // esperar que o pipe seja criado
    if (!WaitNamedPipe(NP_PASS_REGISTER, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NP_PASS_REGISTER);
        exit(-1);
    }
    else {
        _tprintf(_T("NP_REGISTER wait success!\n"));
    }

    if (!WaitNamedPipe(NP_PASS_TALK, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NP_PASS_TALK);
        exit(-1);
    }
    else {
        _tprintf(_T("NP_TALK wait success!\n"));
    }
    
    // criar um handle para conseguir ler do pipe
    hRegister = CreateFile(NP_PASS_REGISTER, /*GENERIC_READ*/ PIPE_ACCESS_DUPLEX, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hRegister == NULL) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), NP_PASS_REGISTER);
        exit(-1);
    }
    else {
        _tprintf(_T("NP_REGISTER create file success!\n"));
    }

    //DWORD dwMode = PIPE_READMODE_MESSAGE;
    //if (!SetNamedPipeHandleState(hRegister, &dwMode, NULL, NULL)) {
    //    _tprintf(_T("SetNamedPipeHandleState failed\n"));
    //    exit(-1);
    //}

    // criar um handle para conseguir ler do pipe
    hTalk = CreateFile(NP_PASS_TALK, /*GENERIC_READ*/ PIPE_ACCESS_DUPLEX, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hTalk == NULL) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), NP_PASS_TALK);
        exit(-1);
    }
    else {
        _tprintf(_T("NP_TALK create file success!\n"));
    }

    //if (!SetNamedPipeHandleState(hTalk, &dwMode, NULL, NULL)) {
    //    _tprintf(_T("SetNamedPipeHandleState failed\n"));
    //    exit(-1);
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
                    _tprintf(_T("Inserted coordinates don't match the city map.\n x: [0, %d]\n y: [0, %d]"), MIN_COL, MIN_LIN);
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

        CopyMemory(&resgMessage.passenger, &me, sizeof(Passenger));
        if (!WriteFile(hRegister, &resgMessage, sizeof(PassRegisterMessage), &nr, NULL)) {
            _tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
            exit(-1);
        }

        ret = ReadFile(hRegister, &resgMessage, sizeof(PassRegisterMessage), &nr, NULL);
        GetOutput(resgMessage.resp, NULL, NULL);
        //if (message.resp == OK) {
        //    _tprintf(_T("Passenger added to central of taxis!\n"));
        //}
        //else {
        //    _tprintf(_T("ERRO\n"));
        //}
    }
    CloseHandle(hRegister);
    CloseHandle(hTalk);
    CloseHandle(cthread);
}