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

  // start listening thread
  thread t(&nodeClient::startListen, this);
  t.detach();

  if (f_ip == "")
  {
    cout << "Creating a new chord ring" << endl;

    // set self for predecessor and successor
    successor = self;
    predecessor = self;
    second_successor = self;

    // set self for fingers
    for (int i = 0; i < KEY_SIZE; i++)
      self_finger_table[i] = self;

    // set data to blank
    for (int i = 0; i < KEY_SIZE; i++)
      self_data[i] = "";

    // set the shared data to blank;
  }
  else
  {
    // reconstruct the friend node
    node_details* my_friend = new node_details;
    my_friend->port = f_port;
    my_friend->ip = f_ip;
    my_friend->node_id = getNodeID(f_ip, f_port);

    join(my_friend);
  }

  cout << my_node_id << endl;

  // if TODO base loc doesn't exits then create
  blackbox = new Logger(base_loc + "/node_log");
}

void nodeClient::makeFileTableFrmResponse(string& response){
  vector<string> rows;
  tokenize(response, rows, "#");

  for(auto& x:rows){

    vector<string> fields;
    tokenize(x, fields, "`");
    //fields[0] is the id of file.

    /* fileLocations contains list of all nodes where file is stored.
     * Each row contains:
     * (1 for id of file, which is key of map entry)+(number of file_details structs)
     * number of fields, so allocating vector size accordingly.
     */
    vector<file_details> fileLocations(fields.size()-1);


    /*
     * each field from index 1 onwards contains file_details struct info
     * separated by colons. Tokenizing "fields" and populating "fileLocations" accordingly.
     */

    for(int i=1; i<fields.size(); i++){
      vector<string> structContent;
      tokenize(fields[i], structContent, ":");
      fileLocations[i-1].ip = structContent[0];
      fileLocations[i-1].port = stoi(structContent[1]);
      fileLocations[i-1].path = structContent[2];
    }

    /*
     * populate the map of filetable for this node.
     */

    int id = stoi(fields[0]);
    my_filetable[id] = fileLocations;
  }
}

void nodeClient::getFileTableInfoFromSuccessor(){
  string command = "get_file_table`";
  command += to_string(my_node_id);

  string response = sendMessage(command, successor);
  makeFileTableFrmResponse(response);
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

    string ip = tokens[0];
    int port = stoi(tokens[1]);
    string filepath = tokens[2];

    downloadFile(filepath, ip, port, outfile);
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
void nodeClient::downloadFile(string filepath, string n_ip, int n_port, string outfile)
{
  // TODO
  // FINGERTABLE LOOKUP TO GET THE RIGHT NODE ID
  // then get the node ip and node port

  int down_port = n_port;
  string node_ip = n_ip;

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

    string download_req_str =  + "pull`" + filepath + "`";

    int len = send(downSock, download_req_str.c_str(), strlen(download_req_str.c_str()), 0);

    bool success = true;

    // TODO get outfile name from the relative path
    string outfile_path = base_loc + "/" + outfile;
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
  uint32_t file_key = getFileID(fileSharePath);

  node_details* resp = find_successor(file_key);

  string cmd_name = "add";
  string command = cmd_name + "`" + to_string(file_key) + "`" + fileSharePath + "`" + to_string(self->port)  + "`" + self->ip;
  string reply = sendMessage(command, resp);
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
  uint32_t file_key = getFileID(fileSharePath);
  node_details* resp = find_successor(file_key);

  string cmd_name = "remove";

  string command = cmd_name + "`" + to_string(file_key) + "`" + fileSharePath + "`" + to_string(self->port)  + "`" + self->ip;
  string reply = sendMessage(command, resp);

  if (reply == "true")
    cout << "File Removed" << endl;
  else
    cout << "File Not Removed" << endl;
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

  uint32_t file_key = getFileID(file_name);

  node_details* resp = find_successor(file_key);

  string command = cmd_name + "`" + to_string(file_key);

  string reply = sendMessage(command, resp);

  vector<string> res;
  tokenize(reply, res, "`");
  search_results = res;

  int len = res.size();

  if (len == 0)
  {
    cout << "Found: 0" << endl;
    haveSearchResults = false;
  }
  else
  {
    haveSearchResults = true;
    cout << "FOUND: " << len << endl;
    for (int i = 0; i < len; i++)
      printf("[%d] %s\n", i+1, search_results[i].c_str());
  }
}

/**
 * This method is used to trigger stablize on this node TODO
 */
void nodeClient::stabilize(void)
{
  // while (1)
  // {
    sleep(5);
    cout << "Stablize Called" << endl;

    string command = "find_p_of_s";//find_predecessor_of_successor

    node_details* temp =  respToNode(sendMessage(command, successor));

    if(temp->node_id > my_node_id && temp->node_id < successor->node_id)
      successor = temp;

    string cmd_str = "notify`";
    command = cmd_str + my_ip + "`"  + to_string(my_port) + "`"  + to_string(my_node_id);

    string response = sendMessage(command, successor);

    cout << "Stablize: " << response << endl;
  // }
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
    else if (cmd == ADD)
    {
      cout << "ADDING FILE" << endl;

      uint32_t file_key = stoull(tokens[1]);
      string fileSharePath = tokens[2];
      int server_port = stoi(tokens[3]);
      string server_ip = tokens[4];

      file_details ft;
      ft.ip = server_ip;
      ft.port = server_port;
      ft.path = fileSharePath;
      ft.file_key = file_key;

      my_filetable[file_key].push_back(ft);

      string response = "true";
      send(connfd, response.c_str(), response.length(), 0);
      close(connfd);
    }
    else if (cmd == SEARCH)
    {
      cout << "SEARCHING FOR FILE" << endl;

      int file_key = stoi(tokens[1]);

      auto it = my_filetable.find(file_key);

      if (it != my_filetable.end())
      {
        vector<file_details> matching = it->second;

        string result;

        for (auto s : matching)
        {
          string line;

          string ip = s.ip;
          string port = to_string(s.port);
          string path = s.path;
          string file_key = to_string(s.file_key);

          line = ip + ":" + port + ":" + path + ":" + file_key + "`";

          result += line;
        }

        send(connfd, result.c_str(), result.length(), 0);
        close(connfd);
      }
      else
      {
        string response = "";
        send(connfd, response.c_str(), response.length(), 0);
        close(connfd);
      }

    }
   else if (cmd == FIND_SUCCESSOR)
   {

    cout << "FIND SUCCESSOR" << endl;
    string response;

    uint32_t id = stoull(tokens[1]);
    cout << "REQUEST FOR SUCCESSOR OF KEY: " << id << endl;
    node_details* succ = find_successor(id);

    cout << "Successor: " << endl;
    printNode(successor);

    response += succ->ip + "`" + to_string(succ->port) + "`" + to_string(succ->node_id);

    cout << "handleRequest: ";
    cout << "response string is: " << response << endl;

    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if (cmd == GET_SUCCESSOR)
  {
    cout << "Handling get successor" << endl;
    string response;

    string ret_ip = successor -> ip;
    string ret_port = to_string(successor -> port);
    string ret_id = to_string(successor -> node_id);

    response = ret_ip + "`" + ret_port + "`" + ret_id;

    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if (cmd == GET_PREDECESSOR)
  {
    cout << "Handling get predecessor" << endl;

    string response;
    string ret_ip = predecessor -> ip;
    string ret_port = to_string(predecessor -> port);
    string ret_id = to_string(predecessor -> node_id);

    response = ret_ip + "`" + ret_port + "`" + ret_id;

    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if(cmd == FIND_CPF)
  {
    cout << "FIND CLOSEST_PRECEDING_FINGER" << endl;

    uint32_t id = stoull(tokens[1]);
    node_details* cpf = closest_preceding_finger(id);

    string response;
    string port_str = to_string(cpf->port);
    string node_id_str = to_string(cpf->node_id);

    response += cpf->ip + "`" + port_str + "`" + node_id_str;

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

    string response = "NOTIFY DONE";
    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if(cmd == GET_FILE_TABLE)
  {
    int id = stoi(tokens[1]);

    //map<int, vector<file_details> > m = my_filetable;
    map<int, vector<file_details> >::iterator ii;

    string response;
    for(  ii = my_filetable.begin(); ii->first <= id; ++ii )
    {
      //cout << ii->first << ": ";
      string id_str = to_string(ii->first) + "`";
      response += id_str;

      vector<file_details>::iterator it;
      for( it = ii->second.begin(); it != ii->second.end(); ++it) {
        //cout << " " << it->ip <<" "<<  it->port <<" "<<it->path;
        string temp = it->ip + ":" + to_string(it->port) + ":" + it->path + "`";
        response += temp;
      }
      response += "#";
      cout<<endl;
    }

    //After transferring the file table, should we delete these entries from current node?
    //because now the new node will store those entries, we don't need it here
    //maybe it helps with mirroring?
  }
  else if (cmd == PING)
  {
    string response = "true";
    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if (cmd == U_SUCC)
  {
    cout << "Handling udpate successor" << endl;

    node_details* n = new node_details;

    // reconstructing the node from tokens
    // see request_update_predecessor
    n->node_id = stoull(tokens[1]);
    n->ip = tokens[2];
    n->port = stoi(tokens[3]);

    this -> successor = n; // ???????
    self_finger_table[0] = n;
    second_successor = fetch_successor(successor);

    cout << "Successor updated during update successor " << endl;
    cout << "Succ: ID " << successor->node_id << " PORT: " << successor->port << endl;

    string response = "true";
    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if (cmd == U_PRED)
  {
    cout << "Handling update predecessor" << endl;

    node_details* n = new node_details;

    // reconstructing the node from tokens
    // see request_update_predecessor
    n->node_id = stoull(tokens[1]);
    n->ip = tokens[2];
    n->port = stoi(tokens[3]);

    this -> predecessor = n; // ???????
    cout << "Predecessor updated during update predecessor " << endl;
    cout << "Pred: ID " << predecessor->node_id << " PORT: " << predecessor->port << endl;

    second_successor = fetch_successor(successor); // new added

    string response = "true";
    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
  }
  else if (cmd == U_FIN)
  {
    cout << "Handling update_finger request" << endl;

    uint32_t idx;
    node_details* t = new node_details;

    t->node_id = stoull(tokens[1]);
    t->ip = tokens[2];
    t->port = stoi(tokens[3]);

    idx = stoi(tokens[4]);

    update_finger_table(t, idx);

    string response = "true";
    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
    cout << "Updated finger" << endl;
  }
  else if (cmd == R_NODE)
  {
    cout << "Handling remove node" << endl;

    node_details* old = new node_details;
    node_details* replace = new node_details;
    int i;

    old->node_id = stoull(tokens[1]);
    old->ip = tokens[2];
    old->port = stoi(tokens[3]);

    cout << "INDEX: " << i << endl;
    cout << "OLD: ";
    printNode(old);
    cout << "REPLACE: ";
    printNode(replace);

    i = stoi(tokens[4]);

    replace->node_id = stoull(tokens[5]);
    replace->ip = tokens[6];
    replace->port = stoi(tokens[7]);

    remove_node(old, i, replace);

    string response = "true";
    send(connfd, response.c_str(), response.length(), 0);
    close(connfd);
    cout << "DONE REMOVING NODE" << endl;
  }
  else if (cmd == TRANSFER)
  {
    cout << "Handling transfer of files due to new node" << endl;

    uint32_t preda = stoull(tokens[1]);
    uint32_t predb = stoull(tokens[2]);

    // iterate over the file table and find is_between entries
    string reply = "";

    for (auto it = my_filetable.cbegin(); it != my_filetable.cend();)
    {
      int key = it->first;

      if (is_between(key, preda, predb))
      {
        // add the file details to the response string and remove it from
        // my_filetable
        vector<file_details> filetable = it->second;
        for (auto mirror : filetable)
        {
          string thisFile = to_string(key) + ":" + mirror.ip + ":" + to_string(mirror.port) + ":" + mirror.path + "`";
          reply += thisFile;
        }
        it = my_filetable.erase(it);
      }
      else
        ++it;
    }

    send(connfd, reply.c_str(), reply.length(), 0);
    close(connfd);
    cout << "DATA SENT" << endl;
  }
  else if (cmd == DEL)
  {
    int key = stoi(tokens[1]);
    string fsp = tokens[2];
    int port = stoi(tokens[3]);
    string ip = tokens[4];

    string ret_val = "false";
    // iterate over the file table to find a matching entry;
    if (my_filetable.find(key) != my_filetable.end())
    {
      vector<file_details> files = my_filetable[key];

      for (auto it = files.begin(); it != files.end(); )
      {
        file_details ft = *it;
        if (ft.port == port && ft.ip == ip)
        {
          files.erase(it);
          ret_val = "true";
          break;
        }
        else
          it++;
      }
      my_filetable[key] = files;
    }
    else
      ret_val = "false";

    send(connfd, ret_val.c_str(), ret_val.length(), 0);
    close(connfd);
  }
  else if (cmd == ADDF)
  {
    string files_string = tokens[1];

    vector<string> files;
    tokenize(files_string, files, "#");

    for (auto f : files)
    {
      vector<string> f_d;
      tokenize(f, f_d, ":");

      int key = stoi(f_d[0]);

      int f_key = stoull(f_d[0]);
      string ip = f_d[1];
      int port = stoi(f_d[2]);
      string path = f_d[3];

      file_details ft;
      ft.ip = ip;
      ft.port = port;
      ft.file_key = f_key;
      ft.path = path;

      my_filetable[key].push_back(ft);
    }

    string ret = "done";
    send(connfd, ret.c_str(), ret.length(), 0);
    close(connfd);
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
  // cout << "Sending out the message: " << message << endl;

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
uint32_t nodeClient::getNodeID(string ip, int port)
{
  string op = ip + ":"  + to_string(port);
  unsigned char hash[SHA_DIGEST_LENGTH];

  const unsigned char* s = reinterpret_cast<const unsigned char *>(op.c_str());
  SHA1(s,  strlen(op.c_str()), hash);

  string hex = GetHexRepresentation(hash, SHA_DIGEST_LENGTH);

  uint32_t n_id = hex2dec(hex);

  return n_id;
}

/**
 * This function hashes the file name
 * Uses SHA1 hashing
 *
 * @param fileName: fileName to be hashed
 * @return: decimal index value
 */
uint32_t nodeClient::getFileID(string fileName)
{
  string op = fileName;
  unsigned char hash[SHA_DIGEST_LENGTH];

  const unsigned char* s = reinterpret_cast<const unsigned char *>(op.c_str());
  SHA1(s,  strlen(op.c_str()), hash);

  string hex = GetHexRepresentation(hash, SHA_DIGEST_LENGTH);

  uint32_t f_id = hex2dec(hex);

  return f_id;
}

void nodeClient::join(node_details* frnd)
{
  // set data to blank
  for (int i = 0; i < KEY_SIZE; i++)
    self_data[i] = "";

  // initalize the predecessor and successor
  successor = query_successor(self -> node_id, frnd);
  self_finger_table[0] = successor;

  cout << "Successor: " << endl;
  printNode(successor);

  predecessor = fetch_predecessor(successor);
  cout << "Predecessor: " << endl;
  printNode(predecessor);

  // TODO error ? should be successor ?
  request_update_successor(self, predecessor);
  request_update_predecessor(self, successor);

  second_successor = fetch_successor(successor);

  // initialize the finger table
  int i = 0;
  int product = 1;
  for (i = 1; i < KEY_SIZE; i++) // n + 2^i , where i: 0->m
  {
    uint32_t start_key = (self->node_id + product) % KEY_SPACE;

    if (is_between(start_key, self->node_id, self_finger_table[i-1]->node_id - 1))
      self_finger_table[i] = self_finger_table[i-1];
    else
    {
      self_finger_table[i] = query_successor(start_key, frnd);

      if (!is_between(self_finger_table[i]->node_id, start_key, self->node_id))
        self_finger_table[i] = self;
    }

    product = product * 2;

    cout << "finger " << i << endl;
    printNode(self_finger_table[i]);
  }

  cout << "UPDATING OTHER NODES TOO" << endl;
  // update other nodes too!
  product = 1;
  // for (int i = 0; i < KEY_SIZE; i++)
  // {
    // node_details* p = find_predecessor(self->node_id - product);
    // printNode(p);
    // request_update_finger_table(self, i, p);
    // product = product * 2;
  // }

  string cmd = "transfer`" + to_string(predecessor->node_id) + "`" + to_string(successor->node_id);
  string reply = sendMessage(cmd.c_str(), successor);

  // TODO iterate over the transferred files and store them into own FT
  vector<string> files;
  tokenize(reply, files, "`");

  for (auto file : files)
  {
    vector<string> fileDetails;
    tokenize(file,  fileDetails, ":");

    int key = stoi(fileDetails[0]);
    uint32_t ukey = stoull(fileDetails[0]);
    string ip = fileDetails[1];
    int port = stoi(fileDetails[2]);
    string path = fileDetails[3];

    file_details ft;
    ft.ip = ip;
    ft.port = port;
    ft.path = path;
    ft.file_key = ukey;

    my_filetable[key].push_back(ft);
  }

  cout << "Joining the chord ring" << endl;
  cout << "ID: " << self->node_id;
  cout << "Predecessor: " << predecessor->node_id;
  cout << "Successor: " << successor->node_id;
}

void nodeClient::notify(node_details new_node)
{
  if(predecessor == NULL || (new_node.node_id > predecessor -> node_id || new_node.node_id < my_node_id))
    *predecessor = new_node;
}

void nodeClient::fix_fingers(void)
{
  // while (1)
  // {
    sleep(8);
    cout << "FIX FINGERS" << endl;
    node_details* temp;
    int random = rand() % (LEN*4);
    int index = pow(2,random) + my_node_id;

    auto iter = my_fingertable.find(index);

    temp = find_successor(index);

    (iter -> second)->s_d = temp;
    (iter -> second)->successor = temp->node_id;
  // }
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
  node->node_id = stoull(tokens[2], nullptr, 10);

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


// successor of the given node id, will be the successor of this node's
// predecessor
node_details* nodeClient::find_successor(uint32_t id)
{
  node_details* n_dash = find_predecessor(id);

  cout << "find_predecessor("<<id<<")"<<endl;

  string command_string = "get_successor`";
  node_details* n_dash_successor = respToNode(sendMessage(command_string, n_dash));

  return n_dash_successor;
}

node_details* nodeClient::find_predecessor(uint32_t id)
{
  // if only one node
  if (self->node_id == successor->node_id)
    return self;

  node_details* n = self;
  node_details* suc = successor;

 // while (!is_between(id, n->node_id + 1, suc->node_id) && id != suc -> node_id)
 // if(is_between(id, n->node_id+1, suc->node_id)) 
  // {
    node_details* n_prime = query_closest_preceding_finger(id, n);

    n = n_prime;
    // suc = fetch_successor(n);
  // }

  return n;
}

node_details* nodeClient::closest_preceding_finger(uint32_t id)
{
  node_details* temp1 = successor;
  node_details* temp2 = self;

  // int i;
  // for (int i = KEY_SIZE - 1; i >= 0; i--)
    // if (is_between(self_finger_table[i]->node_id, self->node_id + 1, id - 1))
      // return self_finger_table[i];

  // return self;

  while(true)
  {
    if (is_between(id, temp2->node_id, temp1->node_id))
      return temp2;
    else
    {
      temp2 = temp1;
      temp1 = fetch_successor(temp1);
    }
  }
}


/**
 * This method is used to ping the existence of a given node
 */
bool nodeClient::ping(node_details* n)
{
  // only node in ring
  if (is_equal(self, n))
    return true;

  string msg = "ping`";

  string reply = sendMessage(msg, n);

  if (reply == "true")
    return true;
  else
    return false;
}


void nodeClient::keep_alive(void)
{
  while (1)
  {
    /* Send ping to successor*/
    if (ping(successor) == false) 
    {
      // if ping was unsuccessful means the current successor is not there
      cout << "Successor has left. Updating...\n";

      // if successor and predecessor are same, then it means only one node is
      // remaining in the chord
      if (is_equal(successor, predecessor))
      {
        /* Set self to predecessor and successor */
        predecessor = self;
        successor = self;
        second_successor = self;

        /* Set self for fingers */
        for (int i = 0; i < KEY_SIZE; i++)
          self_finger_table[i] = self;
      }
      else
      {
        // need to find the next successor ?? by going reverse in the ring
        request_update_predecessor(self, second_successor);
        int product = 1;
        for (int i = 0; i < KEY_SIZE; i++) 
        {
          node_details* p = find_predecessor(self -> node_id - product + 1);
          request_remove_node(self, i, second_successor, p);
          product = product * 2; // ERROR
        }
      }

      cout << "Finished updating all nodes due to successor leaving\n";
    }
    sleep(5);
  }
}

// tell the node N to update its predecessor to pred
void nodeClient::request_update_predecessor(node_details* pred, node_details* n)
{
  if (is_equal(n, self))
  {
    predecessor = pred;
    return;
  }

  string request = "update_predecessor`";
  request += to_string(pred->node_id) + "`" + pred->ip + "`" + to_string(pred->port);

  string resp = sendMessage(request, n);
}

// tell the node N to update its successor to SUCC
void nodeClient::request_update_successor(node_details* succ, node_details* n)
{
  if (is_equal(n, self))
  {
    successor = succ;
    self_finger_table[0] = successor;
    second_successor = fetch_successor(successor);
    return;
  }

  string request = "update_successor`";

  request += to_string(succ->node_id) + "`" + succ->ip + "`" + to_string(succ->port);

  string resp = sendMessage(request, n);
}

// wrapper method for executing find_successor on a given node
node_details* nodeClient::query_successor(uint32_t id, node_details* n)
{
  if (is_equal(n, self))
    return successor;

  string request = "find_successor`";
  request += to_string(id);

  string response = sendMessage(request, n);

  node_details* retVal = respToNode(response);

  return retVal;
}

// wrapper method for executing closest_preceding finger on remote node n
node_details* nodeClient::query_closest_preceding_finger(uint32_t key, node_details* n)
{
  if (is_equal(n, self))
    return closest_preceding_finger(key);

  string request = "find_cpf`" + key;

  string response = sendMessage(request, n);

  node_details* ret_node = respToNode(response);

  return ret_node;
}

// wrapper function to fetch successor of node n
node_details* nodeClient::fetch_successor(node_details* n)
{
  if (is_equal(n, self))
    return successor;

  string request = "get_successor`";

  string response = sendMessage(request, n);

  return respToNode(response);
}

// wrapper function to fetch predecessor of node n
node_details* nodeClient::fetch_predecessor(node_details* n)
{
  if (is_equal(n, self))
    return predecessor;

  string request =  "get_predecessor`";
  string response = sendMessage(request, n);

  node_details* ret = respToNode(response);

  return ret;
}

void nodeClient::request_update_finger_table(node_details* s, int i, node_details* n)
{
  if (is_equal(n, self))
  {
    self_finger_table[i] = s;
    return;
  }

  string request = "update_finger`" + to_string(s->node_id) + "`" + s->ip + "`" + to_string(s->port) + "`" + to_string(i);

  string response = sendMessage(request, n);
}

void nodeClient::update_finger_table(node_details* s, int i)
{
  if (s->node_id == self->node_id)
    return;

  if (is_between(s->node_id, self->node_id + 1, self_finger_table[i]->node_id))
  {
    self_finger_table[i] = s;

    if (i == 0)
    {
      successor = s;
      second_successor = fetch_successor(successor);
    }

    cout << "Finger for index " << i << " is now: " << endl;
    printNode(s);

    node_details* p = predecessor;

    if (s->node_id != p->node_id)
      request_update_finger_table(s, i, p);
  }
}

void nodeClient::request_remove_node(node_details* old, int i, node_details* replace, node_details* n)
{
  if (is_equal(n, self))
  {
    remove_node(old, i, replace);
    return;
  }

  string old_node = to_string(old->node_id) + "`" + old->ip + "`" + to_string(old->port);
  string rep_node = to_string(replace->node_id) + "`" + replace->ip + "`" + to_string(replace->port);

  string request = "remove_node`" + old_node + "`"  + to_string(i)  + "`" + rep_node;

  string response = sendMessage(request, n);
}


void nodeClient::remove_node(node_details* old, int i, node_details* replace)
{
  if (is_equal(self_finger_table[i], old))
  {
    self_finger_table[i] = replace;
    if (i == 0)
    {
      successor = replace;
      second_successor = fetch_successor(successor);
    }
    request_remove_node(old, i, replace, predecessor);
  }
}

void nodeClient::bye(void)
{
  // we need to transfer the files to the successor
  string files = "addfiles`";

  for (auto i : my_filetable)
  {
    int key = i.first;
    vector<file_details> f = i.second;

    for (auto k : f)
    {
      string filepath = k.path;
      string ip = k.ip;
      int port = k.port;

      string thisFile = to_string(key) + ":" + ip + ":" + to_string(port) + ":" + filepath + "#";
      files += thisFile;
    }
  }

  string reply = sendMessage(files, successor);

  request_update_predecessor(predecessor, successor);
  request_update_successor(successor, predecessor);
  // TODO predecessor of predecessor update second_sucessor
}
