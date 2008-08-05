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

  //DANGER!
#define C_PRUN_LESS

  void pruneVar2()
  {
#ifndef C_PRUN_LESS
    if(var1.getMin() + 1 > var2.getMin()) { //check that there will be prunings
      //first create a vector with all prunings off the bottom end of var1
      vector<ExplPtr> prunings;
      const Var var1_var = var1.getBaseVar();
      const DomainInt var1_min = var1.getMin();
      for(DomainInt val = var1.getInitialMin(); val < var1_min; val++)
	prunings.push_back(ExplPtr(new Lit(var1_var, var1.getBaseVal(val), false)));
      //next start storing explanations for each value of var2 that will shortly be pruned
      const DomainInt var2_min = var2.getMin();
      for(DomainInt val = var1.getMin(); val >= var2_min; val--) {
	if(var2.inDomain(val)) //might be quicker here to overwrite anyway, which is safe
	  var2.setExplanation(val, val, ExplPtr(new Conjunction(prunings)));
	if(prunings.size() > 0)
	  prunings.pop_back();
      }
    }
#else
    const DomainInt lim = var1.getMin(); //last value that may be pruned below
    for(DomainInt val = var2.getMin(); val <= lim; val++)
      if(var2.inDomain(val)) //might be quicker to overwrite anyway, and safe, but unknown empirical effect
	var2.setExplanation(val, val, ExplPtr(new LessConstant<Var1>(val - 1, var1)));
    //i.e. whenever var1 > val-1, i.e. var1 >= val, i.e. var1 can't be less than val
#endif
    var2.setMin(var1.getMin() + 1);
  }

  void pruneVar1()
  {
#ifndef C_PRUN_LESS
    if(var2.getMax() - 1 < var1.getMax()) { //check that there will be prunings
      //first create a vector with all prunings off the top end of var2
      vector<ExplPtr> prunings;
      const Var var2_var = var2.getBaseVar();
      const DomainInt var2_max = var2.getMax();
      for(DomainInt val = var2.getInitialMax(); val > var2_max; val--)
	prunings.push_back(ExplPtr(new Lit(var2_var, var2.getBaseVal(val), false)));
      //next start storing explanations for each value of var1 that will shortly be pruned
      const DomainInt var1_max = var1.getMax();
      for(DomainInt val = var2.getMax(); val <= var1_max; val++) {
	if(var1.inDomain(val)) //might be quicker here to overwrite anyway, which is safe
	  var1.setExplanation(val, val, ExplPtr(new Conjunction(prunings)));
	if(prunings.size() > 0)
	  prunings.pop_back();
      }
    }
#else
    const DomainInt lim = var1.getMax(); //last value that may be pruned
    for(DomainInt val = var2.getMax(); val <= lim; val++)
      if(var1.inDomain(val)) //might be quicker to overwrite anyway, and safe, but unknown empirical effect
	var1.setExplanation(val, val, ExplPtr(new LessConstant<Var2>(var2, val + 1)));
    //i.e. whenever var2 < val+1, i.e. var2 <= val, i.e. var2 can't be more than val
#endif
    var1.setMax(var2.getMax() - 1);    
  }

  virtual void full_propagate()
  {  
    DynamicTrigger* dt = dynamic_trigger_start();
	    
    var1.addDynamicTrigger(dt    , LowerBound);
    var2.addDynamicTrigger(dt + 1, UpperBound);

    //first store some explanations for prunings to var2's domain
    pruneVar2();
    pruneVar1();
  }
  
    
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(WatchNEQ);
    DynamicTrigger* dt_start = dynamic_trigger_start();
    
    D_ASSERT(dt == dt_start || dt == dt_start + 1);
    
    if(dt == dt_start) pruneVar2();
    else pruneVar1();
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

  ExplPtr getFalseExpl()
  {
#ifndef C_PRUN_LESS
    //explanation is all the prunings from var1 < var2.getMax() and from var2 > var1.getMin()
    vector<ExplPtr> prunings;
    const Var var2_var = var2.getBaseVar();
    const DomainInt var2_initmax = var2.getInitialMax();
    for(DomainInt val = var1.getMin() + 1; val < var2_initmax; val++)
      prunings.push_back(ExplPtr(new Lit(var2_var, var2.getBaseVal(val), false)));
    const Var var1_var = var1.getBaseVar();
    const DomainInt var2_max = var2.getMax();
    for(DomainInt val = var1.getInitialMin(); val < var2_max; val++)
      prunings.push_back(ExplPtr(new Lit(var1_var, var1.getBaseVal(val), false)));
    return ExplPtr(new Conjunction(prunings));
#else
    return ExplPtr(new GreaterEqual<Var1,Var2>(var1, var2)); //false when var1 >= var2
#endif
  }  
  
  virtual void get_satisfying_assignment(box<pair<int,int> >& assignment)
  {
    if(var1.getMin() < var2.getMax())
    {
      assignment.push_back(make_pair(0,var1.getMin()));
      assignment.push_back(make_pair(1,var2.getMax()));
    }
  }
};

template<typename VarArray1, typename VarArray2>
AbstractConstraint*
WatchLessConDynamic(StateObj* stateObj, const VarArray1& _var_array_1, const VarArray2& _var_array_2)
{ 
  return new WatchLessConstraint<typename VarArray1::value_type, typename VarArray2::value_type>
    (stateObj, _var_array_1[0], _var_array_2[0]); 
}

BUILD_CONSTRAINT2(CT_WATCHED_LESS, WatchLessConDynamic)
