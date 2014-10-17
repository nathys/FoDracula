// HunterView.c ... HunterView ADT implementation

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "Globals.h"
#include "Game.h"
#include "GameView.h"
#include "HunterView.h"
// #include "Map.h" ... if you decide to use the Map ADT
 
// pastPlays string
// the number characters used to describe each play
#define CHARS_PER_PLAY 7

// including the space
#define CHARS_PER_PLAY_BLOCK (CHARS_PER_PLAY+1)

// The maximum number of plays we will accept
// Since each dracula turn reduces the score by one, we will set it to
// at most 366 dracula's turns
// ... plus a bit extra because josh is ultra-conservative
#define MAX_PLAYS (366*5+5+10)

// maximum length of past plays string
#define MAX_PAST_PLAYS_LENGTH (MAX_PLAYS * CHARS_PER_PLAY_BLOCK)

// most recent location in trail
#define LAST_TRAIL_LOC_INDEX 0

// index that the location abbreviation starts in within the char *pastPlays
#define LOC_ABBREV_INDEX 1

// id of the first round
#define FIRST_ROUND 0

struct hunterView {
    GameView g;
    
    // Dracula's last TRAIL_SIZE _locations_; NOT moves in REVERSE
    // chronological order [as best as we know]
    LocationID trailLocs[TRAIL_SIZE];
};
     
// helper functions
static void pushOnTrailLocs (HunterView d, LocationID placeID);

// Creates a new HunterView to summarise the current state of the game
HunterView newHunterView(char *pastPlays, PlayerMessage messages[])
{
    assert(pastPlays != NULL);
    assert(messages != NULL);

    // malloc
    HunterView hunterView = malloc(sizeof(struct hunterView));

    assert(hunterView != NULL);

    // setup the gameview
    hunterView->g = newGameView(pastPlays, messages);

    assert(hunterView->g != NULL);

    int i;

    // initialise Dracula's actual locations to all UNKNOWN_LOCATION
    for(i=0;i<TRAIL_SIZE;i++) {
        hunterView->trailLocs[i] = UNKNOWN_LOCATION;
    }

    // length of pastPlays
    int pastPlaysLength = strnlen(pastPlays, MAX_PAST_PLAYS_LENGTH); 

    // iterate over each turn and process
    for(i=0;i<pastPlaysLength;i+=CHARS_PER_PLAY_BLOCK) {
        // get curPlayer
        PlayerID curPlayer = ( (i/CHARS_PER_PLAY_BLOCK) % NUM_PLAYERS);

        // ensure it's dracula
        if(curPlayer == PLAYER_DRACULA) {
            // try to get current loc (if it's exact)
            LocationID curLoc = abbrevToID(pastPlays+i+LOC_ABBREV_INDEX);

//            fprintf(stderr,"here = %.7s curLoc = %d\n",pastPlays+i,curLoc);

            // test if exact location
            if(!validPlace(curLoc)) {
                // not exact; try to use trail to work out where we really are

                // either a HIDE, DOUBLE_BACK_ or TELEPORT
                if(pastPlays[i+LOC_ABBREV_INDEX] == 'H') {
                    // HIDE: go to most recent location
                    curLoc = hunterView->trailLocs[LAST_TRAIL_LOC_INDEX];
                } else if(pastPlays[i+LOC_ABBREV_INDEX] == 'D') {
                    // DOUBLE BACK
                    int numBack = (int)(pastPlays[i+LOC_ABBREV_INDEX+1]-'0');
                    curLoc = hunterView->trailLocs[numBack];
                } else if(pastPlays[i+LOC_ABBREV_INDEX] == 'T') {
                    // TELEPORT back to CASTLE_DRACULA
                    curLoc = CASTLE_DRACULA;
                } else if(pastPlays[i+LOC_ABBREV_INDEX] == 'C') {
                    curLoc = CITY_UNKNOWN;
                } else if(pastPlays[i+LOC_ABBREV_INDEX] == 'S') {
                    curLoc = SEA_UNKNOWN;
                } else {
                    curLoc = UNKNOWN_LOCATION;
                }
            }

            // push curLoc onto the trail
            pushOnTrailLocs(hunterView, curLoc);
        }
    }

    return hunterView;
}
 
     
// Frees all memory previously allocated for the HunterView toBeDeleted
void disposeHunterView(HunterView toBeDeleted)
{
    assert(toBeDeleted != NULL);

    // drop the gameview
    disposeGameView(toBeDeleted->g);

    // free ourselves
    free( toBeDeleted );
}


//// Functions to return simple information about the current state of the game

// Get the current round
Round giveMeTheRound(HunterView currentView)
{
    assert(currentView != NULL);
    return getRound(currentView->g);
}

// Get the id of current player
PlayerID whoAmI(HunterView currentView)
{
    assert(currentView != NULL);
    return getCurrentPlayer(currentView->g);
}

// Get the current score
int giveMeTheScore(HunterView currentView)
{
    assert(currentView != NULL);
    return getScore(currentView->g);
}

// Get the current health points for a given player
int howHealthyIs(HunterView currentView, PlayerID player)
{
    assert(currentView != NULL);
    assert(0 <= player && player < NUM_PLAYERS);

    return getHealth(currentView->g, player);
}

// Get the current location id of a given player
LocationID whereIs(HunterView currentView, PlayerID player)
{
    assert(currentView != NULL);
    assert(0 <= player && player < NUM_PLAYERS);

    // return value
    LocationID ret;

    // check if dracula
    if(player == PLAYER_DRACULA) {
        // use our trail thing mwahahahaha [if it's not unnown]
        ret = currentView->trailLocs[LAST_TRAIL_LOC_INDEX];

        if(ret == UNKNOWN_LOCATION) {
            // ok we'll use the adt
            ret = getLocation(currentView->g, player);
        }
    } else {
        // use adt
        ret = getLocation(currentView->g, player);
    }

    return ret;
}

//// Functions that return information about the history of the game

// Fills the trail array with the location ids of the last 6 turns
void giveMeTheTrail(HunterView currentView, PlayerID player,
                            LocationID trail[TRAIL_SIZE])
{
    assert(currentView != NULL);
    assert(0 <= player && player < NUM_PLAYERS);
    assert(trail != NULL);

    return getHistory(currentView->g, player, trail);
}

//// Functions that query the map to find information about connectivity

// What are my possible next moves (locations)
LocationID *whereCanIgo(HunterView currentView, int *numLocations, int road, int rail, int sea)
{
    assert(currentView != NULL);
    assert(numLocations != NULL);

    // return value
    LocationID *ret;

    // check if first round
    if(getRound(currentView->g) == FIRST_ROUND) {
        // everywhere!
        ret = (LocationID *)(malloc(sizeof(LocationID)*NUM_MAP_LOCATIONS));
        (*numLocations) = 0;

        int i;
        for(i=0;i<NUM_MAP_LOCATIONS;i++) {
            ret[(*numLocations)] = i;
            (*numLocations)++;
        }
    } else {
        ret = connectedLocations(currentView->g, numLocations,
                                 getLocation(currentView->g, 
                                             getCurrentPlayer(currentView->g)),
                                 getCurrentPlayer(currentView->g),
                                 getRound(currentView->g),
                                 road ,rail, sea);
    }
    
    return ret;
}

// What are the specified player's next possible moves
LocationID *whereCanTheyGo(HunterView currentView, int *numLocations,
                           PlayerID player, int road, int rail, int sea)
{
    assert(currentView != NULL);
    assert(0 <= player && player < NUM_PLAYERS);
    assert(numLocations != NULL);

    Round theirNextRound;

    // check if they're before or after me
    if(player >= getCurrentPlayer(currentView->g)) {
        theirNextRound = getRound(currentView->g);
    } else {
        theirNextRound = getRound(currentView->g) + 1;
    }

    // return value
    LocationID *ret;

    // check if first round
    if(theirNextRound == FIRST_ROUND) {
        ret = (LocationID *)(malloc(sizeof(LocationID)*NUM_MAP_LOCATIONS));
        (*numLocations) = 0;

        // everywhere! 
        int i;
        for(i=0;i<NUM_MAP_LOCATIONS;i++) {
            // dracula can go everywhere except ST_JOSEPH_AND_ST_MARYS
            if(player != PLAYER_DRACULA || i != ST_JOSEPH_AND_ST_MARYS) {
                ret[(*numLocations)] = i;
                (*numLocations)++;
            }
        }
    } else {
        if(player == PLAYER_DRACULA) {
            // dracula
            // dracula's current location
            LocationID dracLoc = whereIs(currentView, PLAYER_DRACULA);

            // see if we can infer dracula's location
            
            // if valid, do the usual
            if(validPlace(dracLoc)) {
                // dracula can't travel by rail even if he wants to
                ret = connectedLocations(currentView->g, numLocations,
                                        whereIs(currentView, PLAYER_DRACULA),
                                        theirNextRound,
                                        player, road, FALSE, sea);
            } else {
                (*numLocations) = 0;

                // FIXME not sure what to return; probably doesn't matter
                ret = NULL;
            }
        } else {
            // a hunter
            ret =  connectedLocations(currentView->g, numLocations,
                                    getLocation(currentView->g, player),
                                    theirNextRound,
                                    player, road, rail, sea);
        }
    }

    return ret;
}

static void pushOnTrailLocs (HunterView h, LocationID placeID) {
    assert(h != NULL);

    int i;
    for(i=TRAIL_SIZE-1; i>=1; i--) {
        h->trailLocs[i] = h->trailLocs[i-1];
    }
    h->trailLocs[0] = placeID;
    return;
}
