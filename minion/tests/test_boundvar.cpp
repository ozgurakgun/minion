/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
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



#include "minion.h"
#include "test_functions.h"


int main(void)
{
  BoundVarRef a;
  a = boundvar_container.get_new_var(0,31);
  Controller::add_constraint(TestCon(a));
  Controller::lock();
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  D_ASSERT(a.getMin() == 0);
  Controller::world_push();
  a.setMin(5);
  Controller::propogate_queue();
  check_trigger_count(1,0,1,0);
  D_ASSERT(a.getMin() == 5);

  
  Controller::world_pop();
  D_ASSERT(a.getMin() == 0);
  Controller::world_push();
  
  
  a.setMin(5);
  
  Controller::propogate_queue();
  check_trigger_count(1,0,1,0);
  
  a.setMax(16);
  
  Controller::propogate_queue();
  check_trigger_count(0,1,1,0);
  
  D_ASSERT(a.getMax() == 16);
  
  Controller::world_pop();
  Controller::world_push();
  
  a.setMax(1);
  Controller::propogate_queue();
  check_trigger_count(0,1,1,0);
  
  a.propogateAssign(1);
  Controller::propogate_queue();
  check_trigger_count(1,0,1,1);
  
  a.propogateAssign(1);
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  
  Controller::world_pop();
  Controller::finish();
}

