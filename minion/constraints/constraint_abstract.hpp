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

#ifdef WDEG
  inline unsigned int AbstractConstraint::getWdeg() { return wdeg; }

  inline void AbstractConstraint::incWdeg() 
  { 
    wdeg += 1; 
    vector<AnyVarRef>* vars = get_vars_singleton();
    size_t vars_s = vars->size();
    for(size_t i = 0; i < vars_s; i++)
      (*vars)[i].incWdeg();
  }
#endif

  inline void DynamicTrigger::propagate()
  { 
    D_ASSERT(sanity_check == 1234);
    constraint->propagate(this); 
  }
