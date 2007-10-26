/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: variables.h 701 2007-10-09 14:12:05Z azumanga $
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

#ifdef MORE_SEARCH_INFO
#include "../get_info/info_var_wrapper.h"
#endif

#include "containers/booleanvariables.h"
#include "containers/intvar.h"
#include "containers/long_intvar.h"
#include "containers/intboundvar.h"
#include "containers/sparse_intboundvar.h"

class VariableContainer
{
  // Stop copying!
  VariableContainer(const VariableContainer&);
  void operator=(const VariableContainer&);

  BoundVarContainer<> boundvarContainer; 
  BooleanContainer booleanContainer;
  LRVCon rangevarContainer;
  BigRangeCon bigRangevarContainer;
  SparseBoundVarContainer<> sparseBoundvarContainer;
public:

    VariableContainer(StateObj* _stateObj) :
    boundvarContainer(_stateObj),
    booleanContainer(_stateObj),
    rangevarContainer(_stateObj),
    bigRangevarContainer(_stateObj),
    sparseBoundvarContainer(_stateObj)
  {}
  
  BoundVarContainer<>& getBoundvarContainer() { return boundvarContainer; }
  BooleanContainer& getBooleanContainer() { return booleanContainer; }
  LRVCon& getRangevarContainer() { return rangevarContainer; }
  BigRangeCon& getBigRangevarContainer() { return bigRangevarContainer; }
  SparseBoundVarContainer<>& getSparseBoundvarContainer() { return sparseBoundvarContainer; }
};


#ifndef MANY_VAR_CONTAINERS

template<typename DomType>
struct BoundVarContainer<DomType>& BoundVarRef_internal<DomType>::getCon() const
{ return getVars(NULL).getBoundvarContainer(); }

template<int var_min, typename d_type>
inline RangeVarContainer<var_min, d_type>& RangeVarRef_internal_template<var_min, d_type>::getCon() const
{ return getVars(NULL).getRangevarContainer(); }

inline BooleanContainer& BoolVarRef_internal::getCon() const
{ return getVars(NULL).getBooleanContainer(); }

template<typename DomType>
inline SparseBoundVarContainer<DomType>& SparseBoundVarRef_internal<DomType>::getCon() const
{ return getVars(NULL).getSparseBoundvarContainer(); }

template<typename d_type>
inline BigRangeVarContainer<d_type>& BigRangeVarRef_internal_template<d_type>::getCon() const
{ return getVars(NULL).getBigRangevarContainer(); }
#endif

struct SmallDiscreteCheck
{
  StateObj* stateObj;
  SmallDiscreteCheck(StateObj* _stateObj) : stateObj(_stateObj)
  {}

  template<typename T>
  bool operator()(const T& lower, const T& upper) const
  { return getVars(stateObj).getRangevarContainer().valid_range(lower, upper); }
};

#include "mappings/variable_neg.h"
#include "mappings/variable_switch_neg.h"
#include "mappings/variable_stretch.h"
#include "mappings/variable_constant.h"
#include "mappings/variable_not.h"
#include "mappings/variable_shift.h"
#include "iterators.h"


