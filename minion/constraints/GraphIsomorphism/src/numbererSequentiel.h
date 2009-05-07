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

#ifndef NUMBERERSEQUENTIEL_H
#define NUMBERERSEQUENTIEL_H

#define NOTFOUND -1
#define ENDOFVECTOR -1

#include "numberer.h"

// this class give a number to each element of a set of vectors of positive 
// integer
// This numberer does not order the vectors so insertion end finding is in 0(n)
// En pratique : c'est plus rapide!
class numbererSequentiel : public numberer{
	public : 
	// create a numberer that can number at most n vector of size s
	numbererSequentiel(int n, int s):numberer(n,s) {
		int i;
		nbMax = n;
		sMax=s;
		tab = new short*[sMax];
		for(i = 0 ; i < nbMax ; i++) {
			tab[i] = new short[sMax];
		}
		nbOfDiffNumbers = 0;
	};
	
	
	// destructor
	~numbererSequentiel(void) {
		int i;
		for(i = 0 ; i < nbMax ; i++) {
			delete [] tab[i];
		}
		delete [] tab;
	}
	
	
	// insert the vector v of size s
	inline const short insert(const short s, const short * v) {
		int j;
		if(s > sMax) {
			cout << "Erreur" << s << endl;
			abort();
		}
		int ind = findIndice(s,v);
		// if the vector is not found
		if(ind == NOTFOUND) {
			ind = nbOfDiffNumbers;
			nbOfDiffNumbers++;
			for(j = 0 ; j < s ; j++) {
				tab[ind][j] = v[j];
			}
			tab[ind][j] = ENDOFVECTOR;
		}
		return ind;
	};
	
	
	// return the number of different elements
	inline const short getNbOfDiffNumbers(void) const {return nbOfDiffNumbers;};
	
	// clear the numberer
	inline void clear(void) {
		nbOfDiffNumbers = 0;
	};
	
	protected :
	int nbMax; // maximum number of labels
	short ** tab; // the labels
	short nbOfDiffNumbers; // number of different labels
	int sMax;
	
	// return the indice of the vector v if found, return NOTFOUND
	// otherwise
	
	inline const short findIndice(short s, const short *v) const {
		int i,j;
		i = 0;
		while(i < nbOfDiffNumbers) {
			j = 0;
			while((j < s) && (tab[i][j] == v[j])) j++;
			if((j == s) && (tab[i][j] == -1)) {
				return i;
			}
			i++;
		}
		return NOTFOUND;
	}
	
};

#endif
