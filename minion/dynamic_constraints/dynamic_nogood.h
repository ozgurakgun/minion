//Constraint which encodes a nogood. It is just a normal constraint, but it
//actually propagates the negative of a nogood. It is just a subclass of
//watched or.

//We assume that the disjuncts provided are allocated on the heap and that they
//can be freed when the constraint is freed (by superclass). If they need to be
//kept might need to start garbage collecting them.

#include "dynamic_new_or.h"

class NogoodConstraint : public Dynamic_OR
{
 public:

  //remove effects of this having been propagated
  virtual void cleanup()
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    for(int i = 0; i < assign_size * 2; ++i)
      dt[i].remove(getQueue(stateObj).getNextQueuePtrRef());
    getState(stateObj).setFailed(false);
    getQueue(stateObj).clearQueues();
  }

  vector<AbstractConstraint*> toCons(vector<VirtConPtr>& vcs)
  {
    vector<AbstractConstraint*> retVal;
    retVal.reserve(vcs.size());
    for(size_t i = 0; i < vcs.size(); i++)
      retVal.push_back(vcs[i]->getNeg());
    return retVal;
  }
  
  NogoodConstraint(StateObj* _stateObj, vector<VirtConPtr> _vcs) :
    Dynamic_OR(_stateObj, toCons(_vcs)) {}

  virtual void full_propagate()
  { 
    full_propagate_called = false; //reset BT memory
    Dynamic_OR::full_propagate(); //call superclass to do the job
  }
};
