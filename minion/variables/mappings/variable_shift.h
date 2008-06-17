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

template<typename VarRef, typename ShiftType>
struct ShiftVar
{
  static const BOOL isBool = false;
  static const BoundType isBoundConst = VarRef::isBoundConst;
  VarRef data;
  
  BOOL isBound() const
  { return data.isBound();}
  
  ShiftType shift;
  ShiftVar(const VarRef& _data, ShiftType _shift) : data(_data), shift(_shift)
  { }
  
  ShiftVar() : data(), shift()
  { }
  
  ShiftVar(const ShiftVar& b) : data(b.data), shift(b.shift)
  { }
  
  BOOL isAssigned() const
  { return data.isAssigned(); }
  
  DomainInt getAssignedValue() const
  { return data.getAssignedValue() + shift; }
  
  BOOL isAssignedValue(DomainInt i) const
  { return data.getAssignedValue() == i - shift; }
  
  BOOL inDomain(DomainInt i) const
  { return data.inDomain(i - shift); }

  BOOL inDomain_noBoundCheck(DomainInt i) const
  { return data.inDomain(i - shift); }
  
  DomainInt getMax() const
  { return data.getMax() + shift; }
  
  DomainInt getMin() const
  { return data.getMin() + shift; }

  DomainInt getInitialMax() const
  { return data.getInitialMax() + shift; }
  
  DomainInt getInitialMin() const
  { return data.getInitialMin() + shift; }
  
  void setMax(DomainInt i, label l)
  { data.setMax(i - shift, l); }
  
  void setMin(DomainInt i, label l)
  { data.setMin(i - shift, l); }
  
  void uncheckedAssign(DomainInt b, label l)
  { data.uncheckedAssign(b - shift, l); }
  
  void propagateAssign(DomainInt b, label l)
  { data.propagateAssign(b - shift, l); }
  
  void decisionAssign(DomainInt b)
  { data.decisionAssign(b - shift); }
  
  void removeFromDomain(DomainInt b, label l)
  { data.removeFromDomain(b - shift, l); }
    
 void addTrigger(Trigger t, TrigType type)
  { 
    switch(type)
	{
	  case UpperBound:
	  case LowerBound:
	  case Assigned:
	  case DomainChanged:
	    data.addTrigger(t, type);
      break;
	  default:
      D_FATAL_ERROR("Fatal error in 'shift' mapper");
	}
  }

  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  data.addDynamicTrigger(t, type, pos); }
#endif

  friend std::ostream& operator<<(std::ostream& o, const ShiftVar& sv)
  { return o << "Shift " << sv.data << "+" << sv.shift; }
  
  int getDomainChange(DomainDelta d)
  { return data.getDomainChange(d); }

  vector<AbstractConstraint*>* getConstraints()
  { return data.getConstraints(); }

  void addConstraint(AbstractConstraint* c)
  { data.addConstraint(c); }

  DomainInt getBaseVal(DomainInt v) const {
    return data.getBaseVal(v - shift);
  }

  Var getBaseVar() const { return data.getBaseVar(); }

  void setDepth(DomainInt v, depth d)
  { data.setDepth(v, d); }

  depth getDepth(DomainInt v)
  { return data.getDepth(v); }

  void setLabel(DomainInt v, label l)
  { data.setLabel(v, l); }

  label getLabel(DomainInt v)
  { return data.getLabel(v); }

#ifdef WDEG
  int getBaseWdeg()
  { return data.getBaseWdeg(); }

  void incWdeg()
  { data.incWdeg(); }
#endif
};

template<typename T, typename U>
struct ShiftType
{ typedef ShiftVar<T,U> type; };

template<typename T,typename U>
struct ShiftType<vector<T>, U>
{ typedef vector<ShiftVar<T, U> > type; };

#ifdef LIGHT_VECTOR
template<typename T,typename U>
struct ShiftType<light_vector<T>, U>
{ typedef light_vector<ShiftVar<T, U> > type; };
#endif

template<typename T, std::size_t i, typename U>
struct ShiftType<array<T, i>, U >
{ typedef array<ShiftVar<T, U>, i> type; };


template<typename VRef, typename Shift>
typename ShiftType<VRef, Shift>::type
ShiftVarRef(VRef var_ref, Shift shift)
{ return ShiftVar<VRef, Shift>(var_ref, shift); }


template<typename VarRef, typename Shift>
vector<ShiftVar<VarRef, Shift> >
ShiftVarRef(const vector<VarRef>& var_array, const Shift& shift)
{
  vector<ShiftVar<VarRef, Shift> > shift_array(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
    shift_array[i] = ShiftVarRef(var_array[i], shift);
  return shift_array;
}

#ifdef LIGHT_VECTOR
template<typename VarRef, typename Shift>
light_vector<ShiftVar<VarRef, Shift> >
ShiftVarRef(const light_vector<VarRef>& var_array, const Shift& shift)
{
  light_vector<ShiftVar<VarRef, Shift> > shift_array(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
    shift_array[i] = ShiftVarRef(var_array[i], shift);
  return shift_array;
}
#endif

template<typename VarRef, typename Shift, std::size_t i>
array<ShiftVar<VarRef, Shift>, i>
ShiftVarRef(const array<VarRef, i>& var_array, const Shift& shift)
{
  array<ShiftVar<VarRef, Shift>, i> shift_array;
  for(unsigned int l = 0; l < i; ++l)
    shift_array[l] = ShiftVarRef(var_array[l], shift);
  return shift_array;
}

