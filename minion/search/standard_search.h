/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: standard_search.h 778 2007-11-11 17:19:48Z azumanga $
*/

#include "common_search.h"

namespace Controller
{

/*   int depth = 0; //a var that stores the current depth in the search */
/* 		 //tree, where the root is at depth 0, after a left or */
/* 		 //right branch from there search is at depth 1, and */
/* 		 //so on */

#include "VariableOrders.h"
  
  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)

  template<typename VarOrder, typename Variables, typename Propogator>
	inline void solve_loop(StateObj* stateObj, VarOrder& order, Variables& v, Propogator prop = PropogateGAC())
  {
	  D_INFO(0, DI_SOLVER, "Non-Boolean Search");
	  
	  maybe_print_search_state(stateObj, "Node: ", v);
	  
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
		  // We have found a solution!
		  check_sol_is_correct(stateObj);
		  // This function may escape from search if solution limit
		  // has been reached.
		  deal_with_solution(stateObj);

		  // fail here to force backtracking.
		  getState(stateObj).setFailed(true);
		  //depth -= 1;
		}
		else
		{
		  maybe_print_search_state(stateObj, "Node: ", v);
		  getVars(stateObj).getBigRangevarContainer().bms_array.before_branch_left();
		  world_push(stateObj);
		  //depth += 1;
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

		  world_pop(stateObj);
          maybe_print_search_action(stateObj, "bt");
	  
	      getVars(stateObj).getBigRangevarContainer().bms_array.before_branch_right();
		  order.branch_right();
		  //N.B. search depth is unchanged by this block
		  getVars(stateObj).getBigRangevarContainer().bms_array.after_branch_right();

		  set_optimise_and_propagate_queue(stateObj);
		}
	  }
  }
  
}




