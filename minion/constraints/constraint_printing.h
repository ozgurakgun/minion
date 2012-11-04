/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-12
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

#include "../variables/mappings/variable_constant.h"

namespace ConOutput
{
  template<typename T>
  string print_vars(const T& t)
  {
    if(t.isAssigned())
      return to_string(t.getAssignedValue());
    else
    {
      vector<Mapper> m = t.getMapperStack();
      if(m.size() == 1)
      {
        D_ASSERT(m[0] == Mapper(MAP_NOT));
        return "!" + t.getBaseVar().get_name();
      }
      else
      {
        D_ASSERT(m.empty());
        return t.getBaseVar().get_name();
      }
    }
  }

  inline
  string print_vars(TupleList* const& t)
  { return t->getName(); }

  inline
  string print_vars(AbstractConstraint* const& c);

  inline
  string print_vars(const DomainInt& i)
  { return to_string(i); }

#ifdef MINION_DEBUG
  inline
  string print_vars(const SysInt& i)
  { return to_string(i); }
#endif

  template<SysInt i>
  string print_vars(const compiletime_val<i>)
  { return to_string(i); }

  inline
  string print_vars(const std::vector<AbstractConstraint*>& t)
  {
    ostringstream o;
    o << "{";
    bool first = true;
    for(size_t i = 0; i < t.size(); ++i)
    {
      if(!first)
        o << ",";
      else
        first = false;
      o << ConOutput::print_vars(t[i]);
    }
    o << "}";
    return o.str();
  }


  template<typename T>
  string print_vars(const std::vector<T>& t)
  {
    ostringstream o;
    o << "[";
    bool first = true;
    for(size_t i = 0; i < t.size(); ++i)
    {
      if(!first)
        o << ",";
      else
        first = false;
      o << ConOutput::print_vars(t[i]);
    }
    o << "]";
    return o.str();
  }

  template<typename T, size_t len>
  string print_vars(const array<T,len>& t)
  {
    ostringstream o;
    o << "[";
    bool first = true;
    for(size_t i = 0; i < t.size(); ++i)
    {
      if(!first)
        o << ",";
      else
        first = false;
      o << ConOutput::print_vars(t[i]);
    }
    o << "]";
    return o.str();
  }

  template<typename T>
  inline
  vector<DomainInt> filter_constants(T& vars)
  {
    vector<DomainInt> constants;
    for(size_t i = 0; i < vars.size(); ++i)
    {
      if(vars[i].isAssigned())
      {
        constants.push_back(vars[i].getAssignedValue());
        vars.erase(vars.begin() + i);
        --i;
      }
    }
    return constants;
  }


  inline 
  void compress_arrays(StateObj* stateObj, string name, vector<AnyVarRef>& vars, AnyVarRef& result)
  {
    if(name.find("sum") != string::npos)
    {
      vector<DomainInt> res = filter_constants(vars);
      DomainInt sum;
      for(size_t i = 0; i < res.size(); ++i)
        sum += res[i];

      if(result.isAssigned())
        result = ConstantVar(stateObj, result.getAssignedValue() - sum);
      else
        vars.push_back(ConstantVar(stateObj, sum));

    }
  }
  
  inline
  string print_con(StateObj* stateObj, string name)
  { return name + "()"; }


  template<typename T>
  string print_con(StateObj* stateObj, string name, const T& args)
  { 
    string s = print_vars(args); 
    return name + "(" + s + ")";
  }

  template<typename T1, typename T2>
  string print_con(StateObj* stateObj, string name, const T1& args1, const T2& args2)
  { 

    string s1 = print_vars(args1);
    string s2 = print_vars(args2); 
    return name + "(" + s1 + "," + s2 + ")";
  }
  
  template<typename T1, typename T2, typename T3>
  string print_con(StateObj* stateObj, string name, const T1& args1, const T2& args2, const T3& args3)
  { 
    string s1 = print_vars(args1); 
    string s2 = print_vars(args2);
    string s3 = print_vars(args3);
    return name + "(" + s1 + "," + s2 + "," + s3 + ")";
  }




  template<typename T1, typename T2>
  string print_reversible_con(StateObj* stateObj, string name, string neg_name, const T1& vars, const T2& res)
  {
    vector<Mapper> m = res.getMapperStack();
    if(!m.empty() && m.back() == Mapper(MAP_NEG))
    {
      vector<AnyVarRef> pops;
      for(size_t i = 0; i < vars.size(); ++i)
      {
        vector<Mapper> mapi = vars[i].getMapperStack();
        if(mapi.empty() || mapi.back() != Mapper(MAP_NEG))
          FATAL_REPORTABLE_ERROR();
        pops.push_back(vars[i].popOneMapper());
      }
      return print_con(stateObj, neg_name, pops, res.popOneMapper());
    }
    else
    {
      return print_con(stateObj, name, vars, res);
    }
  }

template<typename T1, typename T2>
  string print_weighted_con(StateObj* stateObj, string weight, string name, const T1& sumvars, const T2& result)
  {
    if(sumvars.empty())
      return print_con(stateObj, name, sumvars, result);

    vector<Mapper> v = sumvars[0].getMapperStack();
    if(!v.empty() && (v.back().type() == MAP_MULT || v.back().type() == MAP_SWITCH_NEG))
    {
      vector<AnyVarRef> pops;
      vector<DomainInt> weights;
      for(size_t i = 0; i < sumvars.size(); ++i)
      {
        vector<Mapper> mapi = sumvars[i].getMapperStack();
        if(mapi.empty() || (mapi.back().type() != MAP_MULT && mapi.back().type() != MAP_SWITCH_NEG))
          FATAL_REPORTABLE_ERROR();
        pops.push_back(sumvars[i].popOneMapper());
        weights.push_back(mapi.back().val());
      }
      return print_con(stateObj, weight + name,  weights, pops,  result);
    }
    else
    {
      return print_con(stateObj, name, sumvars, result);
    }
  }

template<typename T1, typename T2>
  string print_weighted_reversible_con(StateObj* stateObj, string weight, string name, string neg_name, const T1& vars, const T2& res)
  {
    vector<Mapper> m = res.getMapperStack();
    if(!m.empty() && m.back() == Mapper(MAP_NEG))
    {
      vector<AnyVarRef> pops;
      for(size_t i = 0; i < vars.size(); ++i)
      {
        vector<Mapper> mapi = vars[i].getMapperStack();
        if(mapi.empty() || mapi.back() != Mapper(MAP_NEG))
          FATAL_REPORTABLE_ERROR();
        pops.push_back(vars[i].popOneMapper());
      }
      return print_weighted_con(stateObj, weight, neg_name, pops, res.popOneMapper());
    }
    else
    {
      return print_weighted_con(stateObj, weight, name, vars, res);
    }
  }

}

#define CONSTRAINT_ARG_LIST0() \
virtual string full_output_name() \
{ return ConOutput::print_con(stateObj, constraint_name()); }

#define CONSTRAINT_ARG_LIST1(x) \
virtual string full_output_name() \
{ return ConOutput::print_con(stateObj, constraint_name(), x); }

#define CONSTRAINT_ARG_LIST2(x, y) \
virtual string full_output_name() \
{ return ConOutput::print_con(stateObj, constraint_name(), x, y); }

#define CONSTRAINT_REVERSIBLE_ARG_LIST2(name, revname, x, y) \
virtual string full_output_name() \
{ return ConOutput::print_reversible_con(stateObj, name, revname, x, y); }

#define CONSTRAINT_WEIGHTED_REVERSIBLE_ARG_LIST2(weight, name, revname, x, y) \
virtual string full_output_name() \
{ return ConOutput::print_weighted_reversible_con(stateObj, weight, name, revname, x, y); }

#define CONSTRAINT_ARG_LIST3(x, y, z) \
virtual string full_output_name() \
{ return ConOutput::print_con(stateObj, constraint_name(), x, y, z); }
