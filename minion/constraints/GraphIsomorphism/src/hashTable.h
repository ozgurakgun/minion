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

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include<iostream>
using namespace std;

struct maillon {
	short indice;
	short next;
};


class hashTable{
	public :
	// create an hashtable of size tsize that can save at most tn different 
	// elements
	hashTable(int tn, int tsize) {
		n=tn;
		size=tsize;
		stockMaillon = new maillon[n];
		hash = new int[size];
		clear();
	}
	
	~hashTable() {
		delete [] stockMaillon;
		delete [] hash;
	}
	
	inline void clear(void) {
		int i;
		for(i = 0 ; i < size ; i++) {
			hash[i] = -1;
		}
		nbMaillonsUtilises = 0;
	}
	
	
	inline const int key(const short s, const short * v) {
		unsigned int sum = 0;
		int i;
		for(i = 0 ; i < s ; i++) {
			sum+=(3*sum+v[i]);
		}
		sum = sum%size;
		return sum;
	}
	
	inline int getFirstIndice(const short s, const short * v) {
		currentKey = key(s,v);
		currentMaillon = hash[currentKey];
		if(currentMaillon == -1) return -1;
		return stockMaillon[currentMaillon].indice;
	}
	
	inline int getNextIndice() {
		currentMaillon = stockMaillon[currentMaillon].next;
		return currentMaillon;
	}
	
	inline void insert(short ind) {
		stockMaillon[nbMaillonsUtilises].next=hash[currentKey];
		stockMaillon[nbMaillonsUtilises].indice=ind;
		hash[currentKey]=nbMaillonsUtilises;
		nbMaillonsUtilises++;
	}
	
	protected :
	int size;
	short n;
	maillon * stockMaillon;
	int nbMaillonsUtilises;
	int * hash;
	int currentMaillon;
	int currentKey;
};

#endif

