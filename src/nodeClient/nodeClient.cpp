/**
 * Filename: "nodeClient.cpp"
 *
 * Description: This is file contains all the method definitions of the client
 *
 * Author: Abhishek Shrivastava <abhishek.shrivastava.ts@gmail.com>
 **/
#include "../../includes/logger.h"
#include "../../includes/nodeClient.h"

using namespace std;

/**
 * Default constructor for the nodeClient
 *
 * @param c_ip        = node IP Address
 * @param c_down_port = node Download Port
 * @param c_base      = node base directory;
 * @param f_ip        = ip of the friend which will help in connecting
 * @param f_port      = port at which the friend is operating
 *
 */
nodeClient::nodeClient(string c_ip, int c_port, string f_ip="", int f_port=0)
{
  my_ip = c_ip;
  my_port = c_port;
  haveSearchResults = false;
  my_node_id = getNodeID(my_ip, my_port);

  self = new node_details;
  self->ip = my_ip;
  self->port = my_port;
  self->node_id = my_node_id;

  // TODO: add code if directory doesn't exist

  // TODO: Initialize successor, predecessor, fingertable, stablize here.

  int mod_val = pow(2, LEN*4);
  for (int i = 0; i < LEN*4; i++)
  {
    int key = my_node_id + (int) pow(2, i);// 2^i + my_node_id;

    ft_struct* n = new ft_struct;
    int sec_pair = (my_node_id + (int)pow(2, i+1) - 1) % mod_val;
    n -> interval = make_pair(key, sec_pair);
    n -> s_d = NULL;

    my_fingertable[key] = n;
  }

  if (f_ip == "")
  {
    successor = new node_details;
    successor->port = my_port;
    successor->ip = my_ip;
    successor->node_id = my_node_id;

    predecessor = new node_details;
    predecessor->port = my_port;
    predecessor->ip = my_ip;
    predecessor->node_id = my_node_id;
  }
  else
  {
    node_details* my_friend = new node_details;
    my_friend->port = f_port;
    my_friend->ip = f_ip;
    my_friend->node_id = getNodeID(f_ip, f_port);

    predecessor = NULL;
    successor = join(my_friend);

    auto it = my_fingertable.begin();

    ft_struct* n_ft = it -> second;

    n_ft -> successor = successor -> node_id;
    n_ft -> s_d = successor;
  }

  cout << my_node_id << endl;

  // if TODO base loc doesn't exits then create
  blackbox = new Logger(base_loc + "/node_log");
}

/**
 *
 * This method is a wrapper for downloading a file from another client
 *
 * Its called for request - get [2] <fileName>
 * Search results have to be present for this.
 *
 */
void nodeClient::downloadFile(int hit_no, string outfile)
{
  if (haveSearchResults)
  {
    string current_result = search_results[hit_no - 1];

    vector<string> tokens;
    tokenize(current_result, tokens, ":");

    string relative_path = tokens[1];
    string client_alias = tokens[2];

    downloadFile(relative_path);
  }
  else
  {
    cout << "FAILURE: No search results exist" << endl;
  }
}

/**
 * This method is used to download a file from a node
 *
 * @param node_ip    = IP Address of the node to which we want to connect
 * @param node_port  = Port of the node at which we want to connect
 * @param relative_path= The path of file on the node wrt node_base
 *
 * We get the node IP details from the server and then we directly
 * communicate with the given node
 *
 */
void nodeClient::downloadFile(string filepath)
{
  // TODO
  // FINGERTABLE LOOKUP TO GET THE RIGHT NODE ID
  // then get the node ip and node port

  node_details* nd = lookup_ft(filepath);
  int down_port = nd->port;
  string node_ip = nd->ip;

  int downSock = socket(AF_INET, SOCK_STREAM, 0);

  if (downSock >= 0)
  {
    // (Not necessary under windows -- it has the behaviour we want by default)
    const int trueValue = 1;
    struct timeval tv;
    tv.tv_sec = 5; // timeouts
    tv.tv_usec = 0;
    (void) setsockopt(downSock, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));
    (void) setsockopt(downSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));

    // this is the struct details on which we will try to connect
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(down_port);

    if (inet_pton(AF_INET, node_ip.c_str(), &serv_addr.sin_addr) <= 0)
    {
      cout << "Failure: Error in node IP" << endl;
      return;
    }

    if (connect(downSock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("FAILURE: Can't connect to node");
      return;
    }

    string download_req_str =  + "push`" + filepath + "`";

    int len = send(downSock, download_req_str.c_str(), strlen(download_req_str.c_str()), 0);

    bool success = true;

    // TODO get outfile name from the relative path
    string outfile_path = base_loc + "/" + "random";
    FILE * fp = fopen(outfile_path.c_str(), "w");

    if (fp)
    {
      char fileRecvBuff[1024];
      while(1)
      {
        int len = recv(downSock, fileRecvBuff, sizeof(fileRecvBuff), 0);

        if (len < 0)
        {
          // printout the error and then remoe the created file
          perror("FAILURE: Network Error");
          remove(outfile_path.c_str());
          success = false;
          break;
        }

        if (len == 0)
          break;   // sender closed connection, must be end of file

        if (fwrite(fileRecvBuff, 1, len, fp) != (size_t) len)
        {
          perror("fwrite");
          break;
        }
      }

      fclose(fp);
      if (success)
        cout << "SUCCESS: " << outfile_path << endl;
    }
    else 
      printf("FAILURE: Error, couldn't open file [%s] to receive!\n", outfile_path.c_str());

    close(downSock);
   }
  else
    perror("FAILURE: socket creation");
}

/**
 * This method is used to register a file on the chord network
 * It only creates and packs the command to a string format and passes it to
 * the sendMessage method, which sends the command to the chord network
 *
 * @param filesharepath = The path of the file which is shared
 *
 */
void nodeClient::registerFile(string fileSharePath)
{
  string cmd_name = "share";
  string command = cmd_name + "`" + fileSharePath;
  // string reply = sendMessage(command);
  // TODO
  // if (reply.find("True") != -1)
    cout << "Command sent successfully" << endl;
}

/**
 * This method is used to remove a file from the chord network
 * Only creates the commands, packs it and uses sendMessage
 *
 * @param filesharepath = complete path of file which is to be removed
 *
 */
void nodeClient::deregisterFile(string fileSharePath)
{
  string cmd_name = "del";
  string command_str = cmd_name + "`" + fileSharePath;
  // TODO
  // string reply = sendMessage(command_str);

  // if (reply.find("True") != -1)
    // cout << "Command sent successfully" << endl;
  // else
    // cout << reply << endl;
}

/**
 * This method is used to search for results on the chord network.
 * It creates the command string packs it and the sendsMesssage.
 *
 * @param file_name = Filename to be searched
 */
void nodeClient::searchFile(string file_name)
{
  string cmd_name = "search";
  string command = cmd_name + "`" + file_name + "`";
  // TODO
  // string result = sendMessage(command);

  // int file_id = getFileID(file_name);
}

/**
 * This method is used to trigger stablize on this node TODO
 */
void nodeClient::stabilize(void)
{
  while (1)
  {
    sleep(5);
    cout << "Stablize Called" << endl;
    if (successor->node_id == self->node_id)
      continue;

    string command = "find_p_of_s";//find_predecessor_of_successor

    node_details* temp =  respToNode(sendMessage(command, successor));

    if(temp->node_id > my_node_id && temp->node_id < successor->node_id)
      successor = temp;

    string cmd_str = "notify`";
    command = cmd_str + my_ip + "`"  + to_string(my_port) + "`"  + to_string(my_node_id);

    string response = sendMessage(command, successor);

    if(response == "DONE")
      cout << "Stabilize successful. " << endl;
  }
}

/***
 * Server code for client, start listening on my_port
 *
 * This method is executed as a thread. Starts listening on the given node 
 * download port. In case of accept, spawns a new thread, detaches it, and
 * then continues on to listen.
 *
 */
void nodeClient::startListen(void)
{
  int listenfd = 0;
  int connfd = 0;

  struct sockaddr_in serv_addr;

  // create a socket on listen file descriptor
  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  if (listenfd == -1) // exit the server if socket creation fails
  {
    string msg = (string)__FUNCTION__ + " ERROR: SOCKET CREATION ";
    perror(msg.c_str());
    string err = strerror(errno);
    blackbox -> record(msg + err);
    cout << "Please restart the client." << endl;
    exit(1);
  }

  // intialize the socket struct and buffers
  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET; // defines the family of socket, internet family used
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // which IP to listen on
  serv_addr.sin_port = htons(my_port); // port number to listen on

  // bind the socket now
  int res = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  // if the bind call fails
  if (res == -1)
  {
    string msg = (string)__FUNCTION__ + " ERROR: SOCKET BINDING";
    perror(msg.c_str());
    string err = strerror(errno);
    blackbox -> record(msg + err);
    cout << "Cannot bind. Please restart the client." << endl;
    exit(1);
  }

  listen(listenfd, 15);

  struct sockaddr client_addr;
  socklen_t client_len = sizeof(client_addr);

  while(1)
  {
    connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);
    thread t(&nodeClient::handleRequest, this, connfd);
    t.detach();
  }
}

/***
 * This method is handler for client side server.
 * It parses the given client command and then executes the respective method.
 *
 * @param connfd = The connected socket descriptor
 */
void nodeClient::handleRequest(int connfd)
{
  char recvBuff[1024];
  bzero(recvBuff, sizeof(recvBuff));

  int bytes_read = recv(connfd, recvBuff, sizeof(recvBuff), 0);

  if (bytes_read == 0 || bytes_read == -1)
  {
    string msg = (string)__FUNCTION__ + " ERROR";
    perror(msg.c_str());
    string err = strerror(errno);
    blackbox -> record(msg + err);
    close(connfd);
  }

  string command_string(recvBuff);

  vector<string> tokens;
  tokenize(command_string, tokens, "`");

  cout << command_string << endl;

  int cmd = interpret_command(tokens[0]);

  // TODO handle the node requests here
  if (cmd == GET)
  {
    string file_path = tokens[1];

    string full_file_path = base_loc + "/" + file_path;

    // open the file to be sent in read mode
    FILE* fp = fopen(full_file_path.c_str(), "r");
    if (fp)
    {
      char fileBuff[1024];

      while(1)
      {
        int red = fread(fileBuff, 1, sizeof(fileBuff), fp);

        // check for EOF
        if (red <= 0)
          break;

        int sent = send(connfd, fileBuff, red, 0);
        if (sent != red)
        {
           perror("send");
           break;
        }
      }

      // done close the fp
      fclose(fp);
      close(connfd);
    }
   else
     printf("Error, couldn't open file [%s] to send!\n", full_file_path.c_str());
  }
  else if (cmd == FIND_SUCCESSOR)
  {

    cout << "FIND SUCCESSOR" << endl;
    string response;

    int id = stoi(tokens[1]);
    node_details* succ = find_successor(id);

    response += succ->ip + "`" + to_string(succ->port) + "`" + to_string(succ->node_id);

    cout << "handleRequest: ";
    cout << "response string is: " << response << endl;

    // TODO HANDLE FIND SUCCESSOR
    // cout << "FIND SUCCESSOR" << endl;
    // string st = "MESSAGE RECEIVED";
    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if (cmd == GET_SUCCESSOR)
  {
    string response;

    string ret_ip = successor -> ip;
    string ret_port = to_string(successor -> port);
    string ret_id = to_string(successor -> node_id);

    response += ret_ip + "`" + ret_port + "`" + ret_id;

    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if(cmd == FIND_CPF)
  {
    int id = stoi(tokens[1]);
    node_details* cpf = closest_preceding_finger(id);


    cout << "FIND  CLOSEST_PRECEDING_FINGER" << endl;
    string response;
    string port_str = to_string(cpf->port);
    string node_id_str = to_string(cpf->node_id);

    response += cpf->ip + "`" + port_str + "`" + node_id_str;

    cout << "handleRequest: ";
    cout << "response string is: " << response << endl;

    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if(cmd == FIND_PFS)
  {
  
    cout << "FIND PREDECESSOR" << endl;
    string response;
    string port_str = to_string(predecessor->port);
    string node_id_str = to_string(predecessor->node_id);

    response += predecessor->ip + "`" + port_str + "`" + node_id_str;

    cout << "handleRequest: ";
    cout << "response string is: " << response << endl;

    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if(cmd == NOTIFY)
  {
    node_details node_to_notify ;

    node_to_notify.ip = tokens[1];
    node_to_notify.port = stoi(tokens[2]);
    node_to_notify.node_id = stoi(tokens[3]);

    notify(node_to_notify);
  }
}

/**
 * This method sends out a simple message on to the chord network
 * The node to which the message has to be sent is figured out using the
 * available fingertables.
 *
 * @param message - The message to send to the server
 * @param nd - the node pointer which contains the details about the node to
 * which we ant tot send the message
 */
string nodeClient::sendMessage(string message, node_details* nd)
{
  cout << "Sending out the message: " << message << endl;

  int node_port = nd->port;
  string node_ip = nd->ip;

  int node_sockfd = 0;

  char recvBuff[100000];

  // intialize the socket struct and buffers
  memset(recvBuff, 0, sizeof(recvBuff));

  // create a socket on listen file descriptor
  node_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // ERROR HANDLING
  if (node_sockfd == -1) // if socket creation fails
  {
    string msg = (string)__FUNCTION__ + " ERROR: SOCKET CREATION ";
    perror(msg.c_str());
    string err = strerror(errno);
    blackbox -> record(msg + err);
    cout << "Please try again" << endl;
    return "Fail";
  }

  struct sockaddr_in serv_sockaddr;
  memset(&serv_sockaddr, '0', sizeof(serv_sockaddr));

  serv_sockaddr.sin_family = AF_INET;
  serv_sockaddr.sin_port   = htons(node_port);

  int res = inet_pton(AF_INET, node_ip.c_str(), &serv_sockaddr.sin_addr);
  // ERROR HANDLING CODE
  if (res == 0)
  {
    string msg = "Can't convert server inet_pton";
    perror(msg.c_str());
    blackbox -> record(msg);
    cout << "Exiting"  << endl;
    return "Fail";
  }
  if (res == -1)
  {
    perror("Unknown error: ");
    string error = strerror(errno);
    blackbox -> record("Unknown error: " + error);
    cout << "Exiting" << endl;
    return "Fail";
  }

  res = connect(node_sockfd, (struct sockaddr*)&serv_sockaddr, sizeof(serv_sockaddr));
  // ERROR HANDLING 
  if (res == -1)
  {
    string msg = (string)__FUNCTION__ + " ERROR: SOCKET CONNECT";
    perror(msg.c_str());
    string err = strerror(errno);
    blackbox -> record(msg + err);
    cout << "Cannot Connect. Please try again." << endl;
    return "Fail";
  }

  res = send(node_sockfd, message.c_str(), message.length(), 0);
  // ERROR HANDLING
  if (res == -1)
  {
    string msg = (string)__FUNCTION__ + " ERROR: SEND";
    perror(msg.c_str());
    string err = strerror(errno);
    blackbox -> record(msg + err);
    cout << "Try again." << endl;
    return "Fail";
  }

  cout << "Waiting for reply" << endl;
  // TODO: Add the code to receive the response from peer server
  // response will contain a string containing ip, port and identifier
  int i = 0;
  int bytes_read;
  do
  {
    char c;
    bytes_read = recv(node_sockfd, &c, sizeof(char), 0);
    recvBuff[i] = c;
    i++;
  } while (bytes_read != 0);

  string s(recvBuff,0, i-1); // #Hackey DANGEROUS

  cout << "recv: " << s << endl;

  close(node_sockfd);
  return s;
}

node_details* nodeClient::lookup_ft(string command)
{
  // TODO do a Fingertable lookup and find out the correct ip address
  node_details* nd = new node_details;
  nd->ip = "RANDOM IP ADDRESS";
  nd->port = 123;
  nd->node_id = 123;
  return nd;
}

/**
 * This function hashes the given node ip and port to generate a decimal node
 * id
 *
 * Uses SHA1 hashing
 *
 * @param ip: ip address
 * @param port: port
 *
 * @return: decimal index value
 */
int nodeClient::getNodeID(string ip, int port)
{
  string op = ip + to_string(port);
  unsigned char hash[SHA_DIGEST_LENGTH];

  const unsigned char* s = reinterpret_cast<const unsigned char *>(op.c_str());
  SHA1(s,  strlen(op.c_str()), hash);

  string hex = GetHexRepresentation(hash, SHA_DIGEST_LENGTH);

  int n_id = hex2dec(hex);

  return n_id;
}

/**
 * This function hashes the file name
 * Uses SHA1 hashing
 *
 * @param fileName: fileName to be hashed
 * @return: decimal index value
 */
int nodeClient::getFileID(string fileName)
{
  string op = fileName;
  unsigned char hash[SHA_DIGEST_LENGTH];

  const unsigned char* s = reinterpret_cast<const unsigned char *>(op.c_str());
  cout << strlen(op.c_str()) << endl;
  SHA1(s,  strlen(op.c_str()), hash);

  string hex = GetHexRepresentation(hash, SHA_DIGEST_LENGTH);

  int n_id = hex2dec(hex);

  return n_id;
}

node_details* nodeClient::join(node_details* frnd)
{
  predecessor = NULL;
  node_details* ret = new node_details;

  ret = getSuccessorNode(my_node_id, frnd);

  return ret;
}

void nodeClient::notify(node_details new_node)
{
  if(predecessor == NULL || 
    (new_node.node_id > predecessor -> node_id || new_node.node_id < my_node_id))
    *predecessor = new_node;
}

void nodeClient::fix_fingers(void)
{
  while (1)
  {
    sleep(8);
    cout << "FIX FINGERS" << endl;
    node_details* temp;
    int random = rand() % (LEN*4);
    int index = pow(2,random) + my_node_id;

    auto iter = my_fingertable.find(index);

    temp = find_successor(index);

    (iter -> second)->s_d = temp;
    (iter -> second)->successor = temp->node_id;
  }
}

//convert the response string containing node info to node_details struct
node_details* nodeClient::respToNode(string response)
{
  //resonse will be in format of ip`port`node_id
  //so after tokenizing, vector tokens will be of size 3
  //tokens[0] = ip of the successor
  //tokens[1] = port of the successor
  //tokens[2] = node_id of the successor

  node_details* node = new node_details;
  vector<string> tokens;
  tokenize(response, tokens, "`");

  node->ip = tokens[0];
  node->port = stoi(tokens[1], nullptr, 10);
  node->node_id = stoi(tokens[2], nullptr, 10);

  return node;
}

//this wrapper function takes a command string (eg. "find_successor`id")
//node_id: id whose successor we want to find
//connectToNode: we want to connect to this node to find the successor of "node_id"
node_details* nodeClient::getSuccessorNode(int node_id, node_details* connectToNode)
{
  string nid = to_string(node_id);
  string command = string("find_successor`") + nid;

  cout << "The command is : " << command << endl;
  string response = sendMessage(command, connectToNode);//Parse response, populate n_dash_successor 

  cout << "response for getSuccessorNode: " << response << "\n";

  node_details *successorOfId = respToNode(response);

  return successorOfId;
}

node_details* nodeClient::find_successor(int id)
{
  node_details* n_dash = find_predecessor(id);
  //parse response and populate n_dash_successor
  // return getSuccessorNode(n_dash.node_id, n_dash); //TBD- Approaches- find n_dash successor in previous call or seperate call 
  cout << "find_predecessor("<<id<<")"<<endl;
  string command_string = "get_successor`";
  node_details* n_dash_successor = respToNode(sendMessage(command_string, n_dash));

  return n_dash_successor;
}

node_details* nodeClient::find_predecessor(int id)
{
  bool flag = false;

  // if only node in the network
  if (my_node_id == successor->node_id)
    return self;

  node_details* predNode = self;
  node_details* succOfPredNode = successor;

  while (id <= predNode->node_id || id > succOfPredNode->node_id)
  {
    if(flag == false)
    {
      predNode = closest_preceding_finger(id); //Searching in its own finger table
      flag = true;
    }
    else
    {
      // predNode = sendMessage("closest_preceding_finger(id)");// TO-DO make it RPC Call to temp finger's table
      string command = "find_cpf`" + to_string(id);
      predNode = respToNode(sendMessage(command, succOfPredNode));
    }

    //find the successor of predNode (in case predNode has changed in while loop)
    succOfPredNode = getSuccessorNode(predNode->node_id, predNode);
  }
  return predNode;
}

node_details* nodeClient::closest_preceding_finger(int id)
{
  for (auto i = my_fingertable.rbegin(); i != my_fingertable.rend(); i++)
  {
    int s_val = (i->second) -> successor;

    if (s_val > my_node_id && s_val < id)
      return i->second -> s_d;
  }
  return self;
}
