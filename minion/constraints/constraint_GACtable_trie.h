/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/* Minion
* Copyright (C) 2006
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

#include "tries.h"
#include "../system/rapq.h"

template<typename VarArray, int negative>
struct GACTableConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "TableTrie"; }

  typedef typename VarArray::value_type VarRef;
  VarArray vars;
  
  TupleTrieArray* tupleTrieArrayptr;
  
  //Following is setup globally in constraint to be passed by reference & recycled
  int* recyclableTuple;
  
  /// For each literal, the number of the tuple that supports it.
  //   renamed off from current_support in case both run in parallel
  vector<TrieObj**> trie_current_support;
  
  /// Check if all allowed values in a given tuple are still in the domains of the variables.
  BOOL check_tuple(const vector<int>& v)
  {
	for(unsigned i = 0; i < v.size(); ++i)
	{
	  if(!vars[i].inDomain(v[i]))
		return false;
	}
	return true;
  }
    
  TupleList* tuples;
  
#ifndef NAIVENOGOOD
  struct varval { //variable index and value
    unsigned var;
    DomainInt val;
    varval(unsigned _var, DomainInt _val) : var(_var), val(_val) {}
    varval() : var(0), val(0) {}
    bool operator==(const varval& b) const
    { return var == b.var && val == b.val; }
    bool operator!=(const varval& b) const
    { return !(operator==(b)); }
    bool operator<(const varval& b) const
    { return var < b.var || (var == b.var && val < b.val); }
  };

  struct varvalData {
    unsigned occurences; //number of remaining occurences in uncovered tuples
    vector<varval> neighbours; //all pruned varvals that share a support with the varval
    varvalData() : occurences(0), neighbours() {}
    bool operator<(const varvalData& b) const
    { return occurences < b.occurences; }
  };
  
  RandomAccessPriorityQ<varval, varvalData> rapq;
#endif
  
  GACTableConstraint(StateObj* _stateObj,const VarArray& _vars, TupleList* _tuples) :
    AbstractConstraint(_stateObj), vars(_vars), tuples(_tuples)
#ifndef NAIVENOGOOD
       , rapq(_stateObj)
#endif
  { 
    tupleTrieArrayptr = tuples->getTries();
	int arity = tuples->tuple_size();	  
	D_ASSERT(_vars.size() == arity);
	  
	trie_current_support.resize(tuples->literal_num); 
	for(int i = 0; i < tuples->literal_num; ++i)
	{
	  trie_current_support[i] = new TrieObj*[arity];
	  for(int j = 0; j < arity; j++)
		trie_current_support[i][j] = NULL;
	}
	// initialise supportting tuple for recycle
	recyclableTuple = new int[arity] ;
  }
  
  int dynamic_trigger_count()
  { return tuples->literal_num * ( vars.size() - 1) ; }
  
  BOOL find_new_support(int literal)
  {
     pair<int,int> varval = tuples->get_varval_from_literal(literal);
	 int varIndex = varval.first;
	 int val = varval.second;
     if(negative==0)
     {
         int new_support = 
           tupleTrieArrayptr->getTrie(varIndex).
                                nextSupportingTuple(val ,vars, trie_current_support[literal]);
         if (new_support < 0)
         { // cout << "find_new_support failed literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl ;
             return false;
         }
     }
     else
     {
         int new_support = 
           tupleTrieArrayptr->getTrie(varIndex).
                                nextSupportingTupleNegative(val, vars, trie_current_support[literal], recyclableTuple);
         if (new_support < 0)
         { // cout << "find_new_support failed literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl ;
             return false;
         }
     }
         // cout << "find_new_support sup= "<< new_support << " literal: " << literal << " var: " << varIndex << " val: " << get_val_from_literal(literal) << endl;
         //trie_current_support[literal] = new_support; 
     return true;
  }

  //given depth in trie, and the variable index that is at the first depth,
  //returns the index of the variable at that depth
  size_t map_depth(size_t depth, size_t markedVar) {
    return depth == 0 ? markedVar : (depth <= markedVar ? depth - 1 : depth);
  }   
  
#ifdef NAIVENOGOOD
  
  //traverses <trie> in order to add a pruned value per support to <label>
  template<typename VarArr>
  void getLabel(VarArr& vars, TrieObj* trie, size_t depth, 
		       size_t varIndex, label& l)
  {
    //algorithm: Carry out traversal of trie, but backtrack when a pruned edge
    //is found. This means we have a pruned value for every tuple, i.e., a
    //pruning to cover each lost support.
    while(trie->val != MAXINT) {
      if(!vars[map_depth(depth, varIndex)].inDomain(trie->val)) {
	const int md = map_depth(depth, varIndex);
	l.push_back(literal(false, 
			    vars[md].getBaseVar(), 
			    vars[md].getBaseVal(trie->val)));
      } else {
	getLabel(vars, trie->offset_ptr, depth + 1, varIndex, l);
      }
      trie++;
    }
  }

  template<typename VarArr>
    label getLabel(VarArr& vars, int varIndex, DomainInt val)
  {
    //pseudocode: Carry out traversal of subtrie involving (varIndex,val).
    label l;
    TupleTrie tt = tupleTrieArrayptr->getTrie(varIndex); //get trie with correct variable at depth 0
    TrieObj* to_a = tt.trie_data;
    while(to_a->val != val) to_a++; //find the node with the correct val
    getLabel(vars, to_a->offset_ptr, 1, varIndex, l); //traverse its subtrie
    return l;
  }

#else

  template<typename VarArr>
    label buildLabel(VarArr& vars)
    {
      label l;
      //in this loop when a varval is added it will be removed, but when it merely ends
      //up in no uncovered tuples it will be 0
      while(rapq.size()) { //not empty => uncovered tuples
	varval max_varval = rapq.getMaxKey();
	varvalData& max_vvd = rapq.getData(max_varval);
	rapq.removeMax();
	if(max_vvd.occurences == 0) break; //all supports already covered
	l.push_back(literal(false, 
			    vars[max_varval.var].getBaseVar(), 
			    vars[max_varval.var].getBaseVal(max_varval.val)));
	vector<varval>& neighbours = max_vvd.neighbours;
	const size_t neighbours_s = neighbours.size();
	for(size_t i = 0; i < neighbours_s; i++) { //reduce occurence count of all neighbours
	  varval& n = neighbours[i];
	  if(n != max_varval) {
	    rapq.getData(n).occurences--;
	    rapq.fixOrder();
	  }
	}
	max_vvd.occurences = 0; //now commonest is covered
      }
      return l;
    }

  //setup varvalInfo and occurencesHeap by traversing trie
  //prunings is the set of pruned values on the current path in the trie, trie is the
  //current point in the trie, vars are just the vars the trie represents, depth is the
  //depth of the supplied node, varIndex is the index in <vars> of the variable at depth 0
  template<typename VarArr>
    void setupLabels(VarArr& vars, vector<varval>& prunings, TrieObj* trie, unsigned depth,
		     unsigned varIndex)
    {
      if(depth == vars.size()) { //reached end of a support in trie
	const size_t prunings_s = prunings.size();
	D_ASSERT(prunings_s != 0); //any tuple must have a pruning in it, else contradiction
	for(size_t i = 0; i < prunings_s; i++) {
	  rapq.checkAdd(prunings[i], varvalData()); //if literal not present, add it
	  varvalData& pruning_d = rapq.getData(prunings[i]);
	  pruning_d.occurences += 1;
	  pruning_d.neighbours.insert(pruning_d.neighbours.end(), prunings.begin(), prunings.end());
	}
      } else {
	while(trie->val != MAXINT) {
	  if(!vars[map_depth(depth, varIndex)].inDomain(trie->val)) {
	    prunings.push_back(varval(map_depth(depth, varIndex), trie->val));
	    setupLabels(vars, prunings, trie->offset_ptr, depth + 1, varIndex);
	    prunings.pop_back();
	  } else {
	    setupLabels(vars, prunings, trie->offset_ptr, depth + 1, varIndex);
	  }
	  trie++;
	}
      }
    }

  template<typename VarArr>
    label getLabel(VarArr& vars, unsigned varIndex, DomainInt val)
    {
      rapq.clear();
      TupleTrie tt = tupleTrieArrayptr->getTrie(varIndex);
      TrieObj* to_a = tt.trie_data;
      while(to_a->val != val) to_a++;
      vector<varval> v;
      setupLabels(vars, v, to_a->offset_ptr, 1, varIndex);
      rapq.repair();
      return buildLabel(vars);
    }
  
#endif
  
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* propagated_trig)
  {
	PROP_INFO_ADDONE(DynGACTable);
	D_INFO(1, DI_TABLECON, "Propagation Triggered: " + to_string(propagated_trig));
	DynamicTrigger* dt = dynamic_trigger_start();
	int trigger_pos = propagated_trig - dt;
	int propagated_literal = trigger_pos / (vars.size() - 1);
	
	BOOL is_new_support = find_new_support(propagated_literal);
	
	pair<int,int> varval = tuples->get_varval_from_literal(propagated_literal);
	int varIndex = varval.first;
	int val = varval.second;
	
	if(is_new_support)
	{
	  D_INFO(1, DI_TABLECON, "Found new support!");
	  setup_watches(varIndex, propagated_literal);
	}
	else
	{
	  D_INFO(1, DI_TABLECON, "Failed to find new support");
	  vars[varIndex].removeFromDomain(val, getLabel(vars, varIndex, val));
	}
  }
  
  void setup_watches(int var, int lit)
  {
    // cout << "setup_watches lit= "<< lit << endl ; cout << "calling reconstructTuple from setup_watches" << endl ; 
    if(negative==0)
    {
        tupleTrieArrayptr->getTrie(var).reconstructTuple(recyclableTuple,trie_current_support[lit]);
    }
    // otherwise, the support is already in recyclableTuple. 
    
    // cout << "  " << var << ", literal" << lit << ":";
    // for(int z = 0; z < vars.size(); ++z) cout << recyclableTuple[z] << " "; cout << endl;
    
	DynamicTrigger* dt = dynamic_trigger_start();
	
	int vars_size = vars.size();
	dt += lit * (vars_size - 1);
	for(int v = 0; v < vars_size; ++v)
	{
	  if(v != var)
	  {
		D_ASSERT(vars[v].inDomain(recyclableTuple[v]));
		vars[v].addDynamicTrigger(dt, DomainRemoval, recyclableTuple[v]);
		++dt;
	  }
	}
  }
  
  //for the moment I am re-defining this function to be AT ROOT NODE only, i.e., -fullprop is broken!
  virtual void full_propagate()
  {
      D_INFO(2, DI_TABLECON, "Full prop");
      if(negative==0 && tuples->size()==0)
      {   // it seems to work without this explicit check, but I put it in anyway.
          getState(stateObj).setFailed(true);
          return;
      }
      for(int varIndex = 0; varIndex < vars.size(); ++varIndex) 
      {
	    if(negative==0)
        {
            vars[varIndex].setMin((tuples->dom_smallest)[varIndex], label());
            vars[varIndex].setMax((tuples->dom_smallest)[varIndex] + (tuples->dom_size)[varIndex], label());
		}
        
		if(getState(stateObj).isFailed()) return;
		
        DomainInt max = vars[varIndex].getMax();
        for(DomainInt i = vars[varIndex].getMin(); i <= max; ++i) 
        {
            if(i>= (tuples->dom_smallest)[varIndex] 
                && i<=(tuples->dom_smallest)[varIndex] + (tuples->dom_size)[varIndex])
            {
                int literal = tuples->get_literal(varIndex, i);
                
                int sup;
                if(negative==0)
                {
                    sup = tupleTrieArrayptr->getTrie(varIndex).       
                        nextSupportingTuple(i, vars, trie_current_support[literal]);
                }
                else
                {
                    sup = tupleTrieArrayptr->getTrie(varIndex).       
                        nextSupportingTupleNegative(i, vars, trie_current_support[literal], recyclableTuple);
                }
                
                //trie_current_support[literal] = sup;
                // cout << "    var " << varIndex << " val: " << i << " sup " << sup << " " << endl;
                if(sup < 0)
                {
                  D_INFO(2, DI_TABLECON, "No valid support for " + to_string(i) + " in var " + to_string(varIndex));
                  //cout <<"No valid support for " + to_string(i) + " in var " + to_string(varIndex) << endl;
                  //volatile int * myptr=NULL;
                  //int crashit=*(myptr);
                  vars[varIndex].removeFromDomain(i, label());
                }
                else
                {
                  setup_watches(varIndex, literal);
                }
            }
            else
            {
                D_ASSERT(negative==1);
                // else: if the literal is not contained in any forbidden tuple, then it is 
                // not necessary to find a support for it or set watches. The else case
                // only occurs with negative tuple constraints. 
            }
        }
      }
      // cout << endl; cout << "  fp: finished finding supports: " << endl ;
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    if(negative==0)
    {
        for(unsigned i = 0; i < tuples->size(); ++i)
        {
          if( std::equal(v, v + v_size, (*tuples)[i]) )
            return true;
        }
        return false;
    }
    else
    {
        for(unsigned i = 0; i < tuples->size(); ++i)
        {
          if( std::equal(v, v + v_size, (*tuples)[i]) )
            return false;
        }
        return true;
    }
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> anyvars;
    for(unsigned i = 0; i < vars.size(); ++i)
	  anyvars.push_back(vars[i]);
	return anyvars;
  }
};


template<typename VarArray>
AbstractConstraint*
GACTableCon(StateObj* stateObj, const VarArray& vars, TupleList* tuples)
{ return new GACTableConstraint<VarArray, 0>(stateObj, vars, tuples); }

template<typename VarArray>
AbstractConstraint*
GACNegativeTableCon(StateObj* stateObj, const VarArray& vars, TupleList* tuples)
{ return new GACTableConstraint<VarArray, 1>(stateObj, vars, tuples); }


inline TupleTrieArray* TupleList::getTries()
{
  if(triearray == NULL)
    triearray = new TupleTrieArray(this);
  return triearray;
}

