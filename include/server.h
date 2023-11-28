#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined(linux)

#include <arpa/inet.h>
#include <netdb.h> /* gethostbyname */
#include <netinet/in.h>
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
#define MAX_CLIENTS 100

#define BUF_SIZE 1024
#define MAX_USERNAME_SIZE 20

#include "Awale.h"
#include "client.h"

typedef struct Client {
  int sock;
  char *name;
  GameState *game;
  int turn;
  struct Client *opponent;
} Client;

static void init(void);
static void end(void);
static void appServer(void);
static int init_connection_server(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer, unsigned int size);
static void write_string(SOCKET sock, const char* buffer);
static void write_game(Client* client, int moveResult, int move);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);

#endif /* guard */
