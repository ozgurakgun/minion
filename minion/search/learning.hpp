#ifndef LEARNING_HPP
#define LEARNING_HPP

#include "../dynamic_constraints/dynamic_less.h"
#include "../dynamic_constraints/unary/dynamic_literal.h"
#include "../dynamic_constraints/unary/dynamic_notliteral.h"
#include "../dynamic_constraints/dynamic_new_or.h"
#include "../dynamic_constraints/dynamic_nogood.h"

#include "common_search.h"

inline bool operator!=(const VirtConPtr& l, const VirtConPtr& r)
{ return !(l->equals(r.get())); }

struct VirtConPtrHash : unary_function<VirtCon, size_t>
{
  size_t operator()(const VirtConPtr vc) const
  { return vc->hash(); }
};

typedef pair<pair<unsigned,unsigned>,VirtConPtr> depth_VirtConPtr;

//NB. this comparison is carefully reasoned! The first disjunct is to make the
//first() in the set the deepest depth. See above for definition of != for
//VirtConPtr.
struct comp_d_VCP : binary_function<depth_VirtConPtr,depth_VirtConPtr,bool>
{
  bool operator()(const depth_VirtConPtr& left, const depth_VirtConPtr& right) const
  { 
    D_ASSERT(!(left.first.first == right.first.first && //not allowed multiple decisions at same depth
	       left.second->isDecision() && 
	       right.second->isDecision() &&
	       left.second != right.second));
    return left.first > right.first  //order by depth first
      //then arbitrary order after that...
      || (left.first == right.first && !left.second->isDecision() && !right.second->isDecision() && 
	  left.second != right.second && left.second.get() < right.second.get())
      //except that decisions always come last
      || (left.first == right.first && right.second->isDecision() && left.second != right.second);
  }
};

//Distribute things into curr_d and earlier. Do nothing and return false iff
//this would mean curr_d is empty.
inline pair<bool,int> distribute(StateObj* stateObj, set<depth_VirtConPtr,comp_d_VCP>& curr_d, 
				 unordered_set<VirtConPtr,VirtConPtrHash>& earlier, const vector<VirtConPtr>& things)
{
  D_ASSERT(things.size() != 0);
  const size_t things_s = things.size();
  const pair<unsigned,unsigned> cd = make_pair(getMemory(stateObj).backTrack().current_depth(), 0);
  unsigned retVal = 0;
  vector<depth_VirtConPtr > things_w_depth; //things waiting to be categorised
  //first fill things_w_depth and check that curr_d will not be empty
  bool something_for_curr_d = false;
  for(size_t i = 0; i < things_s; i++) {
    depth_VirtConPtr thing = make_pair(things[i]->getDepth(), things[i]);
    things_w_depth.push_back(thing);
    if(thing.first >= cd)
      something_for_curr_d = true;
  }
  if(curr_d.size() == 0 && !something_for_curr_d)
    return make_pair(false, 0);
  //now distribute the stuff as usual
  for(size_t i = 0; i < things_s; i++) {
    if(things_w_depth[i].first < cd) {
      earlier.insert(things_w_depth[i].second);
      retVal = max(retVal, things_w_depth[i].first.first);
    } else
      curr_d.insert(things_w_depth[i]);
  }
  return make_pair(true,(int)retVal);
}

inline NogoodConstraint* makeCon(StateObj* stateObj,
				 const unordered_set<VirtConPtr,VirtConPtrHash>& earlier,
				 const set<depth_VirtConPtr,comp_d_VCP>& curr_d) {
  vector<VirtConPtr> earlier_vec;
  const size_t earlier_s = earlier.size();
  earlier_vec.reserve(earlier_s + 1);
  for(unordered_set<VirtConPtr,VirtConPtrHash>::const_iterator curr = earlier.begin(); curr != earlier.end(); curr++)
    earlier_vec.push_back(*curr);
  earlier_vec.push_back(curr_d.begin()->second);
  return new NogoodConstraint(stateObj, earlier_vec);
}  

inline void print_cut(const unordered_set<VirtConPtr,VirtConPtrHash>& earlier,
		      const set<depth_VirtConPtr,comp_d_VCP>& curr_d) 
{
  for(unordered_set<VirtConPtr,VirtConPtrHash>::const_iterator curr = earlier.begin(); curr != earlier.end(); curr++)
    cout << **curr << "=" << (*curr)->hash() << "@" << (*curr)->getDepth() << endl;
  for(set<depth_VirtConPtr,comp_d_VCP>::const_iterator curr = curr_d.begin(); curr != curr_d.end(); curr++)
    cout << *(curr->second) << "=" << curr->second->hash() << "@" << curr->second->getDepth() << endl;
}  

namespace Controller {

  template<typename Propagator, typename Var>
  inline int firstUipLearn(StateObj* stateObj, const VirtConPtr& failure, vector<Var>& v,
			   Propagator prop)
  {
    getState(stateObj).setFailed(false);
    set<depth_VirtConPtr,comp_d_VCP> curr_d; 
    unordered_set<VirtConPtr,VirtConPtrHash> earlier; 
    int retVal = 0; //the deepest thing that ends up in earlier
    //make firstUip cut
    pair<bool,int> dist = distribute(stateObj, curr_d, earlier, failure->whyT());
    retVal = dist.second;
    D_ASSERT(dist.first);
    D_ASSERT(curr_d.size() != 0);
    while(curr_d.size() > 1) { 
      VirtConPtr deepest = curr_d.begin()->second; 
      curr_d.erase(curr_d.begin()); 
      retVal = max(retVal, distribute(stateObj, curr_d, earlier, deepest->whyT()).second);
    }
    print_cut(earlier, curr_d);
    NogoodConstraint* firstUIP = makeCon(stateObj, earlier, curr_d);
    //also make lastUIP in case it's needed, code will work if firstUIP=lastUIP
    while(true) {
      depth_VirtConPtr deepest = *curr_d.begin();
      curr_d.erase(curr_d.begin());
      if(deepest.first.second == 0) { //can't resolve a decision, so replace shallowest and stop
	curr_d.insert(deepest);
	break;
      }
      pair<bool,int> dist_res = distribute(stateObj, curr_d, earlier, deepest.second->whyT());
      if(!dist_res.first) { //reached last UIP, so put the shallowest back and stop
	curr_d.insert(deepest);
	break;
      } else { //otherwise continue calculating cut
	retVal = max(retVal, dist_res.second);
      }
    }
    NogoodConstraint* lastUIP = makeCon(stateObj, earlier, curr_d);
    //try firstUIP
    world_pop(stateObj);
    maybe_print_search_action(stateObj, "bt");
    firstUIP->setup();
    unsigned seq_no_bef = getMemory(stateObj).backTrack().seq_no;
    cout << "start trying firstUIP" << endl;
    firstUIP->full_propagate();
    prop(stateObj, v);
    cout << "end trying firstUIP" << endl;
    if(seq_no_bef < getMemory(stateObj).backTrack().seq_no) { //did propagation occur? if so add
      cout << "adding firstUIP" << endl;
      getState(stateObj).addConstraintMidsearch(firstUIP);
    } else { //if not build a first decision cut and add it
      firstUIP->cleanup(); //remove effects of propagating it before
      D_ASSERT(getQueue(stateObj).isQueuesEmpty());
      cout << "adding lastUIP" << endl;
      getState(stateObj).addConstraintMidsearch(lastUIP);
      cout << "start trying lastUIP" << endl;
      lastUIP->setup();
      lastUIP->full_propagate();
      prop(stateObj, v);
      cout << "end trying lastUIP" << endl;
      D_ASSERT(seq_no_bef < getMemory(stateObj).backTrack().seq_no);
    }
    cout << "far BT depth=" << retVal << endl;
    return retVal;
  } 

} //namespace Controller

// template<typename VarRef>
// inline vector<VirtConPtr> DisAssignment<VarRef>::whyT() const
// { return var.getExpl(false, val)->whyT(); } //get the disassignments stored explanation

// template<typename VarRef>
// inline AbstractConstraint* DisAssignment<VarRef>::getNeg() const
// { return new WatchLiteralConstraint<VarRef>(stateObj, var, val); }

// template<typename VarRef>
// inline pair<unsigned,unsigned> DisAssignment<VarRef>::getDepth() const
// { return var.getDepth(false, val); }

// template<typename VarRef>
// inline bool DisAssignment<VarRef>::equals(VirtCon* other) const
// {
//   DisAssignment<VarRef>* other_da = dynamic_cast<DisAssignment<VarRef>*>(other);
//   return other_da && var.getBaseVal(val) == other_da->var.getBaseVal(other_da->val) &&
//     var.getBaseVar() == other_da->var.getBaseVar();
// }

// template<typename VarRef>
// inline void DisAssignment<VarRef>::print(std::ostream& o) const
// { o << "DisAssignment(var=" << var << ",val=" << val << ")"; }

// template<typename VarRef>
// inline size_t DisAssignment<VarRef>::hash() const
// { return (guid + 37 * var.getBaseVal(val) + 111 * var.getBaseVar().pos()) % 16777619; }

// template<typename VarRef>
// inline vector<VirtConPtr> Assignment<VarRef>::whyT() const
// { return var.getExpl(true, val)->whyT(); } //get the assignments stored explanation

// template<typename VarRef>
// inline AbstractConstraint* Assignment<VarRef>::getNeg() const
// { return new WatchNotLiteralConstraint<VarRef>(stateObj, var, val); }

// template<typename VarRef>
// inline pair<unsigned,unsigned> Assignment<VarRef>::getDepth() const
// { return var.getDepth(true, val); }

// template<typename VarRef>
// inline bool Assignment<VarRef>::equals(VirtCon* other) const
// {
//   Assignment<VarRef>* other_da = dynamic_cast<Assignment<VarRef>*>(other);
//   return other_da && var.getBaseVal(val) == other_da->var.getBaseVal(other_da->val) &&
//     var.getBaseVar() == other_da->var.getBaseVar();
// }

// template<typename VarRef>
// inline void Assignment<VarRef>::print(std::ostream& o) const
// { o << "Assignment(var=" << var << ",val=" << val << ")"; }

// template<typename VarRef>
// inline size_t Assignment<VarRef>::hash() const
// { return (guid + 17 * var.getBaseVal(val) + 83 * var.getBaseVar().pos()) % 16777619; }

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
{ return con->copy(); }

inline pair<unsigned,unsigned> NegOfPostedCon::getDepth() const
{ return con->whenF(); }

inline bool NegOfPostedCon::equals(VirtCon* other) const
{ 
  NegOfPostedCon* other_nopc = dynamic_cast<NegOfPostedCon*>(other);
  return other_nopc && con->equal(other_nopc->con);
}

inline void NegOfPostedCon::print(std::ostream& o) const
{ o << "NegOfPostedCon(con=" << *con << ")"; }

inline size_t NegOfPostedCon::hash() const
{ return (guid + con->hash()) % 16777619; }

inline vector<VirtConPtr> DisjunctionPrun::whyT() const
{
  vector<VirtConPtr> retVal;
  vector<AbstractConstraint*>& child_cons = dj->child_constraints;
  const size_t child_cons_s = child_cons.size();
  retVal.reserve(child_cons.size() - 1);
  for(size_t i = 0; i < child_cons_s; i++)
    if(child_cons[i] != doer)
      retVal.push_back(VirtConPtr(new NegOfPostedCon(child_cons[i])));
  const vector<VirtConPtr>& pruning_reason = done->whyT();
  retVal.insert(retVal.end(), pruning_reason.begin(), pruning_reason.end());
  return retVal;
}

inline AbstractConstraint* DisjunctionPrun::getNeg() const
{ return done->getNeg(); }

inline pair<unsigned,unsigned> DisjunctionPrun::getDepth() const
{ return done->getDepth(); }

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
{ return vector<VirtConPtr>(1, var.getExpl(true, assigned)); } //return the assignment that caused it

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
  vector<VirtConPtr> retVal = assg->whyT();
  const vector<VirtConPtr>& prun_whyT = prun->whyT();
  retVal.insert(retVal.end(), prun_whyT.begin(), prun_whyT.end());
  return retVal;
}

inline AbstractConstraint* AssgOrPrun::getNeg() const 
{ D_ASSERT(false); return NULL; }

inline pair<unsigned,unsigned> AssgOrPrun::getDepth() const
{ D_ASSERT(false); return make_pair(-1, -1); }

inline bool AssgOrPrun::equals(VirtCon*) const
{ D_ASSERT(false); return false; }

inline void AssgOrPrun::print(std::ostream& o) const
{ 
  vector<VirtConPtr> cut = whyT();
  cout << "AssgOrPrun(cut=" << "[" << *cut[0];
  for(size_t i = 1; i < cut.size(); i++)
    cout << "," << *cut[i];
  cout << "]" << ")"; 
}

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
{ return var.getDepth(false, val); }

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
{ return var.getDepth(true, val); }

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
