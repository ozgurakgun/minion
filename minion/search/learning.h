//dummy declaration to be replaced later with the proper one

#include <utility>

#include <cctype>

#include "../system/linked_ptr.h"

#include "../CSPSpec.h"

#include <tr1/unordered_set>
using namespace std::tr1;

#include <set>

class AbstractConstraint;

class VirtCon;

class StateObj;

typedef shared_ptr<VirtCon> VirtConPtr;

class VCCompData {
public:
  Var var1;
  Var var2;
  DomainInt val1;
  DomainInt val2;

  VCCompData(Var _var1, Var _var2, DomainInt _val1, DomainInt _val2)
    : var1(_var1), var2(_var2), val1(_val1), val2(_val2) {}
};

class VirtCon {
 public:
  size_t guid;
  VirtCon(size_t _guid) : guid(_guid) {}
  virtual vector<VirtConPtr> whyT() const = 0;
  virtual AbstractConstraint* getNeg() const = 0;
  virtual pair<unsigned,unsigned> getDepth() const = 0;
  friend bool operator==(const VirtConPtr a, const VirtConPtr b) { return a->equals(b.get()); }
  virtual bool equals(VirtCon* other) const = 0;
  virtual bool less(VirtCon* other) const = 0;
  friend std::ostream& operator<<(std::ostream& o, const VirtCon& vc) { if(&vc) vc.print(o); else o << "null"; return o; }
  virtual void print(std::ostream& o) const = 0;
  virtual size_t hash() const = 0;
  virtual bool isDecision() const { return false; }
  virtual VCCompData getVCCompData() const { return VCCompData(Var(), Var(), 0, 0); }
  virtual ~VirtCon() {}
};

#ifdef GLEARN
#ifdef EAGER
class VCEagerWrapper : public VirtCon {
 public:
  
  VirtConPtr b;
  vector<VirtConPtr> expl;

  VCEagerWrapper(VirtConPtr _b) : VirtCon(_b->guid), b(_b)
  { expl = _b->whyT(); } //store expln eagerly
  
  virtual vector<VirtConPtr> whyT() const { return expl; }
  virtual AbstractConstraint* getNeg() const { return b->getNeg(); }
  virtual pair<unsigned,unsigned> getDepth() const { return b->getDepth(); }
  virtual bool equals(VirtCon* other) const { return b->equals(other); }
  virtual bool less(VirtCon* other) const { return b->less(other); }
  virtual void print(std::ostream& o) const { o << "Eager("; b->print(o); o << ")"; }
  virtual size_t hash() const { return b->hash(); }
  virtual VCCompData getVCCompData() const { return b->getVCCompData(); }
};
#endif
#endif

/* template<typename VarRef> */
/* class DisAssignment : public VirtCon { //var != val */
/*   static const size_t guid = 0; */
/*   StateObj* stateObj; */
/*   VarRef var; */
/*   DomainInt val; */

/*  public: */
/*   DisAssignment(StateObj* _stateObj, VarRef _var, DomainInt _val) : stateObj(_stateObj), var(_var), val(_val) {} */
/*   virtual vector<VirtConPtr> whyT() const; */
/*   virtual AbstractConstraint* getNeg() const; */
/*   virtual pair<unsigned,unsigned> getDepth() const; */
/*   virtual bool equals(VirtCon* other) const; */
/*   virtual void print(std::ostream& o) const; */
/*   virtual size_t hash() const; */
/* }; */

/* template<typename VarRef> */
/* class Assignment : public VirtCon { //var == val */
/*   static const size_t guid = 1000; */
/*   StateObj* stateObj; */
/*   VarRef var; */
/*   DomainInt val; */

/*  public: */
/*   Assignment(StateObj* _stateObj, VarRef _var, DomainInt _val) : stateObj(_stateObj), var(_var), val(_val) {} */
/*   virtual vector<VirtConPtr> whyT() const; */
/*   virtual AbstractConstraint* getNeg() const; */
/*   virtual pair<unsigned,unsigned> getDepth() const; */
/*   virtual bool equals(VirtCon* other) const; */
/*   virtual void print(std::ostream& o) const; */
/*   virtual size_t hash() const; */
/* };   */

template<typename VarRef>
class LessConstant : public VirtCon { //var < constant
  StateObj* stateObj;
  VarRef var;
  DomainInt constant;

public:
 LessConstant(StateObj* _stateObj, VarRef _var, DomainInt _constant) : VirtCon(2000), stateObj(_stateObj), var(_var), constant(_constant) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

template<typename VarRef>
class GreaterConstant : public VirtCon { //var > constant ie constant < var
  StateObj* stateObj;
  VarRef var;
  DomainInt constant;

public:
  GreaterConstant(StateObj* _stateObj, VarRef _var, DomainInt _constant) : VirtCon(3000), stateObj(_stateObj), var(_var), constant(_constant) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

template<typename VarRef1, typename VarRef2> //class prototype
class WatchLessConstraint;

template<typename VarRef1, typename VarRef2>
class WatchlessPrunLeft : public VirtCon { //var1 < var2 has pruned val from var1
  WatchLessConstraint<VarRef1, VarRef2>* con;
  DomainInt val;

public:
  WatchlessPrunLeft(WatchLessConstraint<VarRef1, VarRef2>* _con, DomainInt _val) :
    VirtCon(4000), con(_con), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

template<typename VarRef1, typename VarRef2>
class WatchlessPrunRight : public VirtCon { //var1 < var2 has pruned val from var2
  WatchLessConstraint<VarRef1, VarRef2>* con;
  DomainInt val;

public:
  WatchlessPrunRight(WatchLessConstraint<VarRef1, VarRef2>* _con, DomainInt _val) :
     VirtCon(5000), con(_con), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

class NegOfPostedCon : public VirtCon {
  AbstractConstraint* con;
 
 public:
  NegOfPostedCon(AbstractConstraint* _con) : VirtCon(6000), con(_con) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

class Dynamic_OR;

class DisjunctionPrun : public VirtCon {
  AbstractConstraint* doer; //the constraint that did something
  VirtConPtr done;             //what they did
  Dynamic_OR* dj;           //the disjunction it was in

 public:
  DisjunctionPrun(AbstractConstraint* _doer, VirtConPtr _done, Dynamic_OR* _dj) : 
    VirtCon(7000), doer(_doer), done(_done), dj(_dj) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

template<typename VarRef>
class BecauseOfAssignmentPrun : public VirtCon {
  StateObj* stateObj;
  VarRef var;
  DomainInt pruned;
  DomainInt assigned;
 public:
  BecauseOfAssignmentPrun(StateObj* _stateObj, VarRef _var, DomainInt _pruned, DomainInt _assigned) : 
    VirtCon(8000), stateObj(_stateObj), var(_var), pruned(_pruned), assigned(_assigned) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

class MHAV : public VirtCon {
  vector<VirtConPtr>& expls; //reference to the explns in the variable type

 public:
  MHAV(vector<VirtConPtr>& _expls) : VirtCon(9000), expls(_expls) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const; //do nothing
  virtual pair<unsigned,unsigned> getDepth() const; //do nothing
  virtual bool equals(VirtCon* other) const; //do nothing
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

class AssgOrPrun : public VirtCon {
  VirtConPtr assg;
  VirtConPtr prun;

 public:
  AssgOrPrun(VirtConPtr _assg, VirtConPtr _prun) : VirtCon(10000), assg(_assg), prun(_prun) {}
  virtual vector<VirtConPtr> whyT() const; //just return the above in a vector
  virtual AbstractConstraint* getNeg() const; //do nothing
  virtual pair<unsigned,unsigned> getDepth() const; //do nothing
  virtual bool equals(VirtCon* other) const; //do nothing
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

template<typename VarRef>
class BecauseOfPruningsAssignment : public VirtCon {
  StateObj* stateObj;
  VarRef var;
  DomainInt assigned;
 public:
  BecauseOfPruningsAssignment(StateObj* stateObj, VarRef _var, DomainInt _assigned) : VirtCon(11000), var(_var), assigned(_assigned) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

template<typename VarRef>
class DecisionAssg : public VirtCon { //decision did assignment
  StateObj* stateObj;
  VarRef var;
  DomainInt val;

public:
  DecisionAssg(StateObj* _stateObj, VarRef _var, DomainInt _val) : VirtCon(0), stateObj(_stateObj), var(_var), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual bool isDecision() const { return true; }
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

template<typename VarRef>
class NoReasonPrun : public VirtCon {
  StateObj* stateObj;
  VarRef var;
  DomainInt val;

public:
  NoReasonPrun(StateObj* _stateObj, VarRef _var, DomainInt _val) : VirtCon(12000), stateObj(_stateObj), var(_var), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

template<typename VarRef>
class NoReasonAssg : public VirtCon {
  StateObj* stateObj;
  VarRef var;
  DomainInt val;

public:
  NoReasonAssg(StateObj* _stateObj, VarRef _var, DomainInt _val) : VirtCon(13000), stateObj(_stateObj), var(_var), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

class AMOV : public VirtCon { //assign two values for same var
  VirtConPtr assg_first;
  VirtConPtr assg_last;
  
 public:
  AMOV(VirtConPtr _assg_first, VirtConPtr _assg_last) : VirtCon(14000), assg_first(_assg_first), assg_last(_assg_last) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const; //do nothing
  virtual pair<unsigned,unsigned> getDepth() const; //do nothing
  virtual bool equals(VirtCon* other) const; //do nothing
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

template<typename VarRef1, typename VarRef2> //class prototype
class WatchNeqConstraint;

template<typename Var1, typename Var2>
class WatchNeqPrunLeft : public VirtCon {
  WatchNeqConstraint<Var1,Var2>* con;
  DomainInt val; //the value pruned from var1 and assigned to var2

 public:
  WatchNeqPrunLeft(WatchNeqConstraint<Var1,Var2>* _con, DomainInt _val) : VirtCon(15000), con(_con), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

template<typename Var1, typename Var2>
class WatchNeqPrunRight : public VirtCon {
  WatchNeqConstraint<Var1,Var2>* con;
  DomainInt val; //the value pruned from var2 and assigned to var1

 public:
  WatchNeqPrunRight(WatchNeqConstraint<Var1,Var2>* _con, DomainInt _val) : VirtCon(16000), con(_con), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

class TrieData;
class TrieState;
template<typename VarArray, typename TableDataType, typename TableStateType>
struct NewTableConstraint;

template<typename VarArray>
class TablePosPrun : public VirtCon {
  NewTableConstraint<VarArray,TrieData,TrieState>* con;
  size_t var_num;
  DomainInt val;

 public:
 TablePosPrun(NewTableConstraint<VarArray,TrieData,TrieState>* _con, size_t _var_num, DomainInt _val) : 
   VirtCon(17000), con(_con), var_num(_var_num), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual bool less(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
  virtual VCCompData getVCCompData() const;
};

inline void print_recursive(vector<int> count_seq, vector<VirtConPtr> why) {
  for(size_t i = 0; i < why.size(); i++) {
    cout << count_seq << endl;
    if(why[i].get()) {
      cout << i << " virtconptr:" << *why[i] << endl;
      cout << i << "      depth:" << why[i]->getDepth() << endl;
    } else {
      cout << "null" << endl;
    }
  }
  //need to fiddle to avoid going beyond DecisionAssgs!
  for(size_t i = 0; i < why.size(); i++) {
    if(why[i].get() && why[i]->getDepth().second != 0) { //not 0 means not decision or not assumption
      count_seq.push_back(i);
      print_recursive(count_seq, why[i]->whyT());
      count_seq.pop_back();
    } else {
      cout << "hit assumptions" << endl;
    }
  }
}
