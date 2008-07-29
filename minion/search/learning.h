/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

#ifndef EXPLANATIONS_HEADER
#define EXPLANATIONS_HEADER

#include "../system/linked_ptr.h"

#include "../CSPSpec.h"

class Explanation {
  
};

//explanations are stored as garbage collected pointers
typedef shared_ptr<Explanation> ExplPtr;

class Conjunction : public Explanation {
 public:
  vector<ExplPtr> conjuncts;
};

class Lit : public Explanation {
 public:
  Var var;
  DomainInt val; 
  bool assignment; //T iff it's an assignment

  Lit(Var _var, DomainInt _val, bool _assignment) :
    var(_var), val(_val), assignment(_assignment) {}

  //NB. use dynamic_literal and dynamic_notliteral to implement getNegCon()
};

//the following class is for the use of alldiff and it's templated by variable
//type so that the constraint posted can be a nice fast templated version of
//alldiff
template<typename VarRef>
class Pairsame : public Explanation {
public:
  vector<VarRef> vars;

  Pairsame(const vector<VarRef>& _vars) : vars(_vars) {}
};

#endif
