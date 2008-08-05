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
 protected:
  virtual void myPrint(std::ostream& o) const
  { o << "UnknownExpl"; }

 public:
  friend std::ostream& operator<<(std::ostream& o, const Explanation& e)
  { e.myPrint(o); return o; }
};

//explanations are stored as garbage collected pointers
typedef shared_ptr<Explanation> ExplPtr;

class Conjunction : public Explanation {
 public:
  vector<ExplPtr> conjuncts;

  Conjunction(vector<ExplPtr>& _conjuncts) : conjuncts(_conjuncts) {}
  Conjunction() {}

  virtual void myPrint(std::ostream& o) const
  { 
    o << "AND(";
    if(conjuncts.size() != 0) o << *conjuncts[0];
    for(size_t i = 1; i < conjuncts.size(); i++) o << "," << *conjuncts[i];
    o << ")";
  }
};

class Lit : public Explanation {
 public:
  Var var;
  DomainInt val; 
  bool assignment; //T iff it's an assignment

  Lit(Var _var, DomainInt _val, bool _assignment) :
    var(_var), val(_val), assignment(_assignment) {}

  virtual void myPrint(std::ostream& o) const
  { o << "LIT(var=" << var << ",val=" << val << ",ass=" << assignment << ")"; }

  //NB. use dynamic_literal and dynamic_notliteral to implement getNegCon()
};

template<typename Var1, typename Var2>
class GreaterEqual : public Explanation {
public:
  Var1 v1;
  Var2 v2;

  GreaterEqual(const Var1& _v1, const Var2& _v2) : v1(_v1), v2(_v2) {}

  virtual void myPrint(std::ostream& o) const
  { o << "GEQ(" << v1 << "," << v2 << ")"; }
};

template<typename VarRef>
class LessConstant : public Explanation {
 public:
  VarRef v;
  DomainInt c;
  bool varOnLeft; //true iff expln is v < c, false iff c < v

  LessConstant(const DomainInt _c, const VarRef& _v) : c(_c), v(_v), varOnLeft(false) {}
  LessConstant(const VarRef& _v, const DomainInt _c) : c(_c), v(_v), varOnLeft(true) {}

  virtual void myPrint(std::ostream& o) const
  { 
    if(varOnLeft) o << "LESS(" << v << "," << c << ")";
    else o << "LESS(" << c << "," << v << ")";
  }
};

#endif
