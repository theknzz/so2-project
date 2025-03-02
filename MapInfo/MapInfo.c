#include <windows.h>
#include <tchar.h>
#include "rules.h"
#include "structs.h"
#include "MapInfo.h"


/* ===================================================== */
/* Programa base (esqueleto) para aplica��es Windows */
/* ===================================================== */
// Cria uma janela de nome "Janela Principal" e pinta fundo de branco
// Modelo para programas Windows:
// Composto por 2 fun��es:
// WinMain() = Ponto de entrada dos programas windows
// 1) Define, cria e mostra a janela
// 2) Loop de recep��o de mensagens provenientes do Windows
// TrataEventos()= Processamentos da janela (pode ter outro nome)
// 1) � chamada pelo Windows (callback)
// 2) Executa c�digo em fun��o da mensagem recebida
LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
// Nome da classe da janela (para programas de uma s� janela, normalmente este nome �
// igual ao do pr�prio programa) "szprogName" � usado mais abaixo na defini��o das
// propriedades do objecto janela
// ============================================================================
// FUN��O DE IN�CIO DO PROGRAMA: WinMain()
// ============================================================================
// Em Windows, o programa come�a sempre a sua execu��o na fun��o WinMain()que desempenha
// o papel da fun��o main() do C em modo consola WINAPI indica o "tipo da fun��o" (WINAPI
// para todas as declaradas nos headers do Windows e CALLBACK para as fun��es de
// processamento da janela)
// Par�metros:
// hInst: Gerado pelo Windows, � o handle (n�mero) da inst�ncia deste programa
// hPrevInst: Gerado pelo Windows, � sempre NULL para o NT (era usado no Windows 3.1)
// lpCmdLine: Gerado pelo Windows, � um ponteiro para uma string terminada por 0
// destinada a conter par�metros para o programa
// nCmdShow: Par�metro que especifica o modo de exibi��o da janela (usado em
// ShowWindow()
HINSTANCE ghInst;
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd; // hWnd � o handler da janela, gerado mais abaixo por CreateWindow()
	MSG lpMsg; // MSG � uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp; // WNDCLASSEX � uma estrutura cujos membros servem para
	// definir as caracter�sticas da classe da janela
   // ============================================================================
   // 1. Defini��o das caracter�sticas da janela "wcApp"
   // (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
   // ============================================================================
	wcApp.cbSize = sizeof(WNDCLASSEX); // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst; // Inst�ncia da janela actualmente exibida
	// ("hInst" � par�metro de WinMain e vem
	// inicializada da�)
	wcApp.lpszClassName = _T("MapInfo - Graphic Interface"); // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = TrataEventos; // Endere�o da fun��o de processamento da janela
	// ("TrataEventos" foi declarada no in�cio e
	// encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW; // Estilo da janela: Fazer o redraw se for
	// modificada horizontal ou verticalmente
		wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION); // "hIcon" = handler do �con normal
		// "NULL" = Icon definido no Windows
		// "IDI_AP..." �cone "aplica��o"
	wcApp.hIconSm = LoadIcon(NULL, IDI_INFORMATION); // "hIconSm" = handler do �con pequeno
	// "NULL" = Icon definido no Windows
	// "IDI_INF..." �con de informa��o
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW); // "hCursor" = handler do cursor (rato)
   // "NULL" = Forma definida no Windows
   // "IDC_ARROW" Aspecto "seta"
	wcApp.lpszMenuName = (LPCTSTR)IDR_MENU1; // Classe do menu que a janela pode ter
   // (NULL = n�o tem menu)
	wcApp.cbClsExtra = 0; // Livre, para uso particular
	wcApp.cbWndExtra = 0; // Livre, para uso particular
	wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	ghInst = hInst;
	// ============================================================================
	// 2. Registar a classe "wcApp" no Windows
	// ============================================================================
	if (!RegisterClassEx(&wcApp))
		return(0);
	// ============================================================================
	// 3. Criar a janela
	// ============================================================================
	hWnd = CreateWindow(
		_T("MapInfo - Graphic Interface"), // Nome da janela (programa) definido acima
		TEXT("MapInfo"),// Texto que figura na barra do t�tulo
		WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, // Estilo da janela (WS_OVERLAPPED= normal)
		CW_USEDEFAULT, // Posi��o x pixels (default=� direita da �ltima)
		CW_USEDEFAULT, // Posi��o y pixels (default=abaixo da �ltima)
		1600, // Largura da janela (em pixels)
		1060, // Altura da janela (em pixels)
		(HWND)HWND_DESKTOP, // handle da janela pai (se se criar uma a partir de
		// outra) ou HWND_DESKTOP se a janela for a primeira,
		// criada a partir do "desktop"
		(HMENU)NULL, // handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst, // handle da inst�ncia do programa actual ("hInst" �
		// passado num dos par�metros de WinMain()
		0); // N�o h� par�metros adicionais para a janela
		// ============================================================================
		// 4. Mostrar a janela
		// ============================================================================
	ShowWindow(hWnd, nCmdShow); // "hWnd"= handler da janela, devolvido por
   // "CreateWindow"; "nCmdShow"= modo de exibi��o (p.e.
   // normal/modal); � passado como par�metro de WinMain()
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia � janela uma

   // mensagem para pintar, mostrar dados, (refrescar)�
   // ============================================================================
   // 5. Loop de Mensagens
   // ============================================================================
   // O Windows envia mensagens �s janelas (programas). Estas mensagens ficam numa fila de
   // espera at� que GetMessage(...) possa ler "a mensagem seguinte"
   // Par�metros de "getMessage":
   // 1)"&lpMsg"=Endere�o de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no
   // in�cio de WinMain()):
   // HWND hwnd handler da janela a que se destina a mensagem
   // UINT message Identificador da mensagem
   // WPARAM wParam Par�metro, p.e. c�digo da tecla premida
   // LPARAM lParam Par�metro, p.e. se ALT tamb�m estava premida
		// DWORD time Hora a que a mensagem foi enviada pelo Windows
		// POINT pt Localiza��o do mouse (x, y)
		// 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
		// receber as mensagens para todas as janelas pertencentes � thread actual)
		// 3)C�digo limite inferior das mensagens que se pretendem receber
		// 4)C�digo limite superior das mensagens que se pretendem receber
		// NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
		// terminando ent�o o loop de recep��o de mensagens, e o programa
		while (GetMessage(&lpMsg, NULL, 0, 0)) {
			TranslateMessage(&lpMsg); // Pr�-processamento da mensagem (p.e. obter c�digo
		   // ASCII da tecla premida)
			DispatchMessage(&lpMsg); // Enviar a mensagem traduzida de volta ao Windows, que
		   // aguarda at� que a possa reenviar � fun��o de
		   // tratamento da janela, CALLBACK TrataEventos (abaixo)
		}
	// ============================================================================
	// 6. Fim do programa
	// ============================================================================
	return((int)lpMsg.wParam); // Retorna sempre o par�metro wParam da estrutura lpMsg
}
// ============================================================================
// FUN��O DE PROCESSAMENTO DA JANELA
// Esta fun��o pode ter um nome qualquer: Apenas � neces�rio que na inicializa��o da 
//estrutura "wcApp", feita no in�cio de WinMain(), se identifique essa fun��o.Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pr�-processadas 
// no loop "while" da fun��o WinMain()

MapInfo info;

LRESULT CALLBACK ChooseThemeDialog(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam) {
	HWND hwndList, hwndPicture;
	TCHAR str[100];
	HICON icon;
	HBITMAP hbitmap;
	int idFreeTaxi, idBusyTaxi, idPassengerWoTaxi, idPassengerWTaxi, index;

	switch (messg) {

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			hwndList = GetDlgItem(hDlg, IDC_LIST_TAXI_WITH_PASSENGER);
			index = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
			idBusyTaxi = (int)SendMessage(hwndList, LB_GETITEMDATA, index, 0);

			hwndList = GetDlgItem(hDlg, IDC_LIST_TAXI_WITHOUT_PASSENGER);
			index = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
			idFreeTaxi = (int)SendMessage(hwndList, LB_GETITEMDATA, index, 0);

			hwndList = GetDlgItem(hDlg, IDC_LIST_PASSENGER_WITH_TAXI);
			index = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
			idPassengerWTaxi = (int)SendMessage(hwndList, LB_GETITEMDATA, index, 0);

			hwndList = GetDlgItem(hDlg, IDC_LIST_PASSENGER_WITHOUT_TAXI);
			index = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
			idPassengerWoTaxi = (int)SendMessage(hwndList, LB_GETITEMDATA, index, 0);

			//_stprintf(str, _T("FreeTaxi: %d, BusyTaxi: %d, PassengerWoTaxi: %d, PassengerWTaxi: %d"),
			//	idFreeTaxi, idBusyTaxi, idPassengerWoTaxi, idPassengerWTaxi);
			if (idBusyTaxi != -1 && idFreeTaxi != -1 && idPassengerWTaxi != -1 && idPassengerWoTaxi != -1) {
				CreateRegistryForBitMaps(idFreeTaxi, idBusyTaxi, idPassengerWoTaxi, idPassengerWTaxi, TRUE);
				LoadBitMaps(ghInst, &info);
				if (MessageBox(hDlg, _T("The bitmaps have been updated."), _T("MapInfo - Theme Chooser"), MB_OK | MB_ICONINFORMATION) == IDOK) {
					EndDialog(hDlg, 0);
				}
			}
			else {
				MessageBox(hDlg, _T("You need to insert a bitmap theme for all the assets."), _T("MapInfo - Theme Chooser"), MB_OK | MB_ICONWARNING);
			}
			return FALSE;
			break;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return FALSE;
			break;
		}
		break;

	case WM_INITDIALOG:
		hwndList = GetDlgItem(hDlg, IDC_LIST_TAXI_WITH_PASSENGER);
		icon = LoadIcon(ghInst, (LPCTSTR)IDI_SMALL);
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)icon);

		// -------------
		_stprintf(str, _T("%d"), IDB_BUSY_TAXI);
		int pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)_T("Black Taxi"));
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_BUSY_TAXI);
		_stprintf(str, _T("%d"), IDB_BUSY_TAXI1);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)_T("Red Taxi"));
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_BUSY_TAXI1);


		// -------------
		hwndList = GetDlgItem(hDlg, IDC_LIST_TAXI_WITHOUT_PASSENGER);
		_stprintf(str, _T("%d"), IDB_FREE_TAXI);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)_T("Green Taxi"));
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_FREE_TAXI);
		_stprintf(str, _T("%d"), IDB_FREE_TAXI1);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)_T("Yellow Taxi"));
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_FREE_TAXI1);

		// -------------
		hwndList = GetDlgItem(hDlg, IDC_LIST_PASSENGER_WITHOUT_TAXI);
		_stprintf(str, _T("%d"), IDB_PASSENGER_WITHOUT_TAXI);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)_T("Black Passenger"));
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_PASSENGER_WITHOUT_TAXI);
		_stprintf(str, _T("%d"), IDB_PASSENGER_WITHOUT_TAXI1);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)_T("White Passenger"));
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_PASSENGER_WITHOUT_TAXI1);

		// -------------
		hwndList = GetDlgItem(hDlg, IDC_LIST_PASSENGER_WITH_TAXI);
		_stprintf(str, _T("%d"), IDB_PASSENGER_WITH_TAXI);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)_T("Black Passenger"));
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_PASSENGER_WITH_TAXI);
		_stprintf(str, _T("%d"), IDB_PASSENGER_WITH_TAXI1);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)_T("White Passenger"));
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_PASSENGER_WITH_TAXI1);

		return TRUE;
		break;
	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return 0;
		break;
	}
	return(0);
}

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	
	HANDLE commThread;
	PAINTSTRUCT ps;
	HDC hdc;
	static HDC hdcMem;
	info.window = hWnd;
	RECT rc;
	HBITMAP bkground;
	TCHAR str[100];

	TRACKMOUSEEVENT mouse_event;

	int x, y;

	mouse_event.cbSize = sizeof(mouse_event);
	mouse_event.hwndTrack = hWnd;
	mouse_event.dwFlags = TME_HOVER | TME_LEAVE;
	mouse_event.dwHoverTime = HOVER_DEFAULT;

	switch (messg) {
	case WM_CREATE:
		if ((commThread = CreateThread(NULL, 0, TalkToCentral, &info, 0, NULL)) == NULL) {
			// Tratar erro
		}
		//CreateRegistryForBitMaps(IDB_FREE_TAXI, IDB_BUSY_TAXI, IDB_PASSENGER_WITHOUT_TAXI, IDB_PASSENGER_WITH_TAXI, FALSE);
		LoadBitMaps(ghInst, &info);
		GetClientRect(hWnd, &rc);
		hdc = GetDC(hWnd);
		hdcMem = CreateCompatibleDC(hdc);
		bkground = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
		SelectObject(hdcMem, bkground);
		SetDCBrushColor(hdcMem, RGB(255, 255, 255));
		SelectObject(hdcMem, GetStockObject(DC_BRUSH));
		PatBlt(hdcMem, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
		break;
	case WM_PAINT:
		GetClientRect(hWnd, &rc);
		LoadBitMaps(ghInst, &info);
		PaintMap(hdcMem, &info, ghInst);
		hdc = BeginPaint(hWnd, &ps);
		BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdcMem, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	case WM_MOUSEHOVER:
		PatBlt(hdcMem, CONTAXI_X-40, 20, 200, 1000, WHITENESS);
		TrataHover(hdcMem, &info, ghInst, lParam);
		TrackMouseEvent(&mouse_event);
		break;
	case WM_MOUSEMOVE:
		TrackMouseEvent(&mouse_event);
		break;
	case WM_ERASEBKGND:
		return (LRESULT)0;
		break;
	case WM_LBUTTONDOWN:
		x = (int)GET_X_LPARAM(lParam);
		y = (int)GET_Y_LPARAM(lParam);
		PatBlt(hdcMem, CONPASS_X - 40, 20, 200, 1000, WHITENESS);
		TrataClick(hdcMem, &info, ghInst, x, y);
		break;
	case WM_DESTROY: // Destruir a janela e terminar o programa
	// "PostQuitMessage(Exit Status)"
		// change the theme of the bitmaps to the default one
		CreateRegistryForBitMaps(IDB_FREE_TAXI, IDB_BUSY_TAXI, IDB_PASSENGER_WITHOUT_TAXI, IDB_PASSENGER_WITH_TAXI, TRUE);
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_THEME:
			 DialogBox(NULL,
                        MAKEINTRESOURCE(IDD_DIALOG),
                        hWnd,
                        (DLGPROC)ChooseThemeDialog);
			break;
		}
		break;
	default:
			return(DefWindowProc(hWnd, messg, wParam, lParam));
		break;
	}
	return(0);
}