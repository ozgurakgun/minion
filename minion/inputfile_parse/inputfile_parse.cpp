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

#include "ParsingObject.hpp"

template<typename Reader, typename Stream>
    void ReadCSP(Reader& reader, ConcreteFileReader<Stream>* infile)
{
    reader.read(infile) ;
    getTableOut().set(string("Filename"), infile->filename);  
}

template<typename InputReader>
CSPInstance readInput(InputReader* infile, bool parser_verbose)
{  
    string test_name = infile->get_string();
    if(test_name != "MINION")
      INPUT_ERROR("All Minion input files must begin 'MINION'");
  
    int inputFileVersionNumber = infile->read_num();
  
    if(inputFileVersionNumber > 3)
      INPUT_ERROR("This version of Minion only supports formats up to 3");
  

    // C++0x comment : Need MOVE (which is std::move) here to activate r-value references.
    // Normally we wouldn't, but here the compiler can't figure out it can "steal" instance.
    if(inputFileVersionNumber == 3)
    {
      MinionThreeInputReader<InputReader> reader(parser_verbose);
      ReadCSP(reader, infile);
      return MOVE(reader.instance);
    } 
    else
    {
      MinionInputReader<InputReader> reader(parser_verbose);
      ReadCSP(reader, infile);
      return MOVE(reader.instance);
    }  
}

CSPInstance readInputFromStream(ParsingObject& parse_obj, string fname, bool parser_verbose)
{
    ConcreteFileReader<filtering_istream> infile(parse_obj.in, fname.c_str());

    if (infile.failed_open() || infile.eof()) {
        INPUT_ERROR("Can't open given input file '" + fname + "'.");
    }   

    try
    {
        return readInput(&infile, parser_verbose);
          // delete file;
    }
    catch(parse_exception s)
    {
        cerr << "Error in input!" << endl;
        cerr << s.what() << endl;

        cerr << "Error occurred on line " << parse_obj.e_count.lines_prev << ", around character " << parse_obj.e_count.chars_prev << endl;
    #ifdef GET_STRING         
        cerr << "The parser gave up around: '" << parse_obj.e_count.current_line_prev << "'" << endl;
    #endif
        exit(1);
    }

}

CSPInstance readInputFromFile(string fname, bool parser_verbose)
{
    ParsingObject parse_obj(fname, parser_verbose);
    return readInputFromStream(parse_obj, fname, parser_verbose);
}

