/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: constraint.h 701 2007-10-09 14:12:05Z azumanga $
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

struct AbstractTriggerCreator;

typedef vector<shared_ptr<AbstractTriggerCreator> > triggerCollection;

#ifdef  FUNCTIONPOINTER_TRIGGER
#define PROPAGATE_FUNCTION void propagate
#else
#define PROPAGATE_FUNCTION virtual void propagate
#endif

/// Base type from which all constraints are derived.
class Constraint
{
public:
  StateObj* stateObj;
  
  Constraint(StateObj* _stateObj) : stateObj(_stateObj)
  { }
  
  virtual string constraint_name() = 0;
  
  /// Function which impose all the triggers generated by setup_internal, which shouldn't be changed.
  void setup();

  /// Gets all the triggers a constraint wants to set up.
  /** This function shouldn't do any propagation. That is full_propagate's job.*/
  virtual triggerCollection setup_internal() = 0;

  
  // In function_defs.hpp
  //virtual Constraint* get_table_constraint();

  /// Performs a full round of propagation and sets up any data needs by propagate().
  /** This function can be called during search if the function is reified */
  virtual void full_propagate() = 0;
/*  { 
    Constraint* c(get_table_constraint());
    c->full_propagate(); 
	delete c;
  }*/
  
  /// Iterative propagation function.
  /** Can assume full_propagate is always called at least once before propagate */
#ifdef FUNCTIONPOINTER_TRIGGER
  PROPAGATE_FUNCTION(int, DomainDelta) {}
#else
  PROPAGATE_FUNCTION(int, DomainDelta) = 0;
#endif
  // Returns a table constraint which implements this constraint
  /** The main reason for this function is to make the constraint package up all its variables */
  virtual vector<AnyVarRef> get_vars() = 0;  

  /// Checks if an assignment is satisfied.
  /** This takes the variable order returned by, and is mainly only used by, get_table_constraint() */
  virtual BOOL check_assignment(vector<DomainInt>) = 0;
    
  /// Checks if a constraint cannot be satisfied, and sets up any data structures for future incremental checks.
  /// Returns TRUE if constraint cannot be satisfied.
  /** This function is used by rarification */
  virtual BOOL full_check_unsat()
  { 
	cerr << "Reification is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
	FAIL_EXIT(); 
  }
    
  /// Checks incrementaly if constraint cannot be satisfied.
  /// Returns TRUE if constraint cannot be satisfied.
  /** This function should not be called unless check_unsat_full is called first. This is used by rarification */
  virtual BOOL check_unsat(int,DomainDelta)
  { 
	cerr << "Reification is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
	FAIL_EXIT(); 
  }
  
  /// Allows functions to activate a special kind of trigger, run only
  /// after the normal queue is empty.
  virtual void special_check()
  { 
	cerr << "Serious internal error" << endl;
	FAIL_EXIT(); 
  }
  
  virtual void special_unlock()
  { 
	cerr << "Serious internal error" << endl;
	FAIL_EXIT(); 
  }
  
  
  /// Returns the reverse of the current constraint
  /** Used by rarification */
  virtual Constraint* reverse_constraint()
  { 
	cerr << "Reification is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
    FAIL_EXIT();
  }
  
  virtual ~Constraint()
  {}
};


inline void Constraint::setup()
{
  triggerCollection t = setup_internal();
  for(triggerCollection::iterator it = t.begin(); it != t.end(); ++it)
  {
    (*it)->post_trigger();
  }
}

