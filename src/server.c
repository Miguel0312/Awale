#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/Awale.h"
#include "../include/client.h"
#include "../include/protocol.h"
#include "../include/server.h"

static void init(void) {
#ifdef WIN32
  WSADATA wsa;
  int err = WSAStartup(MAKEWORD(2, 2), &wsa);
  if (err < 0) {
    puts("WSAStartup failed !");
    exit(EXIT_FAILURE);
  }
#endif
}

static void end(void) {
#ifdef WIN32
  WSACleanup();
#endif
}

static void appServer(void) {
  srand(time(NULL)); // Initialization, should only be called once.
  SOCKET sock = init_connection_server();
  char buffer[BUF_SIZE];
  /* the index for the array */
  int actual = 0;
  int max = sock;
  /* an array for all clients */
  Client clients[MAX_CLIENTS] = {0};

  fd_set rdfs;

  while (1) {
    int i = 0;
    FD_ZERO(&rdfs);

    /* add STDIN_FILENO */
    FD_SET(STDIN_FILENO, &rdfs);

    /* add the connection socket */
    FD_SET(sock, &rdfs);

    /* add socket of each client */
    for (i = 0; i < actual; i++) {
      FD_SET(clients[i].sock, &rdfs);
    }

    if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1) {
      perror("select()");
      exit(errno);
    }

    /* something from standard input : i.e keyboard */
    if (FD_ISSET(STDIN_FILENO, &rdfs)) {
      /* stop process when type on keyboard */
      break;
    } else if (FD_ISSET(sock, &rdfs)) {
      /* new client */
      SOCKADDR_IN csin = {0};
      size_t sinsize = sizeof csin;
      int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
      if (csock == SOCKET_ERROR) {
        perror("accept()");
        continue;
      }

      /* after connecting the client sends its name */
      if (read_client(csock, buffer) == -1) {
        /* disconnected */
        continue;
      }

      /* what is the new maximum fd ? */
      max = csock > max ? csock : max;

      FD_SET(csock, &rdfs);

      Client c;
      c.sock = csock;
      c.name = (char *)malloc(20 * sizeof(char));
      fflush(stdout);
      memcpy(c.name, buffer, strlen(buffer));
      c.name[strlen(buffer)] = 0;
      clients[actual] = c;
      actual++;
    } else {
      int i = 0;
      for (i = 0; i < actual; i++) {
        /* a client is talking */
        if (FD_ISSET(clients[i].sock, &rdfs)) {
          Client client = clients[i];
          int c = read_client(clients[i].sock, buffer);
          /* client disconnected */
          if (c == 0) {
            closesocket(clients[i].sock);
            if(clients[i].opponent != NULL) {
              buffer[0] = OPPONENT_DISCONNECTED;
              write_client(clients[i].opponent->sock, buffer, 1);
              clients[i].opponent->opponent = NULL;
              clients[i].opponent->game = NULL;
              clients[i].opponent = NULL;
              free(clients[i].game);
              clients[i].game = NULL;
            }
            remove_client(clients, i, &actual);
            strncpy(buffer, client.name, BUF_SIZE - 1);
            strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
            //send_message_to_all_clients(clients, client, actual, buffer, 1);
          } else {
            char request_type = buffer[0];
            switch (request_type) {
            case CHALLENGE_REQUEST: {
              printf("Player challenged: %s\n", &buffer[1]);
              int j;
              for (j = 0; j < actual; j++) {
                if (strcmp(clients[j].name, &buffer[1]) == 0) {
                  char *request = (char *)malloc(MAX_USERNAME_SIZE + 1);
                  request[0] = CONFIRM_CHALLENGE;
                  memcpy(request + 1, clients[i].name, strlen(clients[i].name));
                  request[1 + strlen(clients[i].name)] = 0;
                  write_string(clients[j].sock, request);
                }
              }
              break;
            }
            case CHALLENGE_ACCEPTED: {
              printf("Challenge accepted\n");
              char *challenger = &buffer[1];
              int j;
              for (j = 0; j < actual; j++) {
                if (strcmp(clients[j].name, &buffer[1]) == 0) {
                  char *request = (char *)malloc(MAX_USERNAME_SIZE + 1);
                  request[0] = CHALLENGE_ACCEPTED;
                  memcpy(request + 1, clients[i].name, strlen(clients[i].name));
                  request[1 + strlen(clients[i].name)] = 0;
                  write_string(clients[j].sock, request);
                  GameState *game = newGame();
                  clients[i].game = clients[j].game = game;
                  int turn = rand();
                  clients[i].turn = turn;
                  clients[j].turn = 1 - turn;
                  // Write the game to both players
                  write_game(&clients[i], -1, -1);
                  write_game(&clients[j], -1, -1);
                  clients[i].opponent = &clients[j];
                  clients[j].opponent = &clients[i];
                }
              }
              break;
            }
            case MOVE_DATA : {
              int moveResult = makeMove(buffer[1], clients[i].game);
              write_game(&clients[i], moveResult, buffer[1]);
              if(moveResult){
                write_game(clients[i].opponent, moveResult, buffer[1]);
              }
              if(hasEnded(clients[i].game)) {
                buffer[0] = END_GAME;
                write_client(clients[i].sock, buffer, 1);
                write_client(clients[i].opponent->sock, buffer, 1);
                free(clients[i].game);
                clients[i].game = clients[i].opponent->game = NULL;
                clients[i].opponent->opponent = NULL;
                clients[i].opponent = NULL;
              }
              break;
            }
            case LIST_ONLINE_PLAYERS: {
              int total_len = 0;
              for (int j = 0; j < actual; j++) {
                total_len += strlen(clients[j].name);
              }
              int packet_size = 2 + actual + total_len;
              char *request = (char *)malloc(packet_size);
              request[0] = ONLINE_PLAYERS_RESPONSE;
              request[1] = (char)actual;
              int pos = 2;
              for (int j = 0; j < actual; j++) {
                memcpy(&request[pos], clients[j].name, strlen(clients[j].name));
                pos += strlen(clients[j].name);
                request[pos++] = 0;
              }
              write_client(clients[i].sock, request, packet_size);
            }
              // send_message_to_all_clients(clients, client, actual, buffer,
              // 0);
            } break;
          }
        }
      }
    }
  }

  clear_clients(clients, actual);
  end_connection(sock);
}

static void clear_clients(Client *clients, int actual) {
  int i = 0;
  for (i = 0; i < actual; i++) {
    closesocket(clients[i].sock);
  }
}

static void remove_client(Client *clients, int to_remove, int *actual) {
  /* we remove the client in the array */
  memmove(clients + to_remove, clients + to_remove + 1,
          (*actual - to_remove - 1) * sizeof(Client));
  /* number client - 1 */
  (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender,
                                        int actual, const char *buffer,
                                        char from_server) {
  int i = 0;
  char message[BUF_SIZE];
  message[0] = 0;
  for (i = 0; i < actual; i++) {
    /* we don't send message to the sender */
    if (sender.sock != clients[i].sock) {
      if (from_server == 0) {
        strncpy(message, sender.name, BUF_SIZE - 1);
        strncat(message, " : ", sizeof message - strlen(message) - 1);
      }
      strncat(message, buffer, sizeof message - strlen(message) - 1);
      write_string(clients[i].sock, message);
    }
  }
}

static int init_connection_server(void) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  SOCKADDR_IN sin = {0};

  if (sock == INVALID_SOCKET) {
    perror("socket()");
    exit(errno);
  }

  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(PORT);
  sin.sin_family = AF_INET;

  if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR) {
    perror("bind()");
    exit(errno);
  }

  if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR) {
    perror("listen()");
    exit(errno);
  }

  return sock;
}

static void end_connection(int sock) { closesocket(sock); }

static int read_client(SOCKET sock, char *buffer) {
  int n = 0;

  if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
    perror("recv()");
    /* if recv error we disonnect the client */
    n = 0;
  }

  buffer[n] = 0;

  return n;
}

static void write_client(SOCKET sock, const char *buffer, unsigned int size) {
  if (send(sock, buffer, size, 0) < 0) {
    perror("send()");
    exit(errno);
  }
}

static void write_string(SOCKET sock, const char *buffer) {
  write_client(sock, buffer, strlen(buffer));
}

static void write_game(Client* client, int moveResult, int move) {
  renderGame(client->game);
  char *buffer = malloc((3 + PLAYERS + BOARD_SIZE) * sizeof(char));
  switch(moveResult) {
    case -1: {
      buffer[0] = GAME_DATA; 
      break;
    }
    case 0: {
      buffer[0] = MOVE_FAIL; 
      break;
    } case 1: {
      buffer[0] = MOVE_SUCCESS;
      break;
    }
  }
  buffer[1] = (client->game->turn + client->turn) % 2;
  for (int i = 0; i < PLAYERS; i++) {
    buffer[2 + i] = client->game->scores[i];
  }
  for (int i = 0; i < BOARD_SIZE; i++) {
    buffer[2 + PLAYERS + i] = client->game->board[i];
  }
  buffer[2+PLAYERS+BOARD_SIZE] = move;

  write_client(client->sock, buffer, 3 + PLAYERS + BOARD_SIZE);

  free(buffer);
}
/*static void write_client(SOCKET sock, const char *buffer) {
  if (send(sock, buffer, strlen(buffer), 0) < 0) {
    perror("send()");
    exit(errno);
  }
}*/

int main(int argc, char **argv) {
  init();

  appServer();

  end();

  return EXIT_SUCCESS;
}
