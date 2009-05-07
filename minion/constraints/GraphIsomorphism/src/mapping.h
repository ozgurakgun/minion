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

#ifndef MAPPING_H
#define MAPPING_H

#include "graph.h"

#define forEachVertex(v) for(v = 0 ; v < size ; v++)

class mapping {
	public :
	// constructor
	mapping(Graph * tg1, Graph * tg2) {
		g1=tg1; g2=tg2;
		size = g1->getOrder();
		m = new short[size];
		temp = new short[size];
	}
	
	// recopy constructor
	mapping(mapping * toC) {
		g1 = toC->g1; g2 = toC->g2; size = toC->size;
		m = new short[size];
		temp = new short[size];
		int i;
		for(i = 0 ; i < size ; i++) {
			m[i] = toC->m[i];
		}
	}
	
	// destructor
	~mapping(void) {
		delete [] m;
		delete [] temp;
	}
	
	// from two BIJECTIVE labelling functions (for resp. g1 and g2), find the 
	// corresponfing mapping between the two graphs
	void setFromLabels(Label *l1, Label *l2) {
		int v;
		// compute l2^{-1}
		forEachVertex(v) {
			temp[l2->getLabel(v)] = v;
		}
		
		// build the mapping
		forEachVertex(v) {
			m[v] = temp[l1->getLabel(v)];
		}
		return;
	}
	
	
	
	// check if the mapping is an isomorphic function
	// suppose that the mapping is a bijection and that the graphs have
	// the same number of edges!!!
	int isAnIsoFunction(void) {
		int u,i;
		forEachVertex(u) {
			for(i = 0 ; i < g1->getDegree(u) ; i++) {
				if(!(g2->isAnEdge(m[u],m[g1->getIThSucc(u,i)]))) {
					return 0;
				}
			}
		}
		return 1;
	}
	
	void display(void) {
		int v;
		forEachVertex(v) {
			cout << v << ":" << m[v] << " ";
		}
		cout << endl;
	}
	
	const short getSize(void) const {return size;}
	const short getImage(const short v) const {return m[v];}
	
	protected :
	// ref to the two graphs
	Graph * g1;
	Graph * g2;

	// size of the mapping (order of the graphs)
	int size;	
	// the mapping
	short * m;
	// a temporary set
	short * temp;
};


#endif
