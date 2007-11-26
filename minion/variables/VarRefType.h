/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: VarRefType.h 745 2007-11-02 13:37:26Z azumanga $
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

#ifndef VARREFTYPE_H
#define VARREFTYPE_H

// The follow three types are designed to allow turning a variable type which
// must be fed to a container, into a stand-alone class which is ready to be
// used as a variable.
template<typename InternalRefType>
struct VarRefType
{
  static const BOOL isBool = InternalRefType::isBool;
  static const BoundType isBoundConst = InternalRefType::isBoundConst;

  InternalRefType data;
  
  BOOL isBound()
  { return data.isBound();}
  
  VarRefType(const InternalRefType& _data) : data(_data)
  {}
  
  VarRefType() : data()
  {}
  
  VarRefType(const VarRefType& b) : data(b.data)
  {}
  
  BOOL isAssigned()
  { return GET_CONTAINER().isAssigned(data); }
  
  DomainInt getAssignedValue()
  { return GET_CONTAINER().getAssignedValue(data); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    return GET_CONTAINER().isAssigned(data) &&
    GET_CONTAINER().getAssignedValue(data) == i;
  }
  
  BOOL inDomain(DomainInt b)
  { return GET_CONTAINER().inDomain(data, b); }

  BOOL inDomain_noBoundCheck(DomainInt b)
  { return GET_CONTAINER().inDomain_noBoundCheck(data, b); }
  
  DomainInt getMax()
  { return GET_CONTAINER().getMax(data); }
  
  DomainInt getMin()
  { return GET_CONTAINER().getMin(data); }

  unsigned getDepth()
  { return GET_CONTAINER().getDepth(data); }
  
  DynamicConstraint* getAntecedent()
  { return GET_CONTAINER().getAntecedent(data); }

  int getId()
  { return GET_CONTAINER().getId(data); }

  DomainInt getInitialMax() const
  { return GET_CONTAINER().getInitialMax(data); }
  
  DomainInt getInitialMin() const
  { return GET_CONTAINER().getInitialMin(data); }
  
  void setMax(DomainInt i)
  { GET_CONTAINER().setMax(data,i); }
  
  void setMin(DomainInt i)
  { GET_CONTAINER().setMin(data,i); }

  void setDepth(unsigned d)
  { GET_CONTAINER().setDepth(data, d); }

  void setAntecedent(DynamicConstraint* d)
  { GET_CONTAINER().setAntecedent(data, d); }
  
  void uncheckedAssign(DomainInt b)
  { GET_CONTAINER().uncheckedAssign(data, b); }
  
  void propagateAssign(DomainInt b)
  { GET_CONTAINER().propagateAssign(data, b); }
  
  void removeFromDomain(DomainInt b)
  { GET_CONTAINER().removeFromDomain(data, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { GET_CONTAINER().addTrigger(data, t, type); }

  friend std::ostream& operator<<(std::ostream& o, const VarRefType& v)
  { return o << InternalRefType::name() << v.data.var_num; }
    
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  GET_CONTAINER().addDynamicTrigger(data, t, type, pos); }
#endif
};


template<typename GetContainer, typename InternalRefType>
struct QuickVarRefType
{
  static const BOOL isBool = InternalRefType::isBool;
  static const BoundType isBoundConst = InternalRefType::isBoundConst;
  InternalRefType data;
  
  BOOL isBound()
  { return data.isBound();}
  
  QuickVarRefType(const InternalRefType& _data) : data(_data)
  {}
  
  QuickVarRefType() : data()
  {}
  
  QuickVarRefType(const QuickVarRefType& b) : data(b.data)
  {}
  
  BOOL isAssigned()
  { return data.isAssigned(); }
  
  DomainInt getAssignedValue()
  { return data.getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    return data.isAssigned() &&
    data.getAssignedValue() == i;
  }
  BOOL inDomain(DomainInt b)
  { return data.inDomain(b); }
  
  BOOL inDomain_noBoundCheck(DomainInt b)
  { return data.inDomain_noBoundCheck(b); }

  DomainInt getMax()
  { return data.getMax(); }
  
  DomainInt getMin()
  { return data.getMin(); }

  unsigned getDepth()
  { return data.getDepth(); }

  DynamicConstraint* getAntecedent()
  { return data.getAntecedent(); }

  int getId()
  { return data.getId(); }

  DomainInt getInitialMax() const
  { return data.getInitialMax(); }
  
  DomainInt getInitialMin() const
  { return data.getInitialMin(); }
  
  void setMax(DomainInt i)
  { GET_CONTAINER().setMax(data,i); }
  
  void setMin(DomainInt i)
  { GET_CONTAINER().setMin(data,i); }
  
  void setDepth(unsigned d)
  { GET_CONTAINER().setDepth(data, d); }

  void setAntecedent(DynamicConstraint* dc)
  { GET_CONTAINER().setAntecedent(data, dc); }
  
  void uncheckedAssign(DomainInt b)
  { GET_CONTAINER().uncheckedAssign(data, b); }
  
  void propagateAssign(DomainInt b)
  { GET_CONTAINER().propagateAssign(data, b); }
  
  void removeFromDomain(DomainInt b)
  { GET_CONTAINER().removeFromDomain(data, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { GET_CONTAINER().addTrigger(data, t, type); }

  friend std::ostream& operator<<(std::ostream& o, const QuickVarRefType& b)
  { return o << "Bool:" << b.data; }
  
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  GET_CONTAINER().addDynamicTrigger(data, t, type, pos); }
#endif
};


template<typename InternalRefType>
struct CompleteVarRefType
{
  InternalRefType data;
  CompleteVarRefType(const InternalRefType& _data) : data(_data)
  {}
  
  CompleteVarRefType() 
  {}
  
  CompleteVarRefType(const CompleteVarRefType& b) : data(b.data)
  {}
  
  BOOL isAssigned()
  { return (data.getCon()).isAssigned(data); }
  
  DomainInt getAssignedValue()
  { return (data.getCon()).getAssignedValue(data); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    return (data.getCon()).isAssigned(data) &&
    (data.getCon()).getAssignedValue(data) == i;
  }
  BOOL inDomain(DomainInt b)
  { return (data.getCon()).inDomain(data, b); }
  
  DomainInt getMax()
  { return (data.getCon()).getMax(data); }
  
  DomainInt getMin()
  { return (data.getCon()).getMin(data); }

  unsigned getDepth()
  { return (data.getCon()).getDepth(data); }

  DynamicConstraint* getAntecedent()
  { return (data.getCon()).getAntecedent(data); }

  int getId()
  { return (data.getCon()).getId(data); }
  
  DomainInt getInitialMax() const
  { return (data.getCon()).getInitialMax(data); }
  
  DomainInt getInitialMin() const
  { return (data.getCon()).getInitialMin(data); }
  
  void setMax(DomainInt i)
  { (data.getCon()).setMax(data,i); }
  
  void setMin(DomainInt i)
  { (data.getCon()).setMin(data,i); }

  void setDepth(unsigned d)
  { (data.getCon()).setDepth(data, d); }

  void setAntecedent(DynamicConstraint* dc)
  { (data.getCon()).setAntecedent(data, dc); }
  
  void uncheckedAssign(DomainInt b)
  { (data.getCon()).uncheckedAssign(data, b); }
  
  void propagateAssign(DomainInt b)
  { (data.getCon()).propagateAssign(data, b); }
  
  void removeFromDomain(DomainInt b)
  { (data.getCon()).removeFromDomain(data, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { (data.getCon()).addTrigger(data, t, type); }
  
  friend std::ostream& operator<<(std::ostream& o, const CompleteVarRefType& cv)
  { return o << "CompleteCon:" << cv.data.var_num; }
  
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  (data.getCon()).addDynamicTrigger(data, t, type, pos); }
#endif
};



#endif

