/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: dynamic_sum.h 830 2007-11-20 10:42:15Z azumanga $
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

#include <vector>

template<typename VarArray>
struct BoolOrConstraintDynamic : public DynamicConstraint
{
  typedef typename VarArray::value_type VarRef;

  virtual string constraint_name()
  { return "BoolOr"; }
  
  VarArray var_array;
  vector<int> negs; //negs[i]==0 iff var_array[i] is negated, NB. this
		    //is also the value that must be watched
  size_t no_vars;
  int watched[2];
  int last;

  BoolOrConstraintDynamic(StateObj* _stateObj, const VarArray& _var_array,
			       const vector<int>& _negs) :
    DynamicConstraint(_stateObj), var_array(_var_array), negs(_negs), last(0)
  { 
    D_INFO(2, DI_OR, "Constructor for OR constraint");
    watched[0] = watched[1] = -2;
    no_vars = _var_array.size();
#ifndef WATCHEDLITERALS
    cerr << "This almost certainly isn't going to work... sorry" << endl;
#endif
  }
  
  int dynamic_trigger_count()
  {
    D_INFO(2, DI_OR, "OR constraint: dynamic_trigger_count");
    return 2;
  }

  virtual void full_propagate()
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    int found = 0; //num literals that can be T found so far
    int first_found = -1;
    int next_found = -1;
    for(int i = 0; i < no_vars; i++) {
      if(var_array[i].inDomain(negs[i])) { //can literal be T?
	found++;
	if(found == 1) 
	  first_found = i;
	else {
	  next_found = i;
	  break;
	}
      }
    }
    if(found == 0)
      getState(stateObj).setFailed(true);
    if(found == 1) { //detect unit clause
      var_array[first_found].propagateAssign(negs[first_found]);
      return; //don't bother placing any watches on unit clause
    }
    //not failed or unit, place watches
    var_array[first_found].addDynamicTrigger(dt, DomainRemoval, negs[first_found]);
    dt->trigger_info() = first_found;
    watched[0] = first_found;
    dt++;
    var_array[next_found].addDynamicTrigger(dt, DomainRemoval, negs[next_found]);
    dt->trigger_info() = next_found;
    watched[1] = next_found;
  }

  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
    size_t prev_var = dt->trigger_info();
    size_t other_var = watched[0] == prev_var ? watched[1] : watched[0];
    for(int i = 1; i <= no_vars; i++) {
      size_t j = (last + i) % no_vars;
      VarRef& v = var_array[j];
      int neg = negs[j];
      if(j != other_var && v.inDomain(neg)) {
	v.addDynamicTrigger(dt, DomainRemoval, neg);
	dt->trigger_info() = j;
	last = j;
	watched[watched[0] == prev_var ? 0 : 1] = j;
	return;
      }
    }
    //if we get here, we couldn't find a place to put the watch, do UP
    VarRef& v = var_array[other_var];
    int neg = negs[other_var];
    if(!v.isAssigned()) { //two values remain
      v.propagateAssign(neg);
      cout << v << " has been set, it has addr " << &v << " and antecedent " << this << endl;
      v.setAntecedent(this);
      v.setDepth(getMemory(stateObj).backTrack().current_depth()); //current depth
    } else if(!v.inDomain(neg)) { //wiping out domain
      cout << v << " has failed, it had addr " << &v << " and this time " << this << " caused the problem" << endl;
      BooleanContainer& bc = getVars(stateObj).getBooleanContainer();
      bc.conflict_var = new AnyVarRef(v);
      bc.last_clause = this;
      v.propagateAssign(neg); //now force a conflict
    } //else already satisfied
  }

  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_INFO(2, DI_OR, "Checking soln in or constraint");
    for(int i = 0; i < no_vars; i++)
      if(v[i] == negs[i])
	return true;
    return false;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(no_vars);
    for(unsigned i = 0; i < no_vars; ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    return vars;  
  }

  virtual vector<int>* get_signs() { return &negs; }
};

template<typename T>
inline DynamicConstraint*
BuildCT_WATCHED_OR(StateObj* stateObj, const light_vector<T>& vs, BOOL reify,
		   const BoolVarRef& reifyVar, ConstraintBlob& bl)
{
  size_t vs_s = vs.size();
  for(int i = 0; i < vs_s; i++)
    if(vs[i].getInitialMin() != 0 || vs[i].getInitialMax() != 1)
      cerr << "watched or only works on Boolean variables!" << endl;

  if(reify) {
    cerr << "Cannot reify 'watched or' constraint." << endl;
    exit(0);
  } else {
      return new BoolOrConstraintDynamic<light_vector<T> >(stateObj, vs, bl.negs);
  }
}

