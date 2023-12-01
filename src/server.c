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

static void free_client(Client *client) {
  char buffer[1];
  buffer[0] = OPPONENT_DISCONNECTED;
  closesocket(client->sock);
  if (client->opponent != NULL) {
    write_client(client->opponent->sock, buffer, 1);
    client->opponent->opponent = NULL;
    client->opponent->game = NULL;
    client->opponent = NULL;
    free(client->game);
    client->game = NULL;
  }
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
      c.chat = NULL;
      c.game = NULL;
      c.opponent = NULL;
      c.sock = csock;
      c.name = (char *)malloc(20 * sizeof(char));
      fflush(stdout);
      memcpy(c.name, buffer, strlen(buffer));
      c.name[strlen(buffer)] = 0;
      bool add_to_list = true;
      for (int i = 0; i < actual; i++) {
        if (strcmp(c.name, clients[i].name) == 0) {
          char request[] = {INVALID_USERNAME};
          write_client(csock, request, 1);
          add_to_list = false;
          break;
        }
      }
      if (add_to_list) {
        clients[actual] = c;
        actual++;
      }
    } else {
      int i = 0;
      for (i = 0; i < actual; i++) {
        /* a client is talking */
        if (FD_ISSET(clients[i].sock, &rdfs)) {
          Client client = clients[i];
          int c = read_client(clients[i].sock, buffer);
          /* client disconnected */
          if (c == 0) {
            free_client(&clients[i]);
            update_clients(clients, i, &actual);
            // send_message_to_all_clients(clients, client, actual, buffer, 1);
          } else {
            char request_type = buffer[0];
            switch (request_type) {
            case CHALLENGE_REQUEST: {
              handle_challenge_request(&buffer[1], &clients[i], clients,
                                       actual);
              break;
            }
            case CHALLENGE_ACCEPTED: {
              char *challenger_name = &buffer[1];
              int j;
              for (j = 0; j < actual; j++) {
                if (strcmp(clients[j].name, challenger_name) == 0) {
                  handle_challenge_accepted(&clients[j], &clients[i]);
                  break;
                }
              }
              break;
            }
            case CHALLENGE_REFUSED: {
              int j;
              for (j = 0; j < actual; j++) {
                if (strcmp(clients[j].name, &buffer[1]) == 0) {
                  handle_challenge_refused(&clients[j], &clients[i]);
                }
              }
              break;
            }
            case MOVE_DATA: {
              handle_move_data(&clients[i], buffer[1]);
              break;
            }
            case LIST_ONLINE_PLAYERS: {
              handle_list_online_players(clients, actual, &clients[i]);
              break;
            }
            case CHAT_REQUEST: {
              int j;
              for (j = 0; j < actual; j++) {
                if (strcmp(clients[j].name, &buffer[1]) == 0) {
                  handle_chat_request(&clients[i], &clients[j]);
                  break;
                }
                if (j == actual) {
                  char request[] = {PLAYER_NOT_FOUND};
                  write_client(clients[i].sock, request, 1);
                }
                // send_message_to_all_clients(clients, client, actual, buffer,
                // 0);
              }
              break;
            }
            case CHAT_ACCEPTED: {
              int j;
              for (j = 0; j < actual; j++) {
                if (strcmp(clients[j].name, &buffer[1]) == 0) {
                  handle_chat_accepted(&clients[i], &clients[j]);
                }
              }
              break;
            }
            case CHAT_REFUSED: {
              printf("Chat with %s refused\n", &buffer[1]);
              int j;
              for (j = 0; j < actual; j++) {
                if (strcmp(clients[j].name, &buffer[1]) == 0) {
                  handle_chat_refused(&clients[i], &clients[j]);
                }
              }
              break;
            }
            case CHAT_MESSAGE: {
              handle_chat_message(&clients[i], &buffer[1]);
              break;
            }
              // send_message_to_all_clients(clients, client, actual, buffer,
              // 0);
            }
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

static void update_clients(Client *clients, int to_remove, int *actual) {
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

static void write_game(Client *client, int moveResult, int move) {
  renderGame(client->game);
  char *buffer = malloc((3 + PLAYERS + BOARD_SIZE) * sizeof(char));
  switch (moveResult) {
  case -1: {
    buffer[0] = GAME_DATA;
    break;
  }
  case 0: {
    buffer[0] = MOVE_FAIL;
    break;
  }
  case 1: {
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
  buffer[2 + PLAYERS + BOARD_SIZE] = move;

  write_client(client->sock, buffer, 3 + PLAYERS + BOARD_SIZE);

  free(buffer);
}

// Message handling
static void handle_challenge_request(char *client_name, Client *sender,
                                     Client *clients, int client_number) {
  int j;
  for (j = 0; j < client_number; j++) {
    if (strcmp(clients[j].name, client_name) == 0) {
      char *request = (char *)malloc(MAX_USERNAME_SIZE + 1);
      request[0] = CONFIRM_CHALLENGE;
      memcpy(request + 1, sender->name, strlen(sender->name));
      request[1 + strlen(sender->name)] = 0;
      write_string(clients[j].sock, request);
      free(request);
      break;
    }
  }
  if (j == client_number) {
    char request[] = {PLAYER_NOT_FOUND};
    write_client(sender->sock, request, 1);
  }
}

static void handle_challenge_accepted(Client *challenger, Client *challengee) {
  if (challengee->opponent != NULL) {
    char request[] = {GAME_FORFEIT};
    write_client(challengee->opponent->sock, request, 1);
  }
  char *request = (char *)malloc(MAX_USERNAME_SIZE + 1);
  request[0] = CHALLENGE_ACCEPTED;
  memcpy(request + 1, challengee->name, strlen(challengee->name));
  request[1 + strlen(challengee->name)] = 0;
  write_string(challenger->sock, request);
  GameState *game = newGame();
  challengee->game = challenger->game = game;
  int turn = rand() % 2;
  challengee->turn = turn;
  challenger->turn = 1 - turn;
  // Write the game to both players
  write_game(challenger, -1, -1);
  write_game(challengee, -1, -1);
  challengee->opponent = challenger;
  challenger->opponent = challengee;
  challengee->chat = challenger;
  challenger->chat = challengee;
  free(request);
}

static void handle_challenge_refused(Client *challenger, Client *challengee) {
  char *request = (char *)malloc(MAX_USERNAME_SIZE + 1);
  request[0] = CHALLENGE_REFUSED;
  memcpy(request + 1, challengee->name, strlen(challengee->name));
  request[1 + strlen(challengee->name)] = 0;
  write_string(challenger->sock, request);
  free(request);
}

static void handle_move_data(Client *player, short move) {
  int moveResult = makeMove(move, player->game);
  write_game(player, moveResult, move);
  if (moveResult) {
    write_game(player->opponent, moveResult, move);
  }
  if (hasEnded(player->game)) {
    char end_game_payload[] = {END_GAME};
    write_client(player->sock, end_game_payload, 1);
    write_client(player->opponent->sock, end_game_payload, 1);
    free(player->game);
    player->game = player->opponent->game = NULL;
    player->opponent->opponent = NULL;
    player->opponent = NULL;
  }
}

static void handle_list_online_players(Client *clients, int client_number,
                                       Client *requestor) {
  int total_len = 0;
  for (int j = 0; j < client_number; j++) {
    total_len += strlen(clients[j].name);
  }
  int packet_size = 2 + client_number + total_len;
  char *request = (char *)malloc(packet_size);
  request[0] = ONLINE_PLAYERS_RESPONSE;
  request[1] = (char)client_number;
  int pos = 2;
  for (int j = 0; j < client_number; j++) {
    memcpy(&request[pos], clients[j].name, strlen(clients[j].name));
    pos += strlen(clients[j].name);
    request[pos++] = 0;
  }
  write_client(requestor->sock, request, packet_size);
  free(request);
}

static void handle_chat_request(Client *sender, Client *receiver) {
  char *request = (char *)malloc(MAX_USERNAME_SIZE + 1);
  request[0] = CONFIRM_CHAT;
  memcpy(request + 1, sender->name, strlen(sender->name));
  request[1 + strlen(sender->name)] = 0;
  write_string(receiver->sock, request);
  free(request);
}

static void handle_chat_accepted(Client *sender, Client *receiver) {
  char *request = (char *)malloc(MAX_USERNAME_SIZE + 1);
  request[0] = CHAT_ACCEPTED;
  memcpy(request + 1, sender->name, strlen(sender->name));
  request[1 + strlen(sender->name)] = 0;
  write_string(receiver->sock, request);
  sender->chat = receiver;
  receiver->chat = sender;
  free(request);
}

static void handle_chat_refused(Client *sender, Client *receiver) {
  char *request = (char *)malloc(MAX_USERNAME_SIZE + 1);
  request[0] = CHAT_REFUSED;
  memcpy(request + 1, sender->name, strlen(sender->name));
  request[1 + strlen(sender->name)] = 0;
  write_string(receiver->sock, request);
  sender->chat = receiver;
  receiver->chat = sender;
  free(request);
}

static void handle_chat_message(Client *sender, char *message) {
  if (strcmp("exit", message) == 0) {
    char response[] = {END_CHAT};
    write_string(sender->chat->sock, response);
    write_string(sender->sock, response);
    return;
  }
  char *response = (char *)malloc(4 + strlen(sender->name) + strlen(message));
  response[0] = CHAT_MESSAGE;
  memcpy(response + 1, sender->name, strlen(sender->name));
  int pos = 1 + strlen(sender->name);
  response[pos++] = ':';
  response[pos++] = ' ';
  memcpy(&response[pos], message, strlen(message));
  pos += strlen(message);
  response[pos] = 0;
  write_string(sender->chat->sock, response);
  free(response);
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
