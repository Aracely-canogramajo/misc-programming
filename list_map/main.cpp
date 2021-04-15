// $Id: main.cpp,v , created by ara

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <exception>
#include <unistd.h>
#include <regex>
#include <cassert>
using namespace std;

#include <libgen.h>
#include "listmap.h"
#include "xpair.h"
#include "util.h"

const string cin_name = "-";
using str_str_map = listmap<string,string>;//str_str_map isof typ lm
using str_str_pair = str_str_map::value_type;

void catfile(istream& infile,const string& filename,str_str_map& test){
  regex comment_regex {R"(^\s*(#.*)?$)"};
  regex key_value_regex {R"(^\s*(.*?)\s*=\s*(.*?)\s*$)"};
  regex trimmed_regex {R"(^\s*([^=]+?)\s*$)"};
  
  static string colons (": ");
  int linenum = 1;
  for(;;) {
    string line;
    getline (infile, line);
    if (infile.eof()) break;
    cout << filename << colons << linenum << colons << line << endl;
    ++linenum;
    smatch result;
    if (regex_search (line, result, comment_regex)){ // #
      // cout << "Comment or empty line." << endl;
      continue;
    }
    if (regex_search (line, result, key_value_regex)){ //theres a =
      if(result[1] != "" and result[2] == ""){ //key = 
        str_str_map::iterator f = test.find(result[1]);
        if(f == test.end()){
          cout << result[1] << ": key not found\n";
        }else{
          test.erase(f);
        }
      }  
      if(result[1] != "" and result[2] != ""){//key = value 
        str_str_pair pair(result[1], result[2]);
        test.insert(pair);
        cout << result[1] << " = " << result[2] << endl;
      }
      if(result[1] == "" and result[2] == ""){ // = 
        str_str_map::iterator i = test.begin();       
        for(; i != test.end(); ++i){
          cout << (*i).first << " = " <<(*i).second << "\n";
        }
      }   
      if(result[1] == "" and result[2] != ""){ // = value 
        str_str_pair x(result[1], result[2]);
        test.helper(x);
      }   
    }else if (regex_search (line, result, trimmed_regex)) {//key
      str_str_map::iterator f = test.find(result[1]);
      if(f == test.end()){
        cout << result[1] << ": Key not found\n";
      }else{
       cout << (*f).first << " = " << (*f).second << endl;
      }
    }else {
      assert (false and "This can not happen.");
    } 
  }
}


int main (int argc, char** argv) {
  str_str_map test; //create the list called map
  int status = 0;
  if ( argc > 1 ){ //read from file
    int file = 1;
    int loop = argc -1;
    while(loop > 0){ //go through all files    
     ifstream infile(argv[file]);
     if (infile.fail()) {
       status = 1;
       cerr << basename (argv[0]) << ": " << argv[file] << ": "
            << strerror (errno) << endl;
       break;
     }else {
       const string x = basename (argv[file]);
       catfile (infile, x, test);
       infile.close();
      }
      ++file;
      --loop;
    }      
  }else{ //read from cin
    const string x = "-";
    catfile(cin, x, test);
  }
  return status;
}

