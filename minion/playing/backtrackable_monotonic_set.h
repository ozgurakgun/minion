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


class BacktrackableMonotonicSet
{
  // could possibly store everything as unsigned but int is compatible 
  // with ReversibleInt 

  int _original_size;
  ReversibleInt _current_size;
  MemOffset _last_removed;
  MemOffset _inverse;
  
  BackTrackOffset backtrack_ptr;

  int& last_removed(int i)
  { return static_cast<int*>(_last_removed.get_ptr())[i]; }

  int& inverse(int i)
  { return static_cast<int*>(_inverse.get_ptr())[i]; }

public:
  int original_size()
  {
    return _original_size-1;    // we add a dummy value
  }

  int size()
  {
    return (_current_size.get()-1);     // we add a dummy value
  }

  bool isMember(const int& index)
  {
/*
#ifdef DEBUG
    cout << "    Call to isMember: index " << index << endl ; 
    print_state(8);
#endif
*/
    D_ASSERT( 0 <= index and index < _original_size);
    return (last_removed(index) < _current_size.get());
  }

  void remove(const int& index)
  {
    D_ASSERT( 0 <= index and index < _original_size);

    if (not isMember(index))
      return;

#ifdef DEBUG
    cout << "call to remove index: " << index << endl ;
    print_state(6);
#endif

    int newsize = _current_size.decrement();
    
    //  worried there could be an off-by-1 or other error in following test
    //  It is to avoid restoring an element to the domain incorrectly.
    if (last_removed(inverse(newsize)) <= _current_size.get())
    {
        last_removed(inverse(newsize)) = 0;
    }

    last_removed(index) = newsize;
    inverse(newsize) = index;

/*
#ifdef DEBUG
    print_state(8);
#endif
*/
  }


BacktrackableMonotonicSet(const int& n)
{ 
#ifdef DEBUG
  cout << "initialising BMS with value of n= " << n << endl;
#endif
  _last_removed.request_bytes((n+1)*sizeof(int)); 
  _inverse.request_bytes((n+1)*sizeof(int));

  _original_size = n+1;
  _current_size.set(n+1);
  
  for(int i=0; i < _original_size; ++i) 
  { last_removed(i) = 0;
    inverse(i) = _original_size-1; }

#ifdef DEBUG
  print_state();
#endif

}

void print_state() { print_state(4);}
void print_state(int spaces)
{
  for(int i=0; i<spaces; i++) { cout << " " ;};
  cout << "printing state of BMS: " ;
  cout << "original size: " << original_size() ;
  cout << " current size: " << size() << " bits: " ;
  for(int i = 0; i < _original_size ; ++i) { 
    cout << isMember(i);
  }
  cout << endl;
  for(int i=0; i<spaces; i++) { cout << " " ;};
  cout << "  last removed: ";
    for(int i = 0; i < _original_size ; ++i) { 
       cout << last_removed(i);
    }
  cout << endl ;
  for(int i=0; i<spaces; i++) { cout << " " ;};
  cout << "  inverse: ";
    for(int i = 0; i < _original_size ; ++i) { 
       cout << inverse(i) << " ";
  }
  cout << endl;
}


};

