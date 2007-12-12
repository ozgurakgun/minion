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

struct TrivialBoundVar
{
  // TODO: This really only needs enough to get 'fail'
  StateObj* stateObj;
 
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_Yes;
  
  BOOL isBound()
  { return true;}
  
  Reversible<DomainInt> lower;
  Reversible<DomainInt> upper;
  
  explicit TrivialBoundVar(StateObj* _stateObj, DomainInt _lower, DomainInt _upper) :
  stateObj(_stateObj), lower(stateObj, _lower), upper(stateObj, _upper)
  { D_ASSERT(lower <= upper); }

  TrivialBoundVar(const TrivialBoundVar& b) : stateObj(b.stateObj), lower(b.lower), upper(b.upper)
  {}
  
  BOOL isAssigned() const
  { return lower == upper;}
  
  DomainInt getAssignedValue() const
  { 
    D_ASSERT(lower == upper);
    return lower;
  }
  
  BOOL isAssignedValue(DomainInt i) const
  { return lower == upper && i == lower; }
  
  BOOL inDomain(DomainInt b) const
  { return lower <= b && b <= upper; }
  
  BOOL inDomain_noBoundCheck(DomainInt b) const
  { 
    D_ASSERT(inDomain(b));
	return true;
  }
  
  DomainInt getMax() const
  { return upper; }
  
  DomainInt getMin() const
  { return lower; }

  unsigned getDepth() const
  { return 0; }

  unsigned getSeqNo() const
  { return 0; }

  DynamicConstraint* getAntecedent() const
  { return NULL; }

  int getId() const
  { return 0; }
  
  DomainInt getInitialMax() const
  { return upper; }
  
  DomainInt getInitialMin() const
  { return lower; }
  
  void setMax(DomainInt i)
  { 
    upper = min((DomainInt)upper, i);
    if(lower > upper)
      getState(stateObj).setFailed(true); 
  }
  
  void setMin(DomainInt i)
  {
    lower = max((DomainInt)lower, i);
    if(lower > upper)
      getState(stateObj).setFailed(true); 
  }

  void setDepth(unsigned) {;}

  void setAntecedent(DynamicConstraint*) {;}
  
  void uncheckedAssign(DomainInt)
  { FAIL_EXIT(); }
  
  void propagateAssign(DomainInt b)
  { 
    if(!inDomain(b))
    {
      getState(stateObj).setFailed(true); 
      return;
    }
    lower = b; upper = b;
  }
  
  void removeFromDomain(DomainInt b)
  { D_FATAL_ERROR("Can't do removeFromDomain on a bound var"); }
  
  void addTrigger(Trigger, TrigType)
  { }
  
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* dt, TrigType, DomainInt = -999)
  { dt->remove(); }
#endif
  
  int getDomainChange(DomainDelta d)
  { 
    return d.XXX_get_domain_diff();
  }
  
  friend std::ostream& operator<<(std::ostream& o, const TrivialBoundVar& var)
  { return o << "TrivialBoundVar" << var.lower << ":" << var.upper; }
};

