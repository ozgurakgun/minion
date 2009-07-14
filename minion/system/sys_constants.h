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

#ifndef _SYS_CONSTANTS_H
#define _SYS_CONSTANTS_H

#ifdef USE_GMP
#include <gmpxx.h>
typedef mpz_class BigInt;
#else
typedef long long BigInt;
#endif

/// A placeholder type.
struct EmptyType
{};

/// A big constant, when such a thing is needed.
const int big_constant = 999999;


/// A constant chosen at compile time.
/// Create with the notation compiletime_val<6>().
template<int i>
struct compiletime_val
{ 
  operator int() const
{ return i; }
  
  compiletime_val<-i-1> negminusone() const
{ return compiletime_val<-i-1>(); }
  
  compiletime_val<-i> neg() const
{ return compiletime_val<-i>(); }
  
  friend std::ostream& operator<<(std::ostream& o, const compiletime_val& v)
{ return o << "CompiletimeConst:" << i; }
  
};


/// A constant chosen at run time.
/// Create with the notation runtime_val(6).
struct runtime_val
{
  int i;
  runtime_val(int _i) : i(_i)
  {}
  
  operator int() const
  { return i; }
  
  runtime_val negminusone() const
  { return runtime_val(-i-1); }
  
  template<int j>
  runtime_val neg() const
  { return runtime_val(-i); }
  
  friend std::ostream& operator<<(std::ostream& o, const runtime_val& v)
  { return o << v.i; }

};

template<typename T>
inline T mymin(T t1, T t2)
{
  if(t1 <= t2)
    return t1;
  else
    return t2;
}

template<typename T>
inline T mymax(T t1, T t2)
{
  if(t1 <= t2)
    return t2;
  else
    return t1;
}
#ifdef __CYGWIN__
typedef int MachineInt;
#else
typedef int32_t MachineInt;
#endif

#ifdef BOUNDS_CHECK
typedef Wrapper<MachineInt> DomainInt;
#else
typedef MachineInt DomainInt;
#endif

// Put a ' -1, +1 ' just to have some slack
const DomainInt DomainInt_Max = std::numeric_limits<MachineInt>::max() - 1;
const DomainInt DomainInt_Min = std::numeric_limits<MachineInt>::min() + 1;

template<typename To, typename From>
To checked_cast(const From& t)
{ return static_cast<To>(t); }

template<typename To, typename From>
To checked_cast(const Wrapper<From>& t)
{ return static_cast<To>(t.t); }


#endif // _SYS_CONSTANTS_H
