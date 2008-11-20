//dummy declaration to be replaced later with the proper one

#include <utility>

#include "../system/linked_ptr.h"

#include <tr1/unordered_set>
using namespace std::tr1;

#include <set>

class AbstractConstraint;

class VirtCon;

class StateObj;

typedef shared_ptr<VirtCon> VirtConPtr;

class VirtCon {
 public:
  virtual vector<VirtConPtr> whyT() const = 0;
  virtual AbstractConstraint* getNeg() const = 0;
  virtual pair<unsigned,unsigned> getDepth() const = 0;
  friend bool operator==(const VirtConPtr a, const VirtConPtr b) { return a->equals(b.get()); }
  virtual bool equals(VirtCon* other) const = 0;
  friend std::ostream& operator<<(std::ostream& o, const VirtCon& vc) { if(&vc) vc.print(o); else o << "null"; return o; }
  virtual void print(std::ostream& o) const = 0;
  virtual size_t hash() const = 0;
  virtual ~VirtCon() {}
};

template<typename VarRef>
class DisAssignment : public VirtCon { //var != val
  static const size_t guid = 0;
  StateObj* stateObj;
  VarRef var;
  DomainInt val;

 public:
  DisAssignment(StateObj* _stateObj, VarRef _var, DomainInt _val) : stateObj(_stateObj), var(_var), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

template<typename VarRef>
class Assignment : public VirtCon { //var == val
  static const size_t guid = 1000;
  StateObj* stateObj;
  VarRef var;
  DomainInt val;

 public:
  Assignment(StateObj* _stateObj, VarRef _var, DomainInt _val) : stateObj(_stateObj), var(_var), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};  

template<typename VarRef>
class LessConstant : public VirtCon { //var < constant
  static const size_t guid = 2000;
  StateObj* stateObj;
  VarRef var;
  DomainInt constant;

public:
  LessConstant(StateObj* _stateObj, VarRef _var, DomainInt _constant) : stateObj(_stateObj), var(_var), constant(_constant) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

template<typename VarRef>
class GreaterConstant : public VirtCon { //var > constant ie constant < var
  static const size_t guid = 3000;
  StateObj* stateObj;
  VarRef var;
  DomainInt constant;

public:
  GreaterConstant(StateObj* _stateObj, VarRef _var, DomainInt _constant) : stateObj(_stateObj), var(_var), constant(_constant) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};  

template<typename VarRef1, typename VarRef2> //class prototype
class WatchLessConstraint;

template<typename VarRef1, typename VarRef2>
class WatchlessPrunLeft : public VirtCon { //var1 < var2 has pruned val from var1
  static const size_t guid = 4000;
  WatchLessConstraint<VarRef1, VarRef2>* con;
  DomainInt val;

public:
  WatchlessPrunLeft(WatchLessConstraint<VarRef1, VarRef2>* _con, DomainInt _val) :
    con(_con), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

template<typename VarRef1, typename VarRef2>
class WatchlessPrunRight : public VirtCon { //var1 < var2 has pruned val from var2
  static const size_t guid = 5000;
  WatchLessConstraint<VarRef1, VarRef2>* con;
  DomainInt val;

public:
  WatchlessPrunRight(WatchLessConstraint<VarRef1, VarRef2>* _con, DomainInt _val) :
    con(_con), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

class NegOfPostedCon : public VirtCon {
  static const size_t guid = 6000;
  AbstractConstraint* con;
 
 public:
  NegOfPostedCon(AbstractConstraint* _con) : con(_con) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

class Dynamic_OR;

class DisjunctionPrun : public VirtCon {
  static const size_t guid = 7000;
  AbstractConstraint* doer; //the constraint that did something
  VirtConPtr done;             //what they did
  Dynamic_OR* dj;           //the disjunction it was in

 public:
  DisjunctionPrun(AbstractConstraint* _doer, VirtConPtr _done, Dynamic_OR* _dj) : 
    doer(_doer), done(_done), dj(_dj) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

template<typename VarRef>
class BecauseOfAssignmentPrun : public VirtCon {
  static const size_t guid = 8000;
  StateObj* stateObj;
  VarRef var;
  DomainInt pruned;
 public:
  BecauseOfAssignmentPrun(StateObj* stateObj, VarRef _var, DomainInt _pruned) : var(_var), pruned(_pruned) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
};

class MHAV : public VirtCon {
  static const size_t guid = 9000;
  vector<VirtConPtr>& expls; //reference to the explns in the variable type

 public:
  MHAV(vector<VirtConPtr>& _expls) : expls(_expls) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const; //do nothing
  virtual pair<unsigned,unsigned> getDepth() const; //do nothing
  virtual bool equals(VirtCon* other) const; //do nothing
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

class AssgOrPrun : public VirtCon {
  static const size_t guid = 10000;
  VirtConPtr assg;
  VirtConPtr prun;

 public:
  AssgOrPrun(VirtConPtr _assg, VirtConPtr _prun) : assg(_assg), prun(_prun) {}
  virtual vector<VirtConPtr> whyT() const; //just return the above in a vector
  virtual AbstractConstraint* getNeg() const; //do nothing
  virtual pair<unsigned,unsigned> getDepth() const; //do nothing
  virtual bool equals(VirtCon* other) const; //do nothing
  virtual void print(std::ostream& o) const;
  virtual size_t hash() const;
};

template<typename VarRef>
class BecauseOfPruningsAssignment : public VirtCon {
  static const size_t guid = 11000;
  StateObj* stateObj;
  VarRef var;
  DomainInt assigned;
 public:
  BecauseOfPruningsAssignment(StateObj* stateObj, VarRef _var, DomainInt _assigned) : var(_var), assigned(_assigned) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
};

template<typename VarRef>
class DecisionAssg : public VirtCon { //decision did assignment
  static const size_t guid = 12000;
  StateObj* stateObj;
  VarRef var;
  DomainInt val;

public:
  DecisionAssg(StateObj* _stateObj, VarRef _var, DomainInt _val) : stateObj(_stateObj), var(_var), val(_val) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
};

class Anything : public VirtCon { //any virtcons that are blamed
  static const size_t guid = 13000;
  vector<VirtConPtr> blamed;

public:
  Anything(vector<VirtConPtr>& _blamed) : blamed(_blamed) {}
  virtual vector<VirtConPtr> whyT() const;
  virtual AbstractConstraint* getNeg() const;
  virtual pair<unsigned,unsigned> getDepth() const;
  virtual bool equals(VirtCon* other) const;
  virtual void print(std::ostream& o) const;  
  virtual size_t hash() const;
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
