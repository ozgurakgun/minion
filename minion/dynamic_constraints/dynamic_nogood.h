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
  vector<VirtConPtr> vcs;

  vector<AbstractConstraint*> toCons(vector<VirtConPtr>& vcs)
  {
    vector<AbstractConstraint*> retVal;
    retVal.reserve(vcs.size());
    for(size_t i = 0; i < vcs.size(); i++)
      retVal.push_back(vcs[i]->getNeg());
    return retVal;
  }
  
  NogoodConstraint(StateObj* _stateObj, vector<VirtConPtr> _vcs) :
    Dynamic_OR(_stateObj, toCons(_vcs)), vcs(_vcs) {}
};
