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


  // Must be run before the lock for nonbacktrack.
inline void TrailedMonotonicSet::lock(StateObj * _stateObj)
{
    D_ASSERT(!locked);
    #ifndef NO_DEBUG
    locked=true;
    #endif
    
    stateObj=_stateObj;
    
    _max_undos=_size;
    
    _max_depth = _max_undos;             
    _local_depth = 0;
    
    _array = getMemory(stateObj).nonBackTrack().request_bytes(_size*sizeof(value_type)); 
    _undo_indexes = getMemory(stateObj).nonBackTrack().request_bytes(_max_depth*sizeof(SysInt));
    
    _backtrack_depth_ptr = getMemory(stateObj).backTrack().request_bytes(sizeof(SysInt));
    
    *((SysInt*)_backtrack_depth_ptr.get_ptr()) = 0;
    
    #ifdef DEBUG_TMS
    cout << "initialising TrailedMonotonicSet with value of size= " << size << endl;
    // print_state();
    #endif
    
    for(SysInt i=0; i< _size; i++) {
      array(i) = tms_in_set;
    };
}

