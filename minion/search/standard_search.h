/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: standard_search.h 778 2007-11-11 17:19:48Z azumanga $
*/

#include <iostream>
#include <iomanip>
#include "common_search.h"
#include "literal.h"

namespace Controller
{

#include "VariableOrders.h"
#include "../dynamic_constraints/dynamic_or.h"
  
  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)

  inline void print_clause(list<literal>& c)
  {
    list<literal>::iterator curr = c.begin();
    list<literal>::iterator end = c.end();
    cout << "Clause:" << endl;
    for(; curr != end; curr++) {
      cout << (*curr).var << " (aka. " << (*curr).id << ") ";
      cout << ((*curr).neg == 1 ? "not" : "") << " neg@";
      cout << (*curr).depth << " (antecedent:" << (*curr).antecedent << ")";
      cout << " (seqno: " << (*curr).ant_seq_no << ") END" << endl;
    }
  }
  
  //Simplify and resolve the two supplied clauses, returning result in
  //c1. Assume that they are sorted in id order and that they are
  //themselves simplified.
  inline void combine(list<literal>& c1, list<literal>& c2)
  {
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
  }

  inline bool compare_literal(literal l1, literal l2) { return l1.id < l2.id; }

  //build a clause data structure, sorted by var id
  inline void build_clause(list<literal>& clause, DynamicConstraint* constr)
  {
    vector<AnyVarRef> vars = constr->get_vars();
    size_t vars_s = vars.size();
    vector<int>* negs = constr->get_signs();
    clause.clear();
    literal tmp;
    for(size_t i = 0; i < vars_s; i++) {
      tmp.var = vars[i];
      tmp.id = tmp.var.getId();
      tmp.neg = (*negs)[i];
      tmp.depth = tmp.var.getDepth();
      tmp.antecedent = tmp.var.getAntecedent();
      tmp.ant_seq_no = tmp.var.getSeqNo();
      clause.push_back(tmp);
    }
    clause.sort(compare_literal);
  }

  inline bool compare_seq_no(literal l1, literal l2) 
  { return (l1.depth == l2.depth && l1.ant_seq_no > l2.ant_seq_no) || l1.depth > l2.depth; }

  //Learn a conflict and return a depth to jump back to. <clause> data structure
  //is sorted by var_id throughout. Conflict clause is written into <clause>.
  template<typename VarOrder>
  inline unsigned conflict_learn(StateObj* stateObj, list<literal>& clause, VarOrder& order)
  {
    BooleanContainer& bc = getVars(stateObj).getBooleanContainer();
    AnyVarRef& conflict_var = bc.conflict_var;

    //create initial clause based on clause that fired just before conflict
    clause.clear();
    build_clause(clause, conflict_var.getAntecedent());
    order.update_order(clause);

    unsigned const curr_depth = getMemory(stateObj).backTrack().current_depth();

    //Begin by resolving the other clause contributing to the
    //conflicting variable.
    list<literal> other_conflicting;
    build_clause(other_conflicting, bc.last_clause);
    combine(clause, other_conflicting);
    order.update_order(other_conflicting);

    //find firstUIP cut
    if(clause.size() != 1)
      while(true) {
	list<literal> sorted_clause = clause;
	sorted_clause.sort(compare_seq_no);
	print_clause(sorted_clause);
	literal last = sorted_clause.front();
	sorted_clause.pop_front();
	literal penultimate = sorted_clause.front();
	if(last.depth > penultimate.depth || last.antecedent == 0)
	  break; //already at firstUIP
	else {
	  list<literal> next_clause;
	  build_clause(next_clause, last.antecedent);
	  combine(clause, next_clause);
	  order.update_order(next_clause);
	}
      }

    //do something with successfully created conflict clause
    if(clause.size() == 1)
      return 0; //jump back to root for unit clause
    list<literal>::iterator curr = clause.begin();
    list<literal>::iterator end = clause.end();
    unsigned largest = (*curr).depth;
    unsigned next_largest = 0; //root is default second deepest level
    curr++;
    while(curr != end) {
      if((*curr).depth > largest) {
	next_largest = largest;
	largest = (*curr).depth;
      }
      curr++;
    }
    return largest == 0 ? -1 : next_largest; //if all 0, stop by bj to -1
  }

  //the clause causes a conflict, learn the clause. If clause forces us into an
  //old part of search tree, do nothing and return true.
  inline void learn_clause(StateObj* stateObj, const list<literal>& clause) 
  {
    vector<AnyVarRef> vars;
    vector<int> negs;
    list<literal>::const_iterator curr = clause.begin();
    list<literal>::const_iterator end = clause.end();
    while(curr != end) {
      const literal& lit = (*curr);
      vars.push_back(lit.var);
      negs.push_back(lit.neg); //negate
      curr++;
    }
    BoolOrConstraintDynamic<vector<AnyVarRef> >* cc = 
      new BoolOrConstraintDynamic<vector<AnyVarRef> >(stateObj, vars, negs);
    getState(stateObj).addDynamicConstraint(cc);
    cc->setup();
    cc->full_propagate();
  }

  //NB. This search procedure finds ONE solution, not all solutions, if it finds any at all!
  template<typename VarOrder, typename Variables, typename Propogator>
    inline void solve_loop(StateObj* stateObj, VarOrder& order, Variables& v, Propogator prop = PropogateGAC())
  {
	  D_INFO(0, DI_SOLVER, "Non-Boolean Search");
	  
	  maybe_print_search_state(stateObj, "Node: ", v);
	  
	  while(true)
	  {
		getState(stateObj).incrementNodeCount();
		if(getState(stateObj).getNodeCount() == getOptions(stateObj).nodelimit)
		  return;
	
		if(do_checks(stateObj))
		  return;
		
		// order.find_next_unassigned returns true if all variables assigned.
		if(order.assigned_all())
		{  		  
		  // We have found a solution!
		  check_sol_is_correct(stateObj);
		  // This function may escape from search if solution limit
		  // has been reached.
		  deal_with_solution(stateObj);

		  return; //stop search after first solution
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
     		}
		
		// Either search failed, or a solution was found.
		while(getState(stateObj).isFailed())
		{
		  getState(stateObj).setFailed(false);

		  //failure at depth 0 implies no more solutions
		  if(getMemory(stateObj).backTrack().current_depth() == 0) return;

		  unsigned bj_depth;
		  list<literal> clause;
		  bj_depth = conflict_learn<VarOrder>(stateObj, clause, order);

		  cout << "bj_depth: " << bj_depth << endl;

		  if(bj_depth == -1)
		    return;
		  		  
		  maybe_print_search_action(stateObj, "bt");
		  
		  //learn clause, and do pseudo right branch
		  world_pop(stateObj, bj_depth);
		  learn_clause(stateObj, clause); 
		  print_clause(clause);
		  		  
		  set_optimise_and_propagate_queue(stateObj);
		}
	  }
  }
  
}
