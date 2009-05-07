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

#ifndef MY_FINDISO_H
#define MY_FINDISO_H


#include "graph.h"
#include "fixpoint.h"
#include "label.h"
#include "mapping.h"

// class used to find all the isomorphism between two graphs
class myFindIso {
	public :
	// constructor
	// tg1 and tg2, the two graphs to match
	// tdMax, the maximum distance to use for IDL filtering
	// tsol, a vector of solutions
	// tnbMaxSol : the maximum number of solutions to find
	myFindIso(Graph * tg1, Graph * tg2, int tdMax, mapping ** tsol=NULL, int tnbMaxSol=1,bool hashing=true) {
		g1=tg1; g2=tg2;
		dMax=tdMax;nbMaxSol = tnbMaxSol;
		sol =tsol;
		nbFoundedSol = 0;
		
		// initial labelling functions
		l1 = new Label(g1);
		l2 = new Label(g2);
		
		fp = new fixPointTwoGraphs(g1,g2,dMax,hashing);
		m = new mapping(g1,g2);
	}
	
	// to call before running
	void init(void) {
		fp->init();
	    l1->allTheSameLabel();
    	l2->allTheSameLabel();
    
	}
	
	int propagate()
	{
	    // compute fixed point of the dMax-IDL consistency
        int nBL = fp->computeFixPoint(l1, l2);
        
        // if the labels do not match
        if(nBL == -1)
            return 0;
        
	}
	
	
	
	// make the recursion on the search tree
	// nbNodes memorizes the number of developped nodes
	// depth is the depth in the tree when the function is called
	// return the number of solutions found
	int recursion(int depth) {
		int nbL;
		short v1,lv1;
		short * v2;
		short nbV2;
		short i;
		short * saveL1;
		short * saveL2;
		int res = 0;
		nbNodes++;
		// compute the fix point of the dMax-IDL consistency
		nbL = fp->computeFixPoint(l1,l2);
		
		// if the labels does not match
		if(nbL == -1) {
			return 0;
		}
		
		// if there is one label by vertex
		if(nbL == g1->getOrder()) {
			m->setFromLabels(l1,l2);
			// if m is an isomorphism function
			if(m->isAnIsoFunction()) {
				memorizeCurrentMapping();
				return 1;
			} else { // m is not an isomorphism function
				return 0;
			}
		} else { // at least one label is used two times
			// we find the first vertex v1 having a label lv1 used more than one time
			v1 = 0;
			while(l1->getFreq(l1->getLabel(v1)) == 1) v1++;
			lv1 = l1->getLabel(v1);
			
			// we find the set of vertices of G2 having the label lv1
			v2 = new short[l1->getFreq(lv1)];
			nbV2 = 0;
			for(i = 0 ; i < g2->getOrder() ; i++) {
				if(lv1 == l2->getLabel(i)) {
					v2[nbV2] = i;
					nbV2++;
				}
			}
			
			// we save the labels
			saveL1 = new short[g1->getOrder()];
			saveL2 = new short[g2->getOrder()];
			l1->store(saveL1);
			l2->store(saveL2);
			
			// we give an unique label to v1 and v2 and we recurse
			for(i = 0 ; i < nbV2 ; i++) {
				l1->changeLabel(v1,l1->getNbLabels());
				l2->changeLabel(v2[i],l2->getNbLabels());
				//cout << depth << " " << nbNodes << " " << v1 << " " << v2[i] << endl;
				res+=recursion(nbNodes,depth+1);

				l1->restore(saveL1);
				l2->restore(saveL2);
			}
			delete [] saveL1;
			delete [] saveL2;
			delete [] v2;
		}
		
		// we return the number of solutions found
		return res;
	}
	
	~findIso(void) {
		delete fp;
		delete l1;
		delete l2;
		delete m;
	}
	
	protected :
	// ref to the two graphs
	Graph * g1;
	Graph * g2;
	
	// maximal distance
	short dMax;
	
	// the mapping used to control that we found an isomorphism
	mapping * m;
	
	// the current labelling functions
	Label * l1;
	Label * l2;
	
	// used to compute the ILL consistency
	fixPointTwoGraphs * fp;
	
	// ref to the vector of solution
	mapping ** sol;
	
	// number max of solutions to memorize
	short nbMaxSol;
	
	// number of solution found
	short nbFoundedSol;
	
	// memorize the current mapping m in sol
	void memorizeCurrentMapping(void) {
		if(nbFoundedSol < nbMaxSol) {
			sol[nbFoundedSol] = new mapping(m);
		}
		nbFoundedSol++;
	}
};


#endif
