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

#include "inputfile_parse.h"

#include "MinionInputReader.hpp"
#include "MinionThreeInputReader.hpp"

#include "counter.hpp"

#include <fstream>
#include <iostream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

using boost::iostreams::filtering_istream;
using boost::iostreams::gzip_decompressor;
using boost::iostreams::bzip2_decompressor;

using boost::iostreams::error_counter;

struct ParsingObject
{
    error_counter e_count;
    filtering_istream in;
    auto_ptr<istream> file;
    
    ParsingObject(std::string fname, bool parser_verbose = false)
    {
        const char* filename = fname.c_str();
        string extension;
        if(fname.find_last_of(".") < fname.size())
          extension = fname.substr(fname.find_last_of("."), fname.size());
          
        in.push(boost::ref(e_count));

        if(extension == ".gz" || extension == ".gzip" || extension == ".z" || extension == ".gzp" ||
            extension == ".bz2" || extension == ".bz" || extension == ".bzip2" || extension == ".bzip")
        {  
            if(extension == ".gz" || extension == ".gzip" || extension == ".z" || extension == ".gzp")
            {
                if(parser_verbose)
                    cout << "# Using gzip uncompression" << endl;
                in.push(gzip_decompressor());
            }    

            if(extension == ".bz2" || extension == ".bz" || extension == ".bzip2" || extension == ".bzip")
            {
                if(parser_verbose)
                    cout << "# Using bzip2 uncompression" << endl;
                in.push(bzip2_decompressor());
            }

        }

        if(fname != "--")
        {
            file = auto_ptr<ifstream>(new ifstream(filename, ios_base::in | ios_base::binary));
            if (!(*file)) {
                INPUT_ERROR("Can't open given input file '" + fname + "'.");
            }
            in.push(*file);
        }
        else
            in.push(cin);
    }
private:
    ParsingObject(const ParsingObject&);
    ParsingObject();
};
