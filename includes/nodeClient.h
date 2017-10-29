#ifndef nodeclient_h
#define nodeclient_h

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
  nodeClient(string, int, string, int);

  // member functions go here
  void registerFile(string); // share file details to trackr
  void downloadFile(string); // download a file from other client;
  void downloadFile(int, string); // in case search result present
  void searchFile(string); // search a file on trackr;
  void deregisterFile(string); // remove a file from trackr;
  void start(void); // start the listen server
  string sendMessage(string); // send a simple message to server
  void stablize(void); // sends out the heartbeat to server
  void startListen(void); // start listening foir requests on a port
  void handleRequest(int); // this method handles a client request on client

  node_details lookup_ft(string message);

  // variable declarations;
  string my_ip; // the ip of the client
  int my_port; // port number of client
  bool haveSearchResults; // bool indicating if search results are there or not
  vector <string> search_results; // each string is a search result
  queue<string> requests; // queue for pending requests

  unordered_map<string, vector<string> > my_filetable;
  unordered_map<string, string> my_fingertable;

  Logger* blackbox; // logger for this class

private:
  void makeBaseFolder(string base_location);
};

#endif /* ifndef nodeclient_h */
