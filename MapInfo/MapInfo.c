#include <windows.h>
#include <tchar.h>
#include "rules.h"
#include "structs.h"
#include "MapInfo.h"


/* ===================================================== */
/* Programa base (esqueleto) para aplicações Windows */
/* ===================================================== */
// Cria uma janela de nome "Janela Principal" e pinta fundo de branco
// Modelo para programas Windows:
// Composto por 2 funções:
// WinMain() = Ponto de entrada dos programas windows
// 1) Define, cria e mostra a janela
// 2) Loop de recepção de mensagens provenientes do Windows
// TrataEventos()= Processamentos da janela (pode ter outro nome)
// 1) É chamada pelo Windows (callback)
// 2) Executa código em função da mensagem recebida
LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
// Nome da classe da janela (para programas de uma só janela, normalmente este nome é
// igual ao do próprio programa) "szprogName" é usado mais abaixo na definição das
// propriedades do objecto janela
// ============================================================================
// FUNÇÃO DE INÍCIO DO PROGRAMA: WinMain()
// ============================================================================
// Em Windows, o programa começa sempre a sua execução na função WinMain()que desempenha
// o papel da função main() do C em modo consola WINAPI indica o "tipo da função" (WINAPI
// para todas as declaradas nos headers do Windows e CALLBACK para as funções de
// processamento da janela)
// Parâmetros:
// hInst: Gerado pelo Windows, é o handle (número) da instância deste programa
// hPrevInst: Gerado pelo Windows, é sempre NULL para o NT (era usado no Windows 3.1)
// lpCmdLine: Gerado pelo Windows, é um ponteiro para uma string terminada por 0
// destinada a conter parâmetros para o programa
// nCmdShow: Parâmetro que especifica o modo de exibição da janela (usado em
// ShowWindow()
HINSTANCE ghInst;
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd; // hWnd é o handler da janela, gerado mais abaixo por CreateWindow()
	MSG lpMsg; // MSG é uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp; // WNDCLASSEX é uma estrutura cujos membros servem para
	// definir as características da classe da janela
   // ============================================================================
   // 1. Definição das características da janela "wcApp"
   // (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
   // ============================================================================
	wcApp.cbSize = sizeof(WNDCLASSEX); // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst; // Instância da janela actualmente exibida
	// ("hInst" é parâmetro de WinMain e vem
	// inicializada daí)
	wcApp.lpszClassName = _T("MapInfo - Graphic Interface"); // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = TrataEventos; // Endereço da função de processamento da janela
	// ("TrataEventos" foi declarada no início e
	// encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW; // Estilo da janela: Fazer o redraw se for
	// modificada horizontal ou verticalmente
		wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION); // "hIcon" = handler do ícon normal
		// "NULL" = Icon definido no Windows
		// "IDI_AP..." Ícone "aplicação"
	wcApp.hIconSm = LoadIcon(NULL, IDI_INFORMATION); // "hIconSm" = handler do ícon pequeno
	// "NULL" = Icon definido no Windows
	// "IDI_INF..." Ícon de informação
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW); // "hCursor" = handler do cursor (rato)
   // "NULL" = Forma definida no Windows
   // "IDC_ARROW" Aspecto "seta"
	wcApp.lpszMenuName = (LPCTSTR)IDR_MENU1; // Classe do menu que a janela pode ter
   // (NULL = não tem menu)
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
		TEXT("MapInfo"),// Texto que figura na barra do título
		WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, // Estilo da janela (WS_OVERLAPPED= normal)
		CW_USEDEFAULT, // Posição x pixels (default=à direita da última)
		CW_USEDEFAULT, // Posição y pixels (default=abaixo da última)
		1200, // Largura da janela (em pixels)
		1040, // Altura da janela (em pixels)
		(HWND)HWND_DESKTOP, // handle da janela pai (se se criar uma a partir de
		// outra) ou HWND_DESKTOP se a janela for a primeira,
		// criada a partir do "desktop"
		(HMENU)NULL, // handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst, // handle da instância do programa actual ("hInst" é
		// passado num dos parâmetros de WinMain()
		0); // Não há parâmetros adicionais para a janela
		// ============================================================================
		// 4. Mostrar a janela
		// ============================================================================
	ShowWindow(hWnd, nCmdShow); // "hWnd"= handler da janela, devolvido por
   // "CreateWindow"; "nCmdShow"= modo de exibição (p.e.
   // normal/modal); é passado como parâmetro de WinMain()
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia à janela uma
   // mensagem para pintar, mostrar dados, (refrescar)…
   // ============================================================================
   // 5. Loop de Mensagens
   // ============================================================================
   // O Windows envia mensagens às janelas (programas). Estas mensagens ficam numa fila de
   // espera até que GetMessage(...) possa ler "a mensagem seguinte"
   // Parâmetros de "getMessage":
   // 1)"&lpMsg"=Endereço de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no
   // início de WinMain()):
   // HWND hwnd handler da janela a que se destina a mensagem
   // UINT message Identificador da mensagem
   // WPARAM wParam Parâmetro, p.e. código da tecla premida
   // LPARAM lParam Parâmetro, p.e. se ALT também estava premida
		// DWORD time Hora a que a mensagem foi enviada pelo Windows
		// POINT pt Localização do mouse (x, y)
		// 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
		// receber as mensagens para todas as janelas pertencentes à thread actual)
		// 3)Código limite inferior das mensagens que se pretendem receber
		// 4)Código limite superior das mensagens que se pretendem receber
		// NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
		// terminando então o loop de recepção de mensagens, e o programa
		while (GetMessage(&lpMsg, NULL, 0, 0)) {
			TranslateMessage(&lpMsg); // Pré-processamento da mensagem (p.e. obter código
		   // ASCII da tecla premida)
			DispatchMessage(&lpMsg); // Enviar a mensagem traduzida de volta ao Windows, que
		   // aguarda até que a possa reenviar à função de
		   // tratamento da janela, CALLBACK TrataEventos (abaixo)
		}
	// ============================================================================
	// 6. Fim do programa
	// ============================================================================
	return((int)lpMsg.wParam); // Retorna sempre o parâmetro wParam da estrutura lpMsg
}
// ============================================================================
// FUNÇÃO DE PROCESSAMENTO DA JANELA
// Esta função pode ter um nome qualquer: Apenas é necesário que na inicialização da 
//estrutura "wcApp", feita no início de WinMain(), se identifique essa função.Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pré-processadas 
// no loop "while" da função WinMain()

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

			_stprintf(str, _T("FreeTaxi: %d, BusyTaxi: %d, PassengerWoTaxi: %d, PassengerWTaxi: %d"),
				idFreeTaxi, idBusyTaxi, idPassengerWoTaxi, idPassengerWTaxi);
			if (MessageBox(hDlg, str, _T("MapInfo - Theme Chooser"), MB_OK) == IDOK) {
				CreateRegistryForBitMaps(idFreeTaxi, idBusyTaxi, idPassengerWoTaxi, idPassengerWTaxi);
				LoadBitMaps(ghInst, &info);
			}
			EndDialog(hDlg, 0);
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
		/*hwndPicture = GetDlgItem(hWnd, IDC_STATIC);*/
		icon = LoadIcon(ghInst, (LPCTSTR)IDI_SMALL);
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)icon);

		// -------------
		_stprintf(str, _T("%d"), IDB_BUSY_TAXI);
		int pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)str);
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_BUSY_TAXI);

		/*SendMessage(hwndPicture, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)info.TaxiBusyBitMap);*/

		// -------------
		hwndList = GetDlgItem(hDlg, IDC_LIST_TAXI_WITHOUT_PASSENGER);
		_stprintf(str, _T("%d"), IDB_FREE_TAXI);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)str);
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_FREE_TAXI);
		_stprintf(str, _T("%d"), IDB_FREE_TAXI1);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)str);
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_FREE_TAXI1);

		// -------------
		hwndList = GetDlgItem(hDlg, IDC_LIST_PASSENGER_WITHOUT_TAXI);
		_stprintf(str, _T("%d"), IDB_PASSENGER_WITHOUT_TAXI);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)str);
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_PASSENGER_WITHOUT_TAXI);

		// -------------
		hwndList = GetDlgItem(hDlg, IDC_LIST_PASSENGER_WITH_TAXI);
		_stprintf(str, _T("%d"), IDB_PASSENGER_WITH_TAXI);
		pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)str);
		SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)IDB_PASSENGER_WITH_TAXI);
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
		CreateRegistryForBitMaps(IDB_FREE_TAXI, IDB_BUSY_TAXI, IDB_PASSENGER_WITHOUT_TAXI, IDB_PASSENGER_WITH_TAXI);
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
		PaintMap(hdcMem, &info, ghInst);
		hdc = BeginPaint(hWnd, &ps);
		BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdcMem, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	case WM_MOUSEHOVER:
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
		TrataClick(hdcMem, &info, ghInst, x, y);
		break;
	
	case WM_DESTROY: // Destruir a janela e terminar o programa
	// "PostQuitMessage(Exit Status)"
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