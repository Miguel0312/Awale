#ifndef AWALE_H
#define AWALE_H

#include <stdbool.h>
#include "LinkedList.h"

#define PLAYERS 2
#define BOARD_SIZE 12
#define INITIAL_SEEDS 4
#define WINNING_SCORE 24
#define ROW_COUNT 2
#define COLUMN_COUNT 6

typedef struct {
  Node* root;
  Node* cur;
} Replay;

typedef struct {
  int turn;
  int scores[PLAYERS];
  int board[BOARD_SIZE];
  Replay replay;
} GameState;

GameState *newGame();

void renderGame(const GameState *game);

int makeMove(int n, GameState *game);

bool hasEnded(GameState *game);

void saveGame(char* fileName, GameState* game);

void replayGame(char* fileName);

#endif