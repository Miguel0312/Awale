#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void challenge_request(SOCKET socket, const char *buf) {
  char request[MAX_USERNAME_SIZE + 1];
  request[0] = CHALLENGE_REQUEST;
  memcpy(request + 1, buf, strlen(buf));
  request[1 + strlen(buf)] = 0;
  write_server(socket, request);
}

// TODO: end game state in both the server and client
static void chat_request(SOCKET socket, const char *buf) {
  char request[MAX_USERNAME_SIZE + 1];
  request[0] = CHAT_REQUEST;
  memcpy(request + 1, buf, strlen(buf));
  request[1 + strlen(buf)] = 0;
  write_server(socket, request);
}

static void show_start_menu() {
  printf("Select an option:\n");
  printf("1 - Challenge a player.\n");
  printf("2 - List online players.\n");
  printf("3 - Start chat.\n");
  printf("4 - Watch replay\n");
  printf("5 - Watch a live game\n");
}

static void appClient(const char *address, const char *name) {
  SOCKET sock = init_connection_client(address);
  char buffer[BUF_SIZE];
  GameState *game = newGame();
  int status = MENU_NOT_SHOWN;

  fd_set rdfs;

  /* send our name */
  write_server(sock, name);

  while (1) {
    if (status == MENU_NOT_SHOWN) {
      show_start_menu();
      status = MENU_SHOWN;
    }
    FD_ZERO(&rdfs);

    /* add STDIN_FILENO */
    FD_SET(STDIN_FILENO, &rdfs);

    /* add the socket */
    FD_SET(sock, &rdfs);

    if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1) {
      perror("select()");
      exit(errno);
    }

    /* something from standard input : i.e keyboard */
    if (FD_ISSET(STDIN_FILENO, &rdfs)) {
      if (fgets(buffer, BUF_SIZE - 1, stdin) == NULL || buffer[0] == '\n') {
        continue;
      }
      char *p = NULL;
      p = strstr(buffer, "\n");
      if (p != NULL) {
        *p = 0;
      } else {
        /* fclean */
        buffer[BUF_SIZE - 1] = 0;
      }
      if (status == MENU_SHOWN) {
        char option = buffer[0];
        char buf[BUF_SIZE];
        switch (option) {
        case '1': {
          printf("Enter the player name: ");
          scanf("%s", buf);
          // printf("%s", buf);
          challenge_request(sock, buf);
          break;
        }

        case '2': {
          char buf[] = {LIST_ONLINE_PLAYERS, 0};
          write_server(sock, buf);
          break;
        }
        case '3': {
          printf("Enter the player to contact:");
          char buf[MAX_USERNAME_SIZE];
          scanf("%s", buf);
          chat_request(sock, buf);
        }
        case '4': {
          printf("Enter name of the replay file: ");
          scanf("%s", buf);
          getchar();
          replayGame(buf);
          status = MENU_NOT_SHOWN;
          break;
        }
        case '5': {
          printf("Enter the name of the player that you want to watch.");
          char buf[1 + MAX_USERNAME_SIZE];
          buf[0] = OBSERVER_REQUEST;
          scanf("%s", &buf[1]);
          write_server(sock, buf);
          break;
        }
        }
      } else if ((status == PLAYER_TURN || status == PLAYER_WAIT) &&
                 atoi(buffer) == 0) {
        send_chat_message(sock, buffer);
      } else if (status == PLAYER_TURN) {
        buffer[1] = atoi(buffer) - 1;
        if (buffer[1] < 0 || buffer[1] >= 12) {
          printf("Invalid value %d. Try again.\n", (int)buffer[1] + 1);
          continue;
        }
        buffer[0] = MOVE_DATA;
        write_server(sock, buffer);
      } else if (status == PLAYER_WAIT) {
        printf("It is not your turn. You must wait.\n");
        continue;
      } else if (status == CHAT_MODE) {
        send_chat_message(sock, buffer);
      }

    } else if (FD_ISSET(sock, &rdfs)) {
      int n = read_server(sock, buffer);
      char request_type = buffer[0];
      switch (request_type) {
      case CONFIRM_CHALLENGE: {
        char *challenger = &buffer[1];
        status = handle_confirm_challenge(sock, challenger);
        break;
      }
      case CHALLENGE_ACCEPTED: {
        char *challengee = &buffer[1];
        printf("Challenge to %s accepted\n", challengee);
        status = PLAYER_TURN;
        break;
      }
      case MOVE_FAIL: {
        printf("Invalid move\n");
        status = handle_game_data(game, buffer, 0);
        break;
      }
      case MOVE_SUCCESS: {
        int move = buffer[2 + PLAYERS + BOARD_SIZE];
        handle_move_success(game, move);
        status = handle_game_data(game, buffer, 0);
        break;
      }
      case GAME_DATA: {
        status = handle_game_data(game, buffer, 1);
        break;
      }
      case CHALLENGE_REFUSED: {
        char *challengee = &buffer[1];
        printf("Challenge to %s refused\n", challengee);
        status = MENU_NOT_SHOWN;
        break;
      }
      case OPPONENT_DISCONNECTED: {
        if (status == OBSERVING) {
          printf("A player has disconnected\n");
        } else {
          game->scores[0] = 100;
          printf("Your opponent disconnected\n");
        }
      }
      case END_GAME: {
        if (status == OBSERVING) {
          printf("The game has ended\n");
          status = MENU_NOT_SHOWN;
        } else {
          status = handle_end_game(game);
        }
        break;
      }
      case ONLINE_PLAYERS_RESPONSE: {
        status = handle_online_players_response(buffer);
        break;
      }
      case CONFIRM_CHAT: {
        handle_confirm_chat(sock, &buffer[1]);
        break;
      }
      case CHAT_ACCEPTED: {
        printf("Chat accepted.\n");
        status = CHAT_MODE;
        break;
      }
      case CHAT_REFUSED: {
        printf("Chat refused.\n");
        status = MENU_NOT_SHOWN;
        break;
      }
      case CHAT_MESSAGE: {
        printf("%s\n", &buffer[1]);
        break;
      }
      case END_CHAT: {
        printf("The chat ended.\n");
        status = MENU_NOT_SHOWN;
      }
      case PLAYER_NOT_FOUND: {
        printf("The player isn't connected.\n");
        status = MENU_NOT_SHOWN;
        break;
      }
      case OBSERVER_SUCCESS: {
        status = OBSERVING;
        break;
      }
      case OBSERVER_FAIL: {
        printf("This player is not currently in a game\n");
        status = MENU_NOT_SHOWN;
        break;
      }
      case OBSERVER_GAME_DATA: {
        observer_handle_game_data(game, buffer);
        break;
      }
      }
      /* server down */
      if (n == 0) {
        printf("Server disconnected !\n");
        break;
      }
    }
  }

  end_connection(sock);
}

static int init_connection_client(const char *address) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  SOCKADDR_IN sin = {0};
  struct hostent *hostinfo;

  if (sock == INVALID_SOCKET) {
    perror("socket()");
    exit(errno);
  }

  hostinfo = gethostbyname(address);
  if (hostinfo == NULL) {
    fprintf(stderr, "Unknown host %s.\n", address);
    exit(EXIT_FAILURE);
  }

  sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
  sin.sin_port = htons(PORT);
  sin.sin_family = AF_INET;

  if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR) {
    perror("connect()");
    exit(errno);
  }

  return sock;
}

static void end_connection(int sock) { closesocket(sock); }

static int read_server(SOCKET sock, char *buffer) {
  int n = 0;

  if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
    perror("recv()");
    exit(errno);
  }

  buffer[n] = 0;

  return n;
}

static void write_server(SOCKET sock, const char *buffer) {
  if (send(sock, buffer, strlen(buffer), 0) < 0) {
    perror("send()");
    exit(errno);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage : %s [address] [pseudo]\n", argv[0]);
    return EXIT_FAILURE;
  }

  init();

  appClient(argv[1], argv[2]);

  end();

  return EXIT_SUCCESS;
}

// Message handling
static void send_chat_message(SOCKET sock, const char *message) {
  char *request = (char *)malloc(2 + strlen(message));
  request[0] = CHAT_MESSAGE;
  memcpy(&request[1], message, strlen(message));
  request[1 + strlen(message)] = 0;
  write_server(sock, request);
  free(request);
}

static int handle_confirm_challenge(SOCKET sock, const char *challenger) {
  printf("You have an incoming challenge from %s. Do you want to accept?",
         challenger);
  char answer;
  scanf("%c", &answer);
  if (answer == 'y') {
    char *response = (char *)malloc(1 + MAX_USERNAME_SIZE);
    response[0] = CHALLENGE_ACCEPTED;
    memcpy(response + 1, challenger, strlen(challenger));
    response[1 + strlen(challenger)] = 0;
    write_server(sock, response);
    free(response);
    return PLAYER_TURN;
  } else {
    char *response = (char *)malloc(1 + MAX_USERNAME_SIZE);
    response[0] = CHALLENGE_REFUSED;
    memcpy(response + 1, challenger, strlen(challenger));
    response[1 + strlen(challenger)] = 0;
    write_server(sock, response);
    free(response);
    return MENU_NOT_SHOWN;
  }
}

static void handle_move_success(GameState *game, int move) {
  if (game->replay.root == NULL) {
    game->replay.root = game->replay.cur = newList(move);
  } else {
    game->replay.cur = addNext(game->replay.cur, move);
  }
}

static int handle_game_data(GameState *game, char *buffer, int isGameStart) {
  game->turn = buffer[1];
  for (int i = 0; i < PLAYERS; i++) {
    game->scores[i] = buffer[2 + i];
  }
  for (int i = 0; i < BOARD_SIZE; i++) {
    game->board[i] = buffer[2 + PLAYERS + i];
  }
  renderGame(game);
  if (game->turn == 0) {
    if (isGameStart) {
      printf("You play first. Your squares are on the top row. "
             "Enter a number from 1-6.\n");
    }
    printf("It is your turn.\n");
    return PLAYER_TURN;
  } else {
    if (isGameStart) {
      printf("You play second. Your squares are on the bottom row. "
             "Enter a number from 7-12.\n");
    }
    printf("Wait for your opponent to play.\n");
    return PLAYER_WAIT;
  }
}

static int handle_end_game(GameState *game) {
  char buffer[BUF_SIZE];
  if (game->scores[0] > game->scores[1]) {
    printf("Congratulations! You won!\n");
  } else if (game->scores[0] < game->scores[1]) {
    printf("You lost!\n");
  } else {
    printf("It is a tie!\n");
  }
  printf("Do you want to save the replay?(y/n) ");
  char ans;
  getchar();
  scanf("%c", &ans);
  printf("%c\n", ans);
  if (tolower(ans) == 'y') {
    printf("Enter the name of the file to save the game: ");
    scanf("%s", buffer);
    saveGame(buffer, game);
  }
  free(game);
  game = NULL;
  return MENU_NOT_SHOWN;
}

static int handle_online_players_response(char *buffer) {
  int client_number = buffer[1];
  int pos = 2;
  printf("Online players\n");
  for (int i = 0; i < client_number; i++) {
    printf("%s\n", &buffer[pos]);
    pos += strlen(&buffer[pos]) + 1;
  }
  return MENU_NOT_SHOWN;
}

static int handle_confirm_chat(SOCKET sock, char *name) {
  printf("Player %s want to chat to you. Do you accept(y/n)?", name);
  char accept;
  scanf("%c", &accept);
  if (accept == 'y') {
    char *response = (char *)malloc(1 + MAX_USERNAME_SIZE);
    response[0] = CHAT_ACCEPTED;
    memcpy(&response[1], name, strlen(name));
    response[1 + strlen(name)] = 0;
    write_server(sock, response);
    return CHAT_MODE;
    free(response);
  } else {
    char *response = (char *)malloc(1 + MAX_USERNAME_SIZE);
    response[0] = CHAT_REFUSED;
    memcpy(&response[1], name, strlen(name));
    response[1 + strlen(name)] = 0;
    write_server(sock, response);
    return MENU_NOT_SHOWN;
    free(response);
  }
}

static void observer_handle_game_data(GameState *game, char *buffer) {
  for (int i = 0; i < PLAYERS; i++) {
    game->scores[i] = buffer[1 + i];
  }
  for (int i = 0; i < BOARD_SIZE; i++) {
    game->board[i] = buffer[1 + PLAYERS + i];
  }
  renderGame(game);
}
