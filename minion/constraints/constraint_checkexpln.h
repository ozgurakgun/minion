/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: constraint_checkassign.h 1887 2008-10-27 08:57:48Z ncam $
*/

/*
 *  constraint_checkassign.h
 *  cutecsp
 *
 *  Created by Chris Jefferson on 31/08/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

// This constraint checks that the virtual constraint stored to explain a
// pruning or assignment passes various sanity checks and that the VIG is
// finite. Basically down a path depths should be nonincreasing and explanations
// happen at the same depth as events. The graph is finite if the traversal
// terminates.

#include <limits>

template<typename VarArray, typename OriginalConstraint>
struct CheckExplnConstraint : public AbstractConstraint
{
  //return T iff OK
  bool check_recursive(pair<unsigned,unsigned> prev_d, VirtConPtr node) {
    if(node.get() == NULL || node->getDepth().second == 0) 
      return true; //hit assumptions and decisions, go no further
    else {
      if(prev_d < node->getDepth()) { //make sure depth is sane
	D_ASSERT(false); //trap error for debugger
	return false; //depth is dodgy
      }
      vector<VirtConPtr> preds = why[i]->whyT();
      for(size_t i = 0; i < preds.size(); i++)
	if(!check_recursive(node->getDepth(), preds[i])) {
	  return false;
	}
      return true; //succeeded down all branches 
    }
  }

  virtual string constraint_name()
  { return "CheckExplns"; }
  
  VarArray variables;

  vector<pair<size_t,DomainInt> > varval_trigger_mapping;
  
  CheckExplnConstraint(StateObj* _stateObj, VarArray& vars)
  : AbstractConstraint(_stateObj), variables(vars)
  { D_INFO(2, DI_CHECKCON, "Constructing"); }
  
  virtual triggerCollection setup_internal()
  {
    //Events are numbered -i for the -(i+1)th variable being assigned, positive
    //i events can be related to prunings to particular varvals via the
    //varval_trigger_mapping.
    D_INFO(2,DI_CHECKCON,"Setting up Constraint");
    triggerCollection t;
    //watch all assignments
    for(unsigned i = 0; i < variables.size(); ++i)
      t.push_back(make_trigger(variables[i], Trigger(this, -(i + 1)), Assigned));
    //watch all prunings
    int trig_no = 0;
    for(unsigned var = 0; var < variables.size(); ++var) {
      for(DomainInt val = variables[i].getInitialMin(); val <= variables[i].getInitialMax(); val++) {
	varval_trigger_mapping.push_back(make_pair(var, val));
	t.push_back(make_trigger(variables[var], Trigger(this, trig_no), DomainChanged));
	trig_no++;
      }
    }
    return t;
  }
  
  virtual AbstractConstraint* reverse_constraint()
  { 
    cerr << "shouldn't get reversed." << endl;
    FAIL_EXIT();
  }
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
    size_t var;
    DomainInt val;
    bool assg;
    if(prop_val < 0) {
      //assignment
      assg = true;
      var = -prop_val - 1;
      val = variables[var].getAssignedValue();
    } else {
      //pruning
      pair<size_t,DomainInt> varval = varval_trigger_mapping[prop_val];
      var = varval.first;
      val = varval.second;
      assg = false;
    }
    if(!check_recursive(make_pair(MAX_UINT, MAX_UINT), 
			variables[var].getExpl(assg, val)))
      cout << "problem with nonincreasing depths (assg)" << endl;
    if(variables[var].getDepth(assg, val).first != getMemory(stateObj).current_depth())
      cout << "problem with props happening late (assg)" << endl;
  }
  
  virtual BOOL check_unsat(int,DomainDelta)
  {
    return false;
  }
  
  virtual BOOL full_check_unsat()
  {
    return false;
  }
  
  virtual void full_propagate()
  { ; }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    return true;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    return variables;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  { ; }
};

template<typename VarArray>
AbstractConstraint*
CheckExplnCon(StateObj* stateObj,const VarArray& vars, ConstraintBlob&)
{ 
  return new CheckExplnConstraint<VarArray>(stateObj, vars); 
}

BUILD_CONSTRAINT1_WITH_BLOB(CT_CHECKEXPLN, CheckExplnCon);
