#ifndef LEARNING_HPP
#define LEARNING_HPP

#include "../dynamic_constraints/dynamic_less.h"
#include "../dynamic_constraints/unary/dynamic_literal.h"
#include "../dynamic_constraints/unary/dynamic_notliteral.h"

template<typename VarRef>
class LessConstant : public VirtCon { //var < constant

  StateObj* stateObj;
  VarRef var;
  DomainInt constant;

public:

  LessConstant(StateObj* _stateObj, VarRef _var, DomainInt _constant) : var(_var), constant(_constant) {}

  virtual vector<VirtConPtr> whyT() const
  {
    D_ASSERT(var.getInitialMax() < constant); //shouldn't ask why true- been true since start
    vector<VirtConPtr> retVal;
    retVal.reserve(var.getInitialMax() - constant);
    for(int i = constant + 1; i <= var.getInitialMax(); i++)
      retVal.push_back(var.getExpl(false, i));
    return retVal;
  }

  virtual AbstractConstraint* getNeg() const //constant <= var
  { 
    return new WatchLessConstraint<ConstantVar, VarRef>(stateObj, 
							ConstantVar(stateObj, constant - 1), 
							var); 
  } 
  
  virtual pair<unsigned,unsigned> getDepth() const //return the depth of last value > constant to be pruned out
  { 
    pair<unsigned,unsigned> retVal = make_pair(0,0); //if been true forever, make the depth 0
    for(int i = constant + 1; i <= var.getInitialMax(); i++)
      retVal = max(retVal, var.getDepth(false, i));
    return retVal;
  }

  virtual bool equals(const VirtCon& other) const
  {
    LessConstant<VarRef>* other_lc = dynamic_cast<LessConstant<VarRef>*>(other);
    return other_lc && var.getBaseVal(constant) == other_lc->var.getBaseVal(other_lc->constant) && 
      var.getBaseVal() == other_lc->var.getBaseVal();
  }
};

template<typename VarRef1, typename VarRef2>
class WatchlessPrunLeft : public VirtCon { //var1 < var2 has pruned val from var1

  WatchLessConstraint<VarRef1, VarRef2>* con;
  DomainInt val;

public:

  WatchlessPrunLeft(WatchLessConstraint<VarRef1, VarRef2>* _con, DomainInt _val) :
    con(_con), val(_val) {}
  
  virtual vector<VirtConPtr> whyT() const //return var2 <= v 
  { return vector<VirtConPtr>(1, VirtConPtr(new LessConstant<VarRef2>(con->stateObj, con->var2, val + 1))); }

  virtual AbstractConstraint* getNeg() const
  { return new WatchLiteralConstraint<VarRef1>(con->stateObj, con->var1, val); }

  virtual pair<unsigned,unsigned> getDepth() const
  { return con->var1.getDepth(false, val); }

  //doesn't capture all equalities, e.g., different watchless with same vars
  virtual bool equals(const VirtCon& other) const
  { 
    WatchLessConstraint<VarRef1, VarRef2>* other_wlc = dynamic_cast<WatchLessConstraint<VarRef1, VarRef2>*>(other);
    return other_wlc && val == other_wlc->val && con == other_wlc->con;
  }
};

template<typename VarRef>
class DecisionAssg : public VirtCon { //decision did assignment

  StateObj* stateObj;
  VarRef var;
  DomainInt val;

public:

  DecisionAssg(StateObj* _stateObj, VarRef _var, DomainInt _val) : stateObj(_stateObj), var(_var), val(_val) {}

  virtual vector<VirtConPtr> whyT() const
  {
    D_ASSERT(false); //shouldn't be called
    return VirtConPtr(this);
  }

  virtual AbstractConstraint* getNeg() const
  { return new WatchNotLiteralConstraint<VarRef>(stateObj, var, val); } //replace with (var != val) constraint

  virtual pair<unsigned, unsigned> getDepth() const
  { return var.getDepth(true, val); }

  virtual bool equals(const VirtCon& other) const
  {
    DecisionAssg<VarRef>* other_da = dynamic_cast<DecisionAssg<VarRef>*>(other);
    return other_da && val == other_da->val && var.getBaseVar() == other_da->var.getBaseVar();
  }
};

#endif
