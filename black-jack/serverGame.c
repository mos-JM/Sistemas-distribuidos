#include "serverGame.h"
#include <pthread.h>

void showError(const char *msg){
	perror(msg);
	exit(0);
}



void *threadProcessing(void *threadArgs){

	tSession session;				/** Session of this game */
	int socketPlayer1;				/** Socket descriptor for player 1 */
	int socketPlayer2;				/** Socket descriptor for player 2 */
	tPlayer currentPlayer;			/** Current player */
	int endOfGame;					/** Flag to control the end of the game*/
	unsigned int card;				/** Current card */
	unsigned int code, codeRival;	/** Codes for the active player and the rival */

		// Get sockets for players
		socketPlayer1 = ((tThreadArgs *) threadArgs)->socketPlayer1;
		socketPlayer2 = ((tThreadArgs *) threadArgs)->socketPlayer2;

		// Receive player 1 info
		

		// Receive player 2 info
	

		// Init...
		endOfGame = FALSE;

		while (!endOfGame){

			
		}

		// Close sockets
		close (socketPlayer1);
		close (socketPlayer2);

	return (NULL) ;
}

int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	struct sockaddr_in serverAddress;	/** Server address structure */
	unsigned int port;					/** Listening port */
	struct sockaddr_in player1Address;	/** Client address structure for player 1 */
	struct sockaddr_in player2Address;	/** Client address structure for player 2 */
	int socketPlayer1;					/** Socket descriptor for player 1 */
	int socketPlayer2;					/** Socket descriptor for player 2 */
	unsigned int clientLength;			/** Length of client structure */
	tThreadArgs *threadArgs; 			/** Thread parameters */
	pthread_t threadID;					/** Thread ID */


		// Seed
		srand(time(0));

		// Check arguments
		if (argc != 2) {
			fprintf(stderr,"ERROR wrong number of arguments\n");
			fprintf(stderr,"Usage:\n$>%s port\n", argv[0]);
			exit(1);
		}

		// Create the socket
		
		
		// Init server structure
		memset(&serverAddress, 0, sizeof(serverAddress));

		// Get listening port
		port = atoi(argv[1]);

		// Fill server structure
		

		// Bind
		

		// Listen
		

		// Infinite loop
		while (1){
		
		}
}
