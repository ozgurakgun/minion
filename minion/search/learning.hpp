#ifndef LEARNING_HPP
#define LEARNING_HPP

#include "../dynamic_constraints/dynamic_less.h"
#include "../dynamic_constraints/unary/dynamic_literal.h"
#include "../dynamic_constraints/unary/dynamic_notliteral.h"
#include "../dynamic_constraints/dynamic_new_or.h"

template<typename VarRef>
inline vector<VirtConPtr> DisAssignment<VarRef>::whyT() const
{ return var.getExpl(false, val)->whyT(); } //get the disassignments stored explanation

template<typename VarRef>
inline AbstractConstraint* DisAssignment<VarRef>::getNeg() const
{ return new WatchLiteralConstraint<VarRef>(stateObj, var, val); }

template<typename VarRef>
inline pair<unsigned,unsigned> DisAssignment<VarRef>::getDepth() const
{ return var.getDepth(false, val); }

template<typename VarRef>
inline bool DisAssignment<VarRef>::equals(VirtCon* other) const
{
  DisAssignment<VarRef>* other_da = dynamic_cast<DisAssignment<VarRef>*>(other);
  return other_da && var.getBaseVal(val) == other_da->var.getBaseVal(other_da->val) &&
    var.getBaseVar() == other_da->var.getBaseVar();
}

template<typename VarRef>
inline void DisAssignment<VarRef>::print(std::ostream& o) const
{ o << "DisAssignment(var=" << var << ",val=" << val << ")"; }

template<typename VarRef>
inline vector<VirtConPtr> Assignment<VarRef>::whyT() const
{ return var.getExpl(true, val)->whyT(); } //get the assignments stored explanation

template<typename VarRef>
inline AbstractConstraint* Assignment<VarRef>::getNeg() const
{ return new WatchNotLiteralConstraint<VarRef>(stateObj, var, val); }

template<typename VarRef>
inline pair<unsigned,unsigned> Assignment<VarRef>::getDepth() const
{ return var.getDepth(true, val); }

template<typename VarRef>
inline bool Assignment<VarRef>::equals(VirtCon* other) const
{
  Assignment<VarRef>* other_da = dynamic_cast<Assignment<VarRef>*>(other);
  return other_da && var.getBaseVal(val) == other_da->var.getBaseVal(other_da->val) &&
    var.getBaseVar() == other_da->var.getBaseVar();
}

template<typename VarRef>
inline void Assignment<VarRef>::print(std::ostream& o) const
{ o << "Assignment(var=" << var << ",val=" << val << ")"; }

template<typename VarRef>
inline vector<VirtConPtr> LessConstant<VarRef>::whyT() const
{
  D_ASSERT(var.getInitialMax() >= constant); //make sure hasn't been true since the start
  vector<VirtConPtr> retVal;
  retVal.reserve(var.getInitialMax() - constant + 1);
  for(int i = constant; i <= var.getInitialMax(); i++)
    retVal.push_back(var.getExpl(false, i));
  return retVal;
}

template<typename VarRef>
inline AbstractConstraint* LessConstant<VarRef>::getNeg() const //constant <= var
{ 
  return new WatchLessConstraint<ConstantVar, VarRef>(stateObj, 
						      ConstantVar(stateObj, constant - 1), 
						      var); 
} 
  
template<typename VarRef>
inline pair<unsigned,unsigned> LessConstant<VarRef>::getDepth() const //return the depth of last value >= constant to be pruned out
{ 
  pair<unsigned,unsigned> retVal = make_pair(0,0); //if been true forever, make the depth 0
  for(int i = constant; i <= var.getInitialMax(); i++)
    retVal = max(retVal, var.getDepth(false, i));
  return retVal;
}

template<typename VarRef>
inline bool LessConstant<VarRef>::equals(VirtCon* other) const
{
  LessConstant<VarRef>* other_lc = dynamic_cast<LessConstant<VarRef>*>(other);
  return other_lc && var.getBaseVal(constant) == other_lc->var.getBaseVal(other_lc->constant) && 
    var.getBaseVar() == other_lc->var.getBaseVar();
}

template<typename VarRef>
inline void LessConstant<VarRef>::print(std::ostream& o) const
{ o << "LessConstant(var=" << var << " < val=" << constant << ")"; }

template<typename VarRef>
inline vector<VirtConPtr> GreaterConstant<VarRef>::whyT() const
{
  D_ASSERT(constant >= var.getInitialMin()); //shouldn't ask why true- been true since start
  vector<VirtConPtr> retVal;
  retVal.reserve(constant - var.getInitialMin() + 1);
  for(int i = var.getInitialMin(); i <= constant; i++)
    retVal.push_back(var.getExpl(false, i));
  return retVal;
}

template<typename VarRef>
inline AbstractConstraint* GreaterConstant<VarRef>::getNeg() const //var <= constant
{ 
  return new WatchLessConstraint<VarRef, ConstantVar>(stateObj, 
						      var,
						      ConstantVar(stateObj, constant + 1));
} 
  
template<typename VarRef>
inline pair<unsigned,unsigned> GreaterConstant<VarRef>::getDepth() const //return the depth of last value <= constant to be pruned out
{ 
  pair<unsigned,unsigned> retVal = make_pair(0,0); //if been true forever, make the depth 0
  for(int i = var.getInitialMin(); i <= constant; i++)
    retVal = max(retVal, var.getDepth(false, i));
  return retVal;
}

template<typename VarRef>
inline bool GreaterConstant<VarRef>::equals(VirtCon* other) const
{
  GreaterConstant<VarRef>* other_gc = dynamic_cast<GreaterConstant<VarRef>*>(other);
  return other_gc && var.getBaseVal(constant) == other_gc->var.getBaseVal(other_gc->constant) && 
    var.getBaseVar() == other_gc->var.getBaseVar();
}

template<typename VarRef>
inline void GreaterConstant<VarRef>::print(std::ostream& o) const
{ o << "GreaterConstant(var=" << var << " > val=" << constant << ")"; }

template<typename VarRef1, typename VarRef2>
inline vector<VirtConPtr> WatchlessPrunLeft<VarRef1,VarRef2>::whyT() const //return var2 <= val
{ return vector<VirtConPtr>(1, VirtConPtr(new LessConstant<VarRef2>(con->stateObj, con->var2, val + 1))); }

template<typename VarRef1, typename VarRef2>
inline AbstractConstraint* WatchlessPrunLeft<VarRef1,VarRef2>::getNeg() const
{ return new WatchLiteralConstraint<VarRef1>(con->stateObj, con->var1, val); }

template<typename VarRef1, typename VarRef2>
inline pair<unsigned,unsigned> WatchlessPrunLeft<VarRef1,VarRef2>::getDepth() const
{ return con->var1.getDepth(false, val); }

//doesn't capture all equalities, e.g., different watchless with same vars
template<typename VarRef1, typename VarRef2>
inline bool WatchlessPrunLeft<VarRef1,VarRef2>::equals(VirtCon* other) const
{ 
  WatchlessPrunLeft<VarRef1, VarRef2>* other_wlc = dynamic_cast<WatchlessPrunLeft<VarRef1, VarRef2>*>(other);
  return other_wlc && val == other_wlc->val && con == other_wlc->con;
}

template<typename VarRef1, typename VarRef2>
inline void WatchlessPrunLeft<VarRef1,VarRef2>::print(std::ostream& o) const
{ o << "WatchlessPrunLeft(var=" << con->var1 << ",val=" << val << ")"; }

template<typename VarRef1, typename VarRef2>
inline vector<VirtConPtr> WatchlessPrunRight<VarRef1,VarRef2>::whyT() const //return var1 >= val 
{ return vector<VirtConPtr>(1, VirtConPtr(new GreaterConstant<VarRef1>(con->stateObj, con->var1, val - 1))); }

template<typename VarRef1, typename VarRef2>
inline AbstractConstraint* WatchlessPrunRight<VarRef1,VarRef2>::getNeg() const
{ return new WatchLiteralConstraint<VarRef2>(con->stateObj, con->var2, val); }

template<typename VarRef1, typename VarRef2>
inline pair<unsigned,unsigned> WatchlessPrunRight<VarRef1,VarRef2>::getDepth() const
{ return con->var2.getDepth(false, val); }

//doesn't capture all equalities, e.g., different watchless with same vars
template<typename VarRef1, typename VarRef2>
inline bool WatchlessPrunRight<VarRef1,VarRef2>::equals(VirtCon* other) const
{
  WatchlessPrunRight<VarRef1, VarRef2>* other_wlc = dynamic_cast<WatchlessPrunRight<VarRef1, VarRef2>*>(other);
  return other_wlc && val == other_wlc->val && con == other_wlc->con;
}

template<typename VarRef1, typename VarRef2>
inline void WatchlessPrunRight<VarRef1,VarRef2>::print(std::ostream& o) const
{ o << "WatchlessPrunRight(var=" << con->var2 << ",val=" << val << ")"; }

inline vector<VirtConPtr> NegOfPostedCon::whyT() const
{ return con->whyF(); }

inline AbstractConstraint* NegOfPostedCon::getNeg() const
{ return con; }

inline pair<unsigned,unsigned> NegOfPostedCon::getDepth() const
{ return con->whenF(); }

inline bool NegOfPostedCon::equals(VirtCon* other) const
{ 
  NegOfPostedCon* other_nopc = dynamic_cast<NegOfPostedCon*>(other);
  return other_nopc && con == other_nopc->con;
}

inline void NegOfPostedCon::print(std::ostream& o) const
{ o << "NegOfPostedCon(con=" << con << ")"; }

inline vector<VirtConPtr> DisjunctionPrun::whyT() const
{
  vector<VirtConPtr> retVal;
  vector<AbstractConstraint*>& child_cons = dj->child_constraints;
  const size_t child_cons_s = child_cons.size();
  retVal.reserve(child_cons.size() - 1);
  for(size_t i = 0; i < child_cons_s; i++)
    if(child_cons[i] != doer)
      retVal.push_back(VirtConPtr(new NegOfPostedCon(child_cons[i])));
  if(done.get() != NULL) {
    const vector<VirtConPtr>& pruning_reason = done->whyT();
    retVal.insert(retVal.end(), pruning_reason.begin(), pruning_reason.end());
  }
  return retVal;
}

inline AbstractConstraint* DisjunctionPrun::getNeg() const
{ return done->getNeg(); }

inline pair<unsigned,unsigned> DisjunctionPrun::getDepth() const
{ 
  pair<unsigned,unsigned> retVal;
  if(done.get() != NULL) retVal = done->getDepth();
  else retVal = make_pair(0,0);
  vector<AbstractConstraint*>& child_cons = dj->child_constraints;
  const size_t child_cons_s = child_cons.size();
  for(size_t i = 0; i < child_cons_s; i++)
    if(child_cons[i] != doer)
      retVal = max(retVal, child_cons[i]->whenF());
  return retVal;
}

inline bool DisjunctionPrun::equals(VirtCon* other) const
{
  DisjunctionPrun* other_dp = dynamic_cast<DisjunctionPrun*>(other);
  return other_dp && dj == other_dp->dj && doer == other_dp->doer && done->equals(other_dp->done.get());
}

inline void DisjunctionPrun::print(std::ostream& o) const
{ o << "DisjunctionPrun(done=" << *done << ",doer=" << doer << ",dj=" << dj << ")"; }

template<typename VarRef>
inline vector<VirtConPtr> BecauseOfAssignmentPrun<VarRef>::whyT() const
{ return vector<VirtConPtr>(1, var.getExpl(true, var.getAssignedValue())); } //return the assignment that caused it

template<typename VarRef>
inline AbstractConstraint* BecauseOfAssignmentPrun<VarRef>::getNeg() const
{ return new WatchLiteralConstraint<VarRef>(stateObj, var, pruned); }

template<typename VarRef>
inline pair<unsigned,unsigned> BecauseOfAssignmentPrun<VarRef>::getDepth() const
{ return var.getDepth(false, pruned); }

template<typename VarRef>
inline bool BecauseOfAssignmentPrun<VarRef>::equals(VirtCon* other) const
{
  BecauseOfAssignmentPrun* other_boap = dynamic_cast<BecauseOfAssignmentPrun*>(other);
  return other_boap && var.getBaseVar() == other_boap->var.getBaseVar() &&
    var.getBaseVal(pruned) == other_boap->var.getBaseVal(other_boap->pruned);
}

template<typename VarRef>
inline void BecauseOfAssignmentPrun<VarRef>::print(std::ostream& o) const
{ o << "BecauseOfAssignmentPrun(var=" << var << ",pruned=" << pruned << ")"; }

inline vector<VirtConPtr> MHAV::whyT() const
{ return expls; } //just return all the virtcons for the prunings to the variable

inline AbstractConstraint* MHAV::getNeg() const 
{ D_ASSERT(false); return NULL; }

inline pair<unsigned,unsigned> MHAV::getDepth() const
{ D_ASSERT(false); return make_pair(-1, -1); }

inline bool MHAV::equals(VirtCon*) const
{ D_ASSERT(false); return false; }

inline void MHAV::print(std::ostream& o) const
{ o << "MHAV"; }

inline vector<VirtConPtr> AssgOrPrun::whyT() const
{
  vector<VirtConPtr> retVal;
  retVal.reserve(2);
  retVal.push_back(assg);
  retVal.push_back(prun);
  return retVal;
}

inline AbstractConstraint* AssgOrPrun::getNeg() const 
{ D_ASSERT(false); return NULL; }

inline pair<unsigned,unsigned> AssgOrPrun::getDepth() const
{ D_ASSERT(false); return make_pair(-1, -1); }

inline bool AssgOrPrun::equals(VirtCon*) const
{ D_ASSERT(false); return false; }

inline void AssgOrPrun::print(std::ostream& o) const
{ o << "AssgOrPrun"; }

template<typename VarRef>
inline vector<VirtConPtr> BecauseOfPruningsAssignment<VarRef>::whyT() const
{
  vector<VirtConPtr> retVal;
  const DomainInt var_initmin = var.getInitialMin();
  const DomainInt var_initmax = var.getInitialMax();
  retVal.reserve(var_initmax - var_initmin); //domain size minus 1
  for(int i = var_initmin; i < assigned; i++)
    retVal.push_back(var.getExpl(false, i));
  for(int i = assigned + 1; i <= var_initmax; i++)
    retVal.push_back(var.getExpl(false, i));
  return retVal;
}

template<typename VarRef>
inline AbstractConstraint* BecauseOfPruningsAssignment<VarRef>::getNeg() const
{ return new WatchNotLiteralConstraint<VarRef>(stateObj, var, assigned); }

template<typename VarRef>
inline pair<unsigned,unsigned> BecauseOfPruningsAssignment<VarRef>::getDepth() const
{ return var.getDepth(true, assigned); }

template<typename VarRef>
inline bool BecauseOfPruningsAssignment<VarRef>::equals(VirtCon* other) const
{
  BecauseOfPruningsAssignment<VarRef>* other_bopa = dynamic_cast<BecauseOfPruningsAssignment<VarRef>*>(other);
  return other_bopa && var.getBaseVar() == other_bopa->var.getBaseVar() &&
    var.getBaseVal(assigned) == other_bopa->var.getBaseVal(other_bopa->assigned);
}

template<typename VarRef>
inline void BecauseOfPruningsAssignment<VarRef>::print(std::ostream& o) const
{ o << "BecauseOfPruningsAssignment(var=" << var << ",assigned=" << assigned << ")"; }

template<typename VarRef>
inline vector<VirtConPtr> DecisionAssg<VarRef>::whyT() const
{
  D_ASSERT(false); //shouldn't be called
  return vector<VirtConPtr>(1, VirtConPtr(NULL));
}

template<typename VarRef>
inline AbstractConstraint* DecisionAssg<VarRef>::getNeg() const
{ return new WatchNotLiteralConstraint<VarRef>(stateObj, var, val); } //replace with (var != val) constraint

template<typename VarRef>
inline pair<unsigned, unsigned> DecisionAssg<VarRef>::getDepth() const
{ return var.getDepth(true, val); }

template<typename VarRef>
inline bool DecisionAssg<VarRef>::equals(VirtCon* other) const
{
  DecisionAssg<VarRef>* other_da = dynamic_cast<DecisionAssg<VarRef>*>(other);
  return other_da && val == other_da->val && var.getBaseVar() == other_da->var.getBaseVar();
}

template<typename VarRef>
inline void DecisionAssg<VarRef>::print(std::ostream& o) const
{ o << "DecisionAssg(" << var << " <- " << val << ")"; }

#endif