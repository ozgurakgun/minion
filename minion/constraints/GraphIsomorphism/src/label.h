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

#ifndef LABEL_H
#define LABEL_H

#include "graph.h"

#define UNLABELED -1

#define forEachVertex(v) for(v = 0 ; v < size ; v++)
#define forEachLabel(l) for(l = 0 ; l < size*2 ; l++)

// This class contains the labels of the vertices of a graph
// The vertices must be numbered from 0 to nbVertices-1
// The labels are number between 0 and (2*nbVertices)-1 (used for two graphs)
class Label {
	public :
	// constructor
	// tg the graph that contains the vertices to label
	Label(Graph * tg) {
		g = tg;
		size = g->getOrder();
		lab = new short[size];
		freq = new short[size*2];
	}
	
		
	// destructor
	~Label(void) {
		delete [] lab;
		delete [] freq;
	}
	
	// give all the vertices the label 0
	inline void allTheSameLabel(void) {
		int i;
		forEachVertex(i) lab[i] = 0;
		forEachLabel(i) freq[i] = 0;
		freq[0]=size;
		nbDiffLabels = 1;
	}
	

	// restore a labelling given by the vector t
	// t[i] = label to give to the vertex i
	inline void restore(const short * t) {
		int i;
		forEachLabel(i) freq[i] = 0;
		nbDiffLabels = 0;
		forEachVertex(i) {
			lab[i]=t[i];
			if(freq[t[i]] == 0) nbDiffLabels++;
			freq[t[i]]++;
		}
		return;
	}
	
	// store a labelling in t
	inline void store(short * t) const {
		int i;
		forEachVertex(i) {
			t[i] = lab[i];
		}
	}
	
	// return 0 if the two sets of labels are comparable
	// ATTENTION : suppose that the labels numbers are between 0 and size
	// only!!! so we must have to compare only the
	// lFirstGraph->compare(lSecondGraph)
	inline const int compare(const Label * toComp) const {
		int i;
		if(nbDiffLabels != toComp->nbDiffLabels) {
			//cout << "Nb Labels différents" << endl;
			return 1;
		}
		for(i = 0 ; i < nbDiffLabels ; i++) {
			if(freq[i] != toComp->freq[i]) {
				return 1;
			}
		}
		return 0;
	}
	
	// set the label of v to l
	inline void setLabel(const int v, const int l) {
		if(freq[l] == 0) {
			nbDiffLabels++;
		}
		freq[l]++;
		lab[v]=l;
		return;
	}
	
	// change the label of v to l
	inline void changeLabel(const int v, const int l) {
		if(freq[lab[v]] == 1) {
			nbDiffLabels--;
		}
		freq[lab[v]]--;
		if(freq[l] == 0) {
			nbDiffLabels++;
		}
		freq[l]++;
		lab[v]=l;
		return;
	}
	
	// return the label of v
	inline const short getLabel(const int v) const {
		return lab[v];
	}
	
	// return the frequency of a label
	inline const short getFreq(const int l) const {
		return freq[l];
	}
	
	inline const short getNbLabels(void) const {return nbDiffLabels;}
	
	
	
	private :
	
	short * lab; // the vertex labels
	short * freq; // the number of vertices having this label
	short nbDiffLabels; // the number of different labels used
	short size;	// size of the graph
	Graph * g; // graph
	
};

#endif
