/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: standard_search.h 778 2007-11-11 17:19:48Z azumanga $
*/

#include <iostream>
#include <iomanip>
#include "common_search.h"

namespace Controller
{

#include "VariableOrders.h"
#include "../dynamic_constraints/dynamic_or.h"
  
  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)

  struct literal {
    AnyVarRef* var; //variable
    int id; //variable ID
    int neg; //negation, 0 iff negated
    unsigned depth; //depth set
    DynamicConstraint* antecedent; //cause of instantiation (0 for search var)
    unsigned ant_seq_no; //sequence number of antecedent propagation (> is later)
  };

  inline void print_clause(list<literal>& c)
  {
    list<literal>::iterator curr = c.begin();
    list<literal>::iterator end = c.end();
    cout << "Clause:" << endl;
    for(; curr != end; curr++) {
      cout << (*curr).var << " (aka. " << (*curr).id << ") ";
      cout << ((*curr).neg == 1 ? "not" : "") << " neg@";
      cout << (*curr).depth << " END" << endl;
    }
  }
  
  //Simplify and resolve the two supplied clauses, returning result in
  //c1. Assume that they are sorted in id order and that they are
  //themselves simplified.
  inline void combine(list<literal>& c1, list<literal>& c2)
  {
    //cout << "Combining the following:" << endl;
    //print_clause(c1);
    //print_clause(c2);
    list<literal>::iterator c1curr = c1.begin();
    list<literal>::iterator c1end = c1.end();
    list<literal>::iterator c2curr = c2.begin();
    list<literal>::iterator c2end = c2.end();
    
    while(c1curr != c1end && c2curr != c2end) { //merge the c2 into c1
      if((*c1curr).id == (*c2curr).id) {
	if((*c1curr).neg != (*c2curr).neg) { //resolve
	  c1curr = c1.erase(c1curr); //remove current and advance c1curr
	  c2curr++; //skip c2
	} else { //identical literals, skip
	  c2curr++;
	}
      } else if((*c1curr).id > (*c2curr).id) { //need to put this c2curr into c1
	c1.insert(c1curr, *c2curr);
	c2curr++;
      } else {
	c1curr++;
      }
    }
    while(c2curr != c2end) {
      c1.insert(c1end, *c2curr);
      c2curr++;
    }
    //cout << "to produce" << endl;
    //print_clause(c1);
  }

  inline unsigned count_curr_depth(list<literal>& c, unsigned depth)
  {
    unsigned retval = 0;
    list<literal>::iterator curr = c.begin();
    list<literal>::iterator end = c.end();
    for(; curr != end; curr++)
      if((*curr).depth == depth)
	retval++;
    return retval;
  }

  inline bool compare_literal(literal l1, literal l2)
  {
    return l1.id < l2.id;
  }

  //build a clause data structure
  inline void build_clause(list<literal>& clause, DynamicConstraint* constr)
  {
    vector<AnyVarRef> vars = constr->get_vars();
    size_t vars_s = vars.size();
    vector<int>* negs = constr->get_signs();
    clause.clear();
    literal tmp;
    for(size_t i = 0; i < vars_s; i++) {
      tmp.var = &vars[i];
      tmp.id = tmp.var->getId();
      tmp.neg = (*negs)[i];
      tmp.depth = tmp.var->getDepth();
      tmp.antecedent = tmp.var->getAntecedent();
      tmp.ant_seq_no = tmp.var->getSeqNo();
      clause.push_back(tmp);
    }
    clause.sort(compare_literal);
  }

  //Learn a conflict and return a depth to jump back to. <clause> data structure
  //is sorted by var_id throughout. Conflict clause is written into <clause>.
  inline unsigned conflict_learn(StateObj* stateObj, list<literal>& clause)
  {
    BooleanContainer& bc = getVars(stateObj).getBooleanContainer();
    AnyVarRef* conflict_var = bc.conflict_var;

    //create initial clause based on clause that fired just before conflict
    clause.clear();
    build_clause(clause, conflict_var->getAntecedent());
    unsigned const curr_depth = getMemory(stateObj).backTrack().current_depth();

    //Begin by resolving the other clause contributing to the
    //conflicting variable.
    list<literal> other_conflicting;
    build_clause(other_conflicting, bc.last_clause);
    combine(clause, other_conflicting);

    //Now we have a cut of the implication graph (clause) but we will resolve
    //antecedent clauses in any order until we have only decision variables.
    while(true) {
      list<literal>::iterator curr = clause.begin();
      list<literal>::iterator end = clause.end();
      DynamicConstraint* next = 0;
      while(curr != end && next == 0) {
	next = (*curr).antecedent;
	curr++;
      }
      if(next != 0) {
	list<literal> next_clause;
	build_clause(next_clause, next);
	combine(clause, next_clause);
      } else {
	break; //nothing to resolve, finish now
      }
    }
    //do something with successfully created conflict clause
    cout << "final clause: " << endl;
    print_clause(clause);
    list<literal>::iterator curr = clause.begin();
    list<literal>::iterator end = clause.end();
    unsigned largest = (*curr).depth;
    unsigned next_largest = 0; //if only 1 literal in clause jump to root
    curr++;
    while(curr != end) {
      if((*curr).depth > largest) {
	next_largest = largest;
	largest = (*curr).depth;
      }
      curr++;
    }
    return next_largest;
  }

  //the clause contains a conjunction that would cause a conflict, learn
  //NOT the conjunction, i.e. the disjunction of the negated literals
  inline void learn_clause(StateObj* stateObj, list<literal>& clause) 
  {
    vector<AnyVarRef> vars;
    vector<int> negs;
    list<literal>::iterator curr = clause.begin();
    list<literal>::iterator end = clause.end();
    while(curr != end) {
      literal& lit = (*curr);
      vars.push_back(*(lit.var));
      negs.push_back(lit.neg ? 0 : 1); //negate
      curr++;
    }
    BoolOrConstraintDynamic<vector<AnyVarRef> >* cc = 
      new BoolOrConstraintDynamic<vector<AnyVarRef> >(stateObj, vars, negs);
    getState(stateObj).addDynamicConstraint(cc);
    cc->setup();
    cc->full_propagate();
  }
  
  template<typename VarOrder, typename Variables, typename Propogator>
	inline void solve_loop(StateObj* stateObj, VarOrder& order, Variables& v, Propogator prop = PropogateGAC())
  {
	  D_INFO(0, DI_SOLVER, "Non-Boolean Search");
	  
	  maybe_print_search_state(stateObj, "Node: ", v);
	  
	  bool solution_found; //true if we just found a solution
	  while(true)
	  {
		getState(stateObj).incrementNodeCount();
		if(getState(stateObj).getNodeCount() == getOptions(stateObj).nodelimit)
		  return;
	
		if(do_checks(stateObj))
		  return;
		
		// order.find_next_unassigned returns true if all variables assigned.
		if(order.find_next_unassigned())
		{  		  
		  // We have found a solution!
		  check_sol_is_correct(stateObj);
		  // This function may escape from search if solution limit
		  // has been reached.
		  deal_with_solution(stateObj);

		  // fail here to force backtracking.
		  getState(stateObj).setFailed(true);
		  solution_found = true;
		}
		else
		{
		  maybe_print_search_state(stateObj, "Node: ", v);
		  getVars(stateObj).getBigRangevarContainer().bms_array.before_branch_left();
		  cout << "depth before push:" << getMemory(stateObj).backTrack().current_depth() << endl;
		  world_push(stateObj);
		  cout << "depth after push:" << getMemory(stateObj).backTrack().current_depth() << endl;
		  order.branch_left();
		  cout << "depth after branch:" << getMemory(stateObj).backTrack().current_depth() << endl;
		  getVars(stateObj).getBigRangevarContainer().bms_array.after_branch_left();
		  prop(stateObj, v);
		  solution_found = false;
     		}
		
		// Either search failed, or a solution was found.
		while(getState(stateObj).isFailed())
		{
		  getState(stateObj).setFailed(false);

		  unsigned bj_depth;
		  list<literal> clause;
		  if(!solution_found) //definitely a failure due to a conflict
		    bj_depth = conflict_learn(stateObj, clause);
		  else
		    bj_depth = getMemory(stateObj).backTrack().current_depth() - 1;

		  cout << "bj depth: " << bj_depth << endl;
		  if(bj_depth == -1)
		    return;
		  		  
		  world_pop(stateObj, bj_depth);

		  //learn clause, and do pseudo right branch since clause is asserting
		  learn_clause(stateObj, clause); 
		  
		  //order.find_next_unassigned();
		  
		  maybe_print_search_action(stateObj, "bt");
	  
		  //getVars(stateObj).getBigRangevarContainer().bms_array.before_branch_right();
		  //order.branch_right();
		  //getVars(stateObj).getBigRangevarContainer().bms_array.after_branch_right();

		  if(order.finished_search())
		    return;

		  set_optimise_and_propagate_queue(stateObj);
		}
	  }
  }
  
}




