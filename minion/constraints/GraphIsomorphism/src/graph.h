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

#ifndef GRAPH_H
#define GRAPH_H

#define ForEachVertex(u) for(u = 0 ; u < nbVertices ; u++)
#define ForEachVertexOfG(g,u) for(u = 0 ; u < g->getOrder() ; u++)
#define Degree(u) nbSucc[u]
#define IThSucc(u,i) list[u][i]

#include <iostream>
#include <fstream>
#include <vector>
#include <set>


class Graph {
	public :
	// load a graph from file filename
	// nauty txt format by default
	// other possible format is "dac"
	
	Graph(const std::vector<std::set<short> >& adjacencyMap)
	{
    nbVertices = adjacencyMap.size();
    nbEdges = 0;
    init();
    
    for(int i = 0; i < nbVertices; i++)
    {
      Degree(i) = adjacencyMap.size();
      list[i] = new short[Degree(i)];
      int pos = 0;
      for(std::set<short>::const_iterator it = adjacencyMap[i].begin(); it != adjacencyMap[i].end(); ++it, ++pos)
        list[i][pos] = *it;
      assert(pos == Degree(i));
      nbEdges += Degree(i);
    }
	}
	
	Graph(const char * filename,const char * format = NULL) {
		if(format == NULL) {
			int i,u,v;		
			int tempNbSucc;
			nbEdges=0;
			
			FILE * in = fopen(filename,"r");
			if(in == NULL) {
				cerr << "Can't open file " << filename << endl;
				exit(-1);
			}
			
			fscanf(in,"\nGraph %d, order %d.\n",&u,&i);
			nbVertices = (short int)i;
			init();
			for(i = 0 ; i < nbVertices ; i++) {
				tempNbSucc = 0;
				fscanf(in, "%d : ",&i);
				v = fscanf(in,"%d",&u);
				while(v > 0) {
					temp[tempNbSucc] = u;
					tempNbSucc++;
						nbEdges++;
					v = fscanf(in,"%d",&u);
				}
				fscanf(in,";");
				
				Degree(i)=tempNbSucc;
				list[i] = new short[Degree(i)];
				for(v = 0 ; v < Degree(i) ; v++) {
					list[i][v] = temp[v];
				}
			}
			fclose(in);
		} 
	};
	
	// build a new graph isomorphic to G2
	// vertices correspondance are done by the vector t
	Graph(const Graph * G, short * t) {
		int u,j;
		nbVertices = G->getOrder();
		nbEdges = 0;
		init();
		for(u = 0 ; u < nbVertices ; u++) {
			list[t[u]] = new short[G->getDegree(u)];
			Degree(t[u]) = G->getDegree(u);
			for(j = 0 ; j < G->getDegree(u) ; j++) {
				nbEdges++;
				list[t[u]][j] = t[G->getIThSucc(u,j)];
			}
		}
	}
	
	// destructor
	~Graph(void) {
		int i;
		ForEachVertex(i) {
			delete [] list[i];
		}
		delete [] list;
		delete [] nbSucc;
		delete [] temp;
	};
	
	void print(void) {
		int i,j;
		for(i = 0 ; i < nbVertices ; i++) {
			cout << "V" << i << " : ";
			for(j = 0 ; j < Degree(i) ; j++) {
				cout << list[i][j] << " ";
			}
			cout << endl;
		}
	}
	
	void printSaucy(void) {
		cout << nbVertices << " " << nbEdges << " 1" << endl;
		int i,j;
		for(i = 0 ; i < nbVertices ; i++) {
			for(j = 0; j < Degree(i) ; j++) {
					cout << i << " " << list[i][j] << endl;
			}
		}
	}
	
	// return the number of neighbours of the vertex v
	inline const short getDegree(const short v) const {return Degree(v);};
	
	// return the id of the ith neighbour of v
	// arbitrary order, used to iterate over the neighbours
	inline const short getIThSucc(const short v, const short i) const {return IThSucc(v,i);};
	
	// return the number of vertices of the graph
	inline const short getOrder(void) const {return nbVertices;};
	
	// return the number of edges of the graph
	inline const int getNbEdges(void)const  {return nbEdges;};
	
	inline const bool isAnEdge(const short u, const short v) const {
		for(int i = 0 ; i < getDegree(u) ; i++) {
			if(getIThSucc(u,i) == v) return true;
		}
		return false;
	}
	
	protected :
	
	// number of vertices of the graph
	short nbVertices; 
	
	// number of edges of the graph
	int nbEdges;
	
	// for each vertex, the list of this successor
	short **list;
	
	// number of successor of a vertex
	short * nbSucc;
	
	// adjacency matrix (true, false)
	//bool ** adjMatrice;
	short * temp;
	
	// memory allocation
	void init(void) {
		int i;
		list = new short*[nbVertices];
		nbSucc = new short[nbVertices];
		ForEachVertex(i) {
			nbSucc[i] = 0;
		}
		temp = new short[nbVertices];
	};
};

#endif
