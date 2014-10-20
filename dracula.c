// dracula.c
// Implementation of your "Fury of Dracula" Dracula AI

#include <stdlib.h>
#include <stdio.h>
#include "Game.h"
#include "DracView.h"

void decideDraculaMove(DracView gameState) {
   LocationID nextMove = nameToID("CASTLE_DRACULA");
   LocationID trail[TRAIL_SIZE];
   int numLoc = 0;
   int *numLocations = &numLoc;
   LocationID *moveList = whereCanIgo(gameState, numLocations, TRUE, TRUE);
   if (numLoc != 0) {
       // At the moment, just found out where I am, valid moves and pick a random one.
	   //int dracLocID = whereIs(gameState, PLAYER_DRACULA);
	   
	   giveMeTheTrail(gameState, PLAYER_DRACULA, trail);
	   // Compare trail and possible moves, removing those that appear int the trail
	   int i, j;
	   for (i = 0; i < TRAIL_SIZE; i++) {
	      for (j = 0; j < *numLocations; j++) {
	         if (trail[i] == moveList[j]) {
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
   registerBestPlay(IDToAbbrev(nextMove),"We like pink fluffy unicorns!");
}
