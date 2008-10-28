//dummy declaration to be replaced later with the proper one

#include "../system/linked_ptr.h"

class VirtCon;

typedef shared_ptr<VirtCon> VirtConPtr;

class VirtCon {
 public:
  virtual vector<VirtConPtr> whyT() const = 0;
  virtual VirtConPtr getNeg() const = 0;
  virtual pair<unsigned,unsigned> getDepth() const = 0;
  friend bool operator==(const VirtConPtr a, const VirtConPtr b) { return a->equals(*b); }
  virtual bool equals(const VirtCon& other) const = 0;
  virtual ~VirtCon() {}
};
