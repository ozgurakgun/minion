/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
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

/** @help constraints;occurrence Description
The constraint

   occurrence(vec, elem, count)

ensures that there are count occurrences of the value elem in the
vector vec.
*/

/** @help constraints;occurrence Notes
elem must be a constant, not a variable.
*/

/** @help constraints;occurrence References
help constraints occurrenceleq
help constraints occurrencegeq
*/

/** @help constraints;occurrenceleq Description
The constraint

   occurrenceleq(vec, elem, count)

ensures that there are AT MOST count occurrences of the value elem in
the vector vec.
*/

/** @help constraints;occurrenceleq Notes
elem and count must be constants
*/

/** @help constraints;occurrenceleq References
help constraints occurrence
help constraints occurrencegeq
*/

/** @help constraints;occurrencegeq Description
The constraint

   occurrencegeq(vec, elem, count)

ensures that there are AT LEAST count occurrences of the value elem in
the vector vec.
*/

/** @help constraints;occurrencegeq Notes
elem and count must be constants
*/

/** @help constraints;occurrencegeq References
help constraints occurrence
help constraints occurrenceleq
*/

#ifndef CONSTRAINT_OCCURRENCE_H
#define CONSTRAINT_OCCURRENCE_H

// Negated occurrence; used in reverse_constraint for OccurrenceEqualConstraint
template<typename VarArray, typename Val, typename ValCount>
struct NotOccurrenceEqualConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "NotOccurrenceEqual"; }

  typedef typename VarArray::value_type VarRef;

  ReversibleInt occurrences_count;
  ReversibleInt not_occurrences_count;
  VarArray var_array;

  ValCount val_count;
  Val value;

  NotOccurrenceEqualConstraint(StateObj* _stateObj, const VarArray& _var_array, const Val& _value, const ValCount& _val_count) :
    AbstractConstraint(_stateObj), occurrences_count(_stateObj), not_occurrences_count(_stateObj),
    var_array(_var_array), val_count(_val_count), value(_value), trigger1index(-1), trigger2index(-1)
  { }

  // Put two assignment triggers on the vector, and one on val_count.
  // When all vars in X are assigned, remove count(X=v) from val_count

  // When val_count is assigned and all but one of the vector are assigned,
  // consider the remaining one in the vector, and either fix it to v or
  // remove v from its domain to avoid the value of val_count.

  int dynamic_trigger_count()
  { // two moving assignment triggers.
    return 2;
  }

  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    t.push_back(make_trigger(val_count, Trigger(this, -1), Assigned));
    return t;
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size() + 1);
    DomainInt count = 0;
    for(int i = 0; i < v_size - 1; ++i)
      count += (*(v + i) == value);
    return count != *(v + v_size - 1);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size() + 1);
    for(unsigned i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    vars.push_back(AnyVarRef(val_count));
    return vars;
  }


  virtual BOOL propagate(int z, DomainDelta)
  {
      // val_count has been assigned.
      D_ASSERT(z==-1);
      DynamicTrigger* dt = dynamic_trigger_start();
      if(trigger1index==-1 || var_array[trigger1index].isAssigned())
      {
          trigger1index=watch_unassigned_in_vector(-1, trigger1index, dt);
          if(trigger1index==-1)
          {
              return valcount_assigned();
          }
      }
      if(trigger2index==-1 || var_array[trigger2index].isAssigned())
      {
          trigger2index=watch_unassigned_in_vector(trigger1index, trigger2index, dt+1);
          if(trigger2index==-1)
          {
              return valcount_assigned();
          }
      }
      return true;
  }

  virtual BOOL propagate(DynamicTrigger* trig)
  {
      DynamicTrigger* dt = dynamic_trigger_start();
      if(trig==dt || trigger1index==-1)
      {
          if(val_count.isAssigned())
          {
              // make sure both triggers are in place.
              trigger1index=watch_unassigned_in_vector(-1, trigger1index, dt);
              if(trigger1index==-1)
              {
                  return valcount_assigned();
              }
              if(trigger2index==-1 || var_array[trigger2index].isAssigned())
              {
                  trigger2index=watch_unassigned_in_vector(trigger1index, trigger2index, dt+1);
                  if(trigger2index==-1)
                  {
                      return valcount_assigned();
                  }
              }
          }
          else
          {
              trigger1index=watch_unassigned_in_vector(-1, trigger1index, dt);
              if(trigger1index==-1)
              {
                  return vector_assigned();
              }
          }
          return true;
      }
      D_ASSERT(trig==dt+1);
      if(!val_count.isAssigned())
      {   // don't need two triggers.
          releaseTrigger(stateObj, trig);
          trigger2index=-1;
          return true;
      }

      if(var_array[trigger1index].isAssigned())
      {
          // just wait for the other trigger, then both triggers will be repositioned. lazy coding!
          return true;
      }

      trigger2index=watch_unassigned_in_vector(trigger1index, trigger2index, dt+1);
      if(trigger2index==-1)
      {
          return valcount_assigned();
      }
      return true;
  }

  // unfinished new stuff starts here.

  int watch_unassigned_in_vector(int avoidindex, int oldsupport, DynamicTrigger* dt)
  {
      // move dt to an index other than avoidindex, or return -1.
      int newsupport=oldsupport+1;
      for( ; newsupport<var_array.size(); newsupport++)
      {
          if(newsupport!=avoidindex)
          {
              if(!var_array[newsupport].isAssigned())
              {
                  var_array[newsupport].addDynamicTrigger(dt, Assigned);
                  return newsupport;
              }
          }
      }

      for(newsupport=0; newsupport<=oldsupport; newsupport++)
      {
          if(newsupport!=avoidindex)
          {
              if(!var_array[newsupport].isAssigned())
              {
                  var_array[newsupport].addDynamicTrigger(dt, Assigned);
                  return newsupport;
              }
          }
      }
      return -1;
  }

  int trigger1index;
  int trigger2index;

  BOOL vector_assigned()
  {
      // count occurrences of val
      int occ=0;
      for(int i=0; i<var_array.size(); i++)
      {
          if(var_array[i].getAssignedValue()==value)
              occ++;
      }
      if(val_count.inDomain(occ))
      {
          if(!val_count.removeFromDomain(occ))
            return false;
      }
      return true;
  }

  BOOL valcount_assigned()
  {
      // valcount, and all but one (or all) of the vector, are assigned.
      // count occurrences of val
      int occ=0;
      int unassigned=-1;
      D_ASSERT(val_count.isAssigned());
      for(int i=0; i<var_array.size(); i++)
      {
          if(var_array[i].isAssigned())
          {
              if(var_array[i].getAssignedValue()==value)
              {
                  occ++;
              }
          }
          else
          {
              D_ASSERT(unassigned==-1);
              unassigned=i;
          }
      }

      // and the rest.
      if(unassigned==-1)
      {
          // just check, everything is assigned.
          if(occ==val_count.getAssignedValue())
          {
              return false;
          }
      }
      else
      {
          if(occ==val_count.getAssignedValue())
          { // need another occurrence of the value
              return var_array[unassigned].propagateAssign(value);
          }
          else if(occ+1 == val_count.getAssignedValue())
          { // not allowed to have another value.
              return var_array[unassigned].removeFromDomain(value);
          }
      }
      return true;
  }

  virtual BOOL full_propagate()
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    trigger1index=watch_unassigned_in_vector(-1, -1, dt);
    if(trigger1index==-1)
    {
        return vector_assigned();
    }

    if(val_count.isAssigned())
    {
        // watch a second place in the vector.
        trigger2index=watch_unassigned_in_vector(trigger1index, trigger1index, dt+1);
        if(trigger2index==-1)
        {
            return valcount_assigned();
        }
    }
    return true;
  }

   // Getting a satisfying assignment here is too hard, we don't want to have to
   // build a matching.
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    MAKE_STACK_BOX(c, DomainInt, var_array.size() + 1);

    for(int i = 0; i < var_array.size(); ++i)
    {
      if(!var_array[i].isAssigned())
      {
        assignment.push_back(make_pair(i, var_array[i].getMin()));
        assignment.push_back(make_pair(i, var_array[i].getMax()));
        return true;
      }
      else
        c.push_back(var_array[i].getAssignedValue());
    }

    if(!val_count.isAssigned())
    {
      assignment.push_back(make_pair(var_array.size(), val_count.getMin()));
      assignment.push_back(make_pair(var_array.size(), val_count.getMax()));
      return true;
    }
    else
      c.push_back(val_count.getAssignedValue());

    if(check_assignment(c.begin(), c.size()))
    {  // Put the complete assignment in the box.
      for(int i = 0; i < var_array.size() + 1; ++i)
        assignment.push_back(make_pair(i, c[i]));
      return true;
    }
    return false;
  }
};

template<typename VarArray, typename Val>
struct ConstantOccurrenceEqualConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "OccurrenceLeq/Geq"; }

  typedef typename VarArray::value_type VarRef;

  ReversibleInt occurrences_count;
  ReversibleInt not_occurrences_count;
  VarArray var_array;

  int val_count_min;
  int val_count_max;
  Val value;

  ConstantOccurrenceEqualConstraint(StateObj* _stateObj, const VarArray& _var_array, const Val& _value,
                            int _val_count_min, int _val_count_max) :
    AbstractConstraint(_stateObj), occurrences_count(_stateObj), not_occurrences_count(_stateObj),
    var_array(_var_array), val_count_min(_val_count_min), val_count_max(_val_count_max), value(_value)
  { }

  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    occurrences_count = 0;
    not_occurrences_count = 0;
    for(unsigned int i=0; i < var_array.size(); ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    return t;
  }

  BOOL occurrence_limit_reached()
  {
    D_ASSERT(val_count_max <= occurrences_count);
    int occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
      {
        if(it->getAssignedValue() == value)
        ++occs;
      }
      else
      {
        if(!it->removeFromDomain(value))
            return false;
      }
    }
    if(val_count_max < occs)
      return false;
    return true;
  }

  BOOL not_occurrence_limit_reached()
  {
    D_ASSERT(not_occurrences_count >= static_cast<int>(var_array.size()) - val_count_min);
    int occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for( typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
      {
      if(it->getAssignedValue() != value)
        ++occs;
      }
      else {
          if(!it->propagateAssign(value))
            return false;
      }
    }
    if(val_count_min > static_cast<int>(var_array.size()) - occs)
      return false;
    return true;
  }

  virtual BOOL propagate(int i, DomainDelta)
  {
      PROP_INFO_ADDONE(OccEqual);
    D_ASSERT(i >= 0);

    if( var_array[i].getAssignedValue() == value )
    {
      ++occurrences_count;
      if(val_count_max < occurrences_count)
        return false;
      if(occurrences_count == val_count_max)
        return occurrence_limit_reached();
    }
    else
    {
      ++not_occurrences_count;
      if(val_count_min > static_cast<int>(var_array.size()) - not_occurrences_count)
        return false;
      if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count_min )
        return not_occurrence_limit_reached();
    }
    return true;
  }

  void setup_counters()
  {
    int occs = 0;
      int not_occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
        {
        if(it->getAssignedValue() == value)
          ++occs;
          else
          ++not_occs;
      }
    }
    occurrences_count = occs;
      not_occurrences_count = not_occs;
  }

  virtual BOOL full_propagate()
  {
    if(val_count_max < 0 || val_count_min > (int)var_array.size())
      return false;
    setup_counters();

    if(val_count_max < occurrences_count)
      return false;

    if(val_count_min > static_cast<int>(var_array.size()) - not_occurrences_count)
      return false;

    if(occurrences_count == val_count_max)
      if(!occurrence_limit_reached())
        return false;
    if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count_min)
      if(!not_occurrence_limit_reached())
        return false;
    return true;
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size());
    DomainInt count = 0;
    for(int i = 0; i < v_size; ++i)
      count += (*(v + i) == value);
    return (count >= val_count_min) && (count <= val_count_max);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
      vars.reserve(var_array.size());
      for(unsigned i = 0; i < var_array.size(); ++i)
        vars.push_back(AnyVarRef(var_array[i]));
      return vars;
  }

   // Getting a satisfying assignment here is too hard, we don't want to have to
   // build a matching.
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    MAKE_STACK_BOX(c, DomainInt, var_array.size());

    for(int i = 0; i < var_array.size(); ++i)
    {
      if(!var_array[i].isAssigned())
      {
        assignment.push_back(make_pair(i, var_array[i].getMin()));
        assignment.push_back(make_pair(i, var_array[i].getMax()));
        return true;
      }
      else
        c.push_back(var_array[i].getAssignedValue());
    }

    if(check_assignment(c.begin(), c.size()))
    {  // Put the complete assignment in the box.
      for(int i = 0; i < var_array.size(); ++i)
        assignment.push_back(make_pair(i, c[i]));
      return true;
    }
    return false;
  }

  AbstractConstraint* reverse_constraint()
  {
      // This constraint actually constrains the occurrences of value to an an interval
      // [val_count_min, val_count_max]. But it's apparently only used for less-than
      // and greater-than. So identify the less-than case and make a greater-than, etc.
      if(val_count_min==0)
      {
          return new ConstantOccurrenceEqualConstraint<VarArray, Val>(stateObj, var_array, value, val_count_max+1, var_array.size());
      }
      if(val_count_max==var_array.size())
      {
          return new ConstantOccurrenceEqualConstraint<VarArray, Val>(stateObj, var_array, value, 0, val_count_min-1);
      }
      FAIL_EXIT("Unable to negate an occurrence-interval constraint, sorry.");
      return NULL;
  }
};

template<typename VarArray, typename Val, typename ValCount>
struct OccurrenceEqualConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "OccurrenceEqual"; }

  typedef typename VarArray::value_type VarRef;

  ReversibleInt occurrences_count;
  ReversibleInt not_occurrences_count;
  VarArray var_array;

  ValCount val_count;
  Val value;

  OccurrenceEqualConstraint(StateObj* _stateObj, const VarArray& _var_array, const Val& _value, const ValCount& _val_count) :
    AbstractConstraint(_stateObj), occurrences_count(_stateObj), not_occurrences_count(_stateObj),
    var_array(_var_array), val_count(_val_count), value(_value)
  { }

  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    occurrences_count = 0;
    not_occurrences_count = 0;
    for(unsigned int i=0; i < var_array.size(); ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    t.push_back(make_trigger(val_count, Trigger(this, -1), UpperBound));
    t.push_back(make_trigger(val_count, Trigger(this, -2), LowerBound));
    return t;
  }

  BOOL occurrence_limit_reached()
  {
    D_ASSERT(val_count.getMax() <= occurrences_count);
    int occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
      {
        if(it->getAssignedValue() == value)
        ++occs;
      }
      else
      {
        if(!it->removeFromDomain(value))
            return false;
      }
    }
    return val_count.setMin(occs);
  }

  BOOL not_occurrence_limit_reached()
  {
    D_ASSERT(not_occurrences_count >= static_cast<int>(var_array.size()) - val_count.getMin());
    int occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for( typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
      {
      if(it->getAssignedValue() != value)
        ++occs;
      }
      else {
          if(!it->propagateAssign(value))
            return false;
      }
    }
    return val_count.setMax(static_cast<int>(var_array.size()) - occs);
  }

  virtual BOOL propagate(int i, DomainDelta)
  {
    PROP_INFO_ADDONE(OccEqual);
    if(i < 0)
    { // val_count changed
      if(occurrences_count == val_count.getMax())
        if(!occurrence_limit_reached())
            return false;
      if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count.getMin() )
        if(!not_occurrence_limit_reached())
            return false;
      return true;
    }

    if( var_array[i].getAssignedValue() == value )
    {
      ++occurrences_count;
      if(!val_count.setMin(occurrences_count))
        return false;
      if(occurrences_count == val_count.getMax())
        return occurrence_limit_reached();
    }
    else
    {
      ++not_occurrences_count;
      if(!val_count.setMax(static_cast<int>(var_array.size()) -
      not_occurrences_count))
        return false;
      if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count.getMin() )
        return not_occurrence_limit_reached();
    }
    return true;
  }

  void setup_counters()
  {
    int occs = 0;
    int not_occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
      {
        if(it->getAssignedValue() == value)
          ++occs;
        else
          ++not_occs;
      }
    }
    occurrences_count = occs;
    not_occurrences_count = not_occs;
  }

  virtual BOOL full_propagate()
  {
    if(!val_count.setMin(0))
        return false;
    if(!val_count.setMax(var_array.size()))
        return false;
    setup_counters();
    if(!val_count.setMin(occurrences_count))
        return false;
    if(!val_count.setMax(static_cast<int>(var_array.size()) -
    not_occurrences_count))
        return false;

    if(occurrences_count == val_count.getMax())
     if(! occurrence_limit_reached())
        return false;
    if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count.getMin() )
      if(!not_occurrence_limit_reached())
        return false;
    return true;
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size() + 1);
    DomainInt count = 0;
    for(int i = 0; i < v_size - 1; ++i)
      count += (*(v + i) == value);
    return count == *(v + v_size - 1);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size() + 1);
    for(unsigned i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    vars.push_back(AnyVarRef(val_count));
    return vars;
  }

   // Getting a satisfying assignment here is too hard, we don't want to have to
   // build a matching.
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    MAKE_STACK_BOX(c, DomainInt, var_array.size() + 1);

    for(int i = 0; i < var_array.size(); ++i)
    {
      if(!var_array[i].isAssigned())
      {
        assignment.push_back(make_pair(i, var_array[i].getMin()));
        assignment.push_back(make_pair(i, var_array[i].getMax()));
        return true;
      }
      else
        c.push_back(var_array[i].getAssignedValue());
    }

    if(!val_count.isAssigned())
    {
      assignment.push_back(make_pair(var_array.size(), val_count.getMin()));
      assignment.push_back(make_pair(var_array.size(), val_count.getMax()));
      return true;
    }
    else
      c.push_back(val_count.getAssignedValue());


    if(check_assignment(c.begin(), c.size()))
    {  // Put the complete assignment in the box.
      for(int i = 0; i < var_array.size() + 1; ++i)
        assignment.push_back(make_pair(i, c[i]));
      return true;
    }
    return false;
  }

  AbstractConstraint* reverse_constraint()
  {
      return new NotOccurrenceEqualConstraint<VarArray, Val, ValCount>(stateObj, var_array, value, val_count);
      /*vector<AnyVarRef> v;
      for(int i=0; i<var_array.size(); i++)
      {
          v.push_back((AnyVarRef) var_array[i]);
      }
      v.push_back(val_count);
      return new ForwardCheckingConstraint<vector<AnyVarRef>, FCNotOccurrence>(stateObj, v, value);*/
  }
};



template<typename VarArray, typename Val, typename ValCount>
AbstractConstraint*
OccEqualCon(StateObj* stateObj, const VarArray& _var_array,  const Val& _value, const ValCount& _val_count)
{
  return
  (new OccurrenceEqualConstraint<VarArray,Val, ValCount>(stateObj, _var_array,  _value, _val_count));
}

template<typename VarArray, typename Val>
AbstractConstraint*
ConstantOccEqualCon(StateObj* stateObj, const VarArray& _var_array,  const Val& _value, int _val_count_min, int _val_count_max)
{
  return
  (new ConstantOccurrenceEqualConstraint<VarArray,Val>(stateObj, _var_array,  _value, _val_count_min, _val_count_max));
}
#endif
