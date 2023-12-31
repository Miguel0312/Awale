#ifndef CLIENT_H
#define CLIENT_H

#include "Awale.h"
#ifdef WIN32

#include <winsock2.h>

#elif defined(linux)

#include <arpa/inet.h>
#include <netdb.h> /* gethostbyname */
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> /* close */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF "\r\n"
#define PORT 1984

#define BUF_SIZE 1024

#define MENU_NOT_SHOWN 0
#define MENU_SHOWN 1
#define PLAYER_TURN 2
#define PLAYER_WAIT 3
#define CHAT_MODE 4
#define OBSERVING 5

static void init(void);
static void end(void);
static void appClient(const char *address, const char *name);
static int init_connection_client(const char *address);
static void end_connection(int sock);
static int read_server(SOCKET sock, char *buffer);
static void write_server(SOCKET sock, const char *buffer);

static void send_chat_message(SOCKET sock, const char *message);
static int handle_confirm_challenge(SOCKET sock, const char *challenger);
static void handle_move_success(GameState *game, int move);
static int handle_game_data(GameState *game, char *buffer, int isGameStart);
static int handle_end_game(GameState *game);
static int handle_online_players_response(char *buffer);
static int handle_confirm_chat(SOCKET sock, char *name);
static void observer_handle_game_data(GameState *game, char *buffer);
#endif /* guard */
