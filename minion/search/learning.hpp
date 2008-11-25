#ifndef LEARNING_HPP
#define LEARNING_HPP

#include "../dynamic_constraints/dynamic_less.h"
#include "../dynamic_constraints/unary/dynamic_literal.h"
#include "../dynamic_constraints/unary/dynamic_notliteral.h"
#include "../dynamic_constraints/dynamic_new_or.h"
#include "../dynamic_constraints/dynamic_nogood.h"

inline bool operator!=(const VirtConPtr& l, const VirtConPtr& r)
{ return !(l->equals(r.get())); }

struct VirtConPtrHash : unary_function<VirtCon, size_t>
{
  size_t operator()(const VirtConPtr vc) const
  { return vc->hash(); }
};

typedef pair<pair<unsigned,unsigned>,VirtConPtr> depth_VirtConPtr;

//NB. this comparison is carefully reasoned! The first disjunct is to make the
//first() in the set the deepest depth. However we don't have convenient access
//to a < operator for VirtCons and I don't want to write one, so we don't order
//them within depth (and it's not essential to do so).
struct comp_d_VCP : binary_function<depth_VirtConPtr,depth_VirtConPtr,bool>
{
  bool operator()(const depth_VirtConPtr& left, const depth_VirtConPtr& right) const
  { return left.first > right.first || (left.first == right.first && left.second != right.second); }
};

inline int distribute(StateObj* stateObj, set<depth_VirtConPtr,comp_d_VCP>& curr_d, 
		      unordered_set<VirtConPtr,VirtConPtrHash>& earlier, const vector<VirtConPtr>& things)
{
  D_ASSERT(things.size() != 0);
  const size_t things_s = things.size();
  const pair<unsigned,unsigned> cd = make_pair(getMemory(stateObj).backTrack().current_depth(), 0);
  unsigned retVal = 0;
  for(size_t i = 0; i < things_s; i++) {
    pair<unsigned,unsigned> i_d = things[i]->getDepth();
    if(i_d < cd) {
      earlier.insert(things[i]);
      retVal = max(retVal, i_d.first);
    } else
      curr_d.insert(make_pair(i_d, things[i]));
  }
  return (int)retVal;
}

inline int firstUipLearn(StateObj* stateObj, const VirtConPtr& failure)
{
  set<depth_VirtConPtr,comp_d_VCP> curr_d; 
  unordered_set<VirtConPtr,VirtConPtrHash> earlier; 
  int retVal = 0; //the deepest thing that ends up in earlier
  retVal = max(retVal, distribute(stateObj, curr_d, earlier, failure->whyT()));
  D_ASSERT(curr_d.size() != 0);
  while(curr_d.size() > 1) { 
    cout << endl;
    for(set<depth_VirtConPtr,comp_d_VCP>::iterator curr = curr_d.begin(); curr != curr_d.end(); curr++)
      cout << "d=" << (*curr).first << ",vc=" << *((*curr).second) << endl;
    VirtConPtr shallowest = curr_d.begin()->second; 
    curr_d.erase(curr_d.begin()); 
    retVal = max(retVal, distribute(stateObj, curr_d, earlier, shallowest->whyT()));
  } 
  earlier.insert(curr_d.begin()->second);
  for(unordered_set<VirtConPtr,VirtConPtrHash>::iterator curr = earlier.begin(); curr != earlier.end(); curr++)
    cout << **curr << "=" << (*curr)->hash() << "@" << (*curr)->getDepth() << endl;
  cout << "far BT depth=" << retVal << endl;
  cout << endl;
  vector<VirtConPtr> earlier_vec;
  const size_t earlier_s = earlier.size();
  earlier_vec.reserve(earlier_s);
  for(unordered_set<VirtConPtr,VirtConPtrHash>::iterator curr = earlier.begin(); curr != earlier.end(); curr++)
    earlier_vec.push_back(*curr);
  getState(stateObj).addConstraintMidsearch(new NogoodConstraint(stateObj, earlier_vec));
  return retVal;
} 

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
inline size_t DisAssignment<VarRef>::hash() const
{ return (guid + 37 * var.getBaseVal(val) + 111 * var.getBaseVar().pos()) % 16777619; }

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
inline size_t Assignment<VarRef>::hash() const
{ return (guid + 17 * var.getBaseVal(val) + 83 * var.getBaseVar().pos()) % 16777619; }

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
  return other_lc && constant == other_lc->constant && var.getBaseVar() == other_lc->var.getBaseVar();
}

template<typename VarRef>
inline void LessConstant<VarRef>::print(std::ostream& o) const
{ o << "LessConstant(var=" << var << " < val=" << constant << ")"; }

template<typename VarRef>
inline size_t LessConstant<VarRef>::hash() const
{ return (guid + 13 * constant + 127 * var.getBaseVar().pos()) % 16777619; }

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
  return other_gc && constant == other_gc->constant && var.getBaseVar() == other_gc->var.getBaseVar();
}

template<typename VarRef>
inline void GreaterConstant<VarRef>::print(std::ostream& o) const
{ o << "GreaterConstant(var=" << var << " > val=" << constant << ")"; }

template<typename VarRef>
inline size_t GreaterConstant<VarRef>::hash() const
{ return (guid + 13 * constant + 41 * var.getBaseVar().pos()) % 16777619; }

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
inline size_t WatchlessPrunLeft<VarRef1,VarRef2>::hash() const
{ return (guid + val + (size_t)con) % 16777619; }

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

template<typename VarRef1, typename VarRef2>
inline size_t WatchlessPrunRight<VarRef1,VarRef2>::hash() const
{ return (guid + val + (size_t)con) % 16777619; }

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
{ o << "NegOfPostedCon(con=" << *con << ")"; }

inline size_t NegOfPostedCon::hash() const
{ return (guid + (size_t)con) % 16777619; }

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

inline size_t DisjunctionPrun::hash() const
{ return (guid + (size_t)dj + (size_t)doer) % 16777619; }

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

template<typename VarRef>
inline size_t BecauseOfAssignmentPrun<VarRef>::hash() const
{ return (guid + 17 * var.getBaseVar().pos() + 141 * var.getBaseVal(pruned)) % 16777619; }

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

inline size_t MHAV::hash() const
{ D_ASSERT(false); return 0; }

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
{ o << "AssgOrPrun(assg=" << *assg << ",prun=" << *prun << ")"; }

inline size_t AssgOrPrun::hash() const
{ D_ASSERT(false); return 0; }

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
inline size_t BecauseOfPruningsAssignment<VarRef>::hash() const
{ return (guid + 13 * var.getBaseVar().pos() + 127 * var.getBaseVal(assigned)) % 16777619; }

template<typename VarRef>
inline vector<VirtConPtr> DecisionAssg<VarRef>::whyT() const
{
  D_ASSERT(false); //shouldn't be called
  return vector<VirtConPtr>(1, VirtConPtr(NULL));
}

template<typename VarRef>
inline AbstractConstraint* DecisionAssg<VarRef>::getNeg() const
{ return new WatchNotLiteralConstraint<VarRef>(stateObj, var, val); }

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

template<typename VarRef>
inline size_t DecisionAssg<VarRef>::hash() const
{ return (guid + 7 * val + 17 * var.getBaseVar().pos()) % 16777619; }

template<typename VarRef>
inline vector<VirtConPtr> NoReasonPrun<VarRef>::whyT() const
{ return vector<VirtConPtr>(); }

template<typename VarRef>
inline AbstractConstraint* NoReasonPrun<VarRef>::getNeg() const
{ return new WatchLiteralConstraint<VarRef>(stateObj, var, val); }

template<typename VarRef>
inline pair<unsigned, unsigned> NoReasonPrun<VarRef>::getDepth() const
{ return make_pair(0, 0); }

template<typename VarRef>
inline bool NoReasonPrun<VarRef>::equals(VirtCon* other) const
{
  NoReasonPrun<VarRef>* other_nrp = dynamic_cast<NoReasonPrun<VarRef>*>(other);
  return other_nrp && var.getBaseVal(val) == other_nrp->var.getBaseVal(other_nrp->val) && 
    var.getBaseVar() == other_nrp->var.getBaseVar();
}

template<typename VarRef>
inline void NoReasonPrun<VarRef>::print(std::ostream& o) const
{ o << "NoReasonPrun(" << var << " <-/- " << val << ")"; }

template<typename VarRef>
inline size_t NoReasonPrun<VarRef>::hash() const
{ return (guid + 7 * val + 17 * var.getBaseVar().pos()) % 16777619; }

template<typename VarRef>
inline vector<VirtConPtr> NoReasonAssg<VarRef>::whyT() const
{ return vector<VirtConPtr>(); }

template<typename VarRef>
inline AbstractConstraint* NoReasonAssg<VarRef>::getNeg() const
{ return new WatchNotLiteralConstraint<VarRef>(stateObj, var, val); }

template<typename VarRef>
inline pair<unsigned, unsigned> NoReasonAssg<VarRef>::getDepth() const
{ return make_pair(0, 0); }

template<typename VarRef>
inline bool NoReasonAssg<VarRef>::equals(VirtCon* other) const
{
  NoReasonAssg<VarRef>* other_nra = dynamic_cast<NoReasonAssg<VarRef>*>(other);
  return other_nra && var.getBaseVal(val) == other_nra->var.getBaseVal(other_nra->val) && 
    var.getBaseVar() == other_nra->var.getBaseVar();
}

template<typename VarRef>
inline void NoReasonAssg<VarRef>::print(std::ostream& o) const
{ o << "NoReasonAssg(" << var << " <- " << val << ")"; }

template<typename VarRef>
inline size_t NoReasonAssg<VarRef>::hash() const
{ return (guid + 7 * val + 17 * var.getBaseVar().pos()) % 16777619; }

#endif
