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
//#include "containers/intvar.h"
#include "containers/long_intvar.h"
#include "containers/intboundvar.h"
#include "containers/sparse_intboundvar.h"

class VariableContainer
{
  // Stop copying!
  VariableContainer(const VariableContainer&);
  void operator=(const VariableContainer&);
public:
  BoundVarContainer<> boundVarContainer;
  BoolVarContainer boolVarContainer;
  BigRangeVarContainer<BitContainerType> bigRangeVarContainer;
  SparseBoundVarContainer<> sparseBoundVarContainer;


  VariableContainer(StateObj* _stateObj) :
    boundVarContainer(_stateObj),
    boolVarContainer(_stateObj),
    bigRangeVarContainer(_stateObj),
    sparseBoundVarContainer(_stateObj)
  {}
  
  inline void lock() {
    boundVarContainer.lock();
    boolVarContainer.lock();
    bigRangeVarContainer.lock();
    sparseBoundVarContainer.lock();
  }
};

//NB use of comma operator to produce one statement from the macro!
#define ARE_EQUAL(stateObj, v1, v2) (getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2).getMin() == 1)
#define ARE_DISEQUAL(stateObj, v1, v2) (getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2).getMax() == 0)
#define ARE_DISEQ_UNKNOWN(stateObj, v1, v2) (!getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2).isAssigned())
#define ARE_DISEQ_OR_EQ(stateObj, v1, v2) (getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2).isAssigned())
#define SET_EQUAL(stateObj, v1, v2) \
do { \
D_ASSERT(!getState(stateObj).isFailed()); \
getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2).propagateAssign(1); \
} while(false)
#define SET_DISEQUAL(stateObj, v1, v2) \
do { \
D_ASSERT(!getState(stateObj).isFailed()); \
getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2).propagateAssign(0); \
} while(false)
#define PUSH_EQUALITY_TRIGGER(t, v1, v2, con, num) \
do { \
BoolVarRef b = getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2); \
t.push_back(make_trigger(b, Trigger(this, num), LowerBound)); \
} while(false)
#define PUSH_DISEQUALITY_TRIGGER(t, v1, v2, con, num) \
do { \
BoolVarRef c = getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2); \
t.push_back(make_trigger(c, Trigger(this, num), UpperBound)); \
} while(false)
#define GET_DISEQ_BOOL(stateObj, v1, v2) (getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2))
#define TRIGGER_ON_EQUALITY(v1, v2, trigger) \
do { \
BoolVarRef c = getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2); \
c.addDynamicTrigger(trigger, DomainRemoval, 0); \
} while(false)
#define TRIGGER_ON_DISEQUALITY(v1, v2, trigger)	\
do { \
BoolVarRef c = getVars(stateObj).getBoolVarContainer().getDiseqBool(v1, v2); \
c.addDynamicTrigger(trigger, DomainRemoval, 1); \
} while(false)

#include "mappings/variable_neg.h"
#include "mappings/variable_switch_neg.h"
#include "mappings/variable_stretch.h"
#include "mappings/variable_constant.h"
#include "mappings/variable_not.h"
#include "mappings/variable_shift.h"


