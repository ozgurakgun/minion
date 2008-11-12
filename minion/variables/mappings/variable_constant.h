/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
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

#include "../../CSPSpec.h"

#include "../../constraints/constraint_abstract.h"

struct ConstantVar
{
  // TODO: This really only needs enough to get 'fail'
  StateObj* stateObj;
  
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_Yes;
  
  VirtConPtr vc_prun;
  pair<unsigned,unsigned> d;
  
  // Hmm.. no sure if it's better to make this true or false.
  BOOL isBound() const
  { return true;}
  
  DomainInt val;
  
  explicit ConstantVar(StateObj* _stateObj, DomainInt _val) : stateObj(_stateObj), val(_val)
  {}
  
  ConstantVar() 
  {}
  
  ConstantVar(const ConstantVar& b) : stateObj(b.stateObj), val(b.val)
  {}
  
  BOOL isAssigned() const
  { return true;}
  
  DomainInt getAssignedValue() const
  { return val;}
  
  BOOL isAssignedValue(DomainInt i) const
  { return i == val; }
  
  BOOL inDomain(DomainInt b) const
  { return b == val; }

  BOOL inDomain_noBoundCheck(DomainInt b) const
  { 
    D_ASSERT(b == val);
	return true;
  }
  
  DomainInt getMax() const
  { return val; }
  
  DomainInt getMin() const
  { return val; }

  DomainInt getInitialMax() const
  { return val; }
  
  DomainInt getInitialMin() const
  { return val; }

  //Using the vc for the pruning of the sole value as the nogood, because when
  //whyT() is called it will provide a node that lead to the sole pruning
  //causing the failure. An alternative would have been to return (v <-\- c) but
  //this is pointless and would end up causing (v <- c) to be posted.
  
  void setMax(DomainInt i)
  { 
    if(i<val) {
      getState(stateObj).setFailure(vc_prun);
      getState(stateObj).setFailed(true); 
    }
  }
  
  void setMin(DomainInt i)
  { 
    if(i>val) {
      getState(stateObj).setFailure(vc_prun);
      getState(stateObj).setFailed(true); 
    }
  }
  
  void uncheckedAssign(DomainInt)
  { FAIL_EXIT(); }
  
  void propagateAssign(DomainInt b)
  {
    if(b != val) {
      getState(stateObj).setFailure(vc_prun);      
      getState(stateObj).setFailed(true); 
    }
  }
  
  void decisionAssign(DomainInt b)
  { propagateAssign(b); }
  
  void removeFromDomain(DomainInt b)
  { 
    if(b==val) {
      getState(stateObj).setFailure(vc_prun);
      getState(stateObj).setFailed(true); 
    }
  }
 
  void addTrigger(Trigger, TrigType)
  { }

  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* dt, TrigType, DomainInt = -999)
  { dt->remove(); }
#endif

  vector<AbstractConstraint*>* getConstraints() { return NULL; }

  void addConstraint(AbstractConstraint* c){ ; }

  DomainInt getBaseVal(DomainInt v) const 
  { 
    D_ASSERT(v == val);
    return val; 
  }

  Var getBaseVar() const { return Var(VAR_CONSTANT, val); }

#ifdef WDEG
  int getBaseWdeg() { return 0; } //wdeg is irrelevant for non-search var

  void incWdeg() { ; }
#endif

  pair<unsigned,unsigned> getDepth(bool assg, DomainInt i) const
  { 
    D_ASSERT(i == val);
    if(assg) return make_pair(0, 0); else return d; //constant has been "assigned" since the start, pruning is more recent
  }

  void setExpl(bool assg, DomainInt i, VirtConPtr _vc)
  { 
    D_ASSERT(i == val);
    if(!assg) vc_prun = _vc; //don't store an explanation for why it's assigned, because it's always assigned
  } 
  
  VirtConPtr getExpl(bool assg, DomainInt i) const
  { 
    D_ASSERT(i == val);
    if(assg) return VirtConPtr(); else return vc_prun; 
  }
  
  int getDomainChange(DomainDelta d)
  { 
    D_ASSERT(d.XXX_get_domain_diff() == 0);
	return 0;
  }
  
  friend std::ostream& operator<<(std::ostream& o, const ConstantVar& constant)
  { return o << "Constant" << constant.val; }
};

