/*
Copyright (C) 2007 SORLIN Sébastien

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 
*/



#ifndef DISTANCELIST_H
#define DISTANCELIST_H

#include "graph.h"

#define NBOFTEMP 4

// This class compute the list of vertices at a given distance for a graph G
// An ordered vector of vertices is computed
// One can access to the list by distance by using this ordered vector
// and an other vector that gives the number of vertices
// at distance of at most d from v

class DistanceList {
	public :
	// constructor
	// tg is the graph and tdMax is the maximal distance to compute
	DistanceList(Graph *tg, int tdMax) {
		int i;
		g = tg;
		dMax = tdMax;
		nbV = tg->getOrder();
		nbByDistance = new short[nbV];
		nbByDistanceMax = new short[nbV];
		tOfSizeNbV = new short *[NBOFTEMP];
		for(i = 0 ; i < NBOFTEMP ; i++) {
			tOfSizeNbV[i] = new short[nbV];
		}
		
		distList = new short[nbV];
		
		// clear tOfSizeNbV[0] (set it to -1)
		for(i = 0 ; i < nbV ; i++) {
			tOfSizeNbV[0][i] = -1;
		}
		lastComputed = -1;
	}
	
	
	// destructor
	~DistanceList(void) {
		int i;
		for(i = 0 ; i < NBOFTEMP ; i++) {
			delete [] tOfSizeNbV[i];
		}
		delete [] tOfSizeNbV;
		delete [] nbByDistance;
		delete [] nbByDistanceMax;
		delete [] distList;
	}
	
	// return the number of vertices at distance of at most d from v
	short getNbByDistanceMax(int v, int d) {return nbByDistanceMax[d];}
	
	//
	short getKIThVertex(int v, int k) {return distList[k];}
	

	void computeDistanceForVertex(int i) {
		int j,sizeOfTheList;
		// clear tOfSizeNbV[0] (set it to -1)
		/*for(j = 0 ; j < nbV ; j++) {
			tOfSizeNbV[0][j] = -1;
		}*/
		if(lastComputed == i) return;
		lastComputed = i;
		tOfSizeNbV[0][i] = i;
		sizeOfTheList = 0;
		// add the vertex itself to the set of vertices to explore
		tOfSizeNbV[2][0] = i;
		// run
		run(i,0,sizeOfTheList,1);
		
		// recopy the result
		//distList[i] = new short[sizeOfTheList];
		for(j = 0 ; j < sizeOfTheList ; j++) {
			distList[j] = tOfSizeNbV[1][j];
		}
		
		for(j = 1 ; j <= dMax ; j++) {
				nbByDistanceMax[j] = nbByDistanceMax[j-1] + nbByDistance[j];
		}
		
	}
	protected :


void run(int v, int currentD, int & sizeOfTheList, int nbToExplore) {
	// v : the vertex to compute the distances
	// currentD : the Distance to explore
	// sizeOfTheList : current number of vertices into the ordered list
	// nbToExplore : nb vertices to explore into the queue tOfSizeNbV[2]
	int i,j;
	int u,w;
	int deg;
	int nbToExploreNext = 0;
	short * temp;
	
	// add the vertices to explore into the ordered list
	nbByDistance[currentD] = nbToExplore;
	for(i = 0 ; i < nbToExplore ; i++) {
		tOfSizeNbV[1][sizeOfTheList] = tOfSizeNbV[2][i];
		sizeOfTheList++;
	}
	// if the distance max is not reached		
	if(currentD < dMax) {
		// for each vertex to explore
		for(i = 0 ; i < nbToExplore ; i++) {
			u = tOfSizeNbV[2][i];
			deg = g->getDegree(u);
			// for each successor of the vertex
			for(j = 0 ; j < deg ; j++) {
				w = g->getIThSucc(u,j);
				// if the vertex has been never reached
				if(tOfSizeNbV[0][w] != v) {
					tOfSizeNbV[3][nbToExploreNext] = w;
					nbToExploreNext++;
					tOfSizeNbV[0][w] = v;
				}
			}
		}
		// we switch the vertex soon explored with the ones to explore
		temp = tOfSizeNbV[3];
		tOfSizeNbV[3] = tOfSizeNbV[2];
		tOfSizeNbV[2] = temp;
		// recursion
		run(v,currentD+1,sizeOfTheList,nbToExploreNext);
	}
	
	return;
}

Graph * g; // the graph
int dMax; // maximal distance
int nbV;

short * nbByDistance; 
// nbByDistance[v][d] = number of vertices at distance equal to d from v

short * nbByDistanceMax; 
// nbByDistanceMax[v][d] = number of vertices at distance equal or less to d
// from v

short * distList; 
// disList[v] : list (short*) of vertices ordered by their distance from v

// temporay
short ** tOfSizeNbV;

int lastComputed;
};

#endif

