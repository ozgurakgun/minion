/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: nonbacktrack_memory.h 680 2007-10-01 08:19:53Z azumanga $
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




struct MoveablePointer
{
  MoveablePointer(const MoveablePointer& b);
  
  void operator=(const MoveablePointer& b);

  explicit MoveablePointer(void* _ptr);

  explicit MoveablePointer(const MoveablePointer& b, int offset)
  {
    D_INFO(0,DI_POINTER,"Create offset of " + to_string(b.ptr) + " by " + to_string(offset));
    D_ASSERT(b.get_ptr() != NULL);
    *this = MoveablePointer(((char*)b.ptr) + offset); 
  }

  void* ptr;

  // This is only for debugging code!
  void* get_ptr_noCheck() const
  { return ptr; }

  void* get_ptr() const
#ifdef NO_DEBUG
  { return ptr; }
#else
    ; // Defined at bottom of file.
#endif

  void set_ptr(void* _ptr) 
  { 
    D_INFO(0, DI_POINTER, "Manually set pointer at " + to_string(this) + " to:" + to_string(ptr));
    ptr = _ptr; 
  }
  
  MoveablePointer getOffset(unsigned bytes)
  { 
    D_INFO(0, DI_POINTER, "Offset " + to_string(ptr) + " by " + to_string(bytes));
    return MoveablePointer(((char*)ptr) + bytes); 
  }

  MoveablePointer() : ptr(NULL)
  { D_INFO(0, DI_POINTER, "Create Empty Pointer at:" + to_string(this)); }
  
  ~MoveablePointer();
};

template<typename T>
class MoveableArray
{
  MoveablePointer ptr;
  unsigned size;
public:
  explicit MoveableArray(MoveablePointer _ptr, unsigned _size) : ptr(_ptr), size(_size)
  { }

  MoveableArray()
  {}

  // A common C++ trick - declare an identical method, one for const, one without.
  T& operator[](int pos)
  { 
    D_ASSERT(pos >= 0 && pos < size);
    return *(static_cast<T*>(ptr.get_ptr()) + pos);
  }

  const T& operator[](int pos) const
  { 
    D_ASSERT(pos >= 0 && pos < size);
    return *(static_cast<T*>(ptr.get_ptr()) + pos);
  }

  T* get_ptr()
  { return static_cast<T*>(ptr.get_ptr()); }

  const T* get_ptr() const
  { return static_cast<const T*>(ptr.get_ptr()); }


};

typedef MoveablePointer BackTrackOffset;

class NewMemoryBlock;

class MemBlockCache
{
  // Forbid copying this type!
  MemBlockCache(const MemBlockCache&);
  void operator=(const MemBlockCache&);

public:    
  
  MemBlockCache() { }
  
  set<NewMemoryBlock*> NewMemoryBlockCache;

  void registerNewMemoryBlock(NewMemoryBlock* mb)
  { NewMemoryBlockCache.insert(mb); }

  void unregisterNewMemoryBlock(NewMemoryBlock* mb)
  { NewMemoryBlockCache.erase(mb); }

  inline void addPointerToNewMemoryBlock(MoveablePointer* vp);

  inline void removePointerFromNewMemoryBlock(MoveablePointer* vp);

  inline bool checkPointerValid(const MoveablePointer*const vp);
};

VARDEF(MemBlockCache memBlockCache);


/** A NewMemoryBlock is basically an extendable, moveable block of memory which
  * keeps track of all pointers into it, and moves them when approriate
 */
/// Looks after all pointers to this block, and also the memory itself
class NewMemoryBlock
{
  // Forbid copying this type!
  NewMemoryBlock(const NewMemoryBlock&);
  void operator=(const NewMemoryBlock&);
  
  char* current_data;
  unsigned allocated_bytes;
  unsigned maximum_bytes;
  bool lock_m;
  bool final_lock_m;
  set<MoveablePointer*> pointers;
public:
  
  char* getDataPtr()
  { return current_data; }

  unsigned getDataSize()
  { return allocated_bytes; }

  NewMemoryBlock() : allocated_bytes(0), maximum_bytes(0), current_data(NULL), 
                  lock_m(false), final_lock_m(false)
  { memBlockCache.registerNewMemoryBlock(this);}
  
  MoveablePointer request_bytes(unsigned byte_count)
  {
    // TODO: is the following line necessary?
    if(byte_count % sizeof(int) != 0)
      byte_count += sizeof(int) - (byte_count % sizeof(int));

    D_ASSERT(!lock_m);
    if(maximum_bytes < allocated_bytes + byte_count)
    { reallocate( (allocated_bytes + byte_count) * 2 ); }
    D_INFO(0,DI_MEMBLOCK,"New pointer at:" + to_string((void*)(current_data+allocated_bytes)) + " for " + to_string(byte_count));
    // If no bytes asked for, just stuff it at the start of the block.
    if(byte_count == 0)
      return MoveablePointer(NULL);

    char* return_val = current_data + allocated_bytes;
    allocated_bytes += byte_count;
    return MoveablePointer(return_val);
  }

  template<typename T>
  MoveableArray<T> requestArray(unsigned size)
  {
    MoveablePointer ptr = request_bytes(size * sizeof(T));
    return MoveableArray<T>(ptr, size);
  }


  // Enlarges memory block and moves all pointers
  void reallocate(unsigned byte_count)
  {
    D_ASSERT(!lock_m);
    D_ASSERT(byte_count >= allocated_bytes);
    char* new_data = (char*)malloc(byte_count);
    memset(new_data, 0, byte_count);

    for(set<MoveablePointer*>::iterator it = pointers.begin(); it != pointers.end(); ++it)
    {
      D_ASSERT((*it)->get_ptr() >= current_data && (*it)->get_ptr() < current_data + allocated_bytes);
      (*it)->set_ptr( ((char*)((*it)->get_ptr()) - current_data) + new_data );
    }
   
    // TODO: Search codebase for memcpy, use realloc instead if possible.
    memcpy(new_data, current_data, allocated_bytes);
    free(current_data);
    D_INFO(0, DI_MEMBLOCK, "Moving base from " + to_string((void*)current_data) + " to " + to_string((void*)new_data));
    current_data = new_data;
    maximum_bytes = byte_count;
  }

  // This checks if this pointer could need adding to this tracker, and if so it does and returns true
  bool checkAddToTracker(MoveablePointer* vp)
  {
    D_ASSERT(!final_lock_m);
    void* ptr = vp->get_ptr_noCheck();
    D_INFO(0,DI_MEMBLOCK,"Testing if " + to_string(vp) + " to " + to_string(ptr) + " should be added...");
    if(ptr < current_data || ptr >= current_data + allocated_bytes)
      return false;
    else
    {
      D_INFO(0,DI_MEMBLOCK, "Pointer added!");
      pointers.insert(vp);
      return true;
    }
  }

  // Checks if this pointer belongs to this tracker, and if so removes and returns true
  bool checkRemoveFromTracker(MoveablePointer* vp)
  {
    void* ptr = vp->get_ptr_noCheck();
    if(ptr < current_data || ptr >= current_data + allocated_bytes)
      return false;
    else
    {
      pointers.erase(vp);
      return true;
    }
  }

  bool checkPointerValid(const MoveablePointer*const vp)
  {
    void* ptr = vp->get_ptr_noCheck();
    D_INFO(0, DI_MEMBLOCK, "Checking for " + to_string(vp) + " to " + to_string(ptr) + " in range " + to_string((void*) current_data) + string(":")
      + to_string((void*)(current_data + allocated_bytes)));

    bool check1 = (pointers.count(const_cast<MoveablePointer*>(vp)) > 0);
    bool check2 = (ptr >= current_data && ptr < current_data + allocated_bytes);
    if(check1 != check2)
    {
      D_INFO(0, DI_MEMBLOCK, "Error!" + to_string(check1) + ":" + to_string(check2));
      D_FATAL_ERROR("Fatal Memory corruption - pointer broken!");
    }
    return check1;
  }

  // TODO: Remove
  void final_lock()
  { 
    D_ASSERT(lock_m);
    final_lock_m = true;
  }
  
  void lock()
  {
    reallocate(allocated_bytes);
    lock_m = true;
  }
   
};


inline MoveablePointer::MoveablePointer(const MoveablePointer& b) : ptr(b.ptr)
{
  D_INFO(0,DI_POINTER,"Copy pointer to " + to_string(ptr) + " from vp " + to_string(&b) + " to " + to_string(this));
  memBlockCache.addPointerToNewMemoryBlock(this);
}

inline void MoveablePointer::operator=(const MoveablePointer& b)
{
  D_INFO(0, DI_POINTER, "Make " + to_string(this)+ " point to " + to_string(b.ptr) + " instead of " + to_string(ptr));
  memBlockCache.removePointerFromNewMemoryBlock(this);
  ptr = b.ptr;
  memBlockCache.addPointerToNewMemoryBlock(this);
}

inline MoveablePointer::MoveablePointer(void* _ptr) : ptr(_ptr)
{
  D_INFO(0,DI_POINTER,"Create pointer from raw to:" + to_string(ptr) + " at " + to_string(this));
  memBlockCache.addPointerToNewMemoryBlock(this);
}

inline MoveablePointer::~MoveablePointer()
{
  D_INFO(0,DI_POINTER,"Remove pointer at " + to_string(this));
  memBlockCache.removePointerFromNewMemoryBlock(this);
}

#ifndef NO_DEBUG
inline void* MoveablePointer::get_ptr() const
{
  D_ASSERT(memBlockCache.checkPointerValid(this));
  return ptr;
}
#endif

inline void MemBlockCache::addPointerToNewMemoryBlock(MoveablePointer* vp)
  {
    if(vp->get_ptr_noCheck() == NULL)
      return;

    for(set<NewMemoryBlock*>::iterator it = NewMemoryBlockCache.begin();
        it != NewMemoryBlockCache.end();
        ++it)
    {
      if((*it)->checkAddToTracker(vp))
        return;
    }
    ;
    D_FATAL_ERROR("Fatal Memory Corruption when adding to tracker!");
  }

  inline void MemBlockCache::removePointerFromNewMemoryBlock(MoveablePointer* vp)
  {
    for(set<NewMemoryBlock*>::iterator it = NewMemoryBlockCache.begin();
        it != NewMemoryBlockCache.end();
        ++it)
    {
      if((*it)->checkRemoveFromTracker(vp))
        return;
    }
    //D_FATAL_ERROR("Fatal Memory Corruption when leaving the tracker");
  }

  inline bool MemBlockCache::checkPointerValid(const MoveablePointer *const vp)
  {
    if(vp->get_ptr_noCheck() == NULL)
      return true;
    for(set<NewMemoryBlock*>::iterator it = NewMemoryBlockCache.begin();
        it != NewMemoryBlockCache.end();
        ++it)
    {
      if((*it)->checkPointerValid(vp))
        return true;
    }
    ;
    D_FATAL_ERROR("Fatal Memory Error - invalid Pointer deferenced!");
  }


