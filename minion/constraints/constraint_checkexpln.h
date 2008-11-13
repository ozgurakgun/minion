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

template<typename VarArray>
struct CheckExplnConstraint : public AbstractConstraint
{
  //return T iff OK
  bool check_recursive(pair<unsigned,unsigned> prev_d, VirtConPtr node) {
    if(node.get() == NULL || node->getDepth().second == 0) {
      if(node.get() == NULL) cout << "assumption d=" << node->getDepth();
      else cout << "decision d=" << node->getDepth();
      return true; //hit assumptions and decisions, go no further
    } else {
      cout << *node << " d=" << node->getDepth();
      cout << " {" << endl;
      if(prev_d < node->getDepth()) { //make sure depth is sane
	D_ASSERT(false); //trap error for debugger
	return false; //depth is dodgy
      }
      vector<VirtConPtr> preds = node->whyT();
      for(size_t i = 0; i < preds.size(); i++) {
	if(!check_recursive(node->getDepth(), preds[i])) {
	  return false;
	}
	if(i != preds.size() - 1) cout << "," << endl;
      }
      cout << "}";
      return true; //succeeded down all branches 
    }
  }

  virtual string constraint_name()
  { return "CheckExplns"; }
  
  VarArray variables;

  vector<pair<size_t,DomainInt> > varval_trigger_mapping;
  
  CheckExplnConstraint(StateObj* _stateObj, const VarArray& _variables)
  : AbstractConstraint(_stateObj), variables(_variables)
  { D_INFO(2, DI_CHECKCON, "Constructing"); }
  
  virtual int dynamic_trigger_count()
  {
    unsigned count = 0;
    for(size_t var = 0; var < variables.size(); var++)
      count += (variables[var].getInitialMax() - variables[var].getInitialMin() + 1);
    return count; //one per varval
  }

  virtual triggerCollection setup_internal()
  {
    //Events are numbered i for the i'th variable being assigned
    D_INFO(2,DI_CHECKCON,"Setting up Constraint");
    triggerCollection t;
    //watch all assignments
    for(unsigned i = 0; i < variables.size(); ++i)
      t.push_back(make_trigger(variables[i], Trigger(this, i), Assigned));
    return t;
  }
  
  virtual AbstractConstraint* reverse_constraint()
  { 
    cerr << "shouldn't get reversed." << endl;
    FAIL_EXIT();
  }

  void do_checks(bool assg, size_t var, DomainInt val)
  {
    cout << "assg=" << assg << ",var=" << var << ",val=" << val << endl;
    if(!check_recursive(make_pair(UINT_MAX, UINT_MAX), 
			variables[var].getExpl(assg, val)))
      cout << "problem with nonincreasing depths" << endl;
    cout << endl;
    //next check that propagation doesn't happen at a later depth than it's explanation
    if(variables[var].getDepth(assg, val).first != getMemory(stateObj).backTrack().current_depth())
      cout << "problem with props happening late" << endl;
  }    
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  { do_checks(true, prop_val, variables[prop_val].getAssignedValue()); }

  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* trig)
  {
    pair<size_t, DomainInt> varval = varval_trigger_mapping[trig->trigger_info()];
    do_checks(false, varval.first, varval.second);
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
  { 
    DynamicTrigger* dt = dynamic_trigger_start();
    //setup varval triggers
    unsigned code = 0;
    for(size_t var = 0; var < variables.size(); var++) {
      for(DomainInt val = variables[var].getInitialMin(); val <= variables[var].getInitialMax(); val++) {
	variables[var].addDynamicTrigger(dt, DomainRemoval, val);
	dt->trigger_info() = code;
	varval_trigger_mapping.push_back(make_pair(var, val));
	code++;
	dt++;
      }
    }
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    return true;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> retVal;
    for(int i = 0; i < variables.size(); i++)
      retVal.push_back(AnyVarRef(variables[i]));
    return retVal;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  { ; }
};

template<typename VarArray>
inline AbstractConstraint*
CheckExplnCon(StateObj* stateObj, const VarArray& vars, ConstraintBlob&)
{ 
  return new CheckExplnConstraint<VarArray>(stateObj, vars); 
}

BUILD_CONSTRAINT1_WITH_BLOB(CT_CHECKEXPLN, CheckExplnCon);
