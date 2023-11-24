#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "protocol.h"
#include "server.h"

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

static void show_start_menu() {
  printf("Select an option:\n");
  printf("1 - Challenge a player.\n");
  printf("2 - List online players.\n");
}

static void appClient(const char *address, const char *name) {
  SOCKET sock = init_connection_client(address);
  char buffer[BUF_SIZE];

  fd_set rdfs;

  /* send our name */
  write_server(sock, name);

  show_start_menu();
  while (1) {
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
      fgets(buffer, BUF_SIZE - 1, stdin);
      {
        char *p = NULL;
        p = strstr(buffer, "\n");
        if (p != NULL) {
          *p = 0;
        } else {
          /* fclean */
          buffer[BUF_SIZE - 1] = 0;
        }
      }
      char option = buffer[0];
      switch (option) {
      case '1': {
        printf("Enter the player name: ");
        char buf[MAX_USERNAME_SIZE];
        scanf("%s", buf);
        challenge_request(sock, buf);
        break;
      }
      }
    } else if (FD_ISSET(sock, &rdfs)) {
      int n = read_server(sock, buffer);
      char request_type = buffer[0];
      printf("%d\n", (int)request_type);
      switch (request_type) {
      case CONFIRM_CHALLENGE: {
        char *challenger = &buffer[1];
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
        }
        break;
      }
      case CHALLENGE_ACCEPTED: {
        char *challengee = &buffer[1];
        printf("Challenge to %s accepted\n", challengee);
      }
      }
      /* server down */
      if (n == 0) {
        printf("Server disconnected !\n");
        break;
      }
      puts(buffer);
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
