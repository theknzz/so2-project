#pragma once

// rules
#define MIN_LIN 50
#define MIN_COL 50
#define COMM_BUF_SIZE 5
#define MAX_TAXIS 5
#define MAX_PASSENGERS 5
#define NQ 10
#define WAIT_TIME 5
#define CIRCULAR_BUFFER_SIZE 5
#define TAXI_MAX_BUFFER 50
#define PASSENGER_MAX_BUFFER 50

// dll indices
#define MUTEX 1
#define CRITICAL_SECTION 2
#define SEMAPHORE 3
#define EVENT 4
#define WAITABLE_TIMER 5
#define FILE_MAPPING 6
#define VIEW 7
#define NAMED_PIPE 8

// Map characters
#define S_CHAR '_' // Street char
#define B_CHAR 'X' // Building char
#define T_CHAR 'T' // Taxi char
#define S_DISPLAY '-'
#define B_DISPLAY '+'
#define T_DISPLAY 't'

// registry keys
#define KEY_NAME _T("MapInfo\\Theme")
#define FREE_TAXI _T("FREE_TAXI_Bitmap")
#define BUSY_TAXI _T("BUSY_TAXI_Bitmap")
#define WAITING_PASSENGER_WITHOUT_TAXI _T("WAITING_PASSENGER_WITHOUT_TAXI_Bitmap")
#define WAITING_PASSENGER_WITH_TAXI _T("WAITING_PASSENGER_WITH_TAXI_Bitmap")

#define BITMAP_SIZE 20
#define CONTAXI_X 1040
#define CONPASS_X 1280
// Type of Map Cells
enum type { Street, Building };

// Message intentions
enum message_id {
	RegisterTaxiInCentral,
	UpdateTaxiLocation,
	WarnPassengerCatch,
	WarnPassengerDeliever,
	GetCityMap,
	RequestPassenger,
	NotifySpeedChange,
	NotifyNQChange,
	NotifyTaxiLeaving,
	EstablishNamedPipeComm,
};

enum response_id {
	OK,
	ERRO,
	INVALID_REGISTRATION_TAXI_POSITION,
	OUTOFBOUNDS_TAXI_POSITION,
	LICENSE_PLATED_ALREADY_IN_CENTRAL,
	HAS_NO_AVAILABLE_PASSENGER,
	PASSENGER_DOESNT_EXIST,
	PASSENGER_ALREADY_TAKEN,
	CANT_QUIT_WITH_PASSENGER,
	TAXI_REQUEST_PAUSED,
	TAXI_KICKED,
	PASSENGER_GOT_TAXI_ASSIGNED,
	PASSENGER_CAUGHT,
	PASSENGER_REACHED_DESTINATION,
	NO_TRANSPORT_AVAILABLE,
	COORDINATES_FROM_OTHER_CITY,
	PASSENGER_DELIVERED,
	CENTRAL_GOING_OFFLINE,
	ESTIMATED_TIME,
	NO_ESTIMATED_TIME
};

enum passanger_state {
	Waiting,
	Taken,
	OnDrive,
	Done,
	NotFound
};

enum taxi_direction {
	DOWN,
	LEFT,
	UP,
	RIGHT
};


// Shared memory
#define SHM_CC_REQUEST_NAME _T("SHM_CC_REQUEST_%s")
#define SHM_CC_RESPONSE_NAME _T("SHM_CC_RESPONSE_%s")

#define SHM_LOGIN_REQUEST_NAME _T("SHM_REQUEST_LOGIN")
#define SHM_LOGIN_RESPONSE_NAME _T("SHM_RESPONSE_LOGIN")

#define SHM_BROADCAST_PASSENGER_ARRIVE _T("SHM_BROADCAST_PASSENGER_ARRIVE")

#define SHM_MAP_INFO_NAME _T("SHM_MAPINFO")

// Control mechanisms
#define TAXI_GATEWAY _T("taxi_can_talk")
#define CENTRAL_CAN_READ _T("central_can_read")

#define LOGIN_REQUEST_MUTEX _T("login_request_mutex")
#define LOGIN_RESPONSE_MUTEX _T("login_response_mutex")
#define LOGIN_TAXI_WRITE_MUTEX _T("login_write_mutex")
#define LOGIN_TAXI_READ_MUTEX _T("login_read_mutex")

#define RESPONSE_MUTEX _T("response_mutex_%s")
#define REQUEST_MUTEX _T("request_mutex_%s")

#define BROADCAST_MUTEX _T("broadcast_mutex")

#define MAPINFO_MUTEX _T("mapinfo_mutex")

#define EVENT_GOT_RESPONSE _T("event_got_response")
#define EVENT_READ_FROM_TAXIS _T("event_read_from_taxis")
#define EVENT_WRITE_FROM_TAXIS _T("event_write_from_taxis")

#define EVENT_RESPONSE _T("event_response_%s")
#define EVENT_REQUEST _T("event_request_%s")

#define EVENT_NEW_PASSENGER _T("event_new_passenger")
#define EVENT_NP_CONNECTION _T("event_np_connection")

#define EVENT_NEW_INFO _T("event_new_info")
#define EVENT_NEW_PASSENGER_MSG _T("event_new_passenger_msg")
#define EVENT_NEW_CENTRAL_MSG _T("event_new_central_msg")

#define SMP_CANREAD _T("smp_canread")
#define SMP_CANWRITE _T("smp_canwrite")
#define MTX_CENTRAL_PRODCONS _T("mutex_produtor_consumidor_central")
#define MTX_CONPASS_PRODCONS _T("mutex_produtor_consumidor_conpass")
#define MTX_ACCESS_CONTROL _T("mutex_control")

// Named Pipes
#define NP_PASS_REGISTER _T("\\\\.\\pipe\\np_passenger_register")
#define NP_PASS_TALK _T("\\\\.\\pipe\\np_passenger_talk")
#define NP_TAXI_NAME _T("\\\\.\\pipe\\np_taxi")

// Admin Commands
#define ADM_KICK _T("kick")
#define ADM_CLOSE _T("close")
#define ADM_LIST_TAXIS _T("list_taxis")
#define ADM_LIST_PASSENGERS _T("list_passengers")
#define ADM_PAUSE _T("pause")
#define ADM_RESUME _T("resume")
#define ADM_INTERVAL _T("interval")
#define ADM_HELP _T("help")

// Taxi Commands
#define TXI_TRANSPORT _T("transport") // transport passenger_name
#define TXI_SPEED_UP _T("speed") // speed
#define TXI_SLOW_DOWN _T("slow") // slow
#define TXI_NQ_DEFINE _T("nq") // nq x
#define TXI_AUTOPILOT _T("autopilot") // autopilot
#define TXI_HELP _T("help") // help
#define TXI_UP _T("up")
#define TXI_LEFT _T("left")
#define TXI_DOWN _T("down")
#define TXI_RIGHT _T("right")
#define TXI_CLOSE _T("close")
#define TXI_CATCH _T("catch")
#define TXI_DELIVER _T("deliver")

// Passenger Commands
#define PASS_REGISTER _T("register")

// Debug Commands
#define DBG_ADD_PASSENGER _T("add") // add passenger_name x y dest_x dest_y