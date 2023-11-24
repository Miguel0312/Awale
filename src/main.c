#include <stdio.h>

#include "../include/Awale.h"

int main() {
  GameState* game = newGame();
  
  while(!hasEnded(game)) {
    renderGame(game);
    int move;
    printf("Player %d's turn\nEnter your move: ", game->turn);
    scanf("%d", &move);
    if(!makeMove(move, game)) {
      printf("Invalid move, try again\n");
    }
  }

  return 0;
}