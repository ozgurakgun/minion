/* Minion Constraint Solver
http://minion.sourceforge.net

For Licence Information see file LICENSE.txt 

  $Id$
*/

/* Minion
  * Copyright (C) 2006
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License
  * as published by the Free Software Foundation; either version 2
  * of the License, or (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef DYNAMIC_WATCHED_OR_NEW_H
#define DYNAMIC_WATCHED_OR_NEW_H

#include "../constraints/constraint_abstract.h"
#include "../reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl;
#define P(x)

  struct Dynamic_OR : public ParentConstraint
{
  virtual string constraint_name()
    { return "Dynamic OR:"; }


  Reversible<bool> full_propagate_called;
  bool constraint_locked;
  
  int assign_size;

  int propagated_constraint;

  int watched_constraint[2];

  Dynamic_OR(StateObj* _stateObj, vector<AbstractConstraint*> _con) : 
    ParentConstraint(_stateObj, _con), full_propagate_called(_stateObj, false), assign_size(-1),
       constraint_locked(false)
    {
      size_t max_size = 0;
      for(int i = 0; i < child_constraints.size(); ++i)
        max_size = max(max_size, child_constraints[i]->get_vars_singleton()->size());
      assign_size = max_size;
    }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    for(int i = 0; i < child_constraints.size(); ++i)
    {
      if(child_constraints[i]->check_assignment(v + start_of_constraint[i],
         child_constraints[i]->get_vars_singleton()->size()))
         return true;
    }
    return false;
  }
  
  virtual void get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    for(int i = 0; i < child_constraints.size(); ++i)
    {
      child_constraints[i]->get_satisfying_assignment(assignment);
      if(!assignment.empty())
      {
        // Fix up assignment
        int var_start = start_of_constraint[i];
        for(int j = 0; j < assignment.size(); ++j)
        {
          assignment[j].first += start_of_constraint[i];
        }
        return; 
      }
    }
  }
  

  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vecs;
    for(int i = 0; i < child_constraints.size(); ++i)
    {
      vector<AnyVarRef>* var_ptr = child_constraints[i]->get_vars_singleton(); 
      vecs.insert(vecs.end(), var_ptr->begin(), var_ptr->end());
    }
    return vecs;
  }

  virtual int dynamic_trigger_count() 
  { 
    return assign_size * 2;
  }

  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
    P("Full propagating: " << propagated_constraint);
    child_constraints[propagated_constraint]->full_propagate();
    full_propagate_called = true;
  }

  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
  }

  PROPAGATE_FUNCTION(DynamicTrigger* trig)
  {
    //PROP_INFO_ADDONE(WatchedOr);
    P("Prop");
    P("Current: " << watched_constraint[0] << " . " << watched_constraint[1]);
    if(constraint_locked)
      return;

    DynamicTrigger* dt = dynamic_trigger_start();

    if(!full_propagate_called && //this line may be necessary for either performance or correctness
       trig >= dt && trig < dt + assign_size * 2)
    {
      int tripped_constraint = (trig - dt) / assign_size;
      int other_constraint = 1 - tripped_constraint;
      P("Tripped: " << tripped_constraint << ":" << watched_constraint[tripped_constraint]);
      D_ASSERT(tripped_constraint == 0 || tripped_constraint == 1);

      GET_ASSIGNMENT(assignment_try, child_constraints[watched_constraint[tripped_constraint]]);
      if(!assignment_try.empty())
      { // Found new support without having to move.
        watch_assignment(child_constraints[watched_constraint[tripped_constraint]], 
                         dt + tripped_constraint * assign_size, assignment_try);
        P("Fixed, returning");
        return; 
      }
      
      const size_t cons_s = child_constraints.size();
      for(int i = 0; i < cons_s; ++i)
      {
        if(i != watched_constraint[0] && i != watched_constraint[1])
        {
          GET_ASSIGNMENT(assignment, child_constraints[i]);
          if(!assignment.empty())
          {
            watch_assignment(child_constraints[i], dt + tripped_constraint * assign_size, assignment);
            watched_constraint[tripped_constraint] = i;
            P("New support. Switch " << tripped_constraint << " to " << i);
            return;
          }
        }
      }
      
      P("Start propagating " << watched_constraint[other_constraint]);
      // Need to propagate!
      propagated_constraint = watched_constraint[other_constraint];
      child_constraints[propagated_constraint]->full_propagate();
      full_propagate_called = true;
      //the following may be necessary for correctness for some constraints
      //constraint_locked = true;
      //getQueue(stateObj).pushSpecialTrigger(this);
      return;
    }


    if(full_propagate_called && dynamic_trigger_to_constraint[trig] == propagated_constraint)
    { child_constraints[propagated_constraint]->propagate(trig); }
    else
    {
      // This is an optimisation.
      trig->remove();
    }
  }

  void watch_assignment(AbstractConstraint* con, DynamicTrigger* dt, box<pair<int,DomainInt> >& assignment)
  {
    vector<AnyVarRef>& vars = *(con->get_vars_singleton());
    for(int i = 0; i < assignment.size(); ++i)
      vars[assignment[i].first].addDynamicTrigger(dt + i, DomainRemoval, assignment[i].second);
  }

  virtual void full_propagate()
  {
    P("Full Propagate")
    DynamicTrigger* dt = dynamic_trigger_start();

    // Clean up triggers
    for(int i = 0; i < assign_size * 2; ++i)
      dt[i].remove();

    int loop = 0;

    bool found_watch = false;

    while(loop < child_constraints.size() && !found_watch)
    {
      GET_ASSIGNMENT(assignment, child_constraints[loop]);
      if(!assignment.empty())
      {
        found_watch = true;
        watched_constraint[0] = loop;
        watch_assignment(child_constraints[loop], dt, assignment);
      }
      loop++;
    }

    if(found_watch == false)
    {
      getState(stateObj).setFailed(true);
      return;
    }

    P("Found watch 0: " << loop);
    
    found_watch = false;

    while(loop < child_constraints.size() && !found_watch)
    {
      GET_ASSIGNMENT(assignment, child_constraints[loop]);
      if(!assignment.empty())
      {
        found_watch = true;
        watched_constraint[1] = loop;
        watch_assignment(child_constraints[loop], dt + assign_size, assignment);
        P("Found watch 1: " << loop);
        return;
      }
      loop++;
    }

    if(found_watch == false)
    { 
      propagated_constraint = watched_constraint[0];
      constraint_locked = true;
	    getQueue(stateObj).pushSpecialTrigger(this);
    }

  }
};




inline AbstractConstraint*
BuildCT_WATCHED_NEW_OR(StateObj* stateObj, BOOL reify, 
                       const BoolVarRef& reifyVar, ConstraintBlob& bl)
{
  vector<AbstractConstraint*> cons;
  for(int i = 0; i < bl.internal_constraints.size(); ++i)
    cons.push_back(build_constraint(stateObj, bl.internal_constraints[i]));


  if(reify) {
    cerr << "Cannot reify 'watched or' constraint." << endl;
    exit(0);
  } else {
    return new Dynamic_OR(stateObj, cons);
  }
}


#endif
