#pragma once


// rules
#define MIN_LIN 50
#define MIN_COL 50
#define COMM_BUF_SIZE 5
#define MAX_TAXIS 5
#define MAX_PASSENGERS 5

// Map characters
#define S_CHAR 'R' // Street char
#define B_CHAR 'E' // Building char

// Type of Map Cells
enum type { Street, Building/*, Taxi, Passenger */ };

// Message intentions
enum message_id { 
	RegisterTaxiInCentral,
};

enum response_id {
	OK,
	ERRO,
};

// Shared memory
#define SHM_CC_REQUEST_NAME _T("SHM_CC_REQUEST")
#define SHM_CC_RESPONSE_NAME _T("SHM_CC_RESPONSE")

// Control mechanisms
#define TAXI_CAN_TALK _T("taxi_can_talk")
#define CENTRAL_CAN_READ _T("central_can_read")
#define CENTRAL_MUTEX _T("central_mutex")
#define RESPONSE_MUTEX _T("response_mutex")
#define EVENT_GOT_RESPONSE _T("event_got_response")
#define EVENT_READ_FROM_TAXIS _T("event_read_from_taxis")
#define EVENT_WRITE_FROM_TAXIS _T("event_write_from_taxis")

// Admin Commands
#define ADM_KICK _T("kick")
#define ADM_CLOSE _T("close")
#define ADM_LIST _T("list")
#define ADM_PAUSE _T("pause")
#define ADM_RESUME _T("resume")
#define ADM_INTERVAL _T("interval")
#define ADM_HELP _T("help")