// $Id: cxi.cpp,v 1.1 2020-11-22 16:51:43-08 - - $

#include <iostream>
#include <fstream>      // std::ifstream
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#include "protocol.h"
#include "logstream.h"
#include "sockets.h"

using wordvec = vector<string>;

logstream outlog (cout);
struct cxi_exit: public exception {};

unordered_map<string,cxi_command> command_map {
   {"exit", cxi_command::EXIT},
   {"help", cxi_command::HELP},
   {"ls"  , cxi_command::LS  },
   {"put" , cxi_command::PUT },
   {"rm"  , cxi_command::RM  },
   {"get" , cxi_command::GET },
};

static const char help[] = R"||(
exit         - Exit the program.  Equivalent to EOF.
get filename - Copy remote file to local host.
help         - Print help summary.
ls           - List names of files on remote server.
put filename - Copy local file to remote host.
rm filename  - Remove file from remote server.
)||";

void cxi_help() {
   cout << help;
}

void cxi_ls (client_socket& server) {
   cxi_header header;
   header.command = cxi_command::LS;
   send_packet (server, &header, sizeof header);
   recv_packet (server, &header, sizeof header);
   if (header.command != cxi_command::LSOUT) {
      outlog << "sent LS, server did not return LSOUT" << endl;
      outlog << "server returned " << header << endl;
   }else {
      size_t host_nbytes = ntohl (header.nbytes);
      auto buffer = make_unique<char[]> (host_nbytes + 1);
      recv_packet (server, buffer.get(), host_nbytes);
      buffer[host_nbytes] = '\0';
      cout << buffer.get();
   }
}

void cxi_put(client_socket& server, string& filename) {
  cxi_header header;
  memset(header.filename, 0, FILENAME_SIZE);//clear header b4 using
  header.command = cxi_command::PUT;//define the command(copy from ls)
  strcpy(header.filename, filename.c_str()); //cpy fn into header fn
  ifstream file(header.filename, ifstream::binary);
  if(file.fail()){
    outlog << "cxi : PUT " << strerror (errno) << endl;
  }else{
    file.seekg (0, file.end);
    uint32_t length = file.tellg();
    file.seekg (0, file.beg);
    char * buffer = new char [length];
    header.nbytes = length; //save length of file into header
    file.read (buffer,length);// read data as a block:
    send_packet (server, &header, sizeof header);//send header(from ls)
    send_packet(server, buffer, length); //send the file
    recv_packet (server, &header, sizeof header); //recieve header
    file.close();
    delete[] buffer;
  }
  if(header.command == cxi_command::NAK){
outlog<<"cxi: PUT: "<<"server returned "<<strerror(header.nbytes)<<endl;
  }
    
}

void cxi_rm(client_socket& server, string& filename){
  cxi_header header; //we only send the header
  header.command = cxi_command::RM; 
  stpcpy(header.filename, filename.c_str());
  send_packet(server, &header, sizeof header);
  recv_packet(server, &header, sizeof header);
  if(header.command == cxi_command::NAK){
outlog<<"cxi: RM: "<<"server returned "<<strerror(header.nbytes)<<endl;
  }
}

void cxi_get(client_socket& server, string& filename){
  cxi_header header; //create header
  header.command = cxi_command::GET;
  strcpy(header.filename, filename.c_str());
  send_packet(server, &header, sizeof header);
  recv_packet(server, &header, sizeof header);
  
  if(header.command == cxi_command::NAK){
  outlog<<"cxi : GET: server returned "<<strerror(header.nbytes)<<endl;
  }else{
    auto buffer = make_unique<char[]>(header.nbytes+1); //char array
    ofstream file(header.filename, ofstream::binary);//create file
    if (file.fail()){
      outlog << "cxi : GET" << strerror (errno) << endl;
      header.command = cxi_command::NAK;//change cxi command to NAK
    }else{
      header.command = cxi_command::ACK; //change command
      recv_packet (server, buffer.get(), header.nbytes);
      buffer[header.nbytes] = '\0';
      file.write(buffer.get(), header.nbytes); //write to buffer
      file.close();
    }
  }
}


//------------------------------------------------------
//HELP Functions
wordvec split (const string& line, const string& delimiters) {
   wordvec words;
   size_t end = 0;
   for (;;) {
      size_t start = line.find_first_not_of (delimiters, end);
      if (start == string::npos) break;
      end = line.find_first_of (delimiters, start);
      words.push_back (line.substr (start, end - start));
   }
   return words;
}


void usage() {
   cerr << "Usage: " << outlog.execname() << " [host] [port]" << endl;
   throw cxi_exit();
}

int main (int argc, char** argv) {
   outlog.execname (basename (argv[0]));
   outlog << "starting" << endl;
   vector<string> args (&argv[1], &argv[argc]);
   if (args.size() > 2) usage();
   string host = get_cxi_server_host (args, 0);
   in_port_t port = get_cxi_server_port (args, 1);
   outlog << to_string (hostinfo()) << endl;
   try {
      outlog << "connecting to " << host << " port " << port << endl;
      client_socket server (host, port);
      outlog << "connected to " << to_string (server) << endl;
      for (;;) {
         string line;
         getline (cin, line); // get input 
         auto fn = split(line, " "); //get filename
         if (cin.eof()) throw cxi_exit();
         const auto& itor = command_map.find (fn[0]);
         cxi_command cmd = itor == command_map.end()
                         ? cxi_command::ERROR : itor->second;
         switch (cmd) {
            case cxi_command::EXIT:
               throw cxi_exit();
               break;
            case cxi_command::HELP:
               cxi_help();
               break;
            case cxi_command::LS:
               cxi_ls (server);
               break;
            case cxi_command::PUT:
               cxi_put (server, fn[1]);
               break;
            case cxi_command::RM:
                cxi_rm(server, fn[1]);
                break;
            case cxi_command::GET:
                cxi_get(server, fn[1]);
                break;
            default:
               outlog << line << ": invalid command" << endl;
               break;
         }
      }
   }catch (socket_error& error) {
      outlog << error.what() << endl;
   }catch (cxi_exit& error) {
      outlog << "caught cxi_exit" << endl;
   }
   outlog << "finishing" << endl;
   return 0;
}

