/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/
#define NO_MAIN

#include "minion.h"

namespace Controller
{
/// Lists all structures that must be locked before search.
// @todo This could be done more neatly... 

void lock(StateObj* stateObj)
{
  D_INFO(2, DI_SOLVER, "Starting Locking process");
  stateObj->varCon().getRangevarContainer().lock();
  stateObj->varCon().getBigRangevarContainer().lock();
  stateObj->varCon().getSparseBoundvarContainer().lock();
  stateObj->varCon().getBooleanContainer().lock(); 
  stateObj->varCon().getBoundvarContainer().lock();
#ifdef DYNAMICTRIGGERS
  int dynamic_size = stateObj->state().getDynamicConstraintList().size();
  for(int i = 0; i < dynamic_size; ++i)
	stateObj->state().getDynamicConstraintList()[i]->setup();
#endif
  stateObj->searchMem().backTrack().lock();
  stateObj->searchMem().nonBackTrack().lock();
//  atexit(Controller::finish);
  
  int size = stateObj->state().getConstraintList().size();
  for(int i = 0 ; i < size;i++)
	stateObj->state().getConstraintList()[i]->setup();
  
  stateObj->triggerMem()->finaliseTriggerLists();
  
  bool prop_to_do = true;
#ifdef USE_SETJMP
  int setjmp_return = SYSTEM_SETJMP(*(stateObj->state().getJmpBufPtr()));
  if(setjmp_return != 0)
  {
	stateObj->state().setFailed(true);
	stateObj->queues().clearQueues();
	return;
  }
#endif
  while(prop_to_do)
  {
	prop_to_do = false;
	// We can't use incremental propagate until all constraints
	// have been setup, so this slightly messy loop is necessary
	// To propagate the first node.
	for(int i = 0; i < size; ++i)
	{
	  stateObj->state().getConstraintList()[i]->full_propagate();
	  if(stateObj->state().isFailed()) 
		return;
	  // If queues not empty, more work to do.
	  if(!stateObj->queues().isQueuesEmpty())
	  {
		stateObj->queues().clearQueues();
		prop_to_do = true;
	  }
	}
  }
  
#ifdef DYNAMICTRIGGERS
  for(int i = 0; i < dynamic_size; ++i)
  {
	stateObj->state().getDynamicConstraintList()[i]->full_propagate();
	stateObj->queues().propagateQueue();
	if(stateObj->state().isFailed()) 
	  return;
  }
#endif

  stateObj->state().markLocked();

} // lock()
}
