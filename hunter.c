// hunter.c
// Implementation of your "Fury of Dracula" hunter AI

#include <stdlib.h>
#include <stdio.h>
#include "Game.h"
#include "HunterView.h"

void decideHunterMove(HunterView gameState)
{

	LocationID trail[TRAIL_SIZE];
    int numLoc = 0;
	int *numLocations = &numLoc;
	player = whoAmI(gameState)
	LocationID *moveList = whereCanIgo(gameState, numLocations, TRUE, TRUE, TRUE);
    if (numLoc != 0) {
      // At the moment, just found out where I am, valid moves and pick a random one.
	   //int dracLocID = whereIs(gameState, PLAYER_DRACULA);
	   
	   giveMeTheTrail(gameState, player, trail);
	   // Compare trail and possible moves, removing those that appear int the trail
	   int i, j;
	   for (i = 0; i < TRAIL_SIZE; i++) {
	      for (j = 0; j < *numLocations; j++) {
	         if (trail[i] == moveList[j] || moveList[j] == nameToID("ST_JOSEPH_AND_ST_MARYS")) {
	            moveList[j] = NOWHERE;
	         }
	      }
	   }
    }
    int j;
    for (j = 0; j < *numLocations; j++) {
       if (moveList[j] != NOWHERE) {
         nextMove = moveList[j];
       }
    }
    
    registerBestPlay(IDToAbbrev(nextMove),"The trill of the hunt!!!");
}
