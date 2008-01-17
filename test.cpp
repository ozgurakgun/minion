#include<iostream>
#include<fstream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>

#define GAP_DIR "/Users/caj/gap4r4/bin/"
using namespace std;


class GAPInterface
{
  // Write to 1, read from 0.
  int tube[2];
  
  // 'stdin' for GAP.
  FILE* prog_input;

  string pipe_to_gap_name;
  string pipe_from_gap_name;
  
  void parent(pid_t pid)
  {
     close(tube[0]); // Close read end of pipe
    if( (prog_input = fdopen(tube[1], "w")) == NULL)
     {
       cout << "Fatal error turning GAP's stdin into a FILE*" << endl;
       exit(1);
     }
     
     int pid_val = pid;
     
     char string_buffer[100] = "TEST";
     
     // TODO: Put PID into string_buffer.
     
     // XXX : This could cause problems...
     pipe_to_gap_name = string("/tmp/pipe_to_gap.") + string_buffer;
     pipe_from_gap_name = string("/tmp/pipe_from_gap.") + string_buffer;
     
     send_string("in_filename := \"" + pipe_to_gap_name + "\";;\n");
		 send_string("out_filename := \"" + pipe_from_gap_name + "\";;\n");     
     
     mkfifo(pipe_to_gap_name.c_str(), S_IREAD | S_IWRITE);
     mkfifo(pipe_from_gap_name.c_str(), S_IREAD | S_IWRITE);
     
     send_string("while true do Read(in_filename); od;\n");
  }
  
   // Sends a string via 'stdin'
 void send_string(string s)
  {
   fprintf(prog_input, s.c_str());
   fflush(prog_input);
   //fclose(prog_input);
   cout << "Sent '" << s << "'" << endl;
  }
  
  
	void child()
	{
		close(tube[1]); // Close write end of pipe
		dup2(tube[0], fileno(stdin) ); // stick pipe onto stdin
		close(tube[0]); // close loose pipe
		cout << "Running GAP!" << endl;
		execlp("bash", "bash", "--", GAP_DIR "gap.sh","-L", GAP_DIR "wsgap4", "-q","-A","-r","-b","-n","-T",NULL);
		// Should never get here!
		cout << "Fatal error opening external program" << endl;
		_exit(1); // Don't do exit, parent will do that
	}


public:

  void gapCommand(string s)
  {
		ofstream toGap(pipe_to_gap_name.c_str());
		cout << "Debug: '" << s << "'" << endl;
		toGap << s << ";" << endl;
		toGap.close(); // flushes
  }
  
  string gapQuery(string s)
  {
     gapCommand("outputFile := OutputTextFile(out_filename, true);\n" 
     						"SetPrintFormattingStatus(outputFile, false);\n"
                "PrintTo(outputFile, " + s + ");\n" 
                "PrintTo(outputFile, \"\\n\");\n"
                "CloseStream(outputFile);\n");
     
     ifstream toGap(pipe_from_gap_name.c_str());
     cout << "opened " + pipe_from_gap_name << endl;
     string in;
     cout << "Grabbing string" << endl;
     toGap >> in;
     cout << "Got string" << endl;
     return in;
  }

  
  GAPInterface()
  {
		if(pipe(tube) == -1)
		{  cerr << "Error" << endl; exit(1); }
			
		pid_t pid = fork();
		if(pid == -1)
		{
			 cerr << "Fatal error. Cannot fork!" << endl;
			 exit(1);
		}
		else if(pid == 0)
			child();
		else
			parent(pid);	
	}

     
};


int main(void)
{
  GAPInterface c;
//  c.gapCommand("Print(2+2)");
//  c.gapCommand("Print(Factorial(100))");
  cout.flush();
  cout << "Returned Value: " << c.gapQuery("2+2") << endl;
  cout << "Returned Value: " << c.gapQuery("Factorial(1000)") << endl;
  cout.flush();
   wait(NULL);
   exit(0);

}