// hunter.c
// Implementation of your "Fury of Dracula" hunter AI

#include <stdlib.h>
#include <stdio.h>
#include "Game.h"
#include "HunterView.h"

void decideHunterMove(HunterView gameState) {
    PlayerID player = whoAmI(gameState);
    LocationID nextMove = whereIs(gameState, player);
	LocationID trail[TRAIL_SIZE];
    int numLoc = 0;
	int *numLocations = &numLoc;
	LocationID *moveList = whereCanIgo(gameState, numLocations, TRUE, TRUE, TRUE);
    if (numLoc != 0 && howHealthyIs(gameState, player) > 3) {
	   giveMeTheTrail(gameState, player, trail);
	   // Compare trail and possible moves, removing those that appear int the trail
       int j;
       for (j = 0; j < *numLocations; j++) {
          if (moveList[j] != NOWHERE) {
             nextMove = moveList[j];
          }
       }
    }
    registerBestPlay(IDToAbbrev(nextMove),"The trill of the hunt!!!");
}
