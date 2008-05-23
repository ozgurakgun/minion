/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: minion.cpp 1393 2008-05-14 14:48:26Z caj $
*/
#include <boost/config.hpp>
#include <vector>
#include <list>
#include <boost/graph/biconnected_components.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <iterator>
#include <iostream>

	#define IN_MAIN

	#include "minion.h"
	#include "CSPSpec.h"

	using namespace ProbSpec;

	#include "BuildConstraint.h"

	#include "inputfile_parse.h"
	#include "commandline_parse.h"

	#include "system/defined_macros.h"
	
	
#include "build_graph.h"

#include "articulation_points.h"

#include "partition_graph.h"


template<typename T>
pair<map<T,int>, vector<T> > build_var_mapper(const set<T>& vars)
{
	map<T,int> m;
	vector<T> var_list;
	int p = 0;
	for(typename set<T>::iterator it = vars.begin(); it != vars.end(); ++it)
	{
		m[*it] = p;
		p++;	
		var_list.push_back(*it);
	}	
	
	return make_pair(m, var_list);
}


set<set<int> > replace_vars(const set<set<Var> >& hg, map<Var,int> mapvars)
{
	set<set<int> > out;
	
	for(set<set<Var> >::iterator it = hg.begin(); it != hg.end(); it++)
	{
		set<int> temp;
		for(set<Var>::iterator v1 = it->begin(); v1 != it->end(); v1++)
		{
			temp.insert(mapvars[*v1]);
		}
		out.insert(temp);
	}
	return out;
}

template<typename T>
struct Split
{
  Split* left;
  Split* right;
  set<T> split_set;
  set<T> vertices;
  set<pair<T, T> > base_graph;
  
  Split() : left(NULL), right(NULL)
  {}

  void print(const VarContainer& varCon, const vector<Var>& varlist, string space = "")
  {
    if(left && right)
    {
      cout << space << "Split: ";
      for(typename set<T>::iterator it = split_set.begin(); it != split_set.end(); ++it)
        cout << varCon.getName(varlist[*it]) << ":";
      cout << endl;
      cout << space << "Left:" << endl;
      left->print(varCon, varlist, space + " ");
      cout << endl;
      cout << space << "Right:" << endl;
      right->print(varCon, varlist, space + " ");
    }
    else
    {
      cout << space << "Base:";
       for(typename set<T>::iterator it = vertices.begin(); it != vertices.end(); ++it)
          cout << varCon.getName(varlist[*it]) << ":";
        cout << endl;
      cout << endl;
    }
    
  } 
   
  bool try_build_split(const set<int>& part_set)
  {   
    pair<set<int>, set<int> > split_verts = partition_vertices(base_graph, part_set);
    
    if(split_verts.first.empty() || split_verts.second.empty())
    {
      return false;
    }
    
    left = new Split<T>;
    right = new Split<T>;
    left->base_graph = project_edges(base_graph, split_verts.first);
    left->vertices = split_verts.first;
    right->base_graph = project_edges(base_graph, split_verts.second);
    right->vertices = split_verts.second;
    split_set = part_set;
    cout << "Split into " << split_verts.first.size() << ":" << split_verts.second.size() << endl;

    vertices = set<T>();
    base_graph = set<pair<T,T> >();
    
    return true;    
  }
};

template<typename T>
void make_split(Split<T>* split, vector<int> test_points = vector<int>())
{
  cout << "Start split" << endl;
  if(split->base_graph.empty())
  {
    cout << "Empty part" << endl;
    return;
  }
    
  while(!test_points.empty())
  {
    set<int> part_set;
    part_set.insert(test_points.back());
    test_points.pop_back();
    
    if(split->try_build_split(part_set))
    {
      make_split(split->left);
      make_split(split->right);
      return;
    }      
  }  
  
  vector<int> apoints = find_articulation_points(split->base_graph);
  if(!apoints.empty())
  {
    set<int> part_set;
    part_set.insert(apoints[apoints.size() / 2]);
    split->try_build_split(part_set); 
    make_split(split->left, apoints);
    make_split(split->right, apoints);      
  }  
}

int main(int argc, char** argv) {
  if (argc == 1) {
    return 1;
  }
    
  // filename, parser_verbose
  CSPInstance instance = readInputFromFile(argv[1], false); 
	VarContainer& varCon = instance.vars;
	
	set<set<Var> > hg = build_hypergraph(instance);
		
  set<Var> original_vars = gather_variables(hg);
  
	pair<map<Var, int>, vector<Var> > mapper_pair = build_var_mapper(original_vars);
	
	map<Var,int> mapper = mapper_pair.first;
	vector<Var> varlist = mapper_pair.second;
	
	set<set<int> > hg_int = replace_vars(hg, mapper);
	set<pair<int, int> > graph = make_graph_from_hypergraph(hg_int);
  
  set<int> vertices = gather_variables(graph);
	
	vector<int> apoints = find_articulation_points(graph);
	
  if(!apoints.empty() || vertices.size() != varlist.size())
  {
    cout << "Found " << apoints.size() << " in " << argv[1] << endl;
    cout << "Found " << varlist.size() - vertices.size() << " isolated vertices." << endl;   
    cout << varlist.size() << " variables" << endl;
    cout << hg.size() << " hyper edges" << endl;
    cout << graph.size() << " graph edges" << endl;

    vector<set<pair<int, int> > > components = find_biconnected_components(graph);
    
    for(int i = 0; i < components.size(); ++i)
    {
      if(!components[i].empty())
      {
        cout << gather_variables(components[i]).size () << ",";
      }
    }
    cout << endl;
   /* 
    Split<int> split;
    split.base_graph = graph;
    make_split(&split);
    
    split.print(varCon, varlist);
    */
   
	}
return 0; 
}

