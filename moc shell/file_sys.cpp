// $Id: file_sys.cpp,v 1.8 2020-10-22 14:37:26-07 - - $

#include <iostream>
#include <stdexcept>
#include <unordered_map>

using namespace std;

#include "debug.h"
#include "file_sys.h"

size_t inode::next_inode_nr {1};

struct file_type_hash {
   size_t operator() (file_type type) const {
      return static_cast<size_t> (type);
   }
};

ostream& operator<< (ostream& out, file_type type) {
   static unordered_map<file_type,string,file_type_hash> hash {
      {file_type::PLAIN_TYPE, "PLAIN_TYPE"},
      {file_type::DIRECTORY_TYPE, "DIRECTORY_TYPE"},
   };
   return out << hash[type];
}

inode_state::inode_state() { //constructor for inode
   DEBUGF ('i', "root = " << root << ", cwd = " << cwd
          << ", prompt = \"" << prompt() << "\"");
  root = make_shared<inode>(file_type :: DIRECTORY_TYPE);//create root
  cwd = root; //make cwd to the root
  root->contents->getdirents().insert(pair<string, inode_ptr>(".", root));
  root->contents->getdirents().insert(pair<string, inode_ptr>("..", root));
  size_t root_num = 1;
  root->setnum(root_num);
  size_t x = 2;
  root->setsize(x); // has . & ..
  root->contents->set_isdir(true);
}

const string& inode_state::prompt() const { return prompt_; }

ostream& operator<< (ostream& out, const inode_state& state) {
   out << "inode_state: root = " << state.root
       << ", cwd = " << state.cwd;
   return out;
}

inode::inode(file_type type): inode_nr (next_inode_nr++) {
   switch (type) {
      case file_type::PLAIN_TYPE:
           contents = make_shared<plain_file>();
           break;
      case file_type::DIRECTORY_TYPE:
           contents = make_shared<directory>();
           break;
   }
   DEBUGF ('i', "inode " << inode_nr << ", type = " << type);
}

size_t inode::get_inode_nr() const {
   DEBUGF ('i', "inode = " << inode_nr);
   return inode_nr;
}


file_error::file_error (const string& what):
            runtime_error (what) {
}

//--------------------------------------------------

const wordvec& base_file::readfile() const {
   throw file_error ("is a " + error_file_type());
}

void base_file::writefile (const wordvec&) {
   throw file_error ("is a " + error_file_type());
}

void base_file::remove (const string&) {
   throw file_error ("is a " + error_file_type());
}

inode_ptr base_file::mkdir (const string&) {
   throw file_error ("is a " + error_file_type());
}

inode_ptr base_file::mkfile (const string&) {
   throw file_error ("is a " + error_file_type());
}

//----------------------------------------------------


size_t plain_file::size() const {
   size_t size = this->data.size(); //return size of vector
   DEBUGF ('i', "size = " << size);
   return size;
}

const wordvec& plain_file::readfile() const { //done
   return data;
}

void plain_file::writefile (const wordvec& words) { //done
   this->data.clear(); //clear if it has anything
   for (auto i=words.begin() + 2; i!=words.end(); ++i){
     this->data.push_back(*i);
   }
}

//----------------------------------------------------

size_t directory::size() const {
   size_t size {0};
   DEBUGF ('i', "size = " << size);
   return size;
}

void directory::remove (const string& filename) {
   DEBUGF ('i', filename);
}

inode_ptr directory::mkdir (const string& dirname) { //done
   string newpath = this->getpath() + "/" + dirname;   
   inode_ptr newdir = make_shared<inode>(file_type :: DIRECTORY_TYPE);
   newdir->getcontents()->setpath(newpath);
   newdir->getcontents()->getdirents().insert(pair<string, inode_ptr>(".", newdir));
   newdir->getcontents()->getdirents().insert(pair<string, inode_ptr>("..", this->getdirents().at(".")));
   dirents.insert(pair<string,inode_ptr>(dirname, newdir)); //add dir to dirents
   size_t x = 2; //for . and ..
   newdir->setsize(x); 
   newdir->getcontents()->set_isdir(true);
   return newdir;
}

inode_ptr directory::mkfile (const string& filename) { //done
   string newpath = this->getpath() + "/" + filename;   
   inode_ptr newfile = make_shared<inode>(file_type :: PLAIN_TYPE);
   newfile->getcontents()->setpath(newpath);   
   dirents.insert(pair<string,inode_ptr>(filename, newfile)); //add file to dirents
   newfile->getcontents()->set_isdir(false);
   return newfile;   
}
