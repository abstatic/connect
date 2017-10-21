#ifndef katclient_H
#define katclient_H

#include "base_conf.h"
#include "logger.h"


using namespace std;

/*! \class nodeClient
 *  \brief Brief class description
 *
 *  Detailed description
 */
class nodeClient
{
public:
  // Constructor and Method declarations
  nodeClient(string, string, int, string, int, int, string);
  // virtual ~KatClient(); // desturctor MUST be defined. 

  // member functions go here
  void registerFile(string); // share file details to trackr
  void downloadFile(string, string, string); // download a file from other client;
  void downloadFile(int, string); // in case search result present
  void searchFile(string); // search a file on trackr;
  void deregisterFile(string); // remove a file from trackr;
  void start(void); // start the listen server
  string sendMessage(string); // send a simple message to server
  void alive(void); // send heartbeats to trackr
  void exec_command(string, string); // execute command on server
  string execute_shell_command(const char*);
  void sendPulse(void); // sends out the heartbeat to server
  void startListen(void); // start listening foir requests on a port
  void handleClient(int); // this method handles a client request on client

  // variable declarations;
  string alias; // the client alias name
  string ip; // the ip of the client
  int port; // port number of client
  string server_ip; // the ip address of the listing server
  int server_port; // the server port on which to connect
  int down_port; // the port at which cliend will download
  string base_loc; // the location of base_folder of client
  bool haveSearchResults; // bool indicating if search results are there or not
  vector <string> search_results; // each string is a search result


  Logger* blackbox; // logger for this class
private:
  void makeBaseFolder(string base_location);
};

#endif /* ifndef KatClient_H */
