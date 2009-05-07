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

#ifndef NUMBERER_H
#define NUMBERER_H

#define NOTFOUND -1
#define ENDOFVECTOR -1

// this class give a number to each element of a set of vectors of positive 
// integer
class numberer{
	public : 
	// create a numberer that can number at most n vector of size s
	numberer(int n, int s){;};
	
	// destructor
	virtual ~numberer(void){;};
	
	// insert the vector v of size s
	virtual const short insert(const short s, const short * v)=0;
	
	// return the number of different elements
	virtual const short getNbOfDiffNumbers(void) const =0;
	
	// clear the numberer
	virtual void clear(void)=0;
	
	protected :
};

#endif
