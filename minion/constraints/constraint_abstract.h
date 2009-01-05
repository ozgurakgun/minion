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

/** @help constraints Description
  Minion supports many constraints and these are regularly being
  improved and added to. In some cases multiple implementations of the
  same constraints are provided and we would appreciate additional
  feedback on their relative merits in your problem.

  Minion does not support nesting of constraints, however this can be
  achieved by auxiliary variables and reification.

  Variables can be replaced by constants. You can find out more on
  expressions for variables, vectors, etc. in the section on variables.
*/

/** @help constraints References 
  help variables
*/

#ifndef ABSTRACT_CONSTRAINT
#define ABSTRACT_CONSTRAINT

#include "../system/system.h"
#include "../solver.h"
#include "../variables/AnyVarRef.h"
#include <vector>

  using namespace std;

class AnyVarRef;
class DynamicTrigger;

#include "dynamic_trigger.h"

#define DYNAMIC_PROPAGATE_FUNCTION virtual void propagate

#define PROPAGATE_FUNCTION virtual void propagate

struct AbstractTriggerCreator;
typedef vector<shared_ptr<AbstractTriggerCreator> > triggerCollection;

class ParentConstraint;

/// Base type from which all constraints are derived.
class AbstractConstraint
{
protected:
  /// Private members of the base class.
  
  DynamicTrigger* trigs_ptr;
  vector<AnyVarRef>* singleton_vars;

public:

  StateObj* stateObj;

  #ifdef WDEG
  unsigned int wdeg;
  #endif

  BOOL disjunct; //true iff the constraint is a disjunct in a watched or
  Dynamic_OR* parent; //disjunction it's in, if any

  BOOL full_propagate_done;

  /// Returns a point to the first dynamic trigger of the constraint.
  DynamicTrigger* dynamic_trigger_start()
    { return trigs_ptr; }

  /// Defines the number of dynamic triggers the constraint wants.
  /// Must be implemented by any constraint.
  virtual int dynamic_trigger_count() 
    { return 0; }

  /// Returns the number of dynamic triggers for this constraint, and all the children of
  /// this constraint. Only differs from the above for things like 'reify', 'reifyimply' and 'or'.
  virtual int dynamic_trigger_count_with_children()
    { return dynamic_trigger_count(); }

  /// Gets all the triggers a constraint wants to set up.
  /** This function shouldn't do any propagation. That is full_propagate's job.*/
  virtual triggerCollection setup_internal()
    { return triggerCollection(); }

  /// Iterative propagation function.
  /** Can assume full_propagate is always called at least once before propagate */
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger*)
    { D_FATAL_ERROR("Fatal error in 'Dynamic Propagate' in " + constraint_name()); }

  // Alternative DPF to be used for disjuncts
  DYNAMIC_PROPAGATE_FUNCTION(Dynamic_OR* dj, DynamicTrigger* dt)
  {
    D_ASSERT(disjunct);
    parent = dj;
    propagate(dt);
  }    

  /// Iterative propagation function.
  /** Can assume full_propagate is always called at least once before propagate */
  PROPAGATE_FUNCTION(int, DomainDelta) 
    { D_FATAL_ERROR("Fatal error in 'Static Propagate' in " + constraint_name()); }

  /// Checks if a constraint cannot be satisfied, and sets up any data structures for future incremental checks.
  /// Returns TRUE if constraint cannot be satisfied.
  /** This function is used by rarification */
  virtual BOOL full_check_unsat()
  { 
    cerr << "Static reification is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
    exit(1); 
  }

  /// Checks incrementaly if constraint cannot be satisfied.
  /// Returns TRUE if constraint cannot be satisfied.
  /** This function should not be called unless check_unsat_full is called first. This is used by rarification */
  virtual BOOL check_unsat(int,DomainDelta)
  { 
    cerr << "Static reification is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
    exit(1); 
  }

  /// Looks for a valid partial assignment to a constraint.
  /** The return value (in the box) is pairs of <varnum, domain value>, where varnum is in the same position
  *  as returned by get_vars.
   */
    virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    cerr << "Finding assignment is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
    exit(1);
  }

  /// Returns the reverse of the current constraint
  /** Used by rarification */
  virtual AbstractConstraint* reverse_constraint()
  {
    cerr << "Negation is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
    exit(1);
  }

  AbstractConstraint(StateObj* _stateObj) : 
    singleton_vars(NULL), stateObj(_stateObj), 
#ifdef WDEG
    wdeg(1),
#endif
    disjunct(false), full_propagate_done(false)
    {}

  /// Method to get constraint name for debugging.
  virtual string constraint_name() = 0;

  /// Performs a full round of propagation and sets up any data needs by propagate().
  /** This function can be called during search if the function is reified */
  virtual void full_propagate() = 0;

  // Special FP for disjuncts in a watched or. If a con is a disjunct the other
  // FP should never be invoked.
  virtual void full_propagate(Dynamic_OR* dj)
  { 
    D_ASSERT(disjunct);
    parent = dj;
    full_propagate();
  }

  // Returns the variables of the constraint
  virtual vector<AnyVarRef> get_vars() = 0;

  vector<AnyVarRef>* get_vars_singleton() //piggyback singleton vector on get_vars()
  { 
    if(singleton_vars == NULL) singleton_vars = new vector<AnyVarRef>(get_vars()); //for efficiency: no constraint over 0 variables
    return singleton_vars; 
  }

#ifdef WDEG
  unsigned int getWdeg();

  void incWdeg();
#endif

  /// Allows functions to activate a special kind of trigger, run only
  /// after the normal queue is empty.
  virtual void special_check()
  { 
    cerr << "Serious internal error" << endl;
    FAIL_EXIT(); 
  }

  // Called if failure occurs without actiating a special trigger, so the constraint can unlock.
  virtual void special_unlock()
  { 
    cerr << "Serious internal error" << endl;
    FAIL_EXIT(); 
  }

  /// Checks if an assignment is satisfied.
  /** This takes the variable order returned by, and is mainly only used by, get_table_constraint() */
  virtual BOOL check_assignment(DomainInt* v, int v_size) = 0;

  virtual ~AbstractConstraint()
  { 
    if(!disjunct) delete[] trigs_ptr; //Only non-disjuncts have their own memory
    delete singleton_vars;
  } 

  virtual void setup_dynamic_triggers(DynamicTrigger* ptr)
    { trigs_ptr = ptr; }


  virtual triggerCollection setup_internal_gather_triggers()
    { return setup_internal(); }

  /// Actually creates the dynamic triggers. Calls dynamic_trigger_count from function to get
  /// the number of triggers required.
  virtual void setup()
  {
    // Dynamic initialisation
    int trigs = dynamic_trigger_count();
    D_ASSERT(trigs >= 0);
    setup_dynamic_triggers(new DynamicTrigger[trigs]);

    DynamicTrigger* start = dynamic_trigger_start();
    for(int i = 0 ; i < trigs; ++i)
      new (start+i) DynamicTrigger(this);

    // Static initialisation
    triggerCollection t = setup_internal_gather_triggers();
    for(triggerCollection::iterator it = t.begin(); it != t.end(); ++it)
    {
      (*it)->post_trigger();
    }
  }

  virtual pair<unsigned,unsigned> whenF() const //when did this constraint become disentailed
  { D_ASSERT(false); return make_pair(-1, -1); }

  virtual vector<VirtConPtr> whyF() const //why is it disentailed?
  { D_ASSERT(false); return vector<VirtConPtr>(); }

  template<typename VarRef>
  void storeExpl(bool assg, VarRef& var, DomainInt i, VirtConPtr vcp)
  {
    if(!disjunct) var.setExpl(assg, i, vcp);
    else var.setExpl(assg, i, VirtConPtr(new DisjunctionPrun(this, vcp, parent)));
  }

  virtual void print(std::ostream& o) const { o << "UnknownConstraint"; }
  
  friend std::ostream& operator<<(std::ostream& o, const AbstractConstraint& c) 
  { c.print(o); return o; }
  
  virtual AbstractConstraint* copy() const
  { D_ASSERT(false); return NULL; }

  virtual size_t hash() const
  { D_ASSERT(false); return 0; }

  virtual bool equal(AbstractConstraint* other) const
  { D_ASSERT(false); return false; }
};

/// Constraint from which other constraints can be inherited. Extends dynamicconstraint to allow children to be dynamic.
class ParentConstraint : public AbstractConstraint
{
protected:
  // Maps a dynamic trigger to the constraint which it belongs to.
  vector<int> _dynamic_trigger_to_constraint;
  // Maps a static trigger to a pair { constraint, trigger for that constraint } 
  vector< pair<int, int> > _static_trigger_to_constraint;
  // Maps variables to constraints
  vector<int> variable_to_constraint;
  // Gets start of each constraint
  vector<int> start_of_constraint;
public:

  vector<AbstractConstraint*> child_constraints;

  pair<int,int> getChildStaticTrigger(int i)
    { return _static_trigger_to_constraint[i]; }
    
  int getChildDynamicTrigger(DynamicTrigger* ptr)
  { 
    return _dynamic_trigger_to_constraint[ptr - dynamic_trigger_start()]; 
  }

  /// Gets all the triggers a constraint wants to set up.
  /** This function shouldn't do any propagation. That is full_propagate's job.*/
  virtual triggerCollection setup_internal_gather_triggers()
  {     
    triggerCollection newTriggers;
    
    for(int i = 0; i < child_constraints.size(); i++)
    {
      triggerCollection childTrigs = child_constraints[i]->setup_internal_gather_triggers();
      for(int j = 0; j < childTrigs.size(); ++j)
      {
        // Record each original trigger value, then add the modified trigger to the collection.
        _static_trigger_to_constraint.push_back(make_pair(i, childTrigs[j]->trigger.info));
        // Need a '-1' on the next line, as C++ containers are indexed from 0!
        childTrigs[j]->trigger.info = _static_trigger_to_constraint.size() - 1;
        childTrigs[j]->trigger.constraint = this;
        newTriggers.push_back(childTrigs[j]);
      }
    }
    
    triggerCollection parentTrigs = setup_internal();
    for(int i = 0; i < parentTrigs.size(); ++i)
      D_ASSERT(parentTrigs[i]->trigger.info < 0);
    newTriggers.insert(newTriggers.end(), parentTrigs.begin(), parentTrigs.end());
    
    return newTriggers;
  }

  ParentConstraint(StateObj* _stateObj, const vector<AbstractConstraint*> _children = vector<AbstractConstraint*>()) : 
  AbstractConstraint(_stateObj), child_constraints(_children)
  {
    int var_count = 0;
    for(int i = 0; i < child_constraints.size(); ++i)
    {
      start_of_constraint.push_back(var_count);
      int con_size = child_constraints[i]->get_vars_singleton()->size();
      for(int j = 0; j < con_size; ++j)
      {
        variable_to_constraint.push_back(i);
      }
      var_count += con_size;
    }
  }

  virtual int dynamic_trigger_count_with_children()
  {
    int trigger_count = dynamic_trigger_count();
    for(int i = 0; i < child_constraints.size(); ++i)
      trigger_count += child_constraints[i]->dynamic_trigger_count_with_children();
    return trigger_count;
  }

  virtual void setup_dynamic_triggers(DynamicTrigger* trigs)
  {
    trigs_ptr = trigs;

    int current_trigger_count = dynamic_trigger_count();
    
    for(int count = 0; count < current_trigger_count; count++)
      _dynamic_trigger_to_constraint.push_back(child_constraints.size());
    
    for(int i = 0; i < child_constraints.size(); ++i)
    {
      // We need this check to ensure we don't try constructing a "start of trigger" block one off the
      // the end of memory array.
      if(current_trigger_count == dynamic_trigger_count_with_children())
        return;
        
      // Get start child's dynamic triggers.      
      DynamicTrigger* childPtr = trigs_ptr + current_trigger_count;
      child_constraints[i]->setup_dynamic_triggers(childPtr);
      
      int child_trig_count = child_constraints[i]->dynamic_trigger_count_with_children();
      
      for(int count = current_trigger_count; count < current_trigger_count + child_trig_count; ++count)
        _dynamic_trigger_to_constraint.push_back(i);
        
      current_trigger_count += child_trig_count;
    }
  }

  /// Actually creates the dynamic triggers. Calls dynamic_trigger_count from function to get
  /// the number of triggers required.
  virtual void setup()
  {
    // Dynamic initialisation
    int all_trigs = dynamic_trigger_count_with_children();

    DynamicTrigger* trigMem = new DynamicTrigger[all_trigs];
    
    // Start by allocating triggers in the memory block
    for(int i = 0 ; i < all_trigs; ++i)
      new (trigMem+i) DynamicTrigger(this);

    setup_dynamic_triggers(trigMem);

    // Static initialisation
    triggerCollection t = setup_internal_gather_triggers();
    for(triggerCollection::iterator it = t.begin(); it != t.end(); ++it)
    {
      (*it)->post_trigger();
    }
  }

  virtual ~ParentConstraint()
  {
    for(int i = 0; i < child_constraints.size(); ++i)
      delete child_constraints[i];
  }
};



#endif

