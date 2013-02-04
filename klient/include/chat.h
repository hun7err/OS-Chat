#ifndef CHAT_H
#define CHAT_H

#include <time.h>

#define MAX_SERVER_COUNT 100
#define MAX_USER_COUNT_PER_SERVER 20
#define MAX_USER_NAME_LENGTH 16
#define MAX_ROOM_NAME_LENGTH 16
#define MAX_USER_LIST_LENGTH 200
 
#define SEMAPHORE_COUNT 3
#define SERVER 0
#define CLIENT 1
#define LOG 2

#define LOGFILE "/tmp/czat.log"
 
#define GLOBAL_ROOM_NAME "global"
 
typedef enum { // typ wiadomości
        MSG_HEARTBEAT = 1,
        MSG_REGISTER,
        MSG_UNREGISTER,
        MSG_JOIN,
        MSG_LIST,
        MSG_LEAVE,
        MSG_STATUS, // nieużywane
        MSG_ROOM,
        MSG_PRIVATE,
        MSG_SERVER,
	MSG_HEARTBEAT_SERVER,
        TERM = 0x7fffffffffffffff // dla ustalenia typu enuma
} type_t;
 
typedef struct { // wiadomość klient-klient
        type_t type;
        struct {
                unsigned int id;
                time_t send_date;
                char sender[MAX_USER_NAME_LENGTH];
                char recipient[MAX_USER_NAME_LENGTH];
                char message[512];
        } content;
} standard_message;
 
typedef struct { // m.in. do potwierdzeń
        type_t type;
        struct {
                unsigned int id;              
                char sender[MAX_USER_NAME_LENGTH];
                int value; //status / numer kolejki
        } content;
} compact_message;
 
typedef struct { // lista użytkowników w danym pokoju
        type_t type;
        struct {
                unsigned int id;              
                char list[MAX_USER_LIST_LENGTH][MAX_USER_NAME_LENGTH];
        } content;
} user_list;
 
typedef struct { // wiadomość serwer-serwer
        type_t type; // always MSG_SERVER
        struct {
                standard_message msg;
        } content;
} server_message;
 
typedef struct { // dane klienta
        int server_queue_key; // server queue key
        int queue_key; // client queue key
        char name[MAX_USER_NAME_LENGTH]; // nazwa usera
        char room[MAX_ROOM_NAME_LENGTH]; // nazwa pokoju
} client;
 
typedef struct { // dane servera
        int queue_key; // server queue key
} server;
 
typedef struct { // typ segmentu pamięci współdzielonej
        int key_semaphores; // klucz zestawu semaforów
        server servers[MAX_SERVER_COUNT]; // lista serwerów
        client clients[MAX_SERVER_COUNT * MAX_USER_COUNT_PER_SERVER]; // lista klientów
} shm_type;

#endif // CHAT_H
