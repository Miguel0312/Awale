#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "../include/Awale.h"

GameState* newGame() {
  GameState* game = malloc(sizeof(GameState));
  game->turn = 0;
  game->scores[0] = game->scores[1] = 0;
  for(int i = 0; i < BOARD_SIZE; i++) {
    game->board[i] = INITIAL_SEEDS;
  }

  game->replay.root = NULL;
  game->replay.cur = NULL;

  return game;
}

void renderGame(const GameState* game) {
  for(int i = 0; i < ROW_COUNT; i++) {
    printf("|");
    for(int j = 0; j < COLUMN_COUNT; j++) {
      printf(" %-2d|", game->board[6*i+j]);
    }
    printf("\n");
  }
  printf("\n");

  for(int i = 0; i < PLAYERS; i++) {
    printf("Score of Player %d: %d\n", i+1, game->scores[i]);
  }
}

int next(int index) {
  if(index == 0 || index == BOARD_SIZE - 1) {
    return (index + COLUMN_COUNT)%BOARD_SIZE;
  } else if(index >= COLUMN_COUNT) {
    return index + 1;
  } else {
    return index - 1;
  }
}

int prev(int index) {
  if(index == COLUMN_COUNT - 1 || index == COLUMN_COUNT) {
    return (index + COLUMN_COUNT)%BOARD_SIZE;
  } else if(index >= 6) {
    return index - 1;
  } else {
    return index + 1;
  }
}

int makeMove(int n, GameState* game) {
  if(!game->board[n] ||
    (n < game->turn * COLUMN_COUNT || n >= (game->turn + 1) * COLUMN_COUNT)) {
    return 0;
  }

  int cnt = game->board[n];
  int cur = n;
  game->board[n] = 0;
  while(cnt--) {
    cur = next(cur);
    if(cur == n) {
      cur = next(cur);  
    }
    game->board[cur] += 1;
  }

  int grandSlam = false;
  if((cur == 0 && game->turn == 1) || (cur == 11 && game->turn == 0)) {
    grandSlam = true;
    for(int i = COLUMN_COUNT * (1 - game->turn); i < COLUMN_COUNT * (2 - game->turn); i++){
      if(game->board[i] != 2 && game->board[i] != 3) {
        grandSlam = false;
        break;
      }
    }
  }
  
  if(!grandSlam) {
    while(cur/6 != n/6 && (game->board[cur] == 2 || game->board[cur] == 3)) {
      game->scores[game->turn] += game->board[cur];
      game->board[cur] = 0;
      cur = prev(cur);
    }
  }

  int sum0 = 0, sum1 = 0;
  for(int i = 0; i < COLUMN_COUNT; i++) {
    sum0 += game->board[i];
    sum1 += game->board[COLUMN_COUNT + i];
  }

  if(sum0 == 0 || sum1 == 0) {
    game->scores[0] += sum0;
    game->scores[1] += sum1;
    for(int i = 0; i < BOARD_SIZE; i++) {
      game->board[i] = 0;
    }
  }

  game->turn = 1 - game->turn;

  if(game->replay.root == NULL) {
    game->replay.root = game->replay.cur = newList(n);
  } else {
    game->replay.cur = addNext(game->replay.cur, n);
  }

  return 1;
}

bool hasEnded(GameState* game) {
  return game->scores[0] > WINNING_SCORE || game->scores[1] > WINNING_SCORE || 
    (game->scores[0] + game->scores[1] == 2 * WINNING_SCORE);
}

void saveGame(char *fileName, GameState *game) {
  FILE* file = fopen(fileName, "w");

  Node* cur = game->replay.root;
  while(cur) {
    fprintf(file, "%d ", cur->val);
    cur = cur->next;
  }

  fclose(file);
}

void replayGame(char *fileName) {
  FILE* file = fopen(fileName, "r");

  GameState* game = newGame();
  renderGame(game);
  printf("Press 'Enter' to continue");
  getchar();
  int move;
  while(fscanf(file, "%d ", &move) == 1) {
    makeMove(move, game);
    renderGame(game);
    printf("Press 'Enter' to continue");
    getchar();
  }

  fclose(file);
}