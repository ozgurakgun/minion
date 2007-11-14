/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: variables.h 764 2007-11-06 18:18:36Z azumanga $
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


#ifdef MANY_VAR_CONTAINERS
#define GET_CONTAINER() data.getCon()
#define GET_LOCAL_CON() getCon()
#else
#define GET_CONTAINER() InternalRefType::getCon_Static()
#define GET_LOCAL_CON() getCon_Static()
#endif

#include "VarRefType.h"

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
public:
  BoundVarContainer<> boundvarContainer; 
  BooleanContainer booleanContainer;
  LRVCon rangevarContainer;
  BigRangeCon bigRangevarContainer;
  SparseBoundVarContainer<> sparseBoundvarContainer;


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
#include "mappings/TrivialBoundVar.h"
#include "mappings/variable_not.h"
#include "mappings/variable_shift.h"
#include "iterators.h"


