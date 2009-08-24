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

#include "tries.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

  struct Literal
{
  int var;
  DomainInt val;
  Literal(int _var, DomainInt _val) : var(_var), val(_val) { }
};

class BaseTableData
{
protected:
  TupleList* tuple_data;

public:
  int getVarCount()
    { return tuple_data->tuple_size(); }

  int getNumOfTuples()
    { return tuple_data->size(); }

  int getLiteralPos(Literal l)
    { return tuple_data->get_literal(l.var, l.val); }

  int* getPointer()
    { return tuple_data->getPointer(); }

  int getLiteralCount()
  { return tuple_data->literal_num; }

  Literal getLiteralFromPos(int pos)
  {
    pair<int,int> lit = tuple_data->get_varval_from_literal(pos);
    return Literal(lit.first, lit.second);
  }

  pair<DomainInt,DomainInt> getDomainBounds(int var)
  {
    return make_pair(tuple_data->dom_smallest[var],
      tuple_data->dom_smallest[var] + tuple_data->dom_size[var]);
  }

  BaseTableData(TupleList* _tuple_data) : tuple_data(_tuple_data) { }
};

class TableData : public BaseTableData
{
public:
   TableData(TupleList* _tuple_data) : BaseTableData(_tuple_data) { }

   // TODO : Optimise possibly?
   bool checkTuple(DomainInt* tuple, int tuple_size)
   {
     D_ASSERT(tuple_size == getVarCount());
     for(int i = 0; i < getNumOfTuples(); ++i)
     {
       if(std::equal(tuple, tuple + tuple_size, tuple_data->get_tupleptr(i)))
         return true;
     }
     return false;
   }
};

class TrieData : public BaseTableData
{

public:
    TupleTrieArray* tupleTrieArrayptr;

 TrieData(TupleList* _tuple_data, StateObj* stateObj) :
    BaseTableData(_tuple_data), tupleTrieArrayptr(new TupleTrieArray(_tuple_data, stateObj))
  { }

  // TODO: Optimise possibly?
  bool checkTuple(DomainInt* tuple, int tuple_size)
   {
     D_ASSERT(tuple_size == getVarCount());
     for(int i = 0; i < getNumOfTuples(); ++i)
     {
       if(std::equal(tuple, tuple + tuple_size, tuple_data->get_tupleptr(i)))
         return true;
     }
     return false;
   }
};

//return position of (f,s) in sequence (0,1),(0,2),..,(1,2),..,(f,s),..,(n-2,n-1) (starting from 0)
//the first term is the sum of n-f...n-1 (i.e. pairs involving 1..f-1 as the first number), 
//the second term is the number of preceding pairs involving f before (f,s).
inline int pos(const int f, const int s, const int n)
{ D_ASSERT(f < s); return f * (2 * n - f - 1) / 2 + (s - f - 1); }

class TrieState
{
  TrieData* data;
  vector<TrieObj**> trie_current_support;
  //the following two are used to store the supports last used for propagating 
  //(dis-)equalities between variables, eg. e_q_c_s[i] is asupport with 
  //positions f and s equal, letting i=pos(f,s)
  vector<TrieObj**> equal_pair_current_support;
  vector<TrieObj**> disequal_pair_current_support;
  vector<DomainInt> scratch_tuple;
public:
  TrieState(TrieData* _data) : data(_data)
  {
    const int vars = data->getVarCount();

    trie_current_support.resize(data->getLiteralCount());
    for(int i = 0; i < data->getLiteralCount(); ++i)
    {
      trie_current_support[i] = new TrieObj*[vars];
      for(int j = 0; j < vars; ++j)
        trie_current_support[i][j] = NULL;
    }

    int pair_count = vars * (vars - 1) / 2;
    equal_pair_current_support.resize(pair_count);
    disequal_pair_current_support.resize(pair_count);
    for(int i = 0; i < pair_count; i++) {
      equal_pair_current_support[i] = new TrieObj*[vars];
      disequal_pair_current_support[i] = new TrieObj*[vars];
      for(int j = 0; j < vars; j++)
	equal_pair_current_support[i][j] = disequal_pair_current_support[i][j] = NULL;
    }

    scratch_tuple.resize(data->getVarCount());
  }

  template<typename VarArray>
  vector<DomainInt>* findSupportingTuple(const VarArray& vars, Literal lit)
  {
    //int tuple_size = data->getVarCount();
    //int length = data->getNumOfTuples();
    //int* tuple_data = data->getPointer();

    int varIndex = lit.var;
    int val = lit.val;

    int litnum = data->getLiteralPos(lit);

    int new_support = data->tupleTrieArrayptr->getTrie(varIndex).
      nextSupportingTuple(val, vars, trie_current_support[litnum]);

    if(new_support < 0)
      return NULL;
    else
    {
      data->tupleTrieArrayptr->getTrie(varIndex).
      reconstructTuple(&scratch_tuple.front(), trie_current_support[litnum]);
    return &scratch_tuple;
    }
  }

  //find a valid tuple with an equal pair in components v1 and v2
  template<typename VarArray>
  vector<DomainInt>* findEqualPair(const VarArray& vars, int f, int s) {
    D_ASSERT(f < s);

    int pairNum = pos(f,s, data->getVarCount());

    //use trie[s], no proof that this is a good choice
    int new_support = data->tupleTrieArrayptr->getTrie(s).
      nextSupportingTupleEqual(f, s, vars, equal_pair_current_support[pairNum]);

    if(new_support < 0)
      return NULL;
    else
    {
      data->tupleTrieArrayptr->getTrie(s).
      reconstructTuple(&scratch_tuple.front(), equal_pair_current_support[pairNum]);
      D_ASSERT(scratch_tuple[f] == scratch_tuple[s]);
      return &scratch_tuple;
    }
  }
  
  //find a valid tuple with an disequal pair in components v1 and v2
  template<typename VarArray>
  vector<DomainInt>* findDisequalPair(const VarArray& vars, int f, int s) {
    D_ASSERT(f < s);

    int pairNum = pos(f,s, data->getVarCount());

    //use trie[s], no proof that this is a good choice
    int new_support = data->tupleTrieArrayptr->getTrie(s).
      nextSupportingTupleDisequal(f, s, vars, disequal_pair_current_support[pairNum]);

    if(new_support < 0)
      return NULL;
    else
    {
      data->tupleTrieArrayptr->getTrie(s).
      reconstructTuple(&scratch_tuple.front(), disequal_pair_current_support[pairNum]);
      D_ASSERT(scratch_tuple[f] != scratch_tuple[s]);
      return &scratch_tuple;
    }
    
  }
};

class TableState
{
  TableData* data;

  vector<DomainInt> scratch_tuple;
  /// The constructor of TableState should set up all structures to 'sensible'
  /// default values. It should not look for actual valid supports.
public:
  TableState(TableData* _data) : data(_data)
  { scratch_tuple.resize(data->getVarCount()); }

  /// This function should return a pointer to a valid tuple, if one exists,
  /// and return NULL if none exists. The vector should be stored inside the
  /// state, and need not be thread-safe.
  template<typename VarArray>
  vector<DomainInt>* findSupportingTuple(const VarArray& vars, Literal lit)
  {
    int tuple_size = data->getVarCount();
    int length = data->getNumOfTuples();
    int* tuple_data = data->getPointer();

    for(int i = 0; i < length; ++i)
    {
      int* tuple_start = tuple_data + i*tuple_size;
      bool success = true;
      if(tuple_start[lit.var] != lit.val)
        success = false;
      for(int j = 0; j < tuple_size && success; ++j)
      {
        if(!vars[j].inDomain(tuple_start[j]))
          success = false;
      }
      if(success)
      {
        std::copy(tuple_start, tuple_start + tuple_size, scratch_tuple.begin());
        return &scratch_tuple;
      }
    }
    return NULL;
  }

};

template<typename VarArray, typename TableDataType = TrieData, typename TableStateType = TrieState>
struct NewTableConstraint : public AbstractConstraint
{
  virtual string constraint_name()
    { return "TableDynamic"; }

  typedef typename VarArray::value_type VarRef;
  VarArray vars;

  TableDataType* data;

  TableStateType state;

  vector<pair<int,int> > seqnoToPair;

  int allocation; //how many watches are allocated for each support?
  int numSupportWatches; //how many watches allocated for supporting literals (as opposed to disequalities)
  
  NewTableConstraint(StateObj* stateObj, const VarArray& _vars, TupleList* _tuples) :
  AbstractConstraint(stateObj), vars(_vars), data(new TableDataType(_tuples, stateObj)), state(data)
  {
      if(_tuples->tuple_size()!=_vars.size())
      {
          cout << "Table constraint: Number of variables "
            << _vars.size() << " does not match length of tuples "
            << _tuples->tuple_size() << "." << endl;
          FAIL_EXIT();
      }
      //now store pairs of vars without repetition by index number, ordered lexicographically
      const int vars_s = vars.size();
      seqnoToPair.resize(vars_s * (vars_s - 1) / 2);
      int count = 0;
      for(int i = 0; i < vars_s - 1; i++)
	for(int j = i + 1; j < vars_s; j++)
	  seqnoToPair[count++] = make_pair(i, j);
  }

  LiteralSpecificLists* lists;

  MemOffset _current_support;

  int dynamic_trigger_count() {
    int pairs = vars.size() * (vars.size() - 1) / 2;
    //for each support: one watch per pair of variables (for disequality vars), plus a watch for each var but one
    allocation = pairs + vars.size();
    numSupportWatches = data->getLiteralCount() * allocation;
    return numSupportWatches + allocation * pairs * 2; //also two allocations per pair, one for eq and one for diseq
  } 

  virtual void propagate(DynamicTrigger* propagated_trig)
  {
    PROP_INFO_ADDONE(DynGACTable);

    cout << "prop" << endl;

    DynamicTrigger* dt = dynamic_trigger_start();
    int trigger_pos = propagated_trig - dt;

    if(trigger_pos < numSupportWatches) { //INCORRECT GUARD?
      //this branch is taken when the trigger is for a support failing
      
      int propagated_literal = trigger_pos / allocation;

      Literal lit = data->getLiteralFromPos(propagated_literal);
      
      P(propagated_literal << "." << vars.size() << "." << lit.var << "." << lit.val);
      if(!vars[lit.var].inDomain(lit.val))
      {
	//releaseTrigger(stateObj, propagated_trig BT_CALL_BACKTRACK);
 	P("Quick return");
	return;
      }
      
      vector<DomainInt>* supporting_tuple = state.findSupportingTuple(vars, lit);
      if(supporting_tuple)
      {
	P("Found new support!");
	setup_watches(lit, propagated_literal, *supporting_tuple);
      }
      else
      {
	P("Failed to find new support");
	vars[lit.var].removeFromDomain(lit.val);
	//clear_watches(lit, propagated_literal);
      }
    } else {
      //this branch is taken when a trigger on an equal or disequal pair is lost
      
      int pairNo = (trigger_pos - numSupportWatches) / (2 * allocation);
      pair<int,int> pair = seqnoToPair[pairNo];
      D_ASSERT(pairNo == pos(pair.first, pair.second, data->getVarCount()));
      D_ASSERT(pair.first < pair.second);
      bool onDisequal = ((trigger_pos - numSupportWatches ) % (2 * allocation)) / allocation; //true iff dt was a support for diseq
      Var v1 = vars[pair.first].getBaseVar();
      Var v2 = vars[pair.second].getBaseVar();
      if(onDisequal) {
	if(ARE_EQUAL(stateObj, v1, v2))
	  return;
	vector<DomainInt>* support = state.findDisequalPair(vars, pair.first, pair.second);
	if(support)
	  setup_watches_diseq(pair.first, pair.second, *support);
	else
	  SET_EQUAL(stateObj, v1, v2);
      } else {
	if(ARE_DISEQUAL(stateObj, v1, v2))
	  return;
	vector<DomainInt>* support = state.findEqualPair(vars, pair.first, pair.second);
	if(support)
	  setup_watches_eq(pair.first, pair.second, *support);
	else
	  SET_DISEQUAL(stateObj, v1, v2);
      }
    }
  }

  //function to place the watches on the support vector (including disequality triggers) 
  //dt is the first trigger to use
  //to skip a particular component in the support, set lit_var to the var number
  void place_watches(DynamicTrigger* dt, const vector<DomainInt>& support, int lit_var = -1)
  {
    DynamicTrigger* limit = dt + allocation;
    for(int v = 0; v < support.size(); ++v)
    {
      if(v != lit_var)
      {
        P(vars.size() << ".Watching " << v << "." << support[v] << " for " << lit.var << "." << lit.val);
        D_ASSERT(vars[v].inDomain(support[v]));
        PROP_INFO_ADDONE(CounterA);
        vars[v].addDynamicTrigger(dt, DomainRemoval, support[v] BT_CALL_STORE);
        ++dt;
      }
    }
    //for each pair, if pair is equal add trigger on disequality condition,
    //unless equality is already guaranteed, similarly for disequal
    cout << "assignment=" << support << endl;
    for(int i = 1; i < support.size(); i++) {
      for(int j = 0; j < i; j++) {
	Var vi = vars[i].getBaseVar();
	Var vj = vars[j].getBaseVar();
	if(support[i] != support[j]) {
	  bool areDiseq = ARE_DISEQUAL(stateObj, vi, vj);
	  if(!areDiseq) {
	    cout << "watching " << i << "," << j << " for equality" << endl;
	    TRIGGER_ON_EQUALITY(vi, vj, dt);
	    dt++;
	  }
	} else {
	  bool areEq = ARE_EQUAL(stateObj, vi, vj);
	  if(!areEq) {
	    cout << "watching " << i << "," << j << " for disequality" << endl;
	    TRIGGER_ON_DISEQUALITY(vi, vj, dt);
	    dt++;
	  }
	}
      }
    }
    //also release the triggers on (dis-)equalities that are no longer being used
    while(dt != limit) {
      releaseTrigger(stateObj, dt);
      dt++;
    }    
  }

  void setup_watches(Literal lit, int lit_pos, const vector<DomainInt>& support)
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    D_ASSERT(data->getLiteralPos(lit) == lit_pos);
    dt += lit_pos * allocation;
    place_watches(dt, support, lit.var);
  }

  void setup_watches_eq(int f, int s, const vector<DomainInt>& support)
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    int vars_size = vars.size();
    dt += numSupportWatches + 2 * allocation * pos(f, s, vars_size); //eq watches are the first half of the pos(f,s)'th block
    cout << "eq" << dynamic_trigger_start() << "," << dt << endl;
    place_watches(dt, support);
  }

  void setup_watches_diseq(int f, int s, const vector<DomainInt>& support)
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    int vars_size = vars.size();
    dt += numSupportWatches + 2 * allocation * pos(f, s, vars_size) + allocation; //skip diseq watches in pos(f,s)'th block
    cout << "diseq" << dynamic_trigger_start() << "," << dt << endl;
    place_watches(dt, support);
  }

  void clear_watches(Literal lit, int lit_pos)
  {
    //might want to clear the watches on disequality variables too, but this
    //code is currently unused

    DynamicTrigger* dt = dynamic_trigger_start();
    D_ASSERT(data->getLiteralPos(lit) == lit_pos);
    int vars_size = vars.size();
    dt += lit_pos * (vars_size - 1);
    for(int v = 0; v < vars_size; ++v)
    {
      releaseTrigger(stateObj, dt BT_CALL_BACKTRACK);
      ++dt;
    }
  }

  virtual void full_propagate()
  {
    cout << "full prop" << endl;

    if(vars.size() == 0)
    {
      getState(stateObj).setFailed(true);
      return;
    }

    for(unsigned i = 0; i < vars.size(); ++i)
    {
      pair<DomainInt, DomainInt> bounds = data->getDomainBounds(i);
      vars[i].setMin(bounds.first);
      vars[i].setMax(bounds.second);

      if(getState(stateObj).isFailed()) return;

      for(DomainInt x = vars[i].getMin(); x <= vars[i].getMax(); ++x)
      {
        vector<DomainInt>* support = state.findSupportingTuple(vars, Literal(i, x));
        if(support)
        {
          setup_watches(Literal(i, x), data->getLiteralPos(Literal(i, x)), *support);
        }
        else
        {
          vars[i].removeFromDomain(x);
        }
      }
    }

    if(getState(stateObj).isFailed()) return;

    //either propagate (dis-)equality straight off the bat or place watches
    //to ensure it is done as we go along
    int pair_no = 0;
    for(int i = 0; i < vars.size() - 1; i++) {
      Var vi = vars[i].getBaseVar();
      for(int j = i + 1; j < vars.size(); j++) {
	Var vj = vars[j].getBaseVar();
	cout << "setting up diseq trigs for " << vi << "," << vj << endl;
	if(!ARE_DISEQUAL(stateObj, vi, vj)) {
	  cout << "trying for eq trigs" << endl;
	  vector<DomainInt>* eq_support = state.findEqualPair(vars, i, j);
	  if(!eq_support) {
	    SET_DISEQUAL(stateObj, vi, vj);
	    cout << "couldn't find" << endl;
	  } else {
	    setup_watches_eq(i, j, *eq_support);
	    cout << "found and set up" << endl;
	  }
	} else {
	  cout << "not trying to eq trigs" << endl;
	}
	if(!ARE_EQUAL(stateObj, vi, vj)) {
	  cout << "trying for diseq trigs" << endl;
	  vector<DomainInt>* diseq_support = state.findDisequalPair(vars, i, j);
	  if(!diseq_support) {
	    SET_EQUAL(stateObj, vi, vj);
	    cout << "couldn't find" << endl;
	  } else {
	    setup_watches_diseq(i, j, *diseq_support);
	    cout << "found and set up" << endl;
	  }
	} else {
	  cout << "not trying for diseq trigs" << endl;
	}
	
	if(getState(stateObj).isFailed()) return;
      }
      pair_no++;
    }
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    return data->checkTuple(v, v_size);
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
  { return new NewTableConstraint<VarArray>(stateObj, vars, tuples); }
