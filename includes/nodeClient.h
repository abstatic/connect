#ifndef nodeclient_h
#define nodeclient_h

#include "base_conf.h"
#include "logger.h"

using namespace std;

/*! \class nodeClient
 *  \brief Brief class description
 
 *  Detailed description
 */
class nodeClient
{
public:
  // Constructor and Method declarations
  nodeClient(string, int, string, int);

  // member functions go here
  void registerFile(string); // share file details to trackr
  void downloadFile(string, string, int); // download a file from other client;
  void downloadFile(int, string); // in case search result present
  void searchFile(string); // search a file on trackr;
  void deregisterFile(string); // remove a file from trackr;
  void start(void); // start the listen server
  string sendMessage(string, node_details*); // send a simple message to server
  void stabilize(void); // runs as a thread;
  void startListen(void); // start listening foir requests on a port
  void handleRequest(int); // this method handles a client request on client


  // chord methods
  void getFileTableInfoFromSuccessor(); //new node transfers the file info from successor
  void makeFileTableFrmResponse(string&); //populates my_filetable from string response
  node_details* lookup_ft(string);
  void join(node_details*);
  node_details* getSuccessorNode(int, node_details*);
  node_details* respToNode(string);
  void notify(node_details);
  void fix_fingers();
  void init_finger_table(node_details);

  // join node and initialize chord will be constructor
  void keep_alive();
  bool ping(node_details* n);
  node_details* find_successor(uint32_t);
  node_details* find_predecessor(uint32_t);
  node_details* closest_preceding_finger(uint32_t);

  void update_successor(node_details*);
  void update_predecessor(node_details*);
  void update_finger_table(node_details*, int);

  void remove_node(node_details*, int, node_details*);

  // remote functions
  node_details* fetch_successor(node_details*); // fetch successor of the given node
  node_details* fetch_predecessor(node_details*); // fetch predecessor of the given node
  node_details* query_successor(uint32_t, node_details*);
  node_details* query_predecessor(uint32_t, node_details*);
  node_details* query_closest_preceding_finger(uint32_t, node_details*);
  void request_update_successor(node_details*, node_details*);
  void request_update_predecessor(node_details*, node_details*);
  void request_update_finger_table(node_details*, int, node_details*);
  node_details* parse_incoming_node(int connfd); // similar to resptonode
  void request_remove_node(node_details*, int, node_details*, node_details*);


  // hashing functions
  uint32_t getNodeID(string, int);
  uint32_t getFileID(string);

  node_details fetch_query(node_details*, string);
  node_details send_request(node_details*, string);

  void print_node(node_details*);
  void bye(void);

  // variable declarations;
  string my_ip; // the ip of the client
  int my_port; // port number of client
  uint32_t my_node_id;
  bool haveSearchResults; // bool indicating if search results are there or not
  vector <string> search_results; // each string is a search result
  queue<string> requests; // queue for pending requests

  // stores int value of the hash M bits, value will be struct
  map<int, vector<file_details> > my_filetable;
  map<int, ft_struct*> my_fingertable;

  node_details* self_finger_table[KEY_SIZE];
  string self_data[KEY_SIZE]; // ??? SURE ?? 

  node_details* successor;
  node_details* predecessor;
  node_details* self;
  node_details* second_successor; // for node leaving replacement

  Logger* blackbox; // logger for this class

private:
  void makeBaseFolder(string base_location);
};

#endif /* ifndef nodeclient_h */
