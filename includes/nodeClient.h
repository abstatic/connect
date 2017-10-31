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
  void stabilize(void); // runs as a thread;
  void startListen(void); // start listening foir requests on a port
  void handleRequest(int); // this method handles a client request on client

  // chord methods
  node_details find_successor(int);
  node_details find_predecessor(int);
  node_details closest_preceding_finger(int);
  node_details lookup_ft(string);
  node_details* join(node_details);
  node_details* notify(node_details);
  void fix_fingers();
  void init_finger_table(node_details);


  int getNodeID(string, int);
  int getFileID(string);


  // variable declarations;
  string my_ip; // the ip of the client
  int my_port; // port number of client
  int my_node_id;
  bool haveSearchResults; // bool indicating if search results are there or not
  vector <string> search_results; // each string is a search result
  queue<string> requests; // queue for pending requests

  // stores int value of the hash M bits, value will be struct
  map<int, vector<file_details> > my_filetable;
  map<int, ft_struct> my_fingertable;

  node_details* successor;
  node_details* predecessor;
  node_details self;

  Logger* blackbox; // logger for this class

private:
  void makeBaseFolder(string base_location);
};

#endif /* ifndef nodeclient_h */
