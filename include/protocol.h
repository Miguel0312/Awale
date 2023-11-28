#ifndef PROTOCOL_H
#define PROTOCOL_H

#define CHALLENGE_REQUEST 1
#define LIST_ONLINE_PLAYERS 2
#define CONFIRM_CHALLENGE 3

#define ONLINE_PLAYERS_RESPONSE 5

#define CHALLENGE_ACCEPTED 7
#define PLAYER_NOT_FOUND 8
#define PLAYER_NOT_AVAILABLE 9

// TODO: Add success move and failed move codes
#define GAME_DATA 10
#define MOVE_DATA 11
#define MOVE_SUCCESS 12
#define MOVE_FAIL 13

#endif
