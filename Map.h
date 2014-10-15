// Map.h ... interface to Map data type

#ifndef MAP_H
#define MAP_H

#include "Places.h"

#define NO_EDGE -1
#define NUM_TRANSPORT (ANY+1)

typedef struct edge{
    LocationID  start;
    LocationID  end;
    TransportID type;
} Edge;

// graph representation is hidden 
typedef struct MapRep *Map; 

// operations on graphs 
Map  newMap();  
void disposeMap(Map g); 
void showMap(Map g); 
int  numV(Map g);
int  numE(Map g, TransportID t);

// returns the distance of a direct edge of transport t FROM a TO b
// or NO_EDGE if no such edge exists
int  getDist(Map g, TransportID t, int a, int b);

#endif
