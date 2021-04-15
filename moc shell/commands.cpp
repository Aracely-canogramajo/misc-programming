// $Id: commands.cpp,v 1.19 2020-10-20 18:23:13-07 - - $

#include "commands.h"
#include "debug.h"
#include "util.h"
#include "file_sys.h"
#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <exception>

using namespace std;

bool globalflag = false; //global variable, sorry not sorry

command_hash cmd_hash {
   {"cat"   , fn_cat   },
   {"cd"    , fn_cd    },
   {"echo"  , fn_echo  },
   {"exit"  , fn_exit  },
   {"ls"    , fn_ls    },
   {"lsr"   , fn_lsr   },
   {"make"  , fn_make  },
   {"mkdir" , fn_mkdir },
   {"prompt", fn_prompt},
   {"pwd"   , fn_pwd   },
   {"rm"    , fn_rm    },
   {"rmr"   , fn_rmr   },
   {"#"     , fn_comment},
};

command_fn find_command_fn (const string& cmd) {
   // Note: value_type is pair<const key_type, mapped_type>
   // So: iterator->first is key_type (string)
   // So: iterator->second is mapped_type (command_fn)
   DEBUGF ('c', "[" << cmd << "]");
   const auto result = cmd_hash.find (cmd);
   if (result == cmd_hash.end()) {
      throw command_error (cmd + ": no such function");
   }
   return result->second;
}

command_error::command_error (const string& what):
            runtime_error (what) {
}

int exit_status_message() {
   int status = exec::status();
   cout << exec::execname() << ": exit(" << status << ")" << endl;
   return status;
}

//-------------------------------------------------------------------

void fn_cat (inode_state& state, const wordvec& words){ //DONE
  string s;
  wordvec result;
  inode_ptr startcwd = state.getcwd();
  if(words.size() == 1){
    throw command_error("cat: Requires one or more pathnames");
  }else{
    int loop = words.size() - 1;
    int index = 1;
    while(loop > 0){
     map<string,inode_ptr>::iterator it;
     wordvec pathname = split(words[index], "/");
     if(pathname.size() == 1){
      it = state.getcwd()->getcontents()->getdirents().find(words[index]);
      if(it != state.getcwd()->getcontents()->getdirents().end()){
        if(state.getcwd()->getcontents()->getdirents().at(words[index])->getcontents()->get_isdir() == true){
         s += "cat: ";
         s += words[index];
         s += ": is a Directory";
         throw command_error(s);
        }else{
          result = state.getcwd()->getcontents()->getdirents().at(words[index])->getcontents()->readfile();
        }
      }
     else{
       s += "cat: ";
       s += words[index];
       s += ": No such File or Directory";
       throw command_error(s);
     }
    }else{
      inode_ptr inode = gotocwd(state, words[index], 1);
      state.setcwd(inode);
      it = state.getcwd()->getcontents()->getdirents().find(pathname.back());
      if(it != state.getcwd()->getcontents()->getdirents().end()){
        if(state.getcwd()->getcontents()->getdirents().at(pathname.back())->getcontents()->get_isdir() == true){
         s += "cat: ";
         s += words[index];
         s += ": is a Directory";
         throw command_error(s);
        }else{
          result = state.getcwd()->getcontents()->getdirents().at(pathname.back())->getcontents()->readfile();
        }
      }
      else{
       s += "cat: ";
       s += words[index];
       s += ": No such File or Directory";
       throw command_error(s);
      }
    }
    cout << result << "\n";
    index++;
    loop--;
    }
  }
 state.setcwd(startcwd);
}

void fn_cd (inode_state& state, const wordvec& words){ //DONE
string s;
  inode_ptr startcwd = state.getcwd();
  if(words.size() > 2){
    throw command_error("cd: Invalid Arguments");
  }
  else if(words.size() == 1){
    inode_ptr r = state.getroot();
    state.setcwd(state.getroot()); //goto root
  }else{
    inode_ptr inode = gotocwd(state, words[1], 0);
    if(inode->getcontents()->get_isdir() == false){
      s += "cd: ";
      s += words[1];
      s += " is a File";
      state.setcwd(startcwd);
      throw command_error(s);
    }else{
      state.setcwd(inode); 
    }
  }
}

void fn_echo (inode_state& state, const wordvec& words){ //DONE
   DEBUGF ('c', state);
   cout << word_range (words.cbegin() + 1, words.cend()) << endl;
}


void fn_exit (inode_state& state, const wordvec& words){ //DONE
  DEBUGF ('c', state);
  int exitnum;
   if(words.size() == 1){
     exitnum = 0;
   }else{
     char x = words[1][0];
     exitnum = x - '0';
     if(exitnum < 1 || exitnum > 9)
       exitnum = 127;
   }
   exec::status(exitnum);
   throw ysh_exit();
}

void fn_ls (inode_state& state, const wordvec& words){ //DONE
  inode_ptr startcwd = state.getcwd();
  map<string,inode_ptr>::iterator it;
  int loop = words.size() - 1;
  if(words.size() == 1){ // ls
    if(state.getcwd() == state.getroot()){
       cout << "/:\n";
    }else{
      cout << state.getcwd()->getcontents()->getpath() << ":\n";
    }
    for(it=state.getcwd()->getcontents()->getdirents().begin();it!=state.getcwd()->getcontents()->getdirents().end();it++){
     cout << it->second->get_inode_nr() << setw(6)
     << it->second->size() << "  "
     << it->first  // string (key)
     << std::endl ;
    }
    state.setcwd(startcwd); //goback to cwd
  }else{ // ls pathname
    int index = 1;
    while(loop > 0){ //ls on all directories inputed
      state.setcwd(startcwd);
      inode_ptr inode = gotocwd(state, words[index], 0);
      state.setcwd(inode);
      if(globalflag == true){
        globalflag = false; //set back
        throw command_error("ls: No such File or Directory");
      }else{
        if(state.getcwd() == state.getroot()){
           cout << "/:\n";
        }else{
          cout << state.getcwd()->getcontents()->getpath() << ":\n";
        }
        for(it=state.getcwd()->getcontents()->getdirents().begin();it!=state.getcwd()->getcontents()->getdirents().end();it++){
         cout << it->second->get_inode_nr() << setw(6)
         << it->second->size() << "  "
         << it->first  // string (key)
         << std::endl ;
        }
      }
      loop--;
      index++;
    }
  }
  globalflag = false;
  state.setcwd(startcwd);
}

void fn_lsr (inode_state& state, const wordvec& words){
  inode_ptr startcwd = state.getcwd();
  if(words.size() == 1){
    if(state.getcwd() == state.getroot()){
      cout << "/";
    }
    lsr_helper(state, state.getcwd());
  }else{
    inode_ptr inode = gotocwd(state, words[1], 0);
    if(globalflag == true){
      globalflag = false;
      throw command_error("lsr: No such File or Directory");
    }else{
      lsr_helper(state, inode);
    }
  }
  state.setcwd(startcwd);
}

void fn_make (inode_state& state, const wordvec& words){ //DONE
   inode_ptr startcwd = state.getcwd();
   map<string,inode_ptr>::iterator it;
   wordvec temp;
   size_t r;
   string out;
   
   if(words.size() < 2){
     throw command_error("make: Invalid Arguments");
   }else{
     wordvec path = split(words[1], "/");
     size_t x = state.getcwd()->size() + 1;
     string s;
     if(path.size() == 1){ //at cwd
       it = state.getcwd()->getcontents()->getdirents().find(words[1]);
       if(it != state.getcwd()->getcontents()->getdirents().end()){
        if(state.getcwd()->getcontents()->getdirents().at(words[1])->getcontents()->get_isdir() == false){
        state.getcwd()->getcontents()->getdirents().at(words[1])->getcontents()->writefile(words); 
        temp = state.getcwd()->getcontents()->getdirents().at(words[1])->getcontents()->readfile();
        for (size_t i = 0; i < temp.size(); ++i){
          s.append(temp.at(i).c_str());
          s.append(" ");
        }
        r = s.size() - 1;//-1 for extra space i use
        if(words.size() == 1){
          r = 0;
        }
        state.getcwd()->getcontents()->getdirents().at(words[1])->setsize(r);
       }else{
         out+= "make: ";
         out+= words[1];
         out+= ": is a Directory";
         throw command_error(out);
       }
       }
       else{ //create new file
        state.getcwd()->setsize(x); //increment size
        inode_ptr newfile=state.getcwd()->getcontents()->mkfile(words[1]);
        newfile->getcontents()->writefile(words); //input workds into file
        temp = newfile->getcontents()->readfile();
        for (size_t i = 0; i < temp.size(); ++i){ //getting size of file
          s.append(temp.at(i).c_str());
          s.append(" ");
        }
        r = s.size() - 1;//-1 for extra space i use
        newfile->setsize(r);
      }
     }else{ //goto cwd
       inode_ptr inode = gotocwd(state, words[1], 1);
       state.setcwd(inode);
       it=state.getcwd()->getcontents()->getdirents().find(path.back());
       if(it != state.getcwd()->getcontents()->getdirents().end()){
         if(state.getcwd()->getcontents()->getdirents().at(path.back())->getcontents()->get_isdir() == false){
         state.getcwd()->getcontents()->getdirents().at(path.back())->getcontents()->writefile(words); 
         temp = state.getcwd()->getcontents()->getdirents().at(path.back())->getcontents()->readfile();
         
         for (size_t i = 0; i < temp.size(); ++i){
          s.append(temp.at(i).c_str());
          s.append(" ");
        }
        r = s.size() - 1;//-1 for extra space i use
        state.getcwd()->getcontents()->getdirents().at(path.back())->setsize(r);
       }else{
         out+= "make: ";
         out+= words[1];
         out+= ": is a Directory";
         throw command_error(out);
       }
       }
       else{ //create new file
        state.getcwd()->setsize(x); //increment size 
        inode_ptr newfile=state.getcwd()->getcontents()->mkfile(words[1]);
        newfile->getcontents()->writefile(words); //input workds into file
        temp = newfile->getcontents()->readfile();
        for (size_t i = 0; i < temp.size(); ++i){
          s.append(temp.at(i).c_str());
          s.append(" ");
        }
        r = s.size() - 1;//-1 for extra space i use
        newfile->setsize(r);
       }
     }
   }
   state.setcwd(startcwd);
}

void fn_mkdir (inode_state& state, const wordvec& words){ //DONE
   inode_ptr startcwd = state.getcwd();
   map<string,inode_ptr>::iterator it;
   wordvec pathname = split(words[1], "/");
   size_t x = state.getcwd()->size() + 1;
   
   if(pathname.size() == 1){
     it = state.getcwd()->getcontents()->getdirents().find(words[1]);
     if(it != state.getcwd()->getcontents()->getdirents().end())
      throw command_error("mkdir: File or Directory already exist");
    else{
      state.getcwd()->setsize(x); //increment size
      state.getcwd()->getcontents()->mkdir(words[1]); //call mkdir
    }
   }else{
     inode_ptr inode = gotocwd(state, words[1], 1);
     state.setcwd(inode);
     it = state.getcwd()->getcontents()->getdirents().find(pathname.back());
     if(it != state.getcwd()->getcontents()->getdirents().end())
      throw command_error("mkdir: File or Directory already exists");
     else{
       if(globalflag == true){
         globalflag = false;
         throw command_error("mkdir: Invalid Directory");
       }else{
        state.getcwd()->setsize(x); //increment size 
        state.getcwd()->getcontents()->mkdir(pathname.back()); //call mkdir
       }
     }
   }
   state.setcwd(startcwd);
   
}

void fn_prompt (inode_state& state, const wordvec& words){ //DONE
   string s;
   for (size_t i = 1; i < words.size(); ++i){
     s.append(words.at(i).c_str());
     s.append(" ");
   }
   state.setprompt(s);
}

void fn_pwd (inode_state& state, const wordvec& words){//DONE
  if(state.getcwd() == state.getroot()){
    cout << "/\n";
  }else{
    cout << state.getcwd()->getcontents()->getpath() << "\n";
  } 
  DEBUGF ('c', words);
}

void fn_rm (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_rmr (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

void fn_comment (inode_state& state, const wordvec& words){ //DONE
   DEBUGF ('c', state);
   DEBUGF ('c', words);
}

// HELPER FUNCTION
//----------------------------------------------------------------
inode_ptr gotocwd(inode_state& state, const string& words, int skip){
   int flag = false;
   inode_ptr inode, temp;
   inode_ptr startcwd = state.getcwd();
   int x,i = 0;
   char path[256]; //declare char
   map<string,inode_ptr>::iterator it; //create iterator for map
   strcpy(path, words.c_str()); //convert string to char
   
   if(path[0] == '/'){flag = true;};
   wordvec pathname = split(words, "/"); //parse pathname
   if(flag){ //starts with / - start at root
     inode = state.getroot();
     if(skip == 0){
     x = pathname.size();
     }else{
       x = pathname.size() - 1; //skip the last one 
     }
     while(x!=0){
       it = inode->getcontents()->getdirents().find(pathname[i]);
       if(it == inode->getcontents()->getdirents().end()){
         globalflag = true;
         inode = startcwd;
         break;
       }
       temp = inode->getcontents()->getdirents().at(pathname[i]);
       state.setcwd(temp);
       inode = state.getcwd();
       x--;
       i++;
     }
   }else{ //starts at cwd
     inode = state.getcwd();
     if(skip == 0){
     x = pathname.size();
     }else{
       x = pathname.size() - 1; //skip the last one 
     }
     while(x!=0){
       it = inode->getcontents()->getdirents().find(pathname[i]);
       if(it == inode->getcontents()->getdirents().end()){
         globalflag = true;
         inode = startcwd;
         break;
       }
       temp = state.getcwd()->getcontents()->getdirents().at(pathname[i]);
       state.setcwd(temp);
       inode = state.getcwd();
       x--;
       i++;
     }
   }   
   return inode;
}


//--------------------------------
void lsr_helper(inode_state& state, const inode_ptr& inode){
  inode_ptr startcwd = state.getcwd();
  map<string, inode_ptr>::iterator it;
  cout << inode->getcontents()->getpath() << ":\n";
  vector<string> vec;
  for(it=inode->getcontents()->getdirents().begin();it!=inode->getcontents()->getdirents().end();it++){
    cout << it->second->get_inode_nr() << setw(6)
    << it->second->size() << "  "
    << it->first  << "\n"; 
    
    if(it->second->getcontents()->get_isdir() == true && it->first != ".." && it->first != "."){
      vec.push_back(it->second->getcontents()->getpath());
    }
  }
  
  while(vec.size() > 0){
    inode_ptr temp = gotocwd(state, vec[0], 0);
    lsr_helper(state, temp);
    vec.erase(vec.begin()); //pop off front
  }
  state.setcwd(startcwd);
}

