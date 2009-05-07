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


#include <iostream>
#include <cstdlib>
#include <time.h>
#include <stdlib.h>

using namespace std;

#include "graph.h"
#include "findIso.h"
#include "distanceList.h"

// maximum number of isomorphism functions to find
#define NBSOLMAX 1

// ./Xecut graphefile1 graphfile2 maxdistance
int main(int argc, char *argv[])
{
	int dMax;
	
	// construction of the graphs
	Graph *gr1 = new Graph(argv[1]);
	Graph *gr2 = new Graph(argv[2]);
	sscanf(argv[3],"%i",&dMax);
	
	mapping ** sol = new mapping*[NBSOLMAX];
	

  findIso fi(gr1,gr2,dMax,sol,NBSOLMAX,true);

	clock_t startProgram,endProgram;
	double cpu_time_usedProgram;
	startProgram=clock();
	
	fi.init();
	int res;
	// we run the search
	res=fi.run();
	endProgram=clock();
	cpu_time_usedProgram = ((double) (endProgram-startProgram)) / CLOCKS_PER_SEC;
	cout << cpu_time_usedProgram << endl;


	delete gr1;
	delete gr2;
	
	int i;
		
	/*	
	for(i = 0 ; i < res ; i++) {
		sol[i]->display();
	}
	*/
	
	for(i = 0 ; i < res ; i++) {
		delete sol[i];
	}

	delete [] sol;
	// we diplay the number of solutions found (bounded by NBSOLMAX)
	cout << "Found " << res << " isomorphism(s)" << endl;


	
	return 0;
}

