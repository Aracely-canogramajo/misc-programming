// $Id: cxid.cpp,v 1.2 2020-11-29 12:38:28-08 - - $

#include <iostream>
#include <fstream> 
#include <memory>
#include <string>
#include <vector>
using namespace std;

#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#include "protocol.h"
#include "logstream.h"
#include "sockets.h"

logstream outlog (cout);
struct cxi_exit: public exception {};

void reply_ls (accepted_socket& client_sock, cxi_header& header) {
   const char* ls_cmd = "ls -l 2>&1";
   FILE* ls_pipe = popen (ls_cmd, "r");
   if (ls_pipe == NULL) { 
      outlog << ls_cmd << ": " << strerror (errno) << endl;
      header.command = cxi_command::NAK;
      header.nbytes = htonl (errno);
      send_packet (client_sock, &header, sizeof header);
      return;
   }
   string ls_output;
   char buffer[0x1000];
   for (;;) {
      char* rc = fgets (buffer, sizeof buffer, ls_pipe);
      if (rc == nullptr) break;
      ls_output.append (buffer);
   }
   int status = pclose (ls_pipe);
   if(status < 0) outlog << ls_cmd << ": " << strerror (errno) << endl;
              else outlog << ls_cmd << ": exit " << (status >> 8)
                          << " signal " << (status & 0x7F)
                          << " core " << (status >> 7 & 1) << endl;
   header.command = cxi_command::LSOUT;
   header.nbytes = htonl (ls_output.size());
   memset (header.filename, 0, FILENAME_SIZE);
   send_packet (client_sock, &header, sizeof header);
   send_packet (client_sock, ls_output.c_str(), ls_output.size());
}

void reply_put(accepted_socket& client_sock, cxi_header& header){ 
  auto buffer = make_unique<char[]>(header.nbytes+1); //char array
  ofstream file(header.filename, ofstream::binary);//output file stream
  if (file.fail()){
    header.command = cxi_command::NAK;//change cxi command to NAK
    header.nbytes = htonl (errno); //send errno back in header
  }else{
    header.command = cxi_command::ACK; //change command
    recv_packet(client_sock,buffer.get(),header.nbytes);//recv buffer
    buffer[header.nbytes] = '\0';
    file.write(buffer.get(), header.nbytes); //write to buffer
    file.close();
  }
  send_packet (client_sock, &header, sizeof header);//send back header
}

void reply_rm(accepted_socket& client_sock, cxi_header& header){
  auto rc = unlink(header.filename); //unlink file
  if(rc == 0){ //success
    header.command = cxi_command::ACK;//set ack
  }else{ //failure
    header.command = cxi_command::NAK;//set nak,print error in cxi
    header.nbytes = htonl (errno); //send errno back in header
  }
  send_packet(client_sock, &header, sizeof header);//send back header
}

void reply_get(accepted_socket& client_sock, cxi_header& header){
  ifstream file(header.filename, ifstream::binary);
  if(file.fail()){
    header.command = cxi_command::NAK;
    header.nbytes = htonl (errno);
    send_packet (client_sock, &header, sizeof header);//send header
  }else{
    header.command = cxi_command::ACK;
    file.seekg (0, file.end);
    uint32_t length = file.tellg();
    file.seekg (0, file.beg);
    char * buffer = new char [length];
    header.nbytes = length; //save length of file into header;
    file.read (buffer,length);// read data as a block:
    send_packet (client_sock, &header, sizeof header);//send header
    send_packet(client_sock, buffer, length); //send the file
    file.close();
    delete[] buffer;
  }
}




void run_server (accepted_socket& client_sock) {
   outlog.execname (outlog.execname() + "-server");
   outlog << "connected to " << to_string (client_sock) << endl;
   try {   
      for (;;) {
         cxi_header header; 
         recv_packet (client_sock, &header, sizeof header);
         switch (header.command) {
            case cxi_command::LS: 
               reply_ls (client_sock, header);
               break;
            case cxi_command::PUT:
                reply_put(client_sock, header);
                break;
            case cxi_command::RM:
                reply_rm(client_sock, header);
                break;
            case cxi_command::GET:
                reply_get(client_sock, header);
                break;
            default:
               outlog << "invalid client header:" << header << endl;
               break;
         }
      }
   }catch (socket_error& error) {
      outlog << error.what() << endl;
   }catch (cxi_exit& error) {
      outlog << "caught cxi_exit" << endl;
   }
   outlog << "finishing" << endl;
   throw cxi_exit();
}

void fork_cxiserver (server_socket& server, accepted_socket& accept) {
   pid_t pid = fork();
   if (pid == 0) { // child
      server.close();
      run_server (accept);
      throw cxi_exit();
   }else {
      accept.close();
      if (pid < 0) {
         outlog << "fork failed: " << strerror (errno) << endl;
      }else {
         outlog << "forked cxiserver pid " << pid << endl;
      }
   }
}


void reap_zombies() {
   for (;;) {
      int status;
      pid_t child = waitpid (-1, &status, WNOHANG);
      if (child <= 0) break;
      outlog << "child " << child
             << " exit " << (status >> 8)
             << " signal " << (status & 0x7F)
             << " core " << (status >> 7 & 1) << endl;
   }
}

void signal_handler (int signal) {
   outlog << "signal_handler: caught " << strsignal (signal) << endl;
   reap_zombies();
}

void signal_action (int signal, void (*handler) (int)) {
   struct sigaction action;
   action.sa_handler = handler;
   sigfillset (&action.sa_mask);
   action.sa_flags = 0;
   int rc = sigaction (signal, &action, nullptr);
   if (rc < 0) outlog << "sigaction " << strsignal (signal)
                      << " failed: " << strerror (errno) << endl;
}


int main (int argc, char** argv) {
   outlog.execname (basename (argv[0]));
   outlog << "starting" << endl;
   vector<string> args (&argv[1], &argv[argc]);
   signal_action (SIGCHLD, signal_handler);
   in_port_t port = get_cxi_server_port (args, 0);
   try {
      server_socket listener (port);
      for (;;) {
         outlog << to_string (hostinfo()) << " accepting port "
             << to_string (port) << endl;
         accepted_socket client_sock;
         for (;;) {
            try {
               listener.accept (client_sock);
               break;
            }catch (socket_sys_error& error) {
               switch (error.sys_errno) {
                  case EINTR:
                     outlog << "listener.accept caught "
                         << strerror (EINTR) << endl;
                     break;
                  default:
                     throw;
               }
            }
         }
         outlog << "accepted " << to_string (client_sock) << endl;
         try {
            fork_cxiserver (listener, client_sock);
            reap_zombies();
         }catch (socket_error& error) {
            outlog << error.what() << endl;
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

