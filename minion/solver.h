/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: solver.h 707 2007-10-12 18:06:50Z azumanga $
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


// Some advanced definitions, we don't actually need to know anything about these
// types for SearchState, simply that they exist.
class Constraint;
class DynamicConstraint;
class AnyVarRef;
class TupleListContainer;

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
  
  TupleListContainer* tupleListContainer;

  bool is_locked;

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
  
  void addConstraint(Constraint* c) { constraints.push_back(c); }
  vector<Constraint*>& getConstraintList() { return constraints; }
#ifdef DYNAMICTRIGGERS
  void addDynamicConstraint(DynamicConstraint* c) { dynamic_constraints.push_back(c); }
  vector<DynamicConstraint*>& getDynamicConstraintList() { return dynamic_constraints; }
#endif
  
  long long int getSolutionCount() { return solutions; }
  void setSolutionCount(long long int _sol) { solutions = _sol; }
  void incrementSolutionCount() { solutions++; }
  
  bool isDynamicTriggersUsed() { return dynamic_triggers_used; }
  void setDynamicTriggersUsed(bool b) { dynamic_triggers_used = b; }
  
  bool isFinished() { return finished; }
  bool setFinished(bool b) { finished = b; }
  
  bool isFailed() { return failed; }
  bool setFailed(bool f) {
#ifdef USE_SETJMP
    if(f)
      SYSTEM_LONGJMP(*(stateObj->state().getJmpBufPtr()),1);
#endif
    failed = f; 
  }
  // This function is here because a number of pieces of code want a raw reference to the 'failed' variable.
  // Long term, this may get removed, but it is added for now to minimise changes while removing global
  // variables.
  bool* getFailedPtr() { return &failed; }
  
  TimerClass& getTimer() { return timer; }
  
  jmp_buf* getJmpBufPtr() { return &g_env; }
  
  TupleListContainer* getTupleListContainer() { return tupleListContainer; }
  
  void setTupleListContainer(TupleListContainer* _tupleList) 
  { 
    D_ASSERT(tupleListContainer == NULL);
    tupleListContainer = _tupleList; 
  }
                          
  SearchState() : nodes(0), optimise_var(NULL), current_optimise_position(0), optimise(false), solutions(0),
	dynamic_triggers_used(false), finished(false), failed(false), tupleListContainer(NULL), is_locked(false)
  {}
  
  void markLocked()
  { is_locked = true; }

  bool isLocked()
  { return is_locked; }
  
  
};


class SearchOptions
{
public:
  
  bool print_only_solution;
  bool dumptree;
  int sollimit;
  bool fullpropagate;
  bool nocheck;
  unsigned long long nodelimit;
  bool tableout;
  
  /// This variable contains the name of a function which should be called
  /// Wherever a solution is found.
  //void (*solution_check)(void);
  
  /// Denotes if only one solution should be found.
  bool find_one_sol;
  
  /// Denotes if solutions should be printed.
  bool print_solution;
  
  /// Stores the timelimit, 0 if none given
  clock_t time_limit;
  
  SearchOptions() : print_only_solution(false), dumptree(false), sollimit(-1), fullpropagate(false), 
	nocheck(false), nodelimit(0), tableout(false),  find_one_sol(true), 
    print_solution(true), time_limit(0)
    //,solution_check(NULL)
  {}
  
//  void setSolutionCheckFunction(void(*fun_ptr)(void))
//  { solution_check = fun_ptr; }
  
  void setFindAllSolutions()
  { find_one_sol = false; }
  
  void setFindOneSolution()
  { find_one_sol = true; }

  bool lookingForOneSolution()
  { return find_one_sol; }
};

//VARDEF(SearchOptions* stateObj->options());
//VARDEF(SearchState state);

class Queues;
class MemBlockCache;
class Memory;
class TriggerMem;
class VariableContainer;

class StateObj
{
  // Forbid copying this type!
  StateObj(const StateObj&);
  void operator=(const StateObj&);

  Memory* searchMem_m;
  SearchOptions* options_m;
  SearchState state_m;
  Queues* queues_m;
  TriggerMem* triggerMem_m;
  VariableContainer* varContainer_m;
public:
    
  SearchOptions* options() { return options_m; }
  SearchState& state() { return state_m; }
  Queues& queues() { return *queues_m; }
  Memory& searchMem() { return *searchMem_m; }
  TriggerMem* triggerMem() { return triggerMem_m; }
  VariableContainer& varCon() { return *varContainer_m; }

  // These has to be defined a long way away...
  StateObj();
  ~StateObj();
};

namespace Controller
{
  void lock(StateObj*);
}

