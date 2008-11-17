/* Minion Constraint Solver
http://minion.sourceforge.net

For Licence Information see file LICENSE.txt 

$Id: standard_search.h 478 2006-11-24 09:42:10Z azumanga $
*/

#include "common_search.h"

#include "learning.hpp"

namespace Controller
{
#include "VariableOrders.h"

  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)
  
  //return value is d, and d+1 is the depth at which we start again
  //-2 is stop and -1 restart

  template<typename VarOrder, typename Var, typename Propogator>
  inline int solve_loop_recursive(StateObj* stateObj, VarOrder& order, vector<Var>& v, Propogator prop = PropagateGAC())
  {
    getState(stateObj).incrementNodeCount();
    if(do_checks(stateObj))
      return -2;
    
    if(order.find_next_unassigned()) { //true iff all vars assigned
      deal_with_solution(stateObj);
      return -2;
    }

    Var& cv = v[order.pos];

    for(DomainInt i = cv.getMin(); i <= cv.getMax(); i++) {
      if(cv.inDomain(i)) {
	maybe_print_search_state(stateObj, "Node: ", v);
	world_push(stateObj);
	cv.setExpl(true, i, VirtConPtr(new DecisionAssg<Var>(stateObj, cv, i)));
	cv.decisionAssign(i);
	maybe_print_search_assignment(stateObj, cv, i, true);
	prop(stateObj, v);
	if(getState(stateObj).isFailed()) {
	  firstUipLearn(stateObj, getState(stateObj).getFailure());
	  getState(stateObj).setFailed(false);
	  world_pop(stateObj);
	} else {
	  int target = solve_loop_recursive(stateObj, order, v, prop);
	  if(target + 1 != getMemory(stateObj).backTrack().current_depth())
	    return target;
	  getState(stateObj).setFailed(false);
	  world_pop(stateObj);
	}
        maybe_print_search_action(stateObj, "bt");
      }
    }
    return getMemory(stateObj).backTrack().current_depth() - 1;
  }
}




