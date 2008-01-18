/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: dynamic_element.h 709 2007-10-15 18:05:03Z pete_n $
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

/* This version of allDiff uses watched literals and maintains GAC.
Algorithm is roughly described in a podnote.

*/

#define DYNAMICALLDIFF

#include "alldiff_common.h"

template<typename VarArray>
DynamicConstraint*
DynamicAlldiffCon(StateObj * stateObj, VarArray va)
{
  return new DynamicAlldiff<VarArray>(stateObj, va);
}

BUILD_DYNAMIC_CONSTRAINT1(CT_WATCHED_ALLDIFF, DynamicAlldiffCon);
