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

#define INTTYPE DomainInt

typedef INTTYPE node_number_type;  // make type_maximum consistent with this
static const int type_bits = sizeof(INTTYPE) * CHAR_BIT;
//static const node_number_type node_number_maximum = numeric_limits<INTTYPE>::max();
static const node_number_type node_number_maximum = INT_MAX;

typedef union {
	node_number_type  depth;
        node_number_type  cert;
} ARRAY_ELEMENT;


class BacktrackableMonotonicSet
{
	static const int depth_bits = type_bits;

	static const int certificate_bits = type_bits - depth_bits;

	static const node_number_type search_max_depth = node_number_maximum;

	static const node_number_type max_certificate = node_number_maximum;

	static const node_number_type bms_bottom = 0;
	// bms_bottom is required to equate to depth 0 when masked
	static const node_number_type bms_top = node_number_maximum;

	long _num_sweeps;

	node_number_type absolute_max_depth ;

	// Depth could be quite large and C++ only guarantees int goes up to 32767

	Reversible<node_number_type> 	_backtrack_depth;
	node_number_type		_local_depth;
	node_number_type		_visited_max_depth;	 // only used for print state

	node_number_type	_max_depth;
	DomainInt			_size;
	node_number_type	_certificate ;



	Reversible<node_number_type> _node_number; // i.e. node number for node where we are.


	MemOffset _array;
	MemOffset _depth_numbers;

public:

	// following allows external types destructive changes to the array

	ARRAY_ELEMENT& array(DomainInt i) const
	{
	        D_ASSERT( i >= 0 && i < _size*2);
	        int val = checked_cast<int>(i);
	        return static_cast<ARRAY_ELEMENT*>(_array.get_ptr())[val];
	}

	node_number_type& depth_numbers(DomainInt i) const
	{
	        D_ASSERT( i >= 0 && i < _max_depth);
	        int val = checked_cast<int>(i);
	        return static_cast<node_number_type*>(_depth_numbers.get_ptr())[val];
	}

	bool ifMember_remove(DomainInt index)
	// returns 1 if index was a member (and is not now)
	// returns 0 if index was not a member so no removal necessary
	{

		ARRAY_ELEMENT* array_ptr = static_cast<ARRAY_ELEMENT*>(_array.get_ptr());
		DomainInt first = index*2;
		node_number_type depth = array_ptr[first].depth;

#ifdef BLANKCERTS
		if  ( array_ptr[first+1].cert == bms_bottom || array_ptr[first+1].cert != depth_numbers(depth) )
#else
		if  (array_ptr[first+1].cert != depth_numbers(depth) )
#endif
				{
				        array_ptr[first].depth = _backtrack_depth;
				        array_ptr[first+1].cert = _node_number;
				        return 1;
				}
			{
				return 0;
			}

	}

	bool isMember(DomainInt index) const
	{
	        DomainInt first = index*2;
	        ARRAY_ELEMENT* array_ptr = static_cast<ARRAY_ELEMENT*>(_array.get_ptr());
#ifdef BLANKCERTS
	        if(array_ptr[first + 1].cert == bms_bottom)
	        return true;

	        node_number_type depth = array_ptr[first];
	        if(array_ptr[first + 1].cert != depth_numbers(depth))
	        {
		        array_ptr[first + 1].cert = bms_bottom;
			 return true;
		 }
		 else
			 return false;
#else
			        node_number_type depth = array_ptr[first].depth;
			        return (bool) ( array_ptr[first+1].cert != depth_numbers(depth) ) ;
#endif
			
	}

	node_number_type compute_node_number()
		{
			return (_certificate);
		}

	void before_branch_left() { return ; }

	void after_branch_left()
	{

		++_backtrack_depth;
		++_local_depth;
#ifdef DEBUG
		// only used for print_state
		if (_local_depth > _visited_max_depth)
		{ _visited_max_depth = _local_depth;}
		//
#endif
		++_certificate;
		_node_number = compute_node_number();
		depth_numbers(_backtrack_depth) = _node_number;

		D_ASSERT(_backtrack_depth < search_max_depth);
		D_ASSERT(_certificate < max_certificate) ; // replace with sweep;

		D_ASSERT(_node_number > bms_bottom);
#ifdef DEBUG
		cout << "branch left" << endl; print_state();
#endif
	}

	void before_branch_right() { return ; }

	void after_branch_right()
	{
		// This does nothing so presumably gets optimised away
		// completely

		// _backtrack_depth is backtracked automatically
		// and so is _node_number

		D_ASSERT( _node_number == depth_numbers(_backtrack_depth));

#ifdef DEBUG
		cout << "branch right" << endl; print_state();
#endif
		D_ASSERT(_node_number > bms_bottom);
	}

	node_number_type num_sweeps() { return _num_sweeps ; }

	bool need_to_sweep_state()
	{
		return (_certificate == (max_certificate-1));
	}

	void undo()
	{
#ifdef DEBUG
		cout << "undo called " << endl;
		print_state();
#endif

		D_ASSERT((_backtrack_depth == _local_depth) ||
		         (_backtrack_depth == (_local_depth - 1)) );

		if (_backtrack_depth == (_local_depth - 1))
		{
			depth_numbers(_local_depth) = bms_bottom;
			--_local_depth;
		}
#ifdef DEBUG
		print_state();
#endif
	}

	int size() const
	{
	        return _size;
	}

	void values_reset()
	{
		for(node_number_type i = 0; i < _max_depth; i++)
		{
			depth_numbers(i) = bms_top;
		}

		for(node_number_type j = 0; j < _size; j++)
		{
			array(j*2).depth = bms_bottom;
			array(j*2+1).cert = bms_bottom;
		}
	}

	void initialise(const DomainInt& size, const DomainInt& max_depth)
	{
		// D_ASSERT(max_depth <= search_max_depth);
		// would like to fail gracefully in this situation.
		// Or better of course not fail.

		_size = size;
		// Discard max depth
		// _max_depth = max_depth+1; // because 0 is a dummy depth

		_max_depth = 10000 ;   //// aaargh

		_array.request_bytes(2*_size*sizeof(ARRAY_ELEMENT));
		_depth_numbers.request_bytes((_max_depth+1)*sizeof(node_number_type));

		values_reset();

		_certificate = 1;	// avoid 0 = bms_bottom just in case

		_backtrack_depth = 1;
		_local_depth = 1;
		_visited_max_depth = 1;



		_node_number = compute_node_number();
		depth_numbers(_backtrack_depth) = _node_number;


		_num_sweeps = 0 ;


#ifdef DEBUG
		cout << "initialising BacktrackableMonotonicSet with value of size= " << size << endl;
		cout << " type bits: " << type_bits
		<< " bms_bottom: " << bms_bottom  ;
		cout << " max depth: " << _max_depth   ;

		print_state();
#endif
	}



	BacktrackableMonotonicSet()
	{ }



	void print_state()
	{
		cout << "printing state of BacktrackableMonotonicSet: " ;
		cout << "array size: " << _size;
		cout << " backtracking depth: " << _backtrack_depth ;
		cout << " node number: " << _node_number;
		cout << " local depth: " << _local_depth;
		cout << " numsweeps: " << _num_sweeps << endl;
		cout << endl << "   values: " ;
		for(int i = 0; i < _size; ++i)
		{
			cout << isMember(i) << " ";
		}
		cout << endl ;
		cout << "   array stored data: ";
		for(int i = 0; i < _size*2 ; ++i)
		{
			cout << array(i).cert << " " ;
		}
		cout << endl ;
		cout << "   depth stored data: ";
		for(int i = 0; i < _visited_max_depth; ++i)
		{
			cout << depth_numbers(i) << " " ;
		}
		cout << endl ;
	}


};
