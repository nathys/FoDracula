// dracula.c
// Implementation of your "Fury of Dracula" Dracula AI

#include <stdlib.h>
#include <stdio.h>
#include "Game.h"
#include "DracView.h"

void decideDraculaMove(DracView gameState) {
	LocationID nextMove = abbrevToID("CD");
   LocationID trail[TRAIL_SIZE];
	int *numLocations;
	
   if (numLocations != 0) {
      // At the moment, just found out where I am, valid moves and pick a random one.
	   int dracLocID = whereIs(gameState, PLAYER_DRACULA);
	   LocationID *moveList = whereCanIGo(gameState, numLocations, TRUE, TRUE);
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
   for (j = 0; j < *numLocations; j++) {
      if (moveList[j] != NOWHERE) {
         nextMove = moveList[j];
      }
   }
	registerBestPlay(IDToAbbrev(nextMove),"We like pink fluffy unicorns!");
}
