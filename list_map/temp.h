// $Id: main.cpp,v 1.12 2020-10-22 16:50:08-07 - - $

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <unistd.h>
#include <regex>
#include <cassert>

using namespace std;

#include "listmap.h"
#include "xpair.h"
#include "util.h"

using str_str_map = listmap<string,string>;//str_str_map isof typ lm
using str_str_pair = str_str_map::value_type;

void scan_options (int argc, char** argv) {
   opterr = 0;
   for (;;) {
      int option = getopt (argc, argv, "@:");
      if (option == EOF) break;
      switch (option) {
         case '@':
            debugflags::setflags (optarg);
            break;
         default:
            complain() << "-" << char (optopt) << ": invalid option"
                       << endl;
            break;
      }
   }
}

int main (int argc, char** argv) {
  sys_info::execname (argv[0]);
  scan_options (argc, argv);
  str_str_map test; //create the list called map
  // cout << test << endl;
   
   
  regex comment_regex {R"(^\s*(#.*)?$)"};
  regex key_value_regex {R"(^\s*(.*?)\s*=\s*(.*?)\s*$)"};
  regex trimmed_regex {R"(^\s*([^=]+?)\s*$)"};
  
  for (;;) {
    string line;
    getline (cin, line);
    if (cin.eof()) break;
    cout << endl << "input: \"" << line << "\"" << endl;
    smatch result;
    if (regex_search (line, result, comment_regex)){ // #
      cout << "Comment or empty line." << endl;
      continue;
    }
    if (regex_search (line, result, key_value_regex)){ //theres a =
      // cout << "key  : \"" << result[1] << "\"" << endl;
      // cout << "value: \"" << result[2] << "\"" << endl;
         
      if(result[1] != "" and result[2] == ""){ //key = (done)
        // cout << "debug1: key =\n";
        str_str_map::iterator f = test.find(result[1]);
        if(f == test.end()){
          cout << result[1] << ": Key not found\n";
        }else{
          test.erase(f);
        }
      }
         
      if(result[1] != "" and result[2] != ""){//key = value (done)
        // cout << "debug2: key = value\n";
        str_str_pair pair(result[1], result[2]);
        test.insert(pair);
      }
        
      if(result[1] == "" and result[2] == ""){ // = (done)
        // cout << "debug3: =\n";
        str_str_map::iterator i = test.begin();       
        for(; i != test.end(); ++i){
          cout << (*i).first << " = " <<(*i).second << "\n";
        }
      }
         
      if(result[1] == "" and result[2] != ""){ // = value
        cout << "debug4: =value\n";
        str_str_map::iterator it = test.begin();
        
      }
         
    }else if (regex_search (line, result, trimmed_regex)) { //key (done)
      // cout << "query: \"" << result[1] << "\"" << endl;
      // cout << "debug5: key\n";
      str_str_map::iterator f = test.find(result[1]);
      if(f == test.end()){
        cout << result[1] << ": Key not found\n";
      }else{
       cout << (*f).first << " = " << (*f).second;
      }
    }else {
      assert (false and "This can not happen.");
    }
  }

  return EXIT_SUCCESS;
}

