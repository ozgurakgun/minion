/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

#include "common_search.h"

namespace Controller
{

#include "VariableOrders.h"
  
  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)

  template<typename VarOrder, typename Variables, typename Propogator>
	inline void solve_loop(StateObj* stateObj, VarOrder& order, Variables& v, Propogator prop = PropagateGAC())
  {
	  D_INFO(0, DI_SOLVER, "Non-Boolean Search");
	  
	  maybe_print_search_state(stateObj, "Node: ", v);

	  vector<vector<ExplPtr> > failureExpls;
	  vector<ExplPtr> currFailureExpl; //partial explanation for eventual failure of current LB

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
		}
		else
		{
		  maybe_print_search_state(stateObj, "Node: ", v);
		  failureExpls.push_back(currFailureExpl);
		  currFailureExpl = vector<ExplPtr>();
		  world_push(stateObj);
		  order.branch_left();
          prop(stateObj, v);
		}
		
		// Either search failed, or a solution was found.
		while(getState(stateObj).isFailed())
		{
		  vector<ExplPtr> fail = workOutWhy(stateObj);
		  //complete reason for failure of this LB
		  currFailureExpl.insert(currFailureExpl.end(), fail.begin(), fail.end());

		  getState(stateObj).setFailed(false);
		  
		  if(order.finished_search())
			return;

		  world_pop(stateObj);
		  maybe_print_search_action(stateObj, "bt");
	  
		  //post <fail>
		  
		  pair<int,DomainInt> lastLeft = order.getLast();
		  v[lastLeft.first].setExplanation(lastLeft.second, lastLeft.second,
						   ExplPtr(new Conjunction(currFailureExpl)));
		  //contibute the reason for this LB loss to the explanation for the previous LB
		  currFailureExpl.insert(currFailureExpl.end(),
					 failureExpls.back().begin(),
					 failureExpls.back().end());
		  failureExpls.pop_back();

		  order.branch_right();

		  set_optimise_and_propagate_queue(stateObj);
		}
	  }
  }
  
}




