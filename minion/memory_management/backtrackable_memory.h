/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: backtrackable_memory.h 745 2007-11-02 13:37:26Z azumanga $
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

#include "MemoryBlock.h"

// \addtogroup Memory
// @{

/// Provides a wrapper around \ref NewMemoryBlock for backtrackable memory.
/* This class acts like a stack, allowing the backtrackable state to be poped
 * and pushed as required during search.
 */
class BackTrackMemory
{
  NewMemoryBlock new_memory_block;
  
  char* backtrack_data; 
  int current_depth_m;
  int max_depth;
  bool locked;
public:
    
  
  /// Wraps request_bytes of the internal \ref NewMemoryBlock.
  MoveablePointer request_bytes(unsigned byte_count)
  { 
    D_ASSERT(!locked);
    return new_memory_block.request_bytes(byte_count); 
  }
  
  /// Wraps requestArray of the internal \ref NewMemoryBlock.
  template<typename T>
  MoveableArray<T> requestArray(unsigned size)
  { 
    D_ASSERT(!locked);
    return new_memory_block.requestArray<T>(size); 
  }
  
  BackTrackMemory() : backtrack_data(NULL),
    current_depth_m(0), max_depth(0), locked(false)
  {
      
  }
  
  /// Extends the number of copies of the backtrackable memory that can be stored.
  void extend(int new_max)
  {
    D_ASSERT(locked);
    D_ASSERT(new_max > max_depth);
    unsigned data_size = new_memory_block.getDataSize();
    
    char* new_data = (char*)malloc(new_max * data_size);
    
    memcpy(new_data, backtrack_data, current_depth_m * data_size);
    free(backtrack_data);
    
    max_depth = new_max;
    backtrack_data = new_data;
  }
  
  void lock()
  { 
    new_memory_block.lock(); 
    locked = true;
    extend(10);
  }
  
  /// Copies the current state of backtrackable memory.
  void world_push()
  {
    D_ASSERT(locked);
    if(current_depth_m == max_depth)
      extend(max_depth * 2);
    unsigned data_size = new_memory_block.getDataSize();
    memcpy(backtrack_data + current_depth_m * data_size, new_memory_block.getDataPtr(), data_size);
    current_depth_m++;
  }
  
  /// Restores the state of backtrackable memory to the last stored state.
  void world_pop()
  {
    D_ASSERT(locked);
    D_ASSERT(current_depth_m > 0);
    current_depth_m--;
    
    unsigned data_size = new_memory_block.getDataSize();
    memcpy(new_memory_block.getDataPtr(), backtrack_data + current_depth_m * data_size, data_size);
  }
  
  /// Returns the current number of stored copies of the state.
  int current_depth()
  { return current_depth_m; }
};
