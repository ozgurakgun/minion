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

/**
 * @brief Nots a variable reference.
 *
 * Takes a variable, and returns a new 'psuedo-variable', which is the same as the not of the
 * original. This new variable takes up no extra space of any kind after compilation
 * is performed
 */
template<typename VarRef>
struct VarNot
{
  static const BOOL isBool = true;
  static const BoundType isBoundConst = VarRef::isBoundConst;
  VarRef data;
  
  BOOL isBound()  const
  { return data.isBound();}
  
  VarNot(const VarRef& _data) : data(_data)
  { D_ASSERT(VarRef::isBool); }
  
  VarNot() : data()
  {}
  
  VarNot(const VarNot& b) : data(b.data)
  {}
  
  // There is a good reason this is like this. It is because the 'neg' of an BOOL var
  // might be used in arithmetic. This is an extension to all of the integers which
  // swaps 0 and 1.
  DomainInt swap(DomainInt i) const
  { return -i+1; }

  BOOL isAssigned() const
  { return data.isAssigned(); }
  
  DomainInt getAssignedValue() const
  { return swap(data.getAssignedValue()); }
  
  BOOL isAssignedValue(DomainInt i) const
  { 
    return data.isAssigned() &&
    swap(data.getAssignedValue()) == i;
  }
  
  BOOL inDomain(DomainInt b) const
  { return data.inDomain(swap(b)); }

  BOOL inDomain_noBoundCheck(DomainInt b) const
  { return data.inDomain(swap(b)); }
  
  DomainInt getMax() const
  { return swap(data.getMin()); }
  
  DomainInt getMin() const
  { return swap(data.getMax()); }

  DomainInt getInitialMax() const
  { return swap(data.getInitialMin()); }
  
  DomainInt getInitialMin() const
  { return swap(data.getInitialMax()); }
  
  void setMax(DomainInt i, label l)
  { data.setMin(swap(i), l); }
  
  void setMin(DomainInt i, label l)
  { data.setMax(swap(i), l); }
  
  void uncheckedAssign(DomainInt b, label l)
  { data.uncheckedAssign(swap(b), l); }
  
  void propagateAssign(DomainInt b, label l)
  { data.propagateAssign(swap(b), l); }
  
  void decisionAssign(DomainInt b)
  { data.decisionAssign(swap(b)); }
  
  void removeFromDomain(DomainInt b, label l)
  { data.removeFromDomain(swap(b), l); }
 
  void addTrigger(Trigger t, TrigType type)
  { 
    switch(type)
	{
	  case UpperBound:
		data.addTrigger(t, LowerBound);
		break;
	  case LowerBound:
		data.addTrigger(t, UpperBound);
		break;
	  case Assigned:
	  case DomainChanged:
	    data.addTrigger(t, type);
      break;
	  default:
      D_FATAL_ERROR("Invalid trigger in 'not' mapper");
	}
  }

  friend std::ostream& operator<<(std::ostream& o, const VarNot& n)
  { return o << "Not " << n.data; }
  
  int getDomainChange(DomainDelta d)
  { return data.getDomainChange(d); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  data.addDynamicTrigger(t, type, swap(pos)); }
#endif

  vector<AbstractConstraint*>* getConstraints()
  { return data.getConstraints(); }

  void addConstraint(AbstractConstraint* c)
  { data.addConstraint(c); }

  DomainInt getBaseVal(DomainInt v) const
  { return data.getBaseVal(swap(v)); }

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

template<typename T>
struct NotType
{ typedef VarNot<T> type; };

template<typename T>
struct NotType<vector<T> >
{ typedef vector<VarNot<T> > type; };

#ifdef LIGHT_VECTOR
template<typename T>
struct NotType<light_vector<T> >
{ typedef light_vector<VarNot<T> > type; };
#endif

template<typename T, std::size_t i>
struct NotType<array<T, i> >
{ typedef array<VarNot<T>, i> type; };


template<typename VRef>
typename NotType<VRef>::type
VarNotRef(const VRef& var_ref)
{ return VarNot<VRef>(var_ref); }

template<typename VarRef>
vector<VarNot<VarRef> >
VarNotRef(const vector<VarRef>& var_array)
{
  vector<VarNot<VarRef> > Not_array;
  Not_array.reserve(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
    Not_array.push_back(VarNotRef(var_array[i]));
  return Not_array;
}

#ifdef LIGHT_VECTOR
template<typename VarRef>
light_vector<VarNot<VarRef> >
VarNotRef(const light_vector<VarRef>& var_array)
{
  light_vector<VarNot<VarRef> > Not_array(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
    Not_array[i] = VarNotRef(var_array[i]);
  return Not_array;
}
#endif

template<typename VarRef, std::size_t i>
array<VarNot<VarRef>, i>
VarNotRef(const array<VarRef, i>& var_array)
{
  array<VarNot<VarRef>, i> Not_array;
  for(unsigned int l = 0; l < i; ++l)
    Not_array[l] = VarNotRef(var_array[l]);
  return Not_array;
}

