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

#ifndef FIXPOINT_H
#define FIXPOINT_H

#include "graph.h"
#include "beta.h"
#include "numberer.h"
#include "numbererHashTable.h"
#include "numbererSequentiel.h"
#include "label.h"

#define forEachVertex(v) for(v = 0 ; v < size ; v++)

// compute the fixPoint for two graphs
class fixPointTwoGraphs {
	public :
	// constructor
	fixPointTwoGraphs(Graph *tg1, Graph *tg2, int tdMax,bool hashing=true) {
		g1=tg1; g2=tg2; dMax=tdMax;
		size=g1->getOrder();
		
		// some simple controls
		if(size != g2->getOrder()) {
			cerr << "Graphs must have the same number of vertices" << endl;
			exit(-1);
		}
		if(g1->getNbEdges() != g2->getNbEdges()) {
			cerr << "Graphs must have the same number of edges" << g1->getNbEdges()  << " " << g2->getNbEdges() << endl;
			exit(-1);
		}
		
		// memory allocation
		if(hashing == true) {
			nu = new numbererHashTable(2*size,size*3);
		} else {
			nu = new numbererSequentiel(2*size,size*3);
		}
		
		b1 = new Beta(g1,dMax);
		b2 = new Beta(g2,dMax);
		tempLabel = new short[g1->getOrder()];
		temp = new short[size*3];
	};

	// destructor
	~fixPointTwoGraphs(void) {
		delete [] tempLabel;
		delete [] temp;
		delete b1;
		delete b2;
		delete nu;
	};

	void init(void) {
		b1->init();
		b2->init();
	}
	
	// compute the fix point for the two graphs
	// return the number of different labels or -1 if incomparable
	int computeFixPoint(Label *tl1, Label *tl2) {
		int nbL, newNbL;
		bool toContinue;
		l1=tl1; l2=tl2;
		
		nbL = l1->getNbLabels();
		int nbIter = 0;
		toContinue = true;
		// repeat until the number of labels can increase
		while(toContinue) {
			nbIter++;
			toContinue = false;
			// compute one step of alpha
			newNbL = onePass();
			// if graphs are incompatible
			if(newNbL == -1) {
				return -1;
			} else {
				// if there is more vertex labels than previously
				// one can make another step except when the number of labels
				// is equal to the order of the graph
				if((newNbL > nbL)&&(newNbL != size)) {
					toContinue = true;
				}
				nbL = newNbL;
			}
		}
		return nbL;
	};



	protected :

	// replace vertex label l by Psi(l) for both the graphs and check 
	// compatibility
	short onePass(void) {
		int v, sOfTemp;

		nu->clear();
		//G1
		b1->initCompute();
		forEachVertex(v) {
			// compute the label of v
			sOfTemp = b1->compute(v,temp,l1);
			// return the number for this label
			tempLabel[v] = nu->insert(sOfTemp,temp);
		}
		l1->restore(tempLabel);

		//G2
		b2->initCompute();
		forEachVertex(v) {
			// compute the label of v
			sOfTemp = b2->compute(v,temp,l2);
			// return the number for this label
			tempLabel[v] = nu->insert(sOfTemp,temp);
		}
		l2->restore(tempLabel);
		
		if(l1->compare(l2)) {
			return -1;
		}
		return l1->getNbLabels();
	}
	
	
	short size;
	// ref to the two graphs
	Graph * g1;
	Graph * g2;
	
	// ref to the two initial labelling functions
	Label * l1;
	Label * l2;
	
	// maximal distance
	short dMax;
	
	// a temporary labelling function
	short * tempLabel;
	
	// the label renumberer
	numberer * nu;
	
	// Psi function for graph 1
	Beta * b1;
	// Psi function for graph 2
	Beta * b2;
	
	// temporary set used to build the new labels
	short * temp;

};


#endif
