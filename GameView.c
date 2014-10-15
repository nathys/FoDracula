// GameView.c ... GameView ADT implementation
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "Globals.h"
#include "Game.h"
#include "GameView.h"
#include "Map.h"

#ifdef DEBUG
#define D(x...) fprintf(stderr,x)
#else
#define D(x...)
#endif

// the number characters used to describe each play
#define CHARS_PER_PLAY 7

// including the space
#define CHARS_PER_PLAY_BLOCK (CHARS_PER_PLAY+1)

// char that separates the plays
#define PLAY_SEP_CHAR ' '

// id of the first round
#define FIRST_ROUND 0

// index that the location abbreviation starts in within the char *pastPlays
#define LOC_ABBREV_INDEX 1

// min and max values to double back
#define MIN_DOUBLE_BACK 1
#define MAX_DOUBLE_BACK 5

// first value for doubling back (location id)
#define FIRST_DOUBLE_BACK DOUBLE_BACK_1

// arbitrarily 'big enough' value for infinity = 10^7
#define INFINITY 10000000

// mod that restricts the rail travel of the hunters by the sum of the round
// and the hunter
#define RAIL_RESTRICT 4    

// The maximum number of plays we will accept
// Since each dracula turn reduces the score by one, we will set it to
// at most 366 dracula's turns
// ... plus a bit extra because josh is ultra-conservative
#define MAX_PLAYS (366*5+5+10)

// maximum length of past plays string
#define MAX_PAST_PLAYS_LENGTH (MAX_PLAYS * CHARS_PER_PLAY_BLOCK)

// maximum length of any message
// ... plus a bit extra because josh is ultra-conservative
#define MAX_MESSAGE_LENGTH (MESSAGE_SIZE+10) 

typedef struct _player {
    int health;
    LocationID position;
} player;

struct gameView {
    player players[NUM_PLAYERS];

    // dracula's trail; most recent last
    LocationID trail[TRAIL_SIZE];
    char *messages[MAX_PLAYS];
    char *pastPlays;
    int score;
    int turns;
};
     
// --- Helper functions --- //

// given 2 character string, work out what location id it matches to (not
// necessarily a known place)
static LocationID getNewLocation(char *abbrev);

// given 2 ints, returns the smaller one
static int min(int a, int b);

// Handles the adding/deleting of things on the trail
// Also, updates Dracula's location 
static void pushOnTrail (GameView g, LocationID placeID);

// Creates a new GameView to summarise the current state of the game
GameView newGameView(char *pastPlays, PlayerMessage messages[])
{
    assert(pastPlays != NULL);
    assert(messages != NULL);

    GameView g = malloc(sizeof(struct gameView));

    assert(g != NULL);

    g->pastPlays = malloc(sizeof(char) * MAX_PAST_PLAYS_LENGTH);
    int indexAt = 0;

    // Initialise the hunters
    int i;
    for(i = 0; i < NUM_PLAYERS-1; i++) {
        g->players[i].health = GAME_START_HUNTER_LIFE_POINTS;
        g->players[i].position = NOWHERE;
    }

    // initialise dracula
    g->players[PLAYER_DRACULA].health = GAME_START_BLOOD_POINTS;
    g->players[PLAYER_DRACULA].position = NOWHERE;

    // initialise turn and score
    g->turns = 0;
    g->score = GAME_START_SCORE;

    // initialise the trails
    for(i = 0; i < TRAIL_SIZE; i++) {
        g->trail[i] = NOWHERE;
    }

    // make a copy of past plays
    strncpy(g->pastPlays, pastPlays, MAX_PAST_PLAYS_LENGTH);
    
    // process the plays
    while(pastPlays[indexAt] != '\0') {
//        D("Play to process here is ");
//        int x;
//        for(x = 0; x < CHARS_PER_PLAY; x++) {
//            D("%c",pastPlays[indexAt+x]);
//        }
//        D("\n");
        
        // copy message
        // current turn index [0-based] is g->turns
        // first, allocate space for the string
        g->messages[g->turns] = 
            (char *)(malloc(sizeof(char)*MAX_MESSAGE_LENGTH));

        assert(g->messages[g->turns] != NULL);
        assert(messages[g->turns] != NULL);

        // copy string, using strncpy for safety
        strncpy(g->messages[g->turns], messages[g->turns], MAX_MESSAGE_LENGTH);

        // get the abbreviation for the new location
        char abbrev[3];
        abbrev[0] = pastPlays[indexAt+1];
        abbrev[1] = pastPlays[indexAt+2];

        // add a NUL terminator for good measure
        abbrev[2] = '\0';

        // try to get the place id of the current place
        LocationID placeID = abbrevToID(abbrev);

        // work out if dracula or a hunter
        if(pastPlays[indexAt] == 'D') {
            // This player is dracula
            // We update his position.
            
            int isAtSea;
            int isAtCastle;
            if(placeID != NOWHERE) {
                if(idToType(placeID) == SEA) {
                    isAtSea = TRUE;
                } else {
                    isAtSea = FALSE;
                }
                if(placeID == CASTLE_DRACULA) {
                    isAtCastle = TRUE;
                } else {
                    isAtCastle = FALSE;
                }
                pushOnTrail(g, placeID);
            } else {
                // This is not dracula's string and we do not know where he is
                // And cannot update his position, other than saying if he is
                // on land or at sea.
                // We still need to know if he is at sea
                if(abbrev[0] == 'C') {
                    // He is in some city
                    // That is not castle dracula
                    isAtSea = FALSE;
                    isAtCastle = FALSE;
                    pushOnTrail(g, CITY_UNKNOWN);
                    placeID = CITY_UNKNOWN;
                } else if(abbrev[0] == 'S') {
                    // He is at sea
                    isAtSea = TRUE;
                    isAtCastle = FALSE;
                    pushOnTrail(g, SEA_UNKNOWN);
                    placeID = SEA_UNKNOWN;
                } else if(abbrev[0] == 'T') {
                    // He is at the castle
                    isAtSea = FALSE;
                    isAtCastle = TRUE;
                    pushOnTrail(g, CASTLE_DRACULA);

                    // set placeID to be teleport
                    placeID = TELEPORT;
               } else if(abbrev[0] == 'D') {
                    // He doubled back.
                    // Because of the game's rules, we don't be clever and
                    // instead place only the DOUBLE_BACK_ move type onto the
                    // trail, even though we can (and do) infer the at-sea-ness
                    // and location of dracula
                    int numBack = (int)(abbrev[1]-'0');
                    LocationID newPosition = g->trail[TRAIL_SIZE-numBack];

//                    D("trail:");
//                    for(i=0;i<TRAIL_SIZE;i++) {
//                        D(" %d",g->trail[i]);
//                    }
//                    D("\n");
//                    D("newPosition is %d\n",newPosition);

                    if(newPosition == CITY_UNKNOWN) {
                        isAtSea = FALSE;
                        isAtCastle = FALSE;
                    } else if (newPosition == SEA_UNKNOWN) {
                        isAtSea = TRUE;
                        isAtCastle = FALSE;
                    } else {
                        // We know exactly where dracula is
                        if (idToType(newPosition) == SEA) {
                            isAtSea = TRUE;
                            isAtCastle = FALSE;
                        } else if(newPosition == CASTLE_DRACULA) {
                            isAtSea = FALSE;
                            isAtCastle = TRUE;
                        } else {
                            isAtSea = FALSE;
                            isAtCastle = FALSE;
                        }
                    }
                    pushOnTrail(g, newPosition);

                    // for sake of the getLocation function, we'll set
                    // Dracula's new location to be the TYPE of move as opposed
                    // to the city he's actually at even though we know what
                    // that is
                    placeID = (FIRST_DOUBLE_BACK-MIN_DOUBLE_BACK) + numBack;
                } else if(abbrev[0] == 'H') {
                    // He's HIDING!

                    // push on the most recent location
                    pushOnTrail(g, g->trail[TRAIL_SIZE-1]);

                    // for sake of getLocation, make current location HIDE
                    placeID = HIDE;
                }
            }

            // set Dracula's 'public' location (as returned by getLocation)
            g->players[PLAYER_DRACULA].position = placeID;

            // Now we figure out what exactly dracula does at the new location
            if(isAtSea) {
                g->players[PLAYER_DRACULA].health -= LIFE_LOSS_SEA;
            } else if(isAtCastle) {
                g->players[PLAYER_DRACULA].health += LIFE_GAIN_CASTLE_DRACULA;
            }

            if(pastPlays[indexAt+3] == 'T') {
                // Dracula placed a trap.
                // TODO: Processing on what to do with traps
                // So far, it doesn't seem like we need to do anything
                // Since any encounters of traps are given to us
                // so we don't need to know where these things are
            }
            if(pastPlays[indexAt+4] == 'V') {
                // Dracula placed a young vampire
                // TODO: See the section on traps just above
            }

            // What just left the trail?
            if(pastPlays[indexAt+5] == 'V') {
                // A vampire has matured
                g->score -= SCORE_LOSS_VAMPIRE_MATURES;
            }
        } else {
            // This player is one of the hunters
            PlayerID curHunter;
            switch (pastPlays[indexAt]) {
                case 'G': curHunter = PLAYER_LORD_GODALMING; break;
                case 'S': curHunter = PLAYER_DR_SEWARD; break;
                case 'H': curHunter = PLAYER_VAN_HELSING; break;
                case 'M': curHunter = PLAYER_MINA_HARKER; break;
                default:
                    assert (FALSE && "This is not a valid identifier for a player.");
            }

            LocationID newPosition = abbrevToID(abbrev);

            if(g->players[curHunter].position == ST_JOSEPH_AND_ST_MARYS &&
               g->players[curHunter].health == 0) {
                // Our hunter has grown his legs back now.
                g->players[curHunter].health = GAME_START_HUNTER_LIFE_POINTS;
            }

            // Check if some encounters were made
            int i;

            // only loop while our hunter is alive and kicking
            // (and dracula, of course)
            for(i = 3;i < CHARS_PER_PLAY && 
                      g->players[curHunter].health > 0 &&
                      g->players[PLAYER_DRACULA].health > 0; i++) {
                if(pastPlays[indexAt+i] == 'T') {
                    // Encountered a trap
                    g->players[curHunter].health -= LIFE_LOSS_TRAP_ENCOUNTER;
                } else if(pastPlays[indexAt+i] == 'D') {
                    //D("ENCOUNTERED DRACULA\n");
                    // Encountered Dracula
                    g->players[curHunter].health -= LIFE_LOSS_DRACULA_ENCOUNTER;
                    g->players[PLAYER_DRACULA].health -= LIFE_LOSS_HUNTER_ENCOUNTER;
                }
            }

            // check if our hunter died =(
            if (g->players[curHunter].health <= 0) {
                g->players[curHunter].health = 0;
                g->score -= SCORE_LOSS_HUNTER_HOSPITAL;
                newPosition = ST_JOSEPH_AND_ST_MARYS;
            } else if(newPosition == g->players[curHunter].position) {
                // The hunter rests and regains some health
                // Hunters need a bit of RnR, too!
                g->players[curHunter].health += LIFE_GAIN_REST;

                // cap hunter's health at GAME_START_HUNTER_LIFE_POINTS
                if (g->players[curHunter].health > GAME_START_HUNTER_LIFE_POINTS) {
                    g->players[curHunter].health = GAME_START_HUNTER_LIFE_POINTS;
                }
            }

            // update our hunter's position
            g->players[curHunter].position = newPosition;
        }
        g->turns++;

        // we increment this way just in case we overshoot the NUL terminator
        // at the end of the string
        indexAt += CHARS_PER_PLAY;
        if(pastPlays[indexAt] == PLAY_SEP_CHAR) {
            indexAt++;
        }
    }

    // A little special case to heal the current hunter if they've been 
    // incapacitated last turn.
    int turnPlayer = getCurrentPlayer(g);
    if(g->players[turnPlayer].position == ST_JOSEPH_AND_ST_MARYS &&
       g->players[turnPlayer].health == 0) {
        // The hunter has regrown his legs!
        g->players[turnPlayer].health = GAME_START_HUNTER_LIFE_POINTS;
    }

    // print out messages for teh luls
//    for(i=0;i<g->turns;i++) {
//        D("message for turnplay %d: %s\n",i,g->messages[i]);
//    }
    return g;
}
     
// Frees all memory previously allocated for the GameView toBeDeleted
void disposeGameView(GameView toBeDeleted)
{
    assert(toBeDeleted != NULL);

    // free pastPlays
    free(toBeDeleted->pastPlays);

    // free messages
    int i;
    for(i=0;i<toBeDeleted->turns;i++) {
        free(toBeDeleted->messages[i]);
    }

    // free struct
    free(toBeDeleted);
}


//// Functions to return simple information about the current state of the game

// Get the current round
Round getRound(GameView currentView)
{
    assert(currentView != NULL);

    // just take floor of num turns / num players
    return (Round)(currentView->turns / NUM_PLAYERS);
}

// Get the id of current player - ie whose turn is it?
PlayerID getCurrentPlayer(GameView currentView)
{
    assert(currentView != NULL);

    // just take floor of num turns mod num players
    return (PlayerID)(currentView->turns % NUM_PLAYERS);
}

// Get the current score
int getScore(GameView currentView)
{
    assert(currentView != NULL);
    return currentView->score;
}

// Get the current health points for a given player
int getHealth(GameView currentView, PlayerID player)
{
//    D("health of %d is %d\n",player,currentView->players[player].health);
    assert(currentView != NULL);
    assert(0 <= player && player < NUM_PLAYERS);
    return currentView->players[player].health;
}

// Get the current location id of a given player
LocationID getLocation(GameView currentView, PlayerID player)
{
    //D("location of player %d is %d\n",player,currentView->players[player].position);
    assert(currentView != NULL);
    assert(0 <= player && player < NUM_PLAYERS);
    return currentView->players[player].position;
}

//// Functions that return information about the history of the game

// Fills the trail array with the location ids of the last 6 turns
void getHistory(GameView currentView, PlayerID player,
        LocationID trail[TRAIL_SIZE])
{
    assert(currentView != NULL);
    assert(0 <= player && player < NUM_PLAYERS);
    assert(trail != NULL);

    int i;

    // last round this player made a move in
    Round lastRoundPlayed;
    if(getCurrentPlayer(currentView) > player) {
        // moved this round already
        lastRoundPlayed = getRound(currentView);
    } else {
        // haven't yet made a move this round
        lastRoundPlayed = getRound(currentView) - 1;
    }

    // iterate over past moves
    for(i=0;i<TRAIL_SIZE;i++) {
        // round we're considering
        Round curRound = (Round)(lastRoundPlayed-i);

        if(curRound >= FIRST_ROUND) {
            // number of the turn (NOT type Round since it's a turn not a round)
            int turnNo = (curRound * NUM_PLAYERS) + player;

            // pointer to the start of the play; some pointer arithmetic
            char *startOfPlay = (currentView->pastPlays) + 
                                (turnNo * CHARS_PER_PLAY_BLOCK);

            // get id from the abbreviation (located in the first two characters of
            // start of play and store it in the trail
            trail[i] = (LocationID)(getNewLocation(startOfPlay +
                                                   LOC_ABBREV_INDEX));
        } else {
            // no previous location
            trail[i] = UNKNOWN_LOCATION;
        }
//        D("trail for player %d: %d rounds ago: %d [round %d]\n",player, i, trail[i], curRound);
    }
}

//// Functions that query the map to find information about connectivity

// Returns an array of LocationIDs for all directly connected locations

LocationID *connectedLocations(GameView currentView, int *numLocations,
        LocationID from, PlayerID player, Round round,
        int road, int rail, int sea)
{
    assert(currentView != NULL);
    assert(numLocations != NULL);
    assert(validPlace(from));
    assert(0 <= player && player < NUM_PLAYERS);
    
    int i, j, k;

    // our map
    Map ourMap = newMap();

    // boolean array storing whether or not we can reach each vertex
    int canReach[NUM_MAP_LOCATIONS];

    // initialise it
    for(i=0;i<NUM_MAP_LOCATIONS;i++) {
        canReach[i] = FALSE;
    }
    
    // remember we can always reach ourselves
    canReach[from] = TRUE;

    // pairwise shortest rail distances
    int railDist[NUM_MAP_LOCATIONS][NUM_MAP_LOCATIONS];

    // floyd warshall to calculate pariwse shortest paths.

    // initialise distances
    for(i=0;i<NUM_MAP_LOCATIONS;i++) {
        for(j=0;j<NUM_MAP_LOCATIONS;j++) {
            if(i == j) {
                railDist[i][j] = 0;
            } else {
                int edgeDist = getDist(ourMap, RAIL, i, j);
                if(edgeDist == NO_EDGE) {
                    edgeDist = INFINITY;
                }
                railDist[i][j] = edgeDist;
            }
        }
    }

    // floyd warshall time!
    for(j=0;j<NUM_MAP_LOCATIONS;j++) {
        for(i=0;i<NUM_MAP_LOCATIONS;i++) {
            for(k=0;k<NUM_MAP_LOCATIONS;k++) {
                railDist[i][k] = min(railDist[i][k], railDist[i][j] + railDist[j][k]);
            }
        }
    }

    // actually do the thing
    
    // invoke special rules: transform road, rail and sea not just to store
    // TRUE/FALSE but to store the maximum number of edges of that type we can
    // move in

    if(rail == TRUE) {
        if(player == PLAYER_DRACULA) {
            // dracula can't move by rail
            rail = 0;
        } else {
            // determine how far hunters can move by rail
            rail = (round+player) % RAIL_RESTRICT;
        }
    } else {
        rail = 0;
    }

//    D("rail=%d\n",rail);

    // add rail links
    for(i=0;i<NUM_MAP_LOCATIONS;i++) {
        if(railDist[from][i] <= rail) {
            canReach[i] = TRUE;
        }
    }

    if(road == TRUE) {
        road = 1;
    } else {
        road = 0;
    }

    // add road links
    for(i=0;i<NUM_MAP_LOCATIONS;i++) {
        int distHere = getDist(ourMap, ROAD, from, i);
        if(distHere != NO_EDGE && distHere <= road) {
            canReach[i] = TRUE;
        }
    }

    if(sea == TRUE) {
        sea = 1;
    } else {
        sea = 0;
    }

    // add sea links
    for(i=0;i<NUM_MAP_LOCATIONS;i++) {
        int distHere = getDist(ourMap, BOAT, from, i);
        if(distHere != NO_EDGE && distHere <= sea) {
            canReach[i] = TRUE;
        }
    }

    // ensure Dracula can't move to the hospital
    if(player == PLAYER_DRACULA) {
        canReach[ST_JOSEPH_AND_ST_MARYS] = FALSE;
    }

    // output

    // work out numLocations; initialise it to be 0
    (*numLocations) = 0;

    // count number of locations
    for(i=0;i<NUM_MAP_LOCATIONS;i++) {
        if(canReach[i] == TRUE) {
            (*numLocations)++;
        }
    }

    // our return array; big enough to conserve memory
    LocationID *ret = 
        (LocationID *)(malloc((*numLocations) * sizeof(LocationID)));

    // current index we're up to
    int upto = 0;
    for(i=0;i<NUM_MAP_LOCATIONS;i++) {
        if(canReach[i] == TRUE) {
            ret[upto] = i;
            upto++;
        }
    }

    return ret;
}

static LocationID getNewLocation(char *abbrev) {
    assert(abbrev != NULL);

    // return value
    LocationID ret;

    if(abbrev[0] == 'C' && abbrev[1] == '?') {
        ret = CITY_UNKNOWN;
    } else if(abbrev[0] == 'S' && abbrev[1] == '?') {
        ret = SEA_UNKNOWN;
    } else if(abbrev[0] == 'H' && abbrev[1] == 'I') {
        ret = HIDE;
    } else if(abbrev[0] == 'D' &&
              '0'+MIN_DOUBLE_BACK <= abbrev[1] &&
              abbrev[1] <= '0'+MAX_DOUBLE_BACK) {
        ret = (FIRST_DOUBLE_BACK - MIN_DOUBLE_BACK) + (abbrev[1] - '0');
    } else if(abbrev[0] == 'T' && abbrev[1] == 'P') {
        ret = (LocationID)(CASTLE_DRACULA);
    } else {
        // doesn't match any, try converting to place name
        ret = abbrevToID(abbrev);
    }

    return ret;
}

static int min(int a, int b) {
    // abiding by the style guide: no multiple returns
    int ret;
    if(a < b) {
        ret = a;
    } else {
        ret = b;
    }
    return ret;
}

static void pushOnTrail (GameView g, LocationID placeID) {
    assert(g != NULL);

    int i;
    for(i = 1; i < TRAIL_SIZE; i++) {
        g->trail[i-1] = g->trail[i];
    }
    g->trail[TRAIL_SIZE-1] = placeID;
    return;
}
