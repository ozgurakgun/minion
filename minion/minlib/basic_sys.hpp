#ifndef BASIC_SYS_CQP
#define BASIC_SYS_CQP

#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <ostream>
#include <istream>
#include <sstream>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <memory>
#include <exception>
#include <stdexcept>
#include <tuple>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>


#define SHARED_PTR std::shared_ptr
#define TUPLE std::tuple
#define TUPLE_GET std::get
#define TUPLE_TIE std::tie

/// Stores typedefs for containers.
/// The reason we do this is to templates in templates in
/// the containers, which is horrible
template<typename... Class>
struct TypeDefs;

#endif
