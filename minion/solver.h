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

#ifndef _SOLVER_H
#define _SOLVER_H

#include "system/system.h"

#include "tuple_container.h"
// Some advanced definitions, we don't actually need to know anything about these
// types for SearchState, simply that they exist.
class Constraint;
class DynamicConstraint;
class AnyVarRef;


class SearchState
{
  unsigned long long nodes;
  AnyVarRef* optimise_var;
  DomainInt current_optimise_position;
  bool optimise;
  
  vector<Constraint*> constraints;
#ifdef DYNAMICTRIGGERS
  vector<DynamicConstraint*> dynamic_constraints;
#endif
  
  long long int solutions;
  
  bool dynamic_triggers_used;
  
  bool finished;
  bool failed;
  jmp_buf g_env;
  
  TimerClass timer;
  
  shared_ptr<TupleListContainer> tupleListContainer;

  bool is_locked;
  
  volatile bool alarm_trigger;
  
  volatile bool ctrl_c_pressed;
    
public:

  unsigned long long getNodeCount() { return nodes; }
  void setNodeCount(unsigned long long _nodes) { nodes = _nodes; }
  void incrementNodeCount() { nodes++; }
  
  AnyVarRef* getOptimiseVar() { return optimise_var; }
  void setOptimiseVar(AnyVarRef* _var) { optimise_var = _var; }
  
  DomainInt getOptimiseValue() { return current_optimise_position; }
  void setOptimiseValue(DomainInt optimise_pos) { current_optimise_position = optimise_pos; }
  
  bool isOptimisationProblem() { return optimise; }
  void setOptimisationProblem(bool _optimise) { optimise = _optimise; }
  
  void addConstraint(Constraint* c);
  vector<Constraint*>& getConstraintList() { return constraints; }
#ifdef DYNAMICTRIGGERS
  void addDynamicConstraint(DynamicConstraint* c);
  vector<DynamicConstraint*>& getDynamicConstraintList() { return dynamic_constraints; }
#endif
  
  long long int getSolutionCount() { return solutions; }
  void setSolutionCount(long long int _sol) { solutions = _sol; }
  void incrementSolutionCount() { solutions++; }
  
  bool isDynamicTriggersUsed() { return dynamic_triggers_used; }
  void setDynamicTriggersUsed(bool b) { dynamic_triggers_used = b; }
  
  bool isFinished() { return finished; }
  void setFinished(bool b) { finished = b; }
  
  bool isFailed() { return failed; }
  void setFailed(bool f) {
#ifdef USE_SETJMP
    if(f)
      SYSTEM_LONGJMP(*(getState(stateObj).getJmpBufPtr()),1);
#endif
    failed = f; 
  }

  Var failure;
  //variables should use this function to fail when they wipe out,
  //providing their own identity when they do
  void setFailed(bool f, Var v) {
    failure = v;
    setFailed(f);
  }
  
  //var that most recently failed
  Var getFailedVar() { return failure; }

  // This function is here because a number of pieces of code want a raw reference to the 'failed' variable.
  // Long term, this may get removed, but it is added for now to minimise changes while removing global
  // variables.
  bool* getFailedPtr() { return &failed; }
  
  TimerClass& getTimer() { return timer; }
  
  
  jmp_buf* getJmpBufPtr() { return &g_env; }
  
  TupleListContainer* getTupleListContainer() { return &*tupleListContainer; }
  
  void setTupleListContainer(shared_ptr<TupleListContainer> _tupleList) 
  { tupleListContainer = _tupleList; }
                          
  SearchState() : nodes(0), optimise_var(NULL), current_optimise_position(0), optimise(false), solutions(0),
	dynamic_triggers_used(false), finished(false), failed(false), tupleListContainer(NULL), is_locked(false),
	alarm_trigger(false), ctrl_c_pressed(false)
  {}
  
  // Must be defined later.
  ~SearchState();
  
  void markLocked()
  { is_locked = true; }

  bool isLocked()
  { return is_locked; }
  
  bool isAlarmActivated()
  { return alarm_trigger; }

  void clearAlarm()
  { alarm_trigger = false; }  
  
  void setupAlarm()
  { activate_trigger(&alarm_trigger);}
  
  bool isCtrlcPressed()
  { return ctrl_c_pressed; }
  
  void setCtrlcPressed()
  { ctrl_c_pressed = true; }
  
  
};

/// Stored all the options related to search. This item should not
/// be changed during search.
class SearchOptions
{
public:
#ifdef WDEG
  /// Denotes if wdeg is turned on
  bool wdeg_on;
#endif
  /// Denotes if only solutions should be printed.
  bool print_only_solution;
  /// Denotes if the search tree should be printed.
  bool dumptree;
  /// Gives the solutions which should be found. 
  /// -1 denotes finding all solutions.
  long long sollimit;
  /// Denotes if non-incremental propagation should be used. 
  /// Only for debugging.
  bool fullpropagate;
  /// Denotes if solutions should be checked it they satisfy constraints.
  /// Only for debugging.
  bool nocheck;
  /// Denotes to nodelimit, 0 if none given.
  unsigned long long nodelimit;
  /// Dentoes if information about search should be printed to a file.
  bool tableout;
  
  /// Denotes if solutions should be printed to a seperate file.
  bool solsoutWrite;
    
  /// Denotes if solutions should be printed.
  /// Initialised to true.
  bool print_solution;
  
  /// Stores the timelimit, 0 if none given.
  clock_t time_limit;
  
  /// Denotes if the variable and value orderings should be randomised.
  /// Initialised to false.
  bool randomise_valvarorder;
  
  /// Denotes if parser should output verbosely
  bool parser_verbose;
  
  /// The filename of the current input file (-- if reading from command line)
  string instance_name;
  
  SearchOptions() : 
#ifdef WDEG
    wdeg_on(false),
#endif
    print_only_solution(false), dumptree(false), sollimit(1), fullpropagate(false), 
#ifdef NO_DEBUG
    nocheck(true),
#else
    nocheck(false),
#endif
    nodelimit(0), tableout(false), solsoutWrite(false), randomise_valvarorder(false), 
    print_solution(true), time_limit(0), parser_verbose(false)
  {}
  
  /// Denotes all solutions should be found, by setting sollimit to -1.
  void findAllSolutions()
  { sollimit = -1; }
};


class Queues;
class MemBlockCache;
class Memory;
class TriggerMem;
class VariableContainer;

class StateObj;
class BoolContainer;

inline BoolContainer& getBools(StateObj* stateObj);
inline SearchOptions& getOptions(StateObj* stateObj);
inline SearchState& getState(StateObj* stateObj);
inline Queues& getQueue(StateObj* stateObj);
inline Memory& getMemory(StateObj* stateObj);
inline TriggerMem& getTriggerMem(StateObj* stateObj);
inline VariableContainer& getVars(StateObj* stateObj);

namespace Controller
{
  void lock(StateObj*);
  
  /// Pushes the state of the whole world.
  inline void world_push(StateObj* stateObj);

  /// Pops the state of the whole world.
  inline void world_pop(StateObj* stateObj);

  inline void world_pop_all(StateObj* stateObj);
}

#endif
