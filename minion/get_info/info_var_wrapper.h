#include "../constraints/constraint_abstract.h"

template<typename WrapType, VarType VAR_TYPE>
struct InfoRefType
{
  WrapType data;
  
  static const BOOL isBool = WrapType::isBool;
  static const BoundType isBoundConst = WrapType::isBoundConst;

  BOOL isBound() const
  { return data.isBound();}
  
  InfoRefType(const WrapType& _data) : data(_data)
  { VAR_INFO_ADDONE(VAR_TYPE, copy); }
  
  InfoRefType() 
  {VAR_INFO_ADDONE(VAR_TYPE, construct);}
  
  InfoRefType(const InfoRefType& b) : data(b.data)
  {VAR_INFO_ADDONE(VAR_TYPE, copy);}
  
  BOOL isAssigned() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, isAssigned);
    return data.isAssigned(); 
  }
  
  DomainInt getAssignedValue() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getAssignedValue);
    return data.getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i) const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, isAssignedValue);
    return data.isAssignedValue(i);
  }
  
  BOOL inDomain(DomainInt b) const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, inDomain);
    return data.inDomain( b); 
  }
  
  BOOL inDomain_noBoundCheck(DomainInt b) const
  {
    VAR_INFO_ADDONE(VAR_TYPE, inDomain_noBoundCheck);
    return data.inDomain_noBoundCheck(b);
  }
  
  
  DomainInt getMax() const
  {
    VAR_INFO_ADDONE(VAR_TYPE, getMax);
    return data.getMax(); 
  }
  
  DomainInt getMin() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getMin);
    return data.getMin(); 
  }

  DomainInt getInitialMax() const
  {
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMax);
    return data.getInitialMax(); 
  }
  
  DomainInt getInitialMin() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMin);
    return data.getInitialMin(); 
  }
  
  void setMax(DomainInt i, label l)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setMax);
    data.setMax(i, l); 
  }
  
  void setMin(DomainInt i, label l)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setMin);
    data.setMin(i, l); 
  }
  
  void uncheckedAssign(DomainInt b, label l)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, uncheckedAssign);
    data.uncheckedAssign(b, l); 
  }
  
  void propagateAssign(DomainInt b, label l)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, propagateAssign);
    data.propagateAssign(b, l); 
  }
  
  void decisionAssign(DomainInt b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, decisionAssign);
    data.decisionAssign(b); 
  }
  
  void removeFromDomain(DomainInt b, label l)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, RemoveFromDomain);
    data.removeFromDomain(b, l); 
  }
  
  void addTrigger(Trigger t, TrigType type)
  {
    VAR_INFO_ADDONE(VAR_TYPE, addTrigger);
    data.addTrigger( t, type); 
  }

  vector<AbstractConstraint*>* getConstraints()
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getConstraints);
    return data.getConstraints();
  }
  
  void addConstraint(AbstractConstraint* c)
  {
    VAR_INFO_ADDONE(VAR_TYPE, addConstraint);
    data.addConstraint(c);
  }

  DomainInt getBaseVal(DomainInt v) const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getBaseVal);
    return data.getBaseVal(v);
  }

  Var getBaseVar() const 
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getBaseVar);
    return data.getBaseVar(); 
  }

  void setDepth(DomainInt v, depth d)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setDepth);
    data.setDepth(v, d); 
  }

  depth getDepth(DomainInt v)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getDepth);
    return data.getDepth(v); 
  }

  void setLabel(DomainInt v, label l)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setLabel);
    data.setLabel(v, l); 
  }

  label getLabel(DomainInt v)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getLabel);
    return data.getLabel(v); 
  }

#ifdef WDEG
  int getBaseWdeg()
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getBaseWdeg);
    return data.getBaseWdeg(); 
  }

  void incWdeg()
  { 
    VAR_INFO_ADDONE(VAR_TYPE, incWdeg);
    data.incWdeg(); 
  }
#endif
  
  friend std::ostream& operator<<(std::ostream& o, const InfoRefType& ir)
  {
    return o << "InfoRef " << ir.data;
  }
 
  int getDomainChange(DomainDelta d)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getDomainChange);
    return d.XXX_get_domain_diff(); 
  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, addDynamicTrigger);
    data.addDynamicTrigger( t, type, pos); 
  }
#endif
};


