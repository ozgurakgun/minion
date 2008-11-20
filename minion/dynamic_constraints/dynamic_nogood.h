//Constraint which encodes a nogood. It is just a normal constraint, but it
//actually propagates the negative of a nogood. It is just a subclass of
//watched or: additionally it will deallocate its triggers and children when the
//deconstructor is called, and it it also stores itself at getFailure() when it
//detects disentailment.

//We assume that the disjuncts provided are allocated on the heap and that they
//can be freed when the constraint is freed. If they need to be kept might need
//to start garbage collecting them.

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

  PROPAGATE_FUNCTION(DynamicTrigger* trig)
  {
    //PROP_INFO_ADDONE(WatchedOr);
    P("Prop");
    P("Current: " << watched_constraint[0] << " . " << watched_constraint[1]);
    P("FullPropOn: " << (bool)full_propagate_called << ", on: " << propagated_constraint);
    P("Locked:" << constraint_locked);
    if(constraint_locked)
      return;

    DynamicTrigger* dt = dynamic_trigger_start();

    P("Trig: " << trig - dt);

    if(trig >= dt && trig < dt + assign_size * 2)
    {
      if(full_propagate_called)
        return;

      int tripped_constraint = (trig - dt) / assign_size;
      int other_constraint = 1 - tripped_constraint;
      P("Tripped: " << tripped_constraint << ":" << watched_constraint[tripped_constraint]);
      D_ASSERT(tripped_constraint == 0 || tripped_constraint == 1);
      
      bool flag;
      GET_ASSIGNMENT(assignment_try, child_constraints[watched_constraint[tripped_constraint]]);
      if(flag)
      { // Found new support without having to move.
        watch_assignment(child_constraints[watched_constraint[tripped_constraint]], 
                         dt + tripped_constraint * assign_size, assignment_try);
        for(int i = 0; i < assignment_try.size(); ++i)
          P(assignment_try[i].first << "." << assignment_try[i].second << "  ");
        P(" -- Fixed, returning");
        return; 
      }
      
      const size_t cons_s = child_constraints.size();
      for(int i = 0; i < cons_s; ++i)
      {
        if(i != watched_constraint[0] && i != watched_constraint[1])
        {
          GET_ASSIGNMENT(assignment, child_constraints[i]);
          if(flag)
          {
            watch_assignment(child_constraints[i], dt + tripped_constraint * assign_size, assignment);
            watched_constraint[tripped_constraint] = i;
            P("New support. Switch " << tripped_constraint << " to " << i);
            return;
          }
        }
      }
      
      cout << flag << endl;
      GET_ASSIGNMENT(notneeded, child_constraints[watched_constraint[other_constraint]]);
      cout << child_constraints[watched_constraint[other_constraint]]->whenF() << endl;
      cout << flag << endl;
      if(!flag)
      {
	cout << "nogood failed 2" << endl;
	getState(stateObj).setFailure(VirtConPtr(new Anything(vcs)));
	getState(stateObj).setFailed(true);
	return;
      }
      
      P("Start propagating " << watched_constraint[other_constraint]);
      // Need to propagate!
      propagated_constraint = watched_constraint[other_constraint];
      //the following may be necessary for correctness for some constraints
#ifdef SLOW_WOR
      constraint_locked = true;
      getQueue(stateObj).pushSpecialTrigger(this);
#else
      child_constraints[propagated_constraint]->full_propagate();
      full_propagate_called = true;
#endif
      return;
    }


    if(full_propagate_called && getChildDynamicTrigger(trig) == propagated_constraint)
    { 
      P("Propagating child");
      child_constraints[propagated_constraint]->propagate(trig); 
    }
    else
    {
      P("Clean old trigger");
      // This is an optimisation.
      trig->remove();
    }
  }

  virtual void full_propagate()
  {
    cout << "full nogood prop" << endl;
    P("Full Propagate")
    DynamicTrigger* dt = dynamic_trigger_start();

    // Clean up triggers
    for(int i = 0; i < assign_size * 2; ++i)
      dt[i].remove();

    int loop = 0;

    bool found_watch = false;
    
    while(loop < child_constraints.size() && !found_watch)
    {
      bool flag;
      GET_ASSIGNMENT(assignment, child_constraints[loop]);
      if(flag)
      {
        found_watch = true;
        watched_constraint[0] = loop;
        watch_assignment(child_constraints[loop], dt, assignment);
        for(int i = 0; i < assignment.size(); ++i)
          P(assignment[i].first << "." << assignment[i].second << "  ");
      }
      else
        loop++;
    }

    if(found_watch == false)
    {
      cout << "nogood failed 1" << endl;
      getState(stateObj).setFailure(VirtConPtr(new Anything(vcs)));
      getState(stateObj).setFailed(true);
      return;
    }

    P(" -- Found watch 0: " << loop);
    loop++;
    
    found_watch = false;
    
    while(loop < child_constraints.size() && !found_watch)
    {
      bool flag;
      GET_ASSIGNMENT(assignment, child_constraints[loop]);
      if(flag)
      {
        found_watch = true;
        watched_constraint[1] = loop;
        watch_assignment(child_constraints[loop], dt + assign_size, assignment);
        for(int i = 0; i < assignment.size(); ++i)
          P(assignment[i].first << "." << assignment[i].second << "  ");
        P(" -- Found watch 1: " << loop);
        return;
      }
      else
        loop++;
    }

    if(found_watch == false)
    { 
      propagated_constraint = watched_constraint[0];
      constraint_locked = true;
	    getQueue(stateObj).pushSpecialTrigger(this);
    }

  }
  
  //Be careful about freeing this memory, in case we want to remove these during
  //search.
  virtual ~NogoodConstraint()
  {
    delete[] trigs_ptr;
    for(int i = 0; i < child_constraints.size(); i++)
      delete child_constraints[i];
  }
};
