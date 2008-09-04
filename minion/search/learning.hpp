#ifndef EXPLANATIONS_HPP
#define EXPLANATIONS_HPP

#ifdef C_LEARNING

#include "../constraints/constraint_abstract.h"

#include "../dynamic_constraints/dynamic_new_or.h"
#include "../dynamic_constraints/dynamic_less.h"
#include "../dynamic_constraints/unary/dynamic_literal.h"
#include "../dynamic_constraints/unary/dynamic_notliteral.h"

inline AbstractConstraint* Explanation::getNegCon(StateObj* stateObj) const
{ D_ASSERT(false); return 0; }

template<typename Var1, typename Var2>
inline AbstractConstraint* GreaterEqual<Var1,Var2>::getNegCon(StateObj* stateObj) const
{ return new WatchLessConstraint<Var1,Var2>(stateObj, v1, v2); }

inline AbstractConstraint* Lit::getNegCon(StateObj* stateObj) const
{
  if(assignment) return new WatchNotLiteralConstraint<AnyVarRef>(stateObj, get_AnyVarRef_from_Var(stateObj, var), val);
  else return new WatchLiteralConstraint<AnyVarRef>(stateObj, get_AnyVarRef_from_Var(stateObj, var), val);
}

template<typename VarRef>
inline AbstractConstraint* LessConstant<VarRef>::getNegCon(StateObj* stateObj) const
{
  if(varOnLeft) //make v >= c, i.e. c <= v, i.e. c-1 < v
    return new WatchLessConstraint<ConstantVar,VarRef>(stateObj, ConstantVar(stateObj, c - 1), v);
  else //make c >= v, i.e. v <= c, i.e. v < c+1
    return new WatchLessConstraint<VarRef,ConstantVar>(stateObj, v, ConstantVar(stateObj, c + 1));
}

inline AbstractConstraint* Conjunction::getNegCon(StateObj* stateObj) const
{
  vector<AbstractConstraint*> cons;
  vector<ExplPtr>::const_iterator curr = conjuncts.begin();
  const vector<ExplPtr>::const_iterator end = conjuncts.end();
  for(; curr != end; curr++)
    cons.push_back((*curr)->getNegCon(stateObj));
  return new Dynamic_OR(stateObj, cons);
}

inline vector<ExplPtr> workOutWhy(StateObj* stateObj)
{
  AnyVarRef failed = get_AnyVarRef_from_Var(stateObj, getState(stateObj).failed_var);
  vector<ExplPtr> res;
  DomainInt curr = failed.getInitialMin();
  const DomainInt f_initmax = failed.getInitialMax();
  res.reserve(f_initmax - curr + 1);
  for(; curr <= f_initmax; curr++)
    res.push_back(failed.getExplanation(curr));
  return res;
}

#endif

// inline bool descending_depth(const pair<Literal*,pair<unsigned,unsigned> >& p1,
// 			     const pair<Literal*,pair<unsigned,unsigned> >& p2) {
//   return p1.second < p2.second;
// }

// inline Literal* getLiteralPtrFromExplPtr(ExplPtr& p) {
//   const Explanation* ptr = p.get();
//   D_ASSERT(dynamic_cast<Literal*>(ptr)); //make sure cast is sound
//   return static_cast<Literal*>(ptr); //i know it's correct to do this
// }

// inline Conjunction* getConjunctionPtrFromExplPtr(ExplPtr& p) {
//   const Explanation* ptr = p.get();
//   D_ASSERT(dynamic_cast<Conjunction*>(ptr));
//   return static_cast<Conjunction*>(ptr);
// }

// inline ExplPtr computeFirstUip(StateObj* stateObj, Var v)
// {
//   //heap of literals from current decision level, with the depths they were pruned or assigned
//   vector<pair<Literal*,pair<unsigned,unsigned> > > at_curr_depth; 
//   //literals from earlier decision levels (unsorted, with duplicate)
//   Conjunction* final = new Conjunction(); 
//   //the var that wiped out
//   AnyVarRef v_avr = get_AnyVarRef_from_Var(stateObj, v);
//   //the first depth in the current decision level
//   const pair<unsigned,unsigned> decision_depth(getMemory(stateObj).backTrack().current_depth(), 0);
//   //first we build the "must have a value" failure into at_curr_depth or final as applicable
//   const DomainInt max = v_avr.getInitialMax();
//   for(DomainInt curr = v_avr.getInitialMin(); curr <= max; curr++) {
//     const pair<unsigned,unsigned> curr_d = v_avr.getDepth(curr);
//     if(curr_d >= decision_depth)
//       at_curr_depth.push_back(make_pair(new Literal(v, curr, false), curr_d));
//     else
//       final->conjuncts.push_back(ExplPtr(new Literal(v, curr, false)));
//   }
//   //next we resolve away all but one literal from the current decision level, leaving a firstUIP cut
//   make_heap(at_curr_depth.begin(), at_curr_depth.end(), descending_depth);
//   while(at_curr_depth.size() > 1) { //until 1st uip reached
//     //get and remove deepest literal
//     pair<Literal*,pair<unsigned,unsigned> > deepest = at_curr_depth.front();
//     pop_heap(at_curr_depth.begin(), at_curr_depth.end(), descending_depth); 
//     at_curr_depth.pop_back();
//     //get its explanation
//     const ExplPtr expl_raw = get_AnyVarRef_from_Var(stateObj, deepest.first.var).getLabel(deepest.first.val);
//     const vector<ExplPtr>& lits = getConjunctionPtrFromExplPtr(expl_raw)->conjuncts;
//     for(size_t i = 0; i < lits.size(); i++) {
//       const ExplPtr& lit_i_raw = lits[i];
//       const Literal* lit_i = getLiteralPtrFromExplPtr(lit_i_raw);
//       const pair<unsigned,unsigned> lit_i_d = get_AnyVarRef_from_Var(stateObj, lit_i.var).getDepth(lit_i.val);
//       if(lit_i_d >= decision_depth) { //if at current depth put it into at_curr_depth
// 	//make sure that literal is not a duplicate in at_curr_depth:
// 	if(find(at_curr_depth.begin(), at_curr_depth.end(), make_pair(lit_i, lit_i_d)) == at_curr_depth.end()) {
// 	  at_curr_depth.push_back(make_pair(lit_i, lit_i_d));
// 	  push_heap(at_curr_depth.begin(), at_curr_depth.end(), descending_depth);
// 	}
//       } else
// 	final->conjuncts.push_back(lit_i_raw);
//     }
//   }
//   //add the unique literal at the current decision level, we must allocate a new
//   //literal, because if we put the original pointer into a new ExplPtr it might
//   //be prematurely garbage collected
//   Literal* cd_lit = at_curr_depth.front().first;
//   final.push_back(ExplPtr(new Literal(cd_lit.var, cd_lit.val, cd_lit.assignment)));
//   //simplify and return
//   final.simplify();
//   return ExplPtr(final);
// }

#endif //explanations header
