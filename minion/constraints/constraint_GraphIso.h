/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef CONSTRAINT_GRAPH_ISO
#define CONSTRAINT_GRAPH_ISO

#include "GraphIsomorphism/src/myFindIso.h"

// The following class is based on 'findiso.h'

template<typename VarArray>
struct GraphIsoConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "GraphIso"; }
  
  typedef typename VarArray::value_type VarRef;
  
  VarArray var_array;
  
  TupleList* tuples;
  
  myFindIso* findIso;
  
  GraphIsoConstraint(StateObj* _stateObj, const VarArray& _vars, TupleList* _tuples) : AbstractConstraint(_stateObj),
    var_array(_var_array), tuples(_tuples)
  { 
    vector<set<int> > graph;
    graph.resize(vars.size());
    
    if(tuples->tuple_size() != 2)
    {
      cerr << "Graph Isomorphism only accepts binary graphs!" << endl;
      abort();
    }
    
    for(int i = 0; i < tuples->tuple_size(); ++i)
    {
      int left = tuples[i][0];
      int right = tuples[i][0];
      CHECK(left >= 0 && left < vars.size());
      CHECK(right >= 0 && right < vars.size());
      graph[left].insert(right);
      graph[right].insert(left);
    }  

    // TODO : Build graphs.
    Graph g1 = new Graph(graph);
    Graph g2 = new Graph(graph);
    findIso = new myFindIso(g1, g2, 2);
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    return t;
  }
  
  virtual void propagate(int prop_val, DomainDelta)
  {
      
  }
  
  
  
  virtual void full_propagate()
  {
  
  }
    
    virtual BOOL check_assignment(DomainInt* v, int v_size)
    {
      abort();
      return true;
    }
    
    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> vars;
      vars.reserve(var_array.size());
      for(unsigned i = 0; i < var_array.size(); ++i)
        vars.push_back(var_array[i]);
      return vars;
    }
    
    
   // Getting a satisfying assignment here is too hard, we don't want to have to
   // build a matching.
   virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
   {
     MAKE_STACK_BOX(c, DomainInt, var_array.size());
     abort();
   }
   
   
  };

template <typename T>
AbstractConstraint*
BuildCT_GRAPH_ISO(StateObj* stateObj,const T& t1, ConstraintBlob& b)
{ return GACTableCon(stateObj, t1, b.tuples); }

#endif
