#include "dynamic_new_or.h"

AbstractConstraint* Dynamic_AND::reverse_constraint()
{ // OR of the reverse of all the child constraints..
  vector<AbstractConstraint*> con;
  for(int i=0; i<child_constraints.size(); i++)
  {
      con.push_back(child_constraints[i]->reverse_constraint());
  }
  return new Dynamic_OR(stateObj, con);
}
