/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
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

/** @help constraints;watched-and Description
The constraint

  watched-and({C1,...,Cn})

ensures that the constraints C1,...,Cn are all true.
*/

/** @help constraints;watched-and Notes Conjunctions of constraints may seem
pointless, bearing in mind that a CSP is simply a conjunction of constraints
already! However sometimes it may be necessary to use a conjunction as a child
of another constraint, for example in a reification:

   reify(watched-and({...}),r)
*/

/** @help constraints;watched-and References
  See also

  help constraints watched-or
*/

#ifndef DYNAMIC_WATCHED_AND_NEW_H
#define DYNAMIC_WATCHED_AND_NEW_H

#include "../constraints/constraint_abstract.h"
#include "../memory_management/reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"



#ifdef P
#undef P
#endif

#define P(x) cout << x << endl
//#define P(x)

// Similar to watched or, but has no watching phase, just propagates all
// the time, and propagates all constraints of course.


struct Dynamic_AND : public ParentConstraint
{
  virtual string constraint_name()
    { return "Dynamic AND:"; }

    
  Reversible<bool> full_propagate_called;
  bool constraint_locked;
  
  Dynamic_AND(StateObj* _stateObj, vector<AbstractConstraint*> _con) : 
    ParentConstraint(_stateObj, _con), full_propagate_called(_stateObj, false), constraint_locked(false)
    { }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    for(int i = 0; i < child_constraints.size(); ++i)
    {
      if(! child_constraints[i]->check_assignment(v + start_of_constraint[i],
         child_constraints[i]->get_vars_singleton()->size()))
         return false;
    }
    return true;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
      // get all satisfying assignments of child constraints and stick
      // them together. Even if they contradict each other.
      typedef pair<int,DomainInt> temptype;
      MAKE_STACK_BOX(localassignment, temptype, assignment.capacity());
      
      for(int i=0; i<child_constraints.size(); ++i)
      {
          localassignment.clear();
          bool flag=child_constraints[i]->get_satisfying_assignment(localassignment);
          if(!flag)
          {
              return false;
          }
          
          for(int j=0; j<localassignment.size(); j++)
          {
              assignment.push_back(make_pair(localassignment[j].first+start_of_constraint[i],
                  localassignment[j].second));
          }
      }
      return true;
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
    return 0;
  }

  virtual BOOL special_check()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
    P("Full propagating all constraints in AND");
    for(int i=0; i<child_constraints.size(); i++)
    {
        if(!child_constraints[i]->full_propagate())
            return false;
    }
    full_propagate_called = true;
    return true;
  }

  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
  }
  
  virtual BOOL propagate(int i, DomainDelta domain)
  {
    //PROP_INFO_ADDONE(WatchedOR);
    P("Static propagate start");
    if(constraint_locked)
      return true;

    if(full_propagate_called)
    {
      pair<int,int> childTrigger = getChildStaticTrigger(i);
      P("Got trigger: " << i << ", maps to: " << childTrigger.first << "." << childTrigger.second);
      P("Passing trigger" << childTrigger.second << "on");
      if(!child_constraints[childTrigger.first]->propagate(childTrigger.second, domain))
        return false;
    }
    else
    {
        // add to the special queue so it can be full_propagated later.
        constraint_locked = true;
        getQueue(stateObj).pushSpecialTrigger(this);
    }
    return true;
  }
  
  virtual BOOL propagate(DynamicTrigger* trig)
  {
    //PROP_INFO_ADDONE(WatchedOr);
    P("Prop");
    P("FullProp: " << (bool)full_propagate_called);
    P("Locked:" << constraint_locked);
    if(constraint_locked)
      return true;
    
      if(!full_propagate_called)
      {
          // how did we get here?
          constraint_locked = true;
            getQueue(stateObj).pushSpecialTrigger(this);
            return true;
      }
  
    // pass the trigger down
    P("Propagating child");
    // need to know which child to prop.
    int child = getChildDynamicTrigger(trig);
    return child_constraints[child]->propagate(trig);
  }
  
  virtual BOOL full_propagate()
  {
    P("Full Propagate");
    
    // push it on the special queue to be full_propagated later.
    D_ASSERT(!constraint_locked);
    constraint_locked = true;
    getQueue(stateObj).pushSpecialTrigger(this);
    return true;
  }
  
  // This breaks everything.
  //virtual AbstractConstraint* reverse_constraint();
};

//#include "dynamic_new_or.h"

/*virtual AbstractConstraint* Dynamic_AND::reverse_constraint()
{ // OR of the reverse of all the child constraints..
  vector<AbstractConstraint*> con;
  for(int i=0; i<child_constraints.size(); i++)
  {
      con.push_back(child_constraints[i]->reverse_constraint());
  }
  return new Dynamic_OR(stateObj, con);
}*/
#endif
