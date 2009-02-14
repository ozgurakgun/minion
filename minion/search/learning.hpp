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

inline bool operator<(const depth_VirtConPtr& l, const depth_VirtConPtr& r)
{ return l.first < r.first || (l.first == r.first && l.second->less(r.second.get())); }

//sort descending
struct comp_d_VCP : binary_function<depth_VirtConPtr,depth_VirtConPtr,bool>
{
  bool operator()(const depth_VirtConPtr& left, const depth_VirtConPtr& right) const
  { return right < left; }
};

typedef int state_cert;

inline state_cert state_start(StateObj* stateObj)
{
  getVars(stateObj).getBigRangevarContainer().bound_changed = false;
  return getVars(stateObj).getBigRangevarContainer().bms_array.get_local_depth();
}

inline bool state_changed(StateObj* stateObj, state_cert cert)
{
  return getVars(stateObj).getBigRangevarContainer().bound_changed
    || cert != getVars(stateObj).getBigRangevarContainer().bms_array.get_local_depth();
}
  
//Distribute things into curr_d and earlier. Do nothing and return false iff
//this would mean curr_d is empty.
inline pair<bool,int> distribute(StateObj* stateObj, set<depth_VirtConPtr,comp_d_VCP>& curr_d, 
				 unordered_set<VirtConPtr,VirtConPtrHash>& earlier, const vector<VirtConPtr>& things)
{
  const size_t things_s = things.size();
  const pair<unsigned,unsigned> cd = make_pair(getMemory(stateObj).backTrack().current_depth(), 0);
  unsigned retVal = 0;
  vector<depth_VirtConPtr > things_w_depth; //things waiting to be categorised
  //first fill things_w_depth and check that curr_d will not be empty
  bool something_for_curr_d = false;
  for(size_t i = 0; i < things_s; i++) {
    depth_VirtConPtr thing = make_pair(things[i]->getDepth(), things[i]);
    if(thing.first.first != 0) { //only include stuff that wasn't true at the root node
      things_w_depth.push_back(thing);
      if(thing.first >= cd)
	something_for_curr_d = true;
    }
  }
  if(curr_d.size() == 0 && !something_for_curr_d)
    return make_pair(false, 0);
  //now distribute the stuff as usual
  const size_t things_w_depth_s = things_w_depth.size();
  for(size_t i = 0; i < things_w_depth_s; i++) {
    if(things_w_depth[i].first < cd) {
      earlier.insert(things_w_depth[i].second);
      retVal = max(retVal, things_w_depth[i].first.first);
    } else
      curr_d.insert(things_w_depth[i]);
  }
  return make_pair(true,(int)retVal);
}

inline AbstractConstraint* makeCon(StateObj* stateObj,
				 const unordered_set<VirtConPtr,VirtConPtrHash>& earlier,
				 const set<depth_VirtConPtr,comp_d_VCP>& curr_d) {
  vector<VirtConPtr> earlier_vec;
  const size_t earlier_s = earlier.size();
  if(earlier_s == 0) return curr_d.begin()->second->getNeg(); //unary disjunction special case
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
    //cout << *failure << endl;
    if(getMemory(stateObj).backTrack().current_depth() == 0) {
      //cout << "failed at root node" << endl;
      return -2;
    }
    getState(stateObj).setFailed(false);
    set<depth_VirtConPtr,comp_d_VCP> curr_d; 
    unordered_set<VirtConPtr,VirtConPtrHash> earlier; 
    int retVal = 0; //the deepest thing that ends up in earlier
    cout << "Failure:" << *failure;
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
    AbstractConstraint* firstUIP = makeCon(stateObj, earlier, curr_d);
    cout << "firstUIP:" << *firstUIP << endl;
    
    //also make lastUIP in case it's needed, code will work if firstUIP=lastUIP
    depth_VirtConPtr deepest;
    while(true) {
      deepest = *curr_d.begin();
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
    AbstractConstraint* lastUIP = makeCon(stateObj, earlier, curr_d);
    //try firstUIP
#ifdef LASTUIP
    NogoodConstraint* lastUIP = makeCon(stateObj, earlier, curr_d);
    cout << "lastUIP:" << *lastUIP << endl;
#endif
    world_pop(stateObj);
    maybe_print_search_action(stateObj, "bt");
    firstUIP->setup();
    state_cert cert = state_start(stateObj);
    //cout << "start trying firstUIP" << endl;
    firstUIP->full_propagate();
    prop(stateObj, v);
    //cout << "end trying firstUIP" << endl;
    if(state_changed(stateObj, cert)) { //did propagation occur? if so add
      //cout << "adding firstUIP" << endl;
      getState(stateObj).addConstraintMidsearch(firstUIP);
    } else { //if not use the first decision cut
      firstUIP->cleanup(); //remove effects of propagating it before
      D_ASSERT(getQueue(stateObj).isQueuesEmpty());
#ifndef LASTUIP
      //cout << "adding lastUIP" << endl;
      cout << "lastUIP:" << *lastUIP << endl;
#endif
      getState(stateObj).addConstraintMidsearch(lastUIP);
      //cout << "start trying lastUIP" << endl;
      lastUIP->setup();
      lastUIP->full_propagate();
      prop(stateObj, v);
      //cout << "end trying lastUIP" << endl;
      D_ASSERT(state_changed(stateObj, cert));
    }
    //cout << "far BT depth=" << retVal << endl;
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
  if(guid != other->guid) return false;
  LCCompData* other_data = reinterpret_cast<LCCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<LCCompData*>(other_data));
  bool retVal = constant == other_data->constant && var.getBaseVar() == other_data->var;
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline bool LessConstant<VarRef>::less(VirtCon* other) const
{
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  LCCompData* other_data = reinterpret_cast<LCCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<LCCompData*>(other_data));
  bool retVal = constant < other_data->constant || (constant == other_data->constant && var.getBaseVar() < other_data->var);
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline void LessConstant<VarRef>::print(std::ostream& o) const
{ o << "LessConstant(var=" << var << " < val=" << constant << ")"; }

template<typename VarRef>
inline size_t LessConstant<VarRef>::hash() const
{ return (guid + 13 * constant + 127 * var.getBaseVar().pos()) % 16777619; }

template<typename VarRef>
inline LCCompData* LessConstant<VarRef>::getVCCompData() const
{ return new LCCompData(var.getBaseVar(), constant); }

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
  if(guid != other->guid) return false;
  GCCompData* other_data = reinterpret_cast<GCCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<GCCompData*>(other_data));
  bool retVal = constant == other_data->constant && var.getBaseVar() == other_data->var;
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline bool GreaterConstant<VarRef>::less(VirtCon* other) const
{
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  GCCompData* other_data = reinterpret_cast<GCCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<GCCompData*>(other_data));
  bool retVal = constant < other_data->constant || (constant == other_data->constant && var.getBaseVar() < other_data->var);
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline void GreaterConstant<VarRef>::print(std::ostream& o) const
{ o << "GreaterConstant(var=" << var << " > val=" << constant << ")"; }

template<typename VarRef>
inline size_t GreaterConstant<VarRef>::hash() const
{ return (guid + 13 * constant + 41 * var.getBaseVar().pos()) % 16777619; }

template<typename VarRef>
inline GCCompData* GreaterConstant<VarRef>::getVCCompData() const
{ return new GCCompData(var.getBaseVar(), constant); }

template<typename VarRef1, typename VarRef2>
inline vector<VirtConPtr> WatchlessPrunLeft<VarRef1,VarRef2>::whyT() const //return var2 <= val
{ return vector<VirtConPtr>(1, VirtConPtr(new LessConstant<VarRef2>(con->stateObj, con->var2, val + 1))); }

template<typename VarRef1, typename VarRef2>
inline AbstractConstraint* WatchlessPrunLeft<VarRef1,VarRef2>::getNeg() const
{ return new WatchLiteralConstraint<VarRef1>(con->stateObj, con->var1, val); }

template<typename VarRef1, typename VarRef2>
inline pair<unsigned,unsigned> WatchlessPrunLeft<VarRef1,VarRef2>::getDepth() const
{ return con->var1.getDepth(false, val); }

template<typename VarRef1, typename VarRef2>
inline bool WatchlessPrunLeft<VarRef1,VarRef2>::equals(VirtCon* other) const
{ 
  if(guid != other->guid) return false;
  WPLCompData* other_data = reinterpret_cast<WPLCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<WPLCompData*>(other_data));
  bool retVal = val == other_data->val && con->var1.getBaseVar() == other_data->var1
    && con->var2.getBaseVar() == other_data->var2;
  delete other_data;
  return retVal;
}

template<typename VarRef1, typename VarRef2>
inline bool WatchlessPrunLeft<VarRef1,VarRef2>::less(VirtCon* other) const
{ 
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  WPLCompData* other_data = reinterpret_cast<WPLCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<WPLCompData*>(other_data));
  return val < other_data->val 
    || (val == other_data->val && con->var1.getBaseVar() < other_data->var1)
    || (val == other_data->val && con->var1.getBaseVar() == other_data->var1 && con->var2.getBaseVar() < other_data->var2);
}

template<typename VarRef1, typename VarRef2>
inline void WatchlessPrunLeft<VarRef1,VarRef2>::print(std::ostream& o) const
{ o << "WatchlessPrunLeft(var=" << con->var1 << ",val=" << val << ")"; }

template<typename VarRef1, typename VarRef2>
inline size_t WatchlessPrunLeft<VarRef1,VarRef2>::hash() const
{ return (guid + val + con->hash()) % 16777619; }

template<typename VarRef1, typename VarRef2>
inline WPLCompData* WatchlessPrunLeft<VarRef1,VarRef2>::getVCCompData() const
{ return new WPLCompData(con->var1.getBaseVar(), con->var2.getBaseVar(), val); }

template<typename VarRef1, typename VarRef2>
inline vector<VirtConPtr> WatchlessPrunRight<VarRef1,VarRef2>::whyT() const //return var1 >= val 
{ return vector<VirtConPtr>(1, VirtConPtr(new GreaterConstant<VarRef1>(con->stateObj, con->var1, val - 1))); }

template<typename VarRef1, typename VarRef2>
inline AbstractConstraint* WatchlessPrunRight<VarRef1,VarRef2>::getNeg() const
{ return new WatchLiteralConstraint<VarRef2>(con->stateObj, con->var2, val); }

template<typename VarRef1, typename VarRef2>
inline pair<unsigned,unsigned> WatchlessPrunRight<VarRef1,VarRef2>::getDepth() const
{ return con->var2.getDepth(false, val); }

template<typename VarRef1, typename VarRef2>
inline bool WatchlessPrunRight<VarRef1,VarRef2>::equals(VirtCon* other) const
{ 
  if(guid != other->guid) return false;
  WPRCompData* other_data = reinterpret_cast<WPRCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<WPRCompData*>(other_data));
  bool retVal = val == other_data->val && con->var1.getBaseVar() == other_data->var1
    && con->var2.getBaseVar() == other_data->var2;
  delete other_data;
  return retVal;
}

template<typename VarRef1, typename VarRef2>
inline bool WatchlessPrunRight<VarRef1,VarRef2>::less(VirtCon* other) const
{ 
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  WPRCompData* other_data = reinterpret_cast<WPRCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<WPRCompData*>(other_data));
  return val < other_data->val 
    || (val == other_data->val && con->var1.getBaseVar() < other_data->var1)
    || (val == other_data->val && con->var1.getBaseVar() == other_data->var1 && con->var2.getBaseVar() < other_data->var2);
}

template<typename VarRef1, typename VarRef2>
inline void WatchlessPrunRight<VarRef1,VarRef2>::print(std::ostream& o) const
{ o << "WatchlessPrunRight(var=" << con->var2 << ",val=" << val << ")"; }

template<typename VarRef1, typename VarRef2>
inline size_t WatchlessPrunRight<VarRef1,VarRef2>::hash() const
{ return (guid + val + con->hash()) % 16777619; }

template<typename VarRef1, typename VarRef2>
inline WPRCompData* WatchlessPrunRight<VarRef1,VarRef2>::getVCCompData() const
{ return new WPRCompData(con->var1.getBaseVar(), con->var2.getBaseVar(), val); }

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

inline bool NegOfPostedCon::less(VirtCon* other) const
{ 
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  NegOfPostedCon* other_nopc = dynamic_cast<NegOfPostedCon*>(other);
  D_ASSERT(other_nopc);
  return con->less(other_nopc->con);
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

//doesn't capture all possible equalities, e.g. an identical pruning by an
//identical but distinct disjunction
inline bool DisjunctionPrun::equals(VirtCon* other) const
{
  DisjunctionPrun* other_dp = dynamic_cast<DisjunctionPrun*>(other);
  return other_dp && dj == other_dp->dj && doer == other_dp->doer && done->equals(other_dp->done.get());
}
 
inline bool DisjunctionPrun::less(VirtCon* other) const
{
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  DisjunctionPrun* other_dp = dynamic_cast<DisjunctionPrun*>(other);
  D_ASSERT(other_dp);
  return done->less(other_dp->done.get())
    || (done->equals(other_dp->done.get()) && doer->less(other_dp->doer))
    || (done->equals(other_dp->done.get()) && doer->equal(other_dp->doer) && dj->less(other_dp->dj));
}

inline void DisjunctionPrun::print(std::ostream& o) const
{ o << "DisjunctionPrun(done=" << *done << ",doer=" << doer << ",dj=" << dj << ")"; }

//TODO: might like to make the hash function include the disjunction as well as the disjunct
inline size_t DisjunctionPrun::hash() const
{ return (guid + doer->hash()) % 16777619; }

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
  if(guid != other->guid) return false;
  BOAPCompData* other_data = reinterpret_cast<BOAPCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<BOAPCompData*>(other_data));
  bool retVal = var.getBaseVar() == other_data->var &&
    pruned == other_data->pruned && assigned == other_data->assigned;
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline bool BecauseOfAssignmentPrun<VarRef>::less(VirtCon* other) const
{
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  BOAPCompData* other_data = reinterpret_cast<BOAPCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<BOAPCompData*>(other_data));
  bool retVal = var.getBaseVar() < other_data->var
    || (var.getBaseVar() == other_data->var && pruned < other_data->pruned)
    || (var.getBaseVar() == other_data->var && pruned == other_data->pruned && assigned < other_data->assigned);
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline void BecauseOfAssignmentPrun<VarRef>::print(std::ostream& o) const
{ o << "BecauseOfAssignmentPrun(var=" << var << ",pruned=" << pruned << ")"; }

template<typename VarRef>
inline size_t BecauseOfAssignmentPrun<VarRef>::hash() const
{ return (guid + 17 * var.getBaseVar().pos() + 141 * var.getBaseVal(pruned)) % 16777619; }

template<typename VarRef>
inline BOAPCompData* BecauseOfAssignmentPrun<VarRef>::getVCCompData() const
{ return new BOAPCompData(var.getBaseVar(), pruned, assigned); }

inline vector<VirtConPtr> MHAV::whyT() const
{ return expls; } //just return all the virtcons for the prunings to the variable

inline AbstractConstraint* MHAV::getNeg() const 
{ D_ASSERT(false); return NULL; }

inline pair<unsigned,unsigned> MHAV::getDepth() const
{ D_ASSERT(false); return make_pair(-1, -1); }

inline bool MHAV::equals(VirtCon*) const
{ D_ASSERT(false); return false; }

inline bool MHAV::less(VirtCon*) const
{ D_ASSERT(false); return false; }

inline void MHAV::print(std::ostream& o) const
{ 
  o << "MHAV(" << *expls[0]; 
  for(size_t i = 1; i < expls.size(); i++)
    o << "," << *expls[i];
  cout << ")" << endl;
}

inline size_t MHAV::hash() const
{ D_ASSERT(false); return 0; }

inline vector<VirtConPtr> AssgOrPrun::whyT() const
{
  vector<VirtConPtr> retVal;
  if(assg->getDepth().second != 0)
    retVal = assg->whyT();
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

inline bool AssgOrPrun::less(VirtCon*) const
{ D_ASSERT(false); return false; }

inline void AssgOrPrun::print(std::ostream& o) const
{ 
  vector<VirtConPtr> cut = whyT();
  cout << "AssgOrPrun(cut=" << "[" << *cut[0];
  for(size_t i = 1; i < cut.size(); i++)
    cout << "," << *cut[i];
  cout << "]" << ",assg=" << *assg << ",prun=" << *prun << ")"; 
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
  if(guid != other->guid) return false;
  BOPACompData* other_data = reinterpret_cast<BOPACompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<BOPACompData*>(other_data));
  bool retVal = var.getBaseVar() == other_data->var && assigned == other_data->assigned;
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline bool BecauseOfPruningsAssignment<VarRef>::less(VirtCon* other) const
{
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  BOPACompData* other_data = reinterpret_cast<BOPACompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<BOPACompData*>(other_data));
  bool retVal = var.getBaseVar() < other_data->var
    || (var.getBaseVar() == other_data->var && assigned < other_data->assigned);
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline void BecauseOfPruningsAssignment<VarRef>::print(std::ostream& o) const
{ o << "BecauseOfPruningsAssignment(var=" << var << ",assigned=" << assigned << ")"; }

template<typename VarRef>
inline size_t BecauseOfPruningsAssignment<VarRef>::hash() const
{ return (guid + 13 * var.getBaseVar().pos() + 127 * var.getBaseVal(assigned)) % 16777619; }

template<typename VarRef>
inline BOPACompData* BecauseOfPruningsAssignment<VarRef>::getVCCompData() const
{ return new BOPACompData(var.getBaseVar(), assigned); }

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
  if(guid != other->guid) return false;
  DACompData* other_data = reinterpret_cast<DACompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<DACompData*>(other_data));
  bool retVal = var.getBaseVar() == other_data->var && val == other_data->val;
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline bool DecisionAssg<VarRef>::less(VirtCon* other) const
{
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  DACompData* other_data = reinterpret_cast<DACompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<DACompData*>(other_data));
  bool retVal = var.getBaseVar() < other_data->var
    || (var.getBaseVar() == other_data->var && val < other_data->val);
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline void DecisionAssg<VarRef>::print(std::ostream& o) const
{ o << "DecisionAssg(" << var << " <- " << val << ")"; }

template<typename VarRef>
inline size_t DecisionAssg<VarRef>::hash() const
{ return (guid + 7 * val + 17 * var.getBaseVar().pos()) % 16777619; }

template<typename VarRef>
inline DACompData* DecisionAssg<VarRef>::getVCCompData() const
{ return new DACompData(var.getBaseVar(), val); }

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
  if(guid != other->guid) return false;
  NRPCompData* other_data = reinterpret_cast<NRPCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<NRPCompData*>(other_data));
  bool retVal = var.getBaseVar() == other_data->var && val == other_data->val;
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline bool NoReasonPrun<VarRef>::less(VirtCon* other) const
{
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  NRPCompData* other_data = reinterpret_cast<NRPCompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<NRPCompData*>(other_data));
  bool retVal = var.getBaseVar() < other_data->var
    || (var.getBaseVar() == other_data->var && val < other_data->val);
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline void NoReasonPrun<VarRef>::print(std::ostream& o) const
{ o << "NoReasonPrun(" << var << " <-/- " << val << ")"; }

template<typename VarRef>
inline size_t NoReasonPrun<VarRef>::hash() const
{ return (guid + 7 * val + 17 * var.getBaseVar().pos()) % 16777619; }

template<typename VarRef>
inline NRPCompData* NoReasonPrun<VarRef>::getVCCompData() const
{ return new NRPCompData(var.getBaseVar(), val); }

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
  if(guid != other->guid) return false;
  NRACompData* other_data = reinterpret_cast<NRACompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<NRACompData*>(other_data));
  bool retVal = var.getBaseVar() == other_data->var && val == other_data->val;
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline bool NoReasonAssg<VarRef>::less(VirtCon* other) const
{
  if(guid < other->guid) return true;
  if(other->guid < guid) return false;
  NRACompData* other_data = reinterpret_cast<NRACompData*>(other->getVCCompData());
  D_ASSERT(dynamic_cast<NRACompData*>(other_data));
  bool retVal = var.getBaseVar() < other_data->var
    || (var.getBaseVar() == other_data->var && val < other_data->val);
  delete other_data;
  return retVal;
}

template<typename VarRef>
inline void NoReasonAssg<VarRef>::print(std::ostream& o) const
{ o << "NoReasonAssg(" << var << " <- " << val << ")"; }

template<typename VarRef>
inline size_t NoReasonAssg<VarRef>::hash() const
{ return (guid + 7 * val + 17 * var.getBaseVar().pos()) % 16777619; }

template<typename VarRef>
inline NRACompData* NoReasonAssg<VarRef>::getVCCompData() const
{ return new NRACompData(var.getBaseVar(), val); }

inline vector<VirtConPtr> AMOV::whyT() const
{
  vector<VirtConPtr> retVal;
  retVal.reserve(2);
  retVal.push_back(assg_first);
  retVal.push_back(assg_last);
  return retVal;
}

inline AbstractConstraint* AMOV::getNeg() const 
{ D_ASSERT(false); return NULL; }

inline pair<unsigned,unsigned> AMOV::getDepth() const
{ D_ASSERT(false); return make_pair(-1, -1); }

inline bool AMOV::equals(VirtCon*) const
{ D_ASSERT(false); return false; }

inline bool AMOV::less(VirtCon*) const
{ D_ASSERT(false); return false; }

inline void AMOV::print(std::ostream& o) const
{ 
  vector<VirtConPtr> cut = whyT();
  cout << "AMOV(cut=" << "[" << *cut[0];
  for(size_t i = 1; i < cut.size(); i++)
    cout << "," << *cut[i];
  cout << "]" << ",assg_first=" << *assg_first << ",assg_last=" << *assg_last << ")";
}

inline size_t AMOV::hash() const
{ D_ASSERT(false); return 0; }

#endif
