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

#define INTTYPE unsigned long long

typedef INTTYPE node_number_type;  // make type_maximum consistent with this
static const int type_bits = sizeof(INTTYPE) * CHAR_BIT;

static const node_number_type node_number_maximum = ULLONG_MAX;

class BacktrackableMonotonicSet
{

	static const node_number_type branch_rate = 2;
	static const node_number_type base = branch_rate;

	static const node_number_type one = 1;

	static const node_number_type bms_bottom = 0;
	static const node_number_type bms_top = node_number_maximum;
	
	
	long _num_copies;

	node_number_type absolute_max_depth ;
	//         checked_cast<node_number_type>(floor( log(node_number_maximum)/log(base) )) - 1;
	// -1 above is for paranoia.    If changed also check below

	node_number_type largest_exponent;

	int _size;
	int _copy_depth;

	int _local_world_depth;
	ReversibleInt _world_depth;
	ReversibleInt _backtrack_depth;
	ReversibleInt _sideways_counter;
	Reversible<node_number_type> _node_number; // i.e. node number for node where we are.

	/* We could recompute _node_number on backtracking.  It's easier just to store it in BT memory.
	   It's not an obvious tradeoff but probably not important either.
	*/

	BackTrackOffsetExtra _array;

public:

	// following allows external types destructive changes to the array

	node_number_type& array(DomainInt i) const
	{
		D_ASSERT( i >= 0 && i < size());
		int val = checked_cast<int>(i);
		return static_cast<node_number_type*>(_array.get_ptr())[val];
	}


	void remove(DomainInt index)
	{
		array(index) = _node_number;
	}

	bool isMember(DomainInt index) const
	{
		/*
#ifdef DEBUG
cout << "isMember: index:" << index << ": node_number: " << _node_number 
     << " array(index)" << array(index) 
     << " Result: " << (_node_number > array(index)) << endl;
#endif
		*/
		return (bool) ( _node_number > array(index) ) ;
	}

	void branch_left()
	{
		++_backtrack_depth;
		_sideways_counter = 0;

		if (need_to_copy_state())
		{
			bms_world_push();
		}
		else
		{
			// Suppose we are working base 2, branching rate 2.
			// Want to replace a 1 in base 2 representation with a 0
			// at the relevant depth only.
			D_ASSERT( largest_exponent - _backtrack_depth >= 0)
			_node_number = _node_number - integer_exponent(base,largest_exponent-_backtrack_depth) ;
		}
		D_ASSERT(_node_number >= 0);
#ifdef DEBUG
		cout << "branch left" << endl; print_state();
#endif
	}

	void branch_right()
	{
		++_backtrack_depth;
		_sideways_counter = 1;

		D_ASSERT( _sideways_counter < branch_rate)  ;
		if (need_to_copy_state())
		{
			bms_world_push();
		}
		else
		{
			// Want to replace a 1 in base 2 with a 1
			// Of course this is a no-op
			// _node_number = _node_number - integer_exponent(base,largest_exponent-_backtrack_depth) ;
			D_ASSERT( largest_exponent - _backtrack_depth >= 0)
		}
#ifdef DEBUG
		cout << "branch right" << endl; print_state();
#endif
		D_ASSERT(_node_number > bms_bottom);
	}
	
	bool need_to_copy_state()
	{
		return (_backtrack_depth % _copy_depth) == 0;
	}
	
	void bms_world_push() 
	{
		++_local_world_depth;
		++_world_depth;
		backtrackable_memory_extra.world_push();
		++_num_copies;
		values_sweep();
	}

	void values_sweep()
	{
		node_number_type tmp ;
		for(int i=0; i< _size; i++) {
			tmp = array(i) ;
			if ((tmp == bms_top)||(tmp == bms_bottom))
			{ }	                      // nothing to do
			else if ( _node_number > tmp ) // isMember true
			{ array(i) = bms_bottom ; }
			else			      // isMember false
			{ array(i) = bms_top; }       // only case to be reversed  
		};
		reset_internals() ;
	}

	void reset_internals()
	{
		// I think of initial representation as 11111 ... 11 
		// with as many 1s as depth in the tree 
		// But this means that the number 0 can occur on the leftmost 
		// branch. 
		// To allow bms_bottom to be 0 I add 1 to the representation
		// Thus node_number starts of as 100000000 
		// with as many 0s as depth in the tree
		
		_node_number = integer_exponent(base,largest_exponent) ;
		D_ASSERT(_node_number < bms_top);
		_backtrack_depth = 1;
		_sideways_counter = 0;
	}

        node_number_type num_copies() { return _num_copies ; } 
	
	bool need_to_undo_copy()
	{
		D_ASSERT(_local_world_depth >= _world_depth);
		return _local_world_depth > _world_depth ; 
	}
	
	void undo()
	{
#ifdef DEBUG
		cout << "undo called " << endl;
		print_state();
#endif
		// undo() must be called AFTER the main world_pop.
		//        see world_pop() in solver2.h

		// Note that backtrack depth, sideways counter and node number are all
		//    restored by the main world_pop since they are ReversibleInts.
		// This function checks if this means we need to restore state with world pop
		//    Only need to pop if we are back at a depth where we copied.

		while (need_to_undo_copy())
		{
			--_local_world_depth; 
			backtrackable_memory_extra.world_pop();
#ifdef DEBUG
		cout << "undo popped to world depth " << _world_depth << endl;
		print_state();
#endif
		}
	}


	int size() const
	{
		return _size;
	}

	void initialise(const int& size)
	{

		_num_copies = 0 ;
		absolute_max_depth = type_bits-2; // would like to be smarter here
		_world_depth = 0;
		_local_world_depth = 0;
		
		//checked_cast<node_number_type>(floor( log(node_number_maximum)/log(base) )) - 1;
		// -1 above is for paranoia.    If changed also check below

		largest_exponent = absolute_max_depth + 1; // see above
		_size = size;
		_copy_depth = absolute_max_depth;
		reset_internals();
		_array.request_bytes(_size*sizeof(node_number_type));

#ifdef DEBUG
		cout << "initialising BacktrackableMonotonicSet with value of size= " << size << endl;
		cout << "type bits: " << type_bits << " bms_top: " << bms_top << " bms_bottom: " << bms_bottom << endl ;
		cout << "max depth: " << absolute_max_depth  << " largest exponent: " << largest_exponent ; 
		
		print_state();
#endif

		for(int i=0; i<size; i++) {
			array(i) = bms_bottom;
		};
	}



	BacktrackableMonotonicSet()
	{ }

	node_number_type integer_exponent(node_number_type radix, node_number_type exponent)
	{
		if (exponent < 0) { return 0; }

		node_number_type answer = 1 ;
		for(node_number_type i = 0 ; i < exponent ; i++) {
			answer *= radix;
		}
		return answer;
	}


	void print_state()
	{
		cout << "printing state of BacktrackableMonotonicSet: " ;
		cout << "array size: " << _size;
		cout << " backtracking depth: " << _backtrack_depth ;
		cout << " sideways: " << _sideways_counter ;
		cout << " node number: " << _node_number;
		cout << " copy depth: " << _copy_depth;
		cout << " numcopies: " << _num_copies;
		cout << " local world depth: " << _local_world_depth;
		cout << " world depth: " << _world_depth;
		cout << endl << "   values: " ;
		for(int i = 0; i < _size; ++i)
		{
			cout << isMember(i) << " ";
		}
		cout << endl ;
		cout << "   stored data: ";
		for(int i = 0; i < _size; ++i)
		{
			cout << array(i) << " " ;
		}
		cout << endl ;
	}


};
