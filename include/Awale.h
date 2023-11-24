#ifndef AWALE_H
#define AWALE_H

#include <stdbool.h>

#define PLAYERS 2
#define BOARD_SIZE 12
#define INITIAL_SEEDS 6
#define WINNING_SCORE 24
#define ROW_COUNT 2
#define COLUMN_COUNT 6

typedef struct {
  int turn;
  int scores[PLAYERS];
  int board[BOARD_SIZE];
} GameState;

GameState *newGame();

void renderGame(GameState *game);

bool makeMove(int n, GameState *game);

bool hasEnded(GameState *game);

#endif