
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

  inline void print_clause(const list<literal>& c)
  {
    list<literal>::const_iterator curr = c.begin();
    list<literal>::const_iterator end = c.end();
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
  inline void combine(list<literal>& c1, const list<literal>& c2)
  {
    list<literal>::iterator c1curr = c1.begin();
    list<literal>::iterator c1end = c1.end();
    list<literal>::const_iterator c2curr = c2.begin();
    list<literal>::const_iterator c2end = c2.end();
    
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

  //build a clause data structure, sorted by var id
  inline void build_clause(list<literal>& clause, DynamicConstraint* constr)
  {
    vector<AnyVarRef> vars = constr->get_vars(); //sorted by var id already
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
  }

  //order by presence of antecedent, followed by depth, followed by sequence number
  //of the propagation
  inline bool lit_order(const literal& l1, const literal& l2)
  { 
    return (l1.antecedent && !(l2.antecedent)) ||
      (l1.antecedent && l2.antecedent && l1.depth > l2.depth) ||
      (l1.antecedent && l2.antecedent && l1.depth == l2.depth && 
       l1.ant_seq_no > l2.ant_seq_no);
  }
  
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
	list<literal>::iterator curr = clause.begin();
	list<literal>::iterator end = clause.end();
	literal last = *curr;
	curr++;
	//find the literal that we most want to resolve
	while(curr != end) {
	  if(lit_order(*curr, last)) 
	    last = *curr;
	  curr++;
	}
	//find two greatest depths
	curr = clause.begin();
	bool firstrun = true;
	firstrun = true;
	int max_d = (*curr).depth;
	curr++;
	int next_d = -1; //this will always be initialised before it is used
	while(curr != end) {
	  if(firstrun) {
	    if((*curr).depth > max_d) {
	      next_d = max_d;
	      max_d = (*curr).depth;
	    } else
	      next_d = (*curr).depth;
	    firstrun = false;
	  } else if((*curr).depth > next_d) {
	    if((*curr).depth > max_d) {
	      next_d = max_d;
	      max_d = (*curr).depth;
	    } else
	      next_d = (*curr).depth;
	  }
	  curr++;
	}
	if(max_d > next_d || last.antecedent == 0)
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
		  world_push(stateObj);
		  order.branch_left();
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

		  if(bj_depth == -1)
		    return;
		  		  
		  maybe_print_search_action(stateObj, "bt");
		  
		  //learn clause, and do pseudo right branch
		  world_pop(stateObj, bj_depth);
		  learn_clause(stateObj, clause); 
		  		  
		  set_optimise_and_propagate_queue(stateObj);
		}
	  }
  }
  
}
