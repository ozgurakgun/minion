/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: standard_search.h 778 2007-11-11 17:19:48Z azumanga $
*/

#include "common_search.h"

namespace Controller
{

#include "VariableOrders.h"
  
  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)

  struct literal {
    AnyVarRef* var; //variable
    int id; //variable ID
    int neg; //negation, 0 iff negated
    unsigned depth; //depth set
  };

  //Learn a conflict and return a depth to jump back to. Clause list data
  //structure is sorted by var_id throughout.
  inline unsigned conflict_learn(StateObj* stateObj)
  {
    BooleanContainer& bc = getVars(stateObj).getBooleanContainer();
    AnyVarRef* conflict_var = bc.conflict_var;
    vector<AnyVarRef> vars_vec = conflict_var->getAntecedent()->get_vars();
    vector<int> negs = *(conflict_var->getAntecedent()->get_signs());
    //create initial clause based on clause that fired just before conflict
    list<literal> clause;
    unsigned const depth = getMemory(stateObj).backTrack().current_depth();
    unsigned count_at_depth = 0;
    size_t v_s = vars_vec.size();
    literal tmp;
    for(int i = 0; i < v_s; i++) {
      tmp.var = &vars_vec[i];
      tmp.id = tmp.var->getId();
      tmp.neg = negs[i];
      if((tmp.depth = tmp.var->getDepth()) == depth)
	++count_at_depth;
      clause.push_back(tmp);
    }
    //CAN I ASSUME these are in ID order right now?  

    //Now take one clause after another off the stack and resolve it
    //with the current one until first UIP. Begin by resolving the
    //other clause contributing to the conflicting variable.
    vector<DynamicConstraint*>& props = bc.props; //constraints that have fired
    //next push other clause contributing to conflict, this is a
    //duplicate in props, but adding clauses is idempotent
    props.push_back(bc.last_clause); 
    DynamicConstraint* next_clause; //clause being resolved
    vector<AnyVarRef> c_vars;       //variables in clause
    vector<int> c_negs;             //literal negation in clause
    while(count_at_depth != 1) {    //stop at first UIP
      list<literal>::iterator curr = clause.begin();
      list<literal>::iterator end = clause.end();
      do {
	next_clause = props.back();
	props.pop_back();
      } while(next_clause == 0); //next_clause is the next clause that fired
      c_vars = next_clause->get_vars();
      size_t c_v_s = c_vars.size();
      c_negs = *(next_clause->get_signs());
      unsigned depth;
      int id;
      for(int i = 0; i < c_v_s; i++) {
	depth = c_vars[i].getDepth();
	id = c_vars[i].getId();
	while(id < (*curr).id) //find insertion position
	  curr++;
	if((*curr).id == id) {
	  if((*curr).neg != c_negs[i]) {
	    if((*curr).depth == depth) //is this test always true?
	      --count_at_depth;
	    curr = clause.erase(curr); //resolve
	  } else {
	    curr++; //ignore duplicate literal
	  }
	} else {
	  tmp.var = &c_vars[i];
	  tmp.neg = c_negs[i];
	  tmp.id = id;
	  tmp.depth = tmp.var->getDepth();
	  curr++;
	  clause.insert(curr, tmp);
	}
      }    
    }
    //do something with successfully created conflict clause
    return getMemory(stateObj).backTrack().current_depth() - 1;
  }

  template<typename VarOrder, typename Variables, typename Propogator>
	inline void solve_loop(StateObj* stateObj, VarOrder& order, Variables& v, Propogator prop = PropogateGAC())
  {
	  D_INFO(0, DI_SOLVER, "Non-Boolean Search");
	  
	  maybe_print_search_state(stateObj, "Node: ", v);
	  
	  BOOL solution_found; //true if we just found a solution
	  while(true)
	  {
		getState(stateObj).incrementNodeCount();
		if(getState(stateObj).getNodeCount() == getOptions(stateObj).nodelimit)
		  return;
	
		if(do_checks(stateObj))
		  return;
		
		// order.find_next_unassigned returns true if all variables assigned.
		if(order.find_next_unassigned())
		{  		  
		  solution_found = true;
		  // We have found a solution!
		  check_sol_is_correct(stateObj);
		  // This function may escape from search if solution limit
		  // has been reached.
		  deal_with_solution(stateObj);

		  // fail here to force backtracking.
		  getState(stateObj).setFailed(true);
		}
		else
		{
		  solution_found = false;
		  maybe_print_search_state(stateObj, "Node: ", v);
		  getVars(stateObj).getBigRangevarContainer().bms_array.before_branch_left();
		  world_push(stateObj);
		  cout << getVars(stateObj).getBooleanContainer().props << endl;
		  order.branch_left();
		  getVars(stateObj).getBigRangevarContainer().bms_array.after_branch_left();
		  prop(stateObj, v);
     		}
		
		// Either search failed, or a solution was found.
		while(getState(stateObj).isFailed())
		{
		  getState(stateObj).setFailed(false);
		  
		  if(order.finished_search())
			return;
		  
		  unsigned bj_depth;
		  if(!solution_found) //definitely a failure due to a conflict
		    bj_depth = conflict_learn(stateObj);
		  else
		    bj_depth = getMemory(stateObj).backTrack().current_depth() - 1;
		  
		  world_pop(stateObj, bj_depth);
		  maybe_print_search_action(stateObj, "bt");
	  
		  getVars(stateObj).getBigRangevarContainer().bms_array.before_branch_right();
		  order.branch_right();
		  getVars(stateObj).getBigRangevarContainer().bms_array.after_branch_right();

		  set_optimise_and_propagate_queue(stateObj);
		}
	  }
  }
  
}




