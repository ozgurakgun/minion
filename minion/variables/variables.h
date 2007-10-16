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

/*
struct GetBoundVarContainer
{
  static BoundVarContainer<>& con() 
  {}//{ return stateObj->varCon().getBoundvarContainer(); }
  static string name()
  { return "Bound"; }
};

struct GetBooleanContainer
{ 
  static BooleanContainer& con() 
  {}//{ return stateObj->varCon().getBooleanContainer(); } 
  static string name()
  { return "Bool:"; }
};

struct GetRangeVarContainer
{
  static LRVCon& con() 
  {}//{ return stateObj->varCon().getRangevarContainer(); }
  static string name() { return "RangeVar:"; }
};

struct GetBigRangeVarContainer
{
  static BigRangeCon& con() 
  {}//{ return stateObj->varCon().getBigRangevarContainer(); }
  static string name() { return "BigRangeVar"; }
};


struct GetSparseBoundVarContainer
{
  static SparseBoundVarContainer<>& con() 
  {}//{ return stateObj->varCon().getSparseBoundvarContainer(); }
  
  static string name()
  { return "SparseBound:"; }
};
*/

struct SmallDiscreteCheck
{
  StateObj* stateObj;
  SmallDiscreteCheck(StateObj* _stateObj) : stateObj(_stateObj)
  {}

  template<typename T>
  bool operator()(const T& lower, const T& upper) const
  { return stateObj->varCon().getRangevarContainer().valid_range(lower, upper); }
};

#include "mappings/variable_neg.h"
#include "mappings/variable_switch_neg.h"
#include "mappings/variable_stretch.h"
#include "mappings/variable_constant.h"
#include "mappings/variable_not.h"
#include "mappings/variable_shift.h"
#include "iterators.h"


