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

#include <limits>

#ifndef WATCH_LESS
#define WATCH_LESS

// var1 < var2
template<typename Var1, typename Var2>
struct WatchLessConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "WatchedLess"; }
  
  Var1 var1;
  Var2 var2;

  WatchLessConstraint(StateObj* _stateObj, const Var1& _var1, const Var2& _var2) :
	AbstractConstraint(_stateObj), var1(_var1), var2(_var2)
  { }
  
  int dynamic_trigger_count()
  {	return 2; }
  
  virtual void full_propagate()
  {  
  	DynamicTrigger* dt = dynamic_trigger_start();
	    
    var1.addDynamicTrigger(dt    , LowerBound);
    var2.addDynamicTrigger(dt + 1, UpperBound);

    //explns for var2 prunings
    const DomainInt var1_min = var1.getMin();
    for(DomainInt i = var2.getMin(); i <= var1_min; i++)
      if(var2.inDomain(i))
	storeExpl(false, var2, i, VirtConPtr(new WatchlessPrunRight<Var1,Var2>(this,i)));
    //do prunings
    var2.setMin(var1.getMin() + 1);

    //store explns for prunings from var1
    const DomainInt v1_max = var1.getMax();
    for(DomainInt i = var2.getMax(); i <= v1_max; i++)
      if(var1.inDomain(i)) 
	storeExpl(false, var1, i, VirtConPtr(new WatchlessPrunLeft<Var1,Var2>(this,i)));
    //then do the prunings
    var1.setMax(var2.getMax() - 1);
  }
  
    
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
	  PROP_INFO_ADDONE(WatchNEQ);
	  DynamicTrigger* dt_start = dynamic_trigger_start();
	  
    D_ASSERT(dt == dt_start || dt == dt_start + 1);
    
	  if(dt == dt_start)
	  {
	    //explns
	    const DomainInt var1_min = var1.getMin();
	    for(DomainInt i = var2.getMin(); i <= var1_min; i++)
	      if(var2.inDomain(i))
		storeExpl(false, var2, i, VirtConPtr(new WatchlessPrunRight<Var1,Var2>(this,i)));
	    //prunings
	    var2.setMin(var1.getMin() + 1);
	  }
	  else
	  {
	    //store explns for prunings from var1
	    const DomainInt v1_max = var1.getMax();
	    for(DomainInt i = var2.getMax(); i <= v1_max; i++)
	      if(var1.inDomain(i))
		storeExpl(false, var1, i, VirtConPtr(new WatchlessPrunLeft<Var1,Var2>(this,i)));

	    //then do the prunings
	    var1.setMax(var2.getMax() - 1);
	  }
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 2);
    return v[0] < v[1];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	  vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    if(var1.getMin() < var2.getMax())
    {
      assignment.push_back(make_pair(0,var1.getMin()));
      assignment.push_back(make_pair(1,var2.getMax()));
      return true;
    }
    return false;
  }

  virtual vector<VirtConPtr> whyF() const //see 4/11/08 of neil's notebook for notes
  {
    //calculate the earliest value for which x >= value and value >= y
    vector<pair<unsigned,unsigned> > v1_d_max; //v1_d_max[i] is max depth of pruning from var1 < i+var2.max
    vector<pair<unsigned,unsigned> > v2_d_max; //v2_d_max[i] is max depth of pruning from var2 > i+var2.max
    const DomainInt var2_max = var2.getMax();
    const DomainInt var2_initmax = var2.getInitialMax();
    const DomainInt var1_min = var1.getMin();
    const DomainInt var1_initmin = var1.getInitialMin();
    const size_t array_s = var1_min - var2_max + 1;
    v1_d_max.resize(array_s);
    v2_d_max.resize(array_s);
    //compute v1_d_max[0]
    v1_d_max[0] = make_pair(0, 0);
    for(DomainInt v = var1.getInitialMin(); v < var2_max; v++)
      v1_d_max[0] = max(v1_d_max[0], var1.getDepth(false, v));
    //compute rest of v1_d_max
    for(size_t curr = 1; curr < array_s; curr++)
      v1_d_max[curr] = max(v1_d_max[curr - 1], 
			   var2_max + curr - 1 >= var1_initmin //only if value was once present
			   ? var1.getDepth(false, var2_max + curr - 1)
			   : make_pair((unsigned)0,(unsigned)0));
    //compute v2_d_max.back
    pair<unsigned,unsigned>& v2_d_max_back = v2_d_max.back();
    v2_d_max_back = make_pair(0, 0);
    for(size_t v = var2.getInitialMax(); v > var1_min; v--)
      v2_d_max_back = max(v2_d_max_back, var2.getDepth(false, v));
    //compute rest of v2_d_max
    for(int curr = array_s - 2; curr >= 0; curr--)
      v2_d_max[curr] = max(v2_d_max[curr + 1], 
			   var2_max + curr + 1 <= var2_initmax //only if value was once present
			   ? var2.getDepth(false, var2_max + curr + 1)
			   : make_pair((unsigned)0,(unsigned)0));
    //find i that makes max(v1_d_max[i],v2_d_max[i]) as small as possible
    size_t best_i = -1;
    pair<unsigned,unsigned> best_d = make_pair(UINT_MAX, UINT_MAX);
    for(size_t i = 0; i < array_s; i++) {
      pair<unsigned,unsigned> next = max(v1_d_max[i], v2_d_max[i]);
      if(best_d > next) {
	best_i = i;
	best_d = next;
      }
    }
    //convert to value
    best_i += var2_max;
    //now built VCs
    vector<VirtConPtr> retVal;
    retVal.reserve(2);
    retVal.push_back(VirtConPtr(new GreaterConstant<Var1>(stateObj, var1, best_i - 1))); //x>=best_i
    retVal.push_back(VirtConPtr(new LessConstant<Var2>(stateObj, var2, best_i + 1))); //y<=best_i
    return retVal; 
  }

  virtual pair<unsigned,unsigned> whenF() const //max depth of pruning of x<sv and sv<y
  {
    //calculate the earliest depth for which x >= value and value >= y
    vector<pair<unsigned,unsigned> > v1_d_max; //v1_d_max[i] is max depth of pruning from var1 < i+var2.max
    vector<pair<unsigned,unsigned> > v2_d_max; //v2_d_max[i] is max depth of pruning from var2 > i+var2.max
    const DomainInt var2_max = var2.getMax();
    const DomainInt var2_initmax = var2.getInitialMax();
    const DomainInt var1_min = var1.getMin();
    const DomainInt var1_initmin = var1.getInitialMin();
    const size_t array_s = var1_min - var2_max + 1;
    v1_d_max.resize(array_s);
    v2_d_max.resize(array_s);
    //compute v1_d_max[0]
    v1_d_max[0] = make_pair(0, 0);
    for(DomainInt v = var1.getInitialMin(); v < var2_max; v++)
      v1_d_max[0] = max(v1_d_max[0], var1.getDepth(false, v));
    //compute rest of v1_d_max
    for(size_t curr = 1; curr < array_s; curr++)
      v1_d_max[curr] = max(v1_d_max[curr - 1], 
			   var2_max + curr - 1 >= var1_initmin //only if value was once present
			   ? var1.getDepth(false, var2_max + curr - 1)
			   : make_pair((unsigned)0,(unsigned)0));
    //compute v2_d_max.back
    pair<unsigned,unsigned>& v2_d_max_back = v2_d_max.back();
    v2_d_max_back = make_pair(0, 0);
    for(size_t v = var2.getInitialMax(); v > var1_min; v--)
      v2_d_max_back = max(v2_d_max_back, var2.getDepth(false, v));
    //compute rest of v2_d_max
    for(int curr = array_s - 2; curr >= 0; curr--)
      v2_d_max[curr] = max(v2_d_max[curr + 1], 
			   var2_max + curr + 1 <= var2_initmax //only if value was once present
			   ? var2.getDepth(false, var2_max + curr + 1)
			   : make_pair((unsigned)0,(unsigned)0));
    //find i that makes max(v1_d_max[i],v2_d_max[i]) as small as possible
    size_t best_i = -1;
    pair<unsigned,unsigned> best_d = make_pair(UINT_MAX, UINT_MAX);
    for(size_t i = 0; i < array_s; i++) {
      pair<unsigned,unsigned> next = max(v1_d_max[i], v2_d_max[i]);
      if(best_d > next) {
	best_i = i;
	best_d = next;
      }
    }
    return best_d;
  }
};

template<typename VarArray1, typename VarArray2>
inline AbstractConstraint*
WatchLessConDynamic(StateObj* stateObj, const VarArray1& _var_array_1, const VarArray2& _var_array_2)
{ 
  return new WatchLessConstraint<typename VarArray1::value_type, typename VarArray2::value_type>
    (stateObj, _var_array_1[0], _var_array_2[0]); 
}

BUILD_CONSTRAINT2(CT_WATCHED_LESS, WatchLessConDynamic)

#endif
