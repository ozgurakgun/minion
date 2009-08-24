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

#ifndef _TRIES_H_INCLUDE_15243
#define _TRIES_H_INCLUDE_15243

#include <numeric>

#include <vector>
#include <algorithm>
#include <cassert>

using namespace std;


struct TupleComparator
{
  int significantIndex;
  int arity;
  
  TupleComparator(int i, int a)
  {
    
    significantIndex = i;
    arity = a; 
  }
  
  // returns tuple1 <= tuple2 under our ordering.
  bool operator()(const vector<int>& tuple1, const vector<int>& tuple2)
  {
    if(tuple1[significantIndex] != tuple2[significantIndex])
      return tuple1[significantIndex] < tuple2[significantIndex];
    for(int tupleIndex = 0; tupleIndex < arity; tupleIndex++)
    {
      if(tuple1[tupleIndex] != tuple2[tupleIndex])
        return tuple1[tupleIndex] < tuple2[tupleIndex];
    }
    return false;
  }
};

struct TrieObj
{
  int val;
  TrieObj* offset_ptr;
};

struct TupleTrie
{
  static const int max_arity = 100;
  int arity;
  int sigIndex;
  TupleList* tuplelist;
  // A temporary tuple to store the most recently found
  // complete assignment.
  int current_tuple[max_arity];
  StateObj* stateObj;
  
  vector<vector<int> > tuples_vector;
  
  int map_depth(int depth)
  {
    if(depth==0) return sigIndex;
    if(depth <= sigIndex) return depth - 1;
    return depth;
  }
  
  int map_varno(int varno) //inverse of map_depth (varno->depth in trie)
  {
    if(varno == sigIndex) return 0;
    if(varno < sigIndex) return varno + 1;
    return varno;
  }
  
  int tuples(int num, int depth)
  { return tuples_vector[num][map_depth(depth)]; }
  
  
  TupleTrie(int _significantIndex, TupleList* tuplelist, StateObj* _stateObj) :
  arity(tuplelist->tuple_size()), sigIndex(_significantIndex), stateObj(_stateObj)
  {
    // TODO : Fix this hard limit.
    D_ASSERT(arity < 100);
    trie_data=NULL;
    tuples_vector.resize(tuplelist->size());

    // Need a copy so we can sort it and such things.
    for(int i = 0; i < tuplelist->size(); ++i)
      tuples_vector[i] = tuplelist->get_vector(i);

    std::stable_sort(tuples_vector.begin(), tuples_vector.end(), TupleComparator(sigIndex, arity));
    if(tuplelist->size()>0)
    {
      build_trie(0, tuplelist->size());
      build_final_trie();
    }
  }
  
  struct EarlyTrieObj
  {
    int val;
    int depth;
    int offset_ptr;
  };
  
  vector<EarlyTrieObj> initial_trie;
  
  TrieObj* trie_data;  // This is the actual trie used during search.
  
  void build_final_trie()
  {
    int size = initial_trie.size();
    trie_data = new TrieObj[size];
    for(int i = 0; i < size; ++i)
    {
      trie_data[i].val = initial_trie[i].val;
      if(initial_trie[i].offset_ptr == -1)
        trie_data[i].offset_ptr = NULL;
      else
        trie_data[i].offset_ptr = trie_data + initial_trie[i].offset_ptr;
    }
    
    // Little C++ trick to remove all the memory used by the initial_trie vector.
    vector<EarlyTrieObj> v;
    initial_trie.swap(v);
  }
  
  void print_trie()
  {
    for(int i = 0; i < initial_trie.size(); ++i)
    {
      printf("%d,%d,%d\n", initial_trie[i].depth, initial_trie[i].val, initial_trie[i].offset_ptr);
    }
  }
  
  void build_trie(const int start_pos, const int end_pos, int depth = 0)
  {
    const bool last_stage = (depth == arity - 1);
    if(depth == arity)
      return;
    
    assert(start_pos <= end_pos);
    int values = get_distinct_values(start_pos, end_pos, depth);
    
    int start_section = initial_trie.size();
    // Make space for this list of values.
    // '+1' is for end marker.
    initial_trie.resize(initial_trie.size() + values + 1);
    
    int current_val = tuples(start_pos, depth);
    int current_start = start_pos;
    int num_of_val = 0;
    
    for(int i = start_pos ; i < end_pos; ++i)
    {
      if(current_val != tuples(i, depth))
      {
        initial_trie[start_section + num_of_val].val = current_val;
        initial_trie[start_section + num_of_val].depth = depth;
        if(last_stage)
        {
          initial_trie[start_section + num_of_val].offset_ptr = -1;
        }
        else
        {
          initial_trie[start_section + num_of_val].offset_ptr = initial_trie.size();
          build_trie(current_start, i, depth + 1);
        }
        current_val = tuples(i, depth);
        current_start = i;
        num_of_val++;
      }
    }
    
    // Also have to cover last stretch of values.
    initial_trie[start_section + num_of_val].val = current_val;
    initial_trie[start_section + num_of_val].depth = depth;
    if(last_stage)
      initial_trie[start_section + num_of_val].offset_ptr = -1;
    else  
      initial_trie[start_section + num_of_val].offset_ptr = initial_trie.size();
    
    build_trie(current_start, end_pos, depth + 1);
    
    assert(num_of_val + 1 == values);
    initial_trie[start_section + values].val = DomainInt_Max;
    initial_trie[start_section + values].depth = depth;
    initial_trie[start_section + values].offset_ptr = -1;
  }
  
  // Find how many values there are for index 'depth' between tuples
  // start_pos and end_pos.
  int get_distinct_values(const int start_pos, const int end_pos, int depth)
  {
    int current_val = tuples(start_pos, depth);
    int found_values = 1;
    for(int i = start_pos; i < end_pos; ++i)
    {
      if(current_val != tuples(i, depth))
      {
        current_val = tuples(i, depth);
        found_values++;
      }
    }
    return found_values;
  }

  
  // Starting from the start of an array of TrieObjs, find the
  // values which is find_val
  TrieObj* get_next_ptr(TrieObj* obj, DomainInt find_val)
  {
    while(obj->val < find_val)
      ++obj;
    if(obj->val == find_val)
      return obj;
    else
      return NULL;
  }
 

  
  template<typename ConflictChecker, typename VarArray>
    bool search_trie(const ConflictChecker& checker, const VarArray& _vars, TrieObj** obj_list, int depth)
  {
    cout << "in search_trie" << endl;
    CON_INFO_ADDONE(SearchTrie);
    VarArray& vars = const_cast<VarArray&>(_vars);
    if(depth == arity)
      return true;
    
    if(depth != 0) //don't set up depth 0 starting position, assume it is set up correctly on entry
      obj_list[depth] = obj_list[depth - 1]->offset_ptr;
    
    cout << "trying " << obj_list[depth]->val << " in d" << depth << ",v" << vars[map_depth(depth)] << " as support" << endl;
    while(obj_list[depth]->val != DomainInt_Max)
    {
      cout << "d=" << depth << ",v=" << obj_list[depth]->val << endl;
      if(vars[map_depth(depth)].inDomain(obj_list[depth]->val))
      {
	cout << "indomain" << endl;
        if(checker.check(stateObj, vars, depth, obj_list) && search_trie(checker, _vars, obj_list, depth + 1))
          return true;
      }
      obj_list[depth]++;
    }
    cout << "no support found" << endl;
    return false;
  }
  
  // search_trie_negative searches for a tuple which is not in the trie. 
  // For the negative table constraint.
  template<typename VarArray>
    bool search_trie_negative(const VarArray& _vars, TrieObj** obj_list, int depth, DomainInt* returnTuple)
  {
    CON_INFO_ADDONE(SearchTrie);
    VarArray& vars = const_cast<VarArray&>(_vars);
    if(depth == arity)
      return false;
    
    obj_list[depth] = obj_list[depth - 1]->offset_ptr;
    
    int dep=map_depth(depth);
    for(int i=vars[dep].getMin(); i<=vars[dep].getMax(); i++)
    {
        if(vars[dep].inDomain(i))
        {
            while(obj_list[depth]->val < i) obj_list[depth]++;
            returnTuple[dep]=i;
            if(obj_list[depth]->val > i)
            {   // includes case where val is maxint.
                // if the value is in the domain but not in the trie, we are nearly finished.
                // Just need to fill in the rest of returnTuple.
                // Is there any need to search from the previous position?? Yes. But not doing so yet.
                for(int depth2=depth+1; depth2<arity; depth2++) returnTuple[map_depth(depth2)]=vars[map_depth(depth2)].getMin();
                return true;
            }
            else
            {
                if(search_trie_negative(_vars, obj_list, depth+1, returnTuple))
                    return true;
            }
        }
    }
    return false;
  }
  
  void reconstructTuple(DomainInt* array, TrieObj** obj_list)
  {
    //D_ASSERT(check == obj_list[arity - 1] - obj_list[0]);
    for(int i = 0; i < arity; ++i)
      array[map_depth(i)] = obj_list[i]->val;
  }
  
  //start is a pointer to the very beginning of the trie, meaning this procedure can be used to search the entire trie
  //and not just from depth 1.
  template<typename ConflictChecker, typename VarArray>
    bool loop_search_trie(const ConflictChecker& checker, const VarArray& _vars, TrieObj** obj_list, int depth, TrieObj* start = NULL)
  {
    cout << "in loop_search_trie" << endl;
    D_ASSERT(depth || start);
    CON_INFO_ADDONE(LoopSearchTrie);
      VarArray& vars = const_cast<VarArray&>(_vars);
      if(depth == arity) {
	cout << "d=arity" << endl;
        return true;
      }
      
      if(obj_list[depth]->val == DomainInt_Max)
        return search_trie(checker, _vars, obj_list, depth);
          
      if(vars[map_depth(depth)].inDomain(obj_list[depth]->val))
      {
	cout << "applying checker and looking" << endl;
        if(checker.check(stateObj, vars, depth, obj_list) && loop_search_trie(checker, _vars, obj_list, depth + 1))
          return true;
	cout << "didn't pass or didn't find" << endl;
      }
      
      TrieObj* initial_pos = obj_list[depth]; 
      cout << "initial_pos=" << initial_pos << endl;
      cout << "start=" << start << endl;
      
      obj_list[depth]++;
      while(obj_list[depth]->val != DomainInt_Max)
      {
        if(vars[map_depth(depth)].inDomain(obj_list[depth]->val))
        {
	  if(checker.check(stateObj, vars, depth, obj_list) && search_trie(checker, _vars, obj_list, depth + 1))
	    return true;
        }
        obj_list[depth]++;
      }

      cout << "got to end, restarting" << endl;
      
      if(depth != 0)
	obj_list[depth] = obj_list[depth - 1]->offset_ptr;
      else 
	obj_list[0] = start;
      
      while(obj_list[depth] != initial_pos)
      {
        if(vars[map_depth(depth)].inDomain(obj_list[depth]->val))
        {
	  if(checker.check(stateObj, vars, depth, obj_list) && search_trie(checker, _vars, obj_list, depth + 1))
	    return true;
        }
        obj_list[depth]++;
      }
      return false;
  }

  class DefaultChecker {
    TupleTrie* parent;
    
  public:
    DefaultChecker(TupleTrie* _parent) : parent(_parent) {}
    
    template<typename VarArray>
      bool check(StateObj* stateObj, const VarArray& vars, int depth, TrieObj** trie_obj) const
    { 
      Var curr_base_var = vars[parent->map_depth(depth)].getBaseVar();
      for(int prev_d = 0; prev_d < depth; prev_d++) {
	bool areEqual = trie_obj[prev_d]->val == trie_obj[depth]->val;
	cout << "pair " << trie_obj[prev_d]->val << "," << trie_obj[depth]->val << endl;
	if((areEqual && ARE_DISEQUAL(stateObj, vars[parent->map_depth(prev_d)].getBaseVar(), curr_base_var))
	   || (!areEqual && ARE_EQUAL(stateObj, vars[parent->map_depth(prev_d)].getBaseVar(), curr_base_var))) { 
	  cout << "found conflict" << endl;
	  return false;
	}
      }
      return true;
    }
  };
  friend class DefaultChecker; //allow nested class to access methods for its parent object
  
  class EqualChecker {
    TupleTrie* parent;
    int f; //already mapped to depth in trie
    int s;
    
  public:
  EqualChecker(TupleTrie* _parent, int _f, int _s) : parent(_parent), f(_f), s(_s) { D_ASSERT(s == 0); }
    
    template<typename VarArray>
    bool check(StateObj* stateObj, const VarArray& vars, int depth, TrieObj** trie_obj) const
    {
      D_ASSERT(s < f);
      if(depth == s)
	return vars[parent->map_depth(f)].inDomain(trie_obj[s]->val);
      else if(depth == f)
	return trie_obj[s]->val == trie_obj[f]->val;
      else
	return DefaultChecker(parent).check(stateObj, vars, depth, trie_obj);
    }
  };
  friend class EqualChecker; //allow nested class to access methods for its parent object

  class DisequalChecker {
    TupleTrie* parent;
    int f; //already mapped to depth in trie
    int s;
    
  public:
  DisequalChecker(TupleTrie* _parent, int _f, int _s) : parent(_parent), f(_f), s(_s) { D_ASSERT(s == 0); }
    
    template<typename VarArray>
    bool check(StateObj* stateObj, const VarArray& vars, int depth, TrieObj** trie_obj) const
    {
      D_ASSERT(s < f);
      if(depth == s && vars[parent->map_depth(f)].isAssigned()) {
	cout << "b1" << endl;
	return trie_obj[s]->val != vars[parent->map_depth(f)].getAssignedValue();
      } else if(depth == f) {
	cout << "b2" << endl;
	return trie_obj[s]->val != trie_obj[f]->val;
      } else {
	cout << "b3" << endl;
	return DefaultChecker(parent).check(stateObj, vars, depth, trie_obj);
      }
    }
  };
  friend class DisequalChecker; //allow nested class to access methods for its parent object
  
  // Find support for domain value i. This will be the value used by
  // the first variable.
  template<typename VarArray>
    int nextSupportingTuple(DomainInt domain_val, const VarArray& _vars, TrieObj** obj_list)
  {
    cout << "in nextSupportingTuple" << endl;
    if(trie_data == NULL)
      return -1;
      
    VarArray& vars = const_cast<VarArray&>(_vars);
    DefaultChecker dc = DefaultChecker(this);

    if(obj_list[0] == NULL)
    {
      TrieObj* first_ptr = get_next_ptr(trie_data, domain_val);
      if(first_ptr == NULL)
        return -1;
    
      obj_list[0] = first_ptr;
      if(search_trie(dc, vars, obj_list, 1))
        return obj_list[arity-1] - obj_list[0];  
      else
        return -1;
    }
    else
    {
      if(loop_search_trie(dc, vars, obj_list, 1))
        return obj_list[arity-1] - obj_list[0];  
      else
        return -1;
    }
  }
  
  // Find support for EQUALITY between var #s f and s.
  template<typename VarArray>
  int nextSupportingTupleEqual(int f, int s, const VarArray& _vars, TrieObj** obj_list)
  {
    cout << "in nextSupportingTupleEqual" << endl;
    if(trie_data == NULL)
      return -1;
      
    VarArray& vars = const_cast<VarArray&>(_vars);
    EqualChecker ec = EqualChecker(this, map_varno(f), map_varno(s));
      
    if(obj_list[0] == NULL)
    {
      TrieObj* first_ptr = trie_data;
      if(first_ptr == NULL)
        return -1;
    
      obj_list[0] = first_ptr;
      if(search_trie(ec, vars, obj_list, 0))
        return obj_list[arity-1] - obj_list[0];  
      else
        return -1;
    }
    else
    {
      if(loop_search_trie(ec, vars, obj_list, 0, trie_data))
        return obj_list[arity-1] - obj_list[0];  
      else
        return -1;
    }
  }
  
  // Find support for DISEQUALITY between var #s f and s.
  template<typename VarArray>
  int nextSupportingTupleDisequal(int f, int s, const VarArray& _vars, TrieObj** obj_list)
  {
    cout << "in nextSupportingTupleDisequal" << endl;
    if(trie_data == NULL)
      return -1;
      
    VarArray& vars = const_cast<VarArray&>(_vars);
    DisequalChecker dc = DisequalChecker(this, map_varno(f), map_varno(s));
      
    if(obj_list[0] == NULL)
    {
      TrieObj* first_ptr = trie_data;
      if(first_ptr == NULL)
        return -1;
    
      obj_list[0] = first_ptr;
      if(search_trie(dc, vars, obj_list, 0))
        return obj_list[arity-1] - obj_list[0];  
      else
        return -1;
    }
    else
    {
      if(loop_search_trie(dc, vars, obj_list, 0, trie_data))
        return obj_list[arity-1] - obj_list[0];  
      else
        return -1;
    }
  }
  
  // Find support for domain value i. This will be the value used by
  // the first variable.
  template<typename VarArray>
    int nextSupportingTupleNegative(DomainInt domain_val, const VarArray& _vars, TrieObj** obj_list, DomainInt* recycTuple)
  {
    VarArray& vars = const_cast<VarArray&>(_vars);
    
    //if(obj_list[0] == NULL)
    {
      TrieObj* first_ptr = get_next_ptr(trie_data, domain_val);
      if(first_ptr == NULL)
      {
          // Hang on a minute. How do we ever get here? Should only be at root node.
          for(int depth2=1; depth2<arity; depth2++) recycTuple[map_depth(depth2)]=vars[map_depth(depth2)].getMin();
          recycTuple[map_depth(0)]=domain_val;
          return 0;
      }
    
      obj_list[0] = first_ptr;
    }
      
    bool flag=search_trie_negative(vars, obj_list, 1, recycTuple);
    
    if(!flag) return -1;
    return 0;
  }
};

//template<typename VarArray>
class TupleTrieArray {
public:
  TupleList* tuplelist;
  
  int arity; 
  TupleTrie* tupleTries;
  
  TupleTrie & getTrie(int varIndex) 
  { return tupleTries[varIndex]; };
  
  /* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    Constructor
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
  
  TupleTrieArray(TupleList* _tuplelist, StateObj* stateObj) :
    tuplelist(_tuplelist)
  {
      tuplelist->finalise_tuples();
      arity = tuplelist->tuple_size();
      vector<int> dom_size(arity);
      vector<int> offset(arity);
      
      // create one trie for each element of scope.
      tupleTries = (TupleTrie*) malloc(sizeof(TupleTrie) * arity);
      if(!tupleTries)
      {
        cerr << "Out of memory in TupleTrie construction" << endl;
        FAIL_EXIT();
      }
      for (unsigned varIndex = 0; varIndex < arity; varIndex++)
        new (tupleTries + varIndex) TupleTrie(varIndex, tuplelist, stateObj);
  }
};

#endif
