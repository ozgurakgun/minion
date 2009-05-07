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

#define LOOP

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

#ifndef CONSTRAINT_CLIQUE_H
#define CONSTRAINT_CLIQUE_H

template<typename VarArray>
    struct CliqueConstraint : public AbstractConstraint
{
    virtual string constraint_name()
        { return "Clique"; }

  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
    typedef typename VarArray::value_type VarRef;

    VarArray var_array;
    int dom_min;
    int dom_max;
    int dom_size;

    vector<vector<int> > domains;
    vector<vector<int> > current_support;

#ifdef LOOP
    vector<Reversible<int> > check_val1;
    vector<Reversible<int> > check_val2;
#endif

    CliqueConstraint(StateObj* _stateObj, const VarArray& _var_array, TupleList* tuples) : AbstractConstraint(_stateObj),
        var_array(_var_array)
    {
        CHECK(tuples->tuple_size() == 2, "Clique requires binary constraints");
        dom_max = var_array[0].getInitialMax();
        dom_min = var_array[0].getInitialMin();
        dom_size = dom_max - dom_min + 1;
        domains.resize(dom_max - dom_min + 1);

        current_support.resize(var_array.size());
        for(int i = 0; i < var_array.size(); ++i)
            current_support[i].resize(dom_size, 0);
#ifdef LOOP
        check_val1.reserve(dom_size);
        check_val2.reserve(dom_size);

        for(int i = 0; i < dom_size; ++i)
        {
          check_val1.push_back(Reversible<int>(stateObj, 0));
          check_val2.push_back(Reversible<int>(stateObj, 1));
        }
#endif
        for(int i = 0; i < tuples->size(); ++i)
        {
            int first = (*tuples)[i][0];
            int second = (*tuples)[i][1];

            if(first >= dom_min && first <= dom_max &&
               second >= dom_min && second <= dom_max)
            {
                domains[first - dom_min].push_back(second);
            }
        }

        for(int i = 0; i < (dom_max - dom_min + 1); ++i)
            sort(domains[i].begin(), domains[i].end());
    }

    int dynamic_trigger_count()
    { return var_array.size() * dom_size * 2; }

    virtual AbstractConstraint* reverse_constraint()
        { abort(); }

    void blast_value(int var, int val)
    {
        int array_dom = val - dom_min;
        int array_loop = check_val1[array_dom];
D_DATA(\
        for(int i = 0; i < array_loop; ++i) \
            D_ASSERT(!var_array[i].inDomain(val)); \
);

        for(int i = array_loop; i < var_array.size(); ++i)
        {
            if(i != var)
            {
                var_array[i].removeFromDomain(val);
            }
        }
    }

#ifdef LOOP
    void update_checkval(const int dom)
    {
        const int real_dom = dom + dom_min;
        const int size = var_array.size();
        int i,j;
        for(i = check_val1[dom]; i < size; ++i)
        {
            if(var_array[i].inDomain(real_dom))
                break;
        }

        int loop_start = max(i+1, int(check_val2[dom]));
        loop_start = min(loop_start, size);
        for(j = loop_start; j < size; ++j)
        {
            if(var_array[j].inDomain(real_dom))
                break;
        }

        D_ASSERT((i == size && j == size) || (i < j));

        D_ASSERT(i == size || var_array[i].inDomain(real_dom));
        D_ASSERT(j == size || var_array[j].inDomain(real_dom));

        check_val1[dom] = i;
        check_val2[dom] = j;
    }
#endif

    /// Returns -1 for no new support.
    int find_new_support(int var, int dom)
    {
        const vector<int>& supporting = domains[dom];
        // Lost support for 'dom_value' from 'var_activated'.
        const int initial_pos = current_support[var][dom];
        int pos = initial_pos;
        int supporting_size = supporting.size();
        D_ASSERT(supporting_size > 0);
        D_ASSERT(pos >= 0 && pos <= supporting_size);
        while(pos < supporting_size && !var_array[var].inDomain(supporting[pos]))
            ++pos;

        if(pos != supporting_size)
        {
            P("." << var << "." << dom << "." << pos << "." << supporting.size() << "." << supporting[pos]);
            return pos;
        }
        pos = 0;
        while(pos <= initial_pos && !var_array[var].inDomain(supporting[pos]))
            ++pos;

        if(pos <= initial_pos)
        {
            P("!" << var << "." << dom << "." << pos << "." << supporting.size() << "." << supporting[pos]);
            return pos;
        }
        else
        {
            return -1;
        }
    }

    virtual void propagate(DynamicTrigger* dt)
    {
        PROP_INFO_ADDONE(Clique);
        int trigger_activated = dt - dynamic_trigger_start();

        const int var_activated = trigger_activated / dom_size;
        const int dom_value = trigger_activated % dom_size;

#ifdef LOOP
        update_checkval(dom_value);
        if(check_val1[dom_value] == var_array.size())
        {
            PROP_INFO_ADDONE(Counter2);
            //releaseTrigger(stateObj, dt BT_CALL_BACKTRACK);
            return;
        }
        if(check_val2[dom_value] == var_array.size() && check_val1[dom_value] == var_activated)
        {
            PROP_INFO_ADDONE(Counter3);
            //releaseTrigger(stateObj, dt BT_CALL_BACKTRACK);
            return;
        }        
#endif
        // Lost support for 'dom_value' from 'var_activated'.
        int pos = find_new_support(var_activated, dom_value);
        
        P(":" << var_activated << "." << dom_value << "." << pos);

        if(pos != -1)
        {
            current_support[var_activated][dom_value] = pos;
            var_array[var_activated].addDynamicTrigger(dt, DomainRemoval, domains[dom_value][pos]);// BT_CALL_STORE);
            return;
        }
        else
        {
            PROP_INFO_ADDONE(Counter1);

            blast_value(var_activated, dom_value + dom_min);
#ifdef LOOP
            check_val1[dom_value] = var_activated;
            check_val2[dom_value] = var_array.size();
#endif
        }
    }

    virtual void full_propagate()
    {
        DynamicTrigger* dt = dynamic_trigger_start();

        for(int j = 0; j < dom_size; ++j)
        {
            const vector<int>& supporting = domains[j];
            if(supporting.size() == 0)
            {
                for(int i = 0; i < var_array.size(); ++i)
                {
                    var_array[i].removeFromDomain(j + dom_min);
                }
            }
            else
            {
                for(int i = 0; i < var_array.size(); ++i)
                    propagate(dt + i*dom_size + j);
            }
        }
    }

    virtual BOOL check_assignment(DomainInt* v, int v_size)
    {
        D_ASSERT(v_size == var_array.size());
        int array_size = v_size;
        for(int i=0;i<array_size;i++)
            for( int j=i+1;j<array_size;j++)
            {
                const vector<int>& vals = domains[v[i] - dom_min];
                if(find(vals.begin(), vals.end(), v[j]) == vals.end())
                    return false;
            }
        return true;
    }

    virtual vector<AnyVarRef> get_vars()
    {
        vector<AnyVarRef> vars;
        vars.reserve(var_array.size());
        for(unsigned i = 0; i < var_array.size(); ++i)
            vars.push_back(var_array[i]);
        return vars;
    }

    virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
        { abort(); }

};

template<typename VarArray>
AbstractConstraint*
BuildCT_CLIQUE(StateObj* stateObj, const VarArray& var_array, ConstraintBlob& b)
{ return new CliqueConstraint<VarArray>(stateObj, var_array, b.tuples); }

#endif
