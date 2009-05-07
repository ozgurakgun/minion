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

#ifndef BETA_H
#define BETA_H

#include "graph.h"
#include "label.h"
#include "distanceList.h"

// This class compute the beta_k value of the vertices of a labelled graph
// For each labelled vertex v of a graph, it computes the set of triples (d,k,l)
// where k is the number of vertices labelled by l at distance d from v 

class Beta {
	public :
	// constructor
	// tg, the graph and tdMax the maximal distance
	Beta(Graph * tg, int tdMax) {
		g=tg; dMax = tdMax;
		bSeen = new short[g->getOrder()];
		lFreq = new short[g->getOrder()];
		lStack = new short[g->getOrder()];
		dl = new DistanceList(g,dMax);
	}
	
	// destructor
	~Beta(void) {
		delete[] bSeen;
		delete[] lFreq;
		delete[] lStack;
		delete dl;
	}
	
	// to run only one time before use the class
	void init(void) {
		return;
	}
	
	// to call when begining the first call to compute
	void initCompute(void) {
		int i;
		for(i = 0 ; i < g->getOrder() ; i++) {
			bSeen[i]=-1;
		}
	}
	
	// compute the vector corresponding to Beta_k(v)
	// t is the vector, t[0]=\alpha(v) then t[1]t[2][3]=(d,k,\alpha)...
	// return the size of t
	// each element of bSeen must be < v when called
	short compute(short v, short *t, Label * l) {
		short i,d;
		short ltemp, tempNbLabel;
		short currentPosInT = 0;
		
		// we set the first part only composed of the label of v
		t[0] = l->getLabel(v);
		currentPosInT = 1;

		dl->computeDistanceForVertex(v);		
		// for each distance
		for(d = 1 ; d <= dMax ; d++) {
			
			// we get the multiset of labels of the vertices at distance of d from  v
			// bSeen[l]=v if l is soon seen at distance d from v
			// lFreq[l]=number of vertices at distance d from v and having the label l
			// lStack contains the labels at distance d from v
			tempNbLabel = 0;
			
			// for each 
			for(i = dl->getNbByDistanceMax(v,d-1) ; i < dl->getNbByDistanceMax(v,d) ; i++) {
				ltemp = l->getLabel(dl->getKIThVertex(v,i));
				
				
				// if new label
				if(bSeen[ltemp] < v) {
					bSeen[ltemp] = v;
					lFreq[ltemp] = 1;
					lStack[tempNbLabel] = ltemp;
					tempNbLabel++;
				} else {
					lFreq[ltemp]++;
				}
			}
			
			// we sort the seen label
			quickSort(lStack,0,tempNbLabel-1);
			
			
			// we clear the seen label (for the next execution)
			for(i = 0 ; i < tempNbLabel ; i++) {
				bSeen[lStack[i]] = -1;
			}
			
			// we build the vector
			for(i = 0 ; i < tempNbLabel ; i++) {
				t[currentPosInT] = d;
				t[currentPosInT+1] = lStack[i];
				t[currentPosInT+2] = lFreq[lStack[i]];
				currentPosInT+=3;
			}
			
		}
		
		return currentPosInT;
	};
	
	// set t[u]=n for each vertex such that 1 <= d(u,v) <=dMax
	void mark(int v, short * t, int n) {
		int i;
		int nbMax = dl->getNbByDistanceMax(v,dMax);
		for(i = 1 ; i < nbMax ; i++) {
			t[dl->getKIThVertex(v,i)]=n;
		}
		return;
	}
	
	protected :
	
	Graph * g;
	int dMax;
	DistanceList * dl;
		
	short * bSeen; // temp used to know if a label has been seen
	short * lFreq; // frequency of a label
	short * lStack; // stack that contains all the labels seen
	
	
	
  // used for sorting
	
	int partition(short * a, short p, short r) {
		int x = a[r];
		int j = p - 1;
		for (int i = p; i < r; i++) {
			if (x <= a[i]) {
				j = j + 1;
				int temp = a[j];
				a[j] = a[i];
				a[i] = temp;
			}
		}
		a[r] = a[j + 1];
		a[j + 1] = x;
		return (j + 1);
	}
	
	void quickSort(short * a, short p, short r) {
		if (p < r) {
			int q = partition(a, p, r);
			quickSort(a, p, q - 1);
			quickSort(a, q + 1, r);
		}
	}
	
	
	
	
};

#endif
