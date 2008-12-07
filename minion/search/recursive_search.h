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
  inline int search(StateObj* stateObj, VarOrder& order, vector<Var>& v, Propogator prop = PropagateGAC())
  {
    D_ASSERT(getQueue(stateObj).isQueuesEmpty());

    getState(stateObj).incrementNodeCount();
    if(do_checks(stateObj))
      return -2;
    
    if(order.find_next_unassigned()) { //true iff all vars assigned
      deal_with_solution(stateObj);
      return -2;
    }

    int target;
    Var& cv = v[order.pos];

    maybe_print_search_state(stateObj, "Node: ", v);
    world_push(stateObj);
    cv.setExpl(true, cv.getMin(), VirtConPtr(new DecisionAssg<Var>(stateObj, cv, cv.getMin())));
    cv.decisionAssign(cv.getMin());
    maybe_print_search_assignment(stateObj, cv, cv.getMin(), true);
    prop(stateObj, v);
    if(getState(stateObj).isFailed()) {
      cout << "failed after prop" << endl;
      target = firstUipLearn(stateObj, getState(stateObj).getFailure());
      D_ASSERT(getQueue(stateObj).isQueuesEmpty());
    } else {
      cout << "assignment succeeded - recursing" << endl;
      target = search(stateObj, order, v, prop);
    }
    while(true) {
      getState(stateObj).setFailed(false);
      if(target < getMemory(stateObj).backTrack().current_depth()) {
	cout << "jumping beyond" << endl;
	getQueue(stateObj).clearQueues();
	D_ASSERT(getQueue(stateObj).isQueuesEmpty());
	world_pop(stateObj);
	maybe_print_search_action(stateObj, "bt");
	return target;
      }
      prop(stateObj, v);
      if(getState(stateObj).isFailed()) {
	cout << "failed on return" << endl;
	target = firstUipLearn(stateObj, getState(stateObj).getFailure());
	D_ASSERT(getQueue(stateObj).isQueuesEmpty());	
	world_pop(stateObj);
	maybe_print_search_action(stateObj, "bt");
	return target;
      }
      cout << "recursing on return" << endl;
      prop(stateObj, v);
      target = search(stateObj, order, v, prop);
    }
  }

  template<typename VarOrder, typename Var, typename Propogator>
  inline int solve_loop_recursive(StateObj* stateObj, VarOrder& order, vector<Var>& v, Propogator prop = PropagateGAC())
  {
    while(true) { 
      int res = search(stateObj, order, v, prop);   //possibilities are 0, -1 and -2
      D_ASSERT(res == 0 || res == -1 || res == -2); //resp. retry 1st branch, restart and finished
      if(res == -2) //this signals completion
	return -2;
      prop(stateObj, v);
      if(getState(stateObj).isFailed())
	return -2; //failed at root node means failed!
    }
  }
}
