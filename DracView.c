// DracView.c ... DracView ADT implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "Globals.h"
#include "Game.h"
#include "GameView.h"
#include "DracView.h"
// #include "Map.h" ... if you decide to use the Map ADT

#ifdef DEBUG
#define D(x...) fprintf(stderr,x)
#else
#define D(x...)
#endif

// pastPlays string
// the number characters used to describe each play
#define CHARS_PER_PLAY 7

// including the space
#define CHARS_PER_PLAY_BLOCK (CHARS_PER_PLAY+1)

// indexes in pastPlays string

// index that the location abbreviation starts in within the char *pastPlays
#define LOC_ABBREV_INDEX 1

// index of whether or not dracula placed a trap
#define DRACULA_TRAP_INDEX 3

// index of whether or not dracula placed a vamp
#define DRACULA_VAMP_INDEX 4

// index of action phase on dracula's turn
#define DRACULA_ACTION_INDEX 5

// index of the start of hunter encounters
#define HUNTER_ENCOUNTERS_START_INDEX 3

// maximum number of encounters a hunter can face in any one turn
#define MAX_HUNTER_ENCOUNTERS 3

// The maximum number of plays we will accept
// Since each dracula turn reduces the score by one, we will set it to
// at most 366 dracula's turns
// ... plus a bit extra because josh is ultra-conservative
#define MAX_PLAYS (366*5+5+10)

// maximum length of past plays string
#define MAX_PAST_PLAYS_LENGTH (MAX_PLAYS * CHARS_PER_PLAY_BLOCK)

// id of the first round
#define FIRST_ROUND 0

// first and last double back
#define DOUBLE_BACK_FIRST DOUBLE_BACK_1
#define DOUBLE_BACK_LAST DOUBLE_BACK_5

// most recent location in trail
#define LAST_TRAIL_LOC_INDEX 0

// dracView struct
struct dracView {
    // the gameview that's within!
    GameView g;

    // last TRAIL_SIZE _locations_; NOT moves
    // in REVERSE chronological order
    LocationID trailLocs[TRAIL_SIZE];

    // number of each type of encounter at each location
    int numTraps[NUM_MAP_LOCATIONS];

    // location of the vampire; at most 1 at any tim
    int vampLoc;
};

// helper functions
static void pushOnTrailLocs (DracView d, LocationID placeID);

// Creates a new DracView to summarise the current state of the game
DracView newDracView(char *pastPlays, PlayerMessage messages[])
{
    assert(pastPlays != NULL);
    assert(messages != NULL);

    int i, j;

    // malloc
    DracView d = (DracView)(malloc(sizeof(struct dracView)));
    assert(d != NULL);

    // make the GameView
    d->g = newGameView(pastPlays, messages);
    assert(d->g != NULL);

    // initialise Dracula's actual locations to all UNKNOWN_LOCATION
    for(i=0;i<TRAIL_SIZE;i++) {
        d->trailLocs[i] = UNKNOWN_LOCATION;
    }

    // fill out numTraps and find vamp location
    // also iterate over past moves to generate the trailLocs[]

    // initialise
    for(i=0;i<NUM_MAP_LOCATIONS;i++) {
        d->numTraps[i] = 0;
    }
    d->vampLoc = NOWHERE;

    // length of pastPlays
    int pastPlaysLength = strnlen(pastPlays, MAX_PAST_PLAYS_LENGTH); 

    // iterate over each turn and process
    for(i=0;i<pastPlaysLength;i+=CHARS_PER_PLAY_BLOCK) {
        // get curPlayer
        PlayerID curPlayer = ( (i/CHARS_PER_PLAY_BLOCK) % NUM_PLAYERS);

        // try to get current loc (if it's exact)
        LocationID curLoc = abbrevToID(pastPlays+i+LOC_ABBREV_INDEX);
        //D("curLoc = %d, abbrev=%.2s\n",curLoc,pastPlays+i+LOC_ABBREV_INDEX);

        // test if exact location
        if(!validPlace(curLoc)) {
            // it's not; we must be dracula
            // use trail to work out where we really are

            // either a HIDE, DOUBLE_BACK_ or TELEPORT
            if(pastPlays[i+LOC_ABBREV_INDEX] == 'H') {
                // HIDE: go to most recent location
                curLoc = d->trailLocs[LAST_TRAIL_LOC_INDEX];
            } else if(pastPlays[i+LOC_ABBREV_INDEX] == 'D') {
                // DOUBLE BACK
                int numBack = (int)(pastPlays[i+LOC_ABBREV_INDEX+1]-'0');
                curLoc = d->trailLocs[numBack];
            } else if(pastPlays[i+LOC_ABBREV_INDEX] == 'T') {
                // TELEPORT back to CASTLE_DRACULA
                curLoc = CASTLE_DRACULA;
            } else {
                // should never happen
                assert(TRUE == FALSE);
            }
        }

        // just to be sure
        assert(validPlace(curLoc));

        // test what player we are considering
        if(curPlayer == PLAYER_DRACULA) {
            // dracula

            // trap
            if(pastPlays[i+DRACULA_TRAP_INDEX] == 'T') {
                d->numTraps[curLoc]++;
            }

            // vamp
            if(pastPlays[i+DRACULA_VAMP_INDEX] == 'V') {
                d->vampLoc = curLoc;
            }

            // action
            if(pastPlays[i+DRACULA_ACTION_INDEX] == 'M') {
                // trap from 6 turns ago (end of the trail) has malfunctioned
                d->numTraps[d->trailLocs[TRAIL_SIZE-1]]--;
            } else if(pastPlays[i+DRACULA_ACTION_INDEX] == 'V') {
                // vampire has matured
                d->vampLoc = NOWHERE;
            }

            // push curLoc onto the trail
            pushOnTrailLocs(d, curLoc);
        } else {
            // a hunter

            // loop through all encounters
            for(j=0;j<MAX_HUNTER_ENCOUNTERS;j++) {
                char curEncounter = 
                    pastPlays[i+HUNTER_ENCOUNTERS_START_INDEX+j];

                if(curEncounter == 'T') {
                    // fell into a trap but disarmed it
                    d->numTraps[curLoc]--;
                } else if(curEncounter == 'V') {
                    // vanquished a vampire
                    d->vampLoc = NOWHERE;
                }
            }
        }
    }

    return d;
}


// Frees all memory previously allocated for the DracView toBeDeleted
void disposeDracView(DracView toBeDeleted)
{
    assert(toBeDeleted != NULL);

    // free the GameView
    disposeGameView(toBeDeleted->g);
    free( toBeDeleted );
}


//// Functions to return simple information about the current state of the game

// Get the current round
Round giveMeTheRound(DracView currentView)
{
    assert(currentView != NULL);
    return getRound(currentView->g);
}

// Get the current score
int giveMeTheScore(DracView currentView)
{
    assert(currentView != NULL);
    return getScore(currentView->g);
}

// Get the current health points for a given player
int howHealthyIs(DracView currentView, PlayerID player)
{
    assert(currentView != NULL);
    return getHealth(currentView->g, player);
}

// Get the current location id of a given player
LocationID whereIs(DracView currentView, PlayerID player)
{
    assert(currentView != NULL);

    // return value
    LocationID ret;
    
    // check if we're dracula
    if(player == PLAYER_DRACULA) {
        // return from our trail
        ret = currentView->trailLocs[LAST_TRAIL_LOC_INDEX];
    } else {
        // call GameView ADT
       ret = getLocation(currentView->g, player);
    }
    return ret;
}

// Get the most recent move of a given player
void lastMove(DracView currentView, PlayerID player,
        LocationID *start, LocationID *end)
{
    assert(currentView != NULL);
    assert(0 <= player && player < NUM_PLAYERS);
    assert(start != NULL);
    assert(end != NULL);

    // read it off the trail
    LocationID trail[TRAIL_SIZE];

    // get the trail from the gameview
    getHistory(currentView->g, player, trail);

    // return last two locations from the first two spots in trail (reverse
    // chronological order)
    (*end) = trail[0];
    (*start) = trail[1];
}

// Find out what minions are placed at the specified location
void whatsThere(DracView currentView, LocationID where,
        int *numTraps, int *numVamps)
{
    assert(currentView != NULL);
    assert(validPlace(where));

    // check if vamp there
    if(where == currentView->vampLoc) {
        (*numVamps) = 1;
    } else {
        (*numVamps) = 0;
    }

    // get numTraps
    (*numTraps) = currentView->numTraps[where];

//    D("whatsThere: where=%d, nT=%d, nV=%d\n",where,(*numTraps),(*numVamps));
}

//// Functions that return information about the history of the game

// Fills the trail array with the location ids of the last 6 turns
void giveMeTheTrail(DracView currentView, PlayerID player,
        LocationID trail[TRAIL_SIZE])
{
    assert(currentView != NULL);
    assert(0 <= player && player < NUM_PLAYERS);
    assert(trail != NULL);

    getHistory(currentView->g, player, trail);
}

//// Functions that query the map to find information about connectivity

// What are my (Dracula's) possible next moves (locations)
LocationID *whereCanIgo(DracView currentView, int *numLocations, int road, int sea)
{
    assert(currentView != NULL);
    assert(numLocations != NULL);

    int i, j;

    // output array
    LocationID *out = 
        (LocationID *)(malloc(sizeof(LocationID) * NUM_MAP_LOCATIONS));
    assert(out != NULL);

    (*numLocations) = 0;

    // check if first round
    if(getRound(currentView->g) == FIRST_ROUND) {
        // everywhere except ST_JOSEPH_AND_ST_MARYS
        for(i=0;i<NUM_MAP_LOCATIONS;i++) {
            if(i != ST_JOSEPH_AND_ST_MARYS) {
                out[(*numLocations)] = i;
                (*numLocations)++;
            }
        }
    } else {
        // find all the connected locations, then rule out the ones that are in our
        // TRAIL, unless he's been there before in the trail unless he hasn't done
        // the appropriate (HIDE | DOUBLE_BACK)

        // get those connected to us
        int numConnected;
        LocationID *connected = 
            connectedLocations(currentView->g,
                            &numConnected,
                            whereIs(currentView, PLAYER_DRACULA),
                            PLAYER_DRACULA,
                            getRound(currentView->g),
                            road, // road
                            FALSE, // rail
                            sea); // sea

        assert(connected != NULL);

        // get our history
        LocationID *hist = (LocationID *)(malloc(sizeof(LocationID)*TRAIL_SIZE));
        getHistory(currentView->g, PLAYER_DRACULA, hist);

        // check getHistory actually did something and returned a non-NULL
        assert(hist != NULL);

        // work out if we've HIDE or DOUBLE_BACK recently
        int hasHide = FALSE;
        int hasDoubleBack = FALSE;

        // only consider last TRAIL_SIZE-1 moves since the last move 'falls off the
        // end' before we get our current move
        for(i=0;i<TRAIL_SIZE-1;i++) {
            // check doubling back
            if(DOUBLE_BACK_FIRST <= hist[i] &&
            hist[i] <= DOUBLE_BACK_LAST) {
                hasDoubleBack = TRUE;
            }

            if(hist[i] == HIDE) {
                hasHide = TRUE;
            }
        }

        // iterate over all the possibilities
        for(i=0;i<numConnected;i++) {
            // i don't trust anyone... *looks around suspiciously*
            assert(validPlace(connected[i]));

            // check if it's anywhere in our trail
            int isCurPos = FALSE;

            // this one excludes current pos (double back)
            int inTrail = FALSE;
            for(j=0;j<TRAIL_SIZE;j++) {
                // see if it matches
                if(connected[i] == currentView->trailLocs[j]) {
                    // check if it's equal to our current pos
                    if(j == 0) {
                        isCurPos = TRUE;
                    } else {
                        inTrail = TRUE;
                    }
                }
            }

            // check if legit
            int isLegit = TRUE;

            // ensure we can't HIDE at sea
            if(isCurPos == TRUE && idToType(connected[i]) == SEA) {
                isLegit = FALSE;
            }

            if(isLegit == TRUE) {
                isLegit = FALSE;

                // test what kind of move we can make to do this

                // see if we just go there normally
                if(isCurPos == FALSE && inTrail == FALSE) {
                    isLegit = TRUE;
                }

                // see if we can hide
                if(isCurPos == TRUE) {
                    // MUST be considered a HIDE move
                    if(hasHide == FALSE) {
    //                    D("at %s\n",idToName(connected[i]));
                        isLegit = TRUE;
                    }
                } else {
                    // see if we can DOUBLE_BACK
                    if(inTrail == TRUE && hasDoubleBack == FALSE) {
                        isLegit = TRUE;
                    }
                }

                if(isLegit == TRUE) {
                    // legit! add and increment
                    out[(*numLocations)] = connected[i];
                    (*numLocations)++;
                }
            }
        }
    }

    return out;
}

// What are the specified player's next possible moves
LocationID *whereCanTheyGo(DracView currentView, int *numLocations,
        PlayerID player, int road, int rail, int sea)
{
    assert(currentView != NULL);
    assert(numLocations != NULL);
    assert(0 <= player && player < NUM_PLAYERS);

    // return value
    LocationID *ret;

    if(player == PLAYER_DRACULA) {
        // call whereCanIgo
        ret = whereCanIgo(currentView, numLocations, road, sea);
    } else {
        // call connectedLocations
        Round theirNextRound;

        // check if they're before or after me
        if(player >= getCurrentPlayer(currentView->g)) {
            theirNextRound = getRound(currentView->g);
        } else {
            theirNextRound = getRound(currentView->g) + 1;
        }

        if(theirNextRound == FIRST_ROUND) {
            // ANYWHERE!
            ret = (LocationID *)(malloc(sizeof(LocationID)*NUM_MAP_LOCATIONS));

            (*numLocations) = 0;

            int i;
            for(i=0;i<NUM_MAP_LOCATIONS;i++) {
                ret[(*numLocations)] = i;
                (*numLocations)++;
            }
        } else {
            ret = connectedLocations(currentView->g, numLocations,
                                    whereIs(currentView, player), player,
                                    theirNextRound,
                                    road, rail, sea);
        }
    }

    assert(ret != NULL);
    return ret;
}

static void pushOnTrailLocs (DracView d, LocationID placeID) {
    assert(d != NULL);
    assert(validPlace(placeID));

    int i;
    for(i=TRAIL_SIZE-1; i>=1; i--) {
        d->trailLocs[i] = d->trailLocs[i-1];
    }
    d->trailLocs[0] = placeID;
    return;
}
