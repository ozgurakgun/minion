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

#include "../system/system.h"
#include "common_search.h"
#include "standard_search.h"

namespace Controller
{

    template<typename VarOrder, typename Variables, typename Permutation, typename Propogator>
    BOOL group_solve_loop(StateObj* stateObj, function<BOOL (void)> next_search, VarOrder& original_order, Variables& v, Permutation& perm, Propogator prop = PropagateGAC())
    {
      int sol_count = 0;
      for(int i = 0; i < perm.size(); ++i)
      {
        if(!perm[i].setMin(1))
            return false;
        if(!perm[i].setMax(perm.size()))
            return false;
      }

      return getQueue(stateObj).propagateQueue();

      for(int i = 0; i < perm.size(); ++i)
      {
        for(int j = i + 2; j <= perm.size(); ++j)
        {
          int world_depth = get_world_depth(stateObj);
          world_push(stateObj);
          for(int k = 0; k < i; ++k)
            if(!perm[k].propagateAssign(k+1))
                return false;
          if(!perm[i].propagateAssign(j))
            return false;

          BOOL retval = getQueue(stateObj).propagateQueue();
          if(retval)
          {
            try
            {
              VarOrder order(original_order);
              retval = solve_loop(stateObj, next_search, order, v, prop, true);
            }
            catch(EndOfSearch)
            { 
              sol_count += getState(stateObj).getSolutionCount();
              getState(stateObj).setSolutionCount(0);
              getQueue(stateObj).clearQueues();
            }
          }
          retval = true;
          // We need this as we can exit search deep in search.
          D_ASSERT(world_depth < get_world_depth(stateObj));
          world_pop_to_depth(stateObj, world_depth);          
        }        
      }

      printf("Generators: %d\n", sol_count);
      return true;
    }
}


