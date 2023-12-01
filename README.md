# Awale

This project provides a server and client to play the game Awale.

# Client

The client connects by providing the ip address of the server and a username. If the username is not already taken, the connection succeeds.

The client then has a few options.
- Challenge a player: he gives a username, and if the other person accepts, a game is started.
- List online players: the client receives a list from the server containing what players are online
- Start chat: the client sends an offer to another user to talk to them. Once connection is established, they can type messsages to send to one another. If one of them sends the message "exit", both users are disconnected from the chat.
- Watch replay: the user inserts the name of a file containing a replay. He can then see the board after each move.
- Observe a match: the user inserts the name of another client, and if they are in a game, the status of the game is sent after each move.


# Gameplay loop

When the game starts, each player receives instruction on the side of the board that they are playing (top or bottom), who has the first move and the range of allowed values. Then, they alternates moves until one winner is declared.

During the game, if the player insert a line that do not start with a number, they send it as a chat message to their opponent.

At the end of the game, the player has the option to save the game in a text file.
