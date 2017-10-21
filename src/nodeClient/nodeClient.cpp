/**
 * Filename: "nodeClient.cpp"
 *
 * Description: This is file contains all the method definitions of the client
 *
 * Author: Abhishek Shrivastava <abhishek.shrivastava.ts@gmail.com>
 **/
#include "../../includes/nodeClient.h"
#include "../../includes/logger.h"

using namespace std;

/**
 * Default constructor for the nodeClient
 *
 * @param c_alias     = Client Alias
 * @param c_ip        = Client IP Address
 * @param c_port      = Client Communication port
 * @param s_ip        = Server IP
 * @param s_port      = Server Listen Port
 * @param c_down_port = Client Download Port
 * @param c_base      = Client base directory;
 *
 */
nodeClient::nodeClient(string c_alias, string c_ip, int c_port, string s_ip, int s_port, int c_down_port, string c_base)
{
  alias = c_alias;
  ip = c_ip;
  port = c_port;
  server_ip = s_ip;
  server_port = s_port;
  down_port = c_down_port;
  base_loc = c_base;
  haveSearchResults = false;

  blackbox = new Logger(base_loc + "/client_log");
}


/**
 *
 * This method is a wrapper for downloading a file from another client
 *
 * Its called for request - get [2] <fileNma>
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

    downloadFile(client_alias, relative_path, outfile);
  }
  else
  {
    cout << "FAILURE: No search results exist" << endl;
  }
}

/**
 * This method is used to download a file from a client
 *
 * @param client_alias = Alias of client from where to download
 * @param relative_path= The path of file on the client wrt client_base
 * @param outfile      = Name with which the copied file will be saved
 *
 * We get the client IP details from the server and then we directly
 * communicate with the given client
 *
 */
void nodeClient::downloadFile(string client_alias, string relative_path, string outfile)
{
  // first we need to get the client details from trackr
  string command = "ip`" + client_alias + "`" + client_alias;
  string client_detail_str = sendMessage(command);

  vector<string> client_details;
  tokenize(client_detail_str, client_details, ":");

  if (client_details[0] == "Fail")
  {
    cout << "FAILURE: Client Not Found On Server" << endl;
    return;
  }

  string ip = client_details[0];
  int down_port = stoi(client_details[1]);

  int downSock = socket(AF_INET, SOCK_STREAM, 0);

  if (downSock >= 0)
  {
    // (Not necessary under windows -- it has the behaviour we want by default)
    const int trueValue = 1;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    (void) setsockopt(downSock, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));
    (void) setsockopt(downSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));

    // this is the struct details on which we will try to connect
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(down_port);

    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
    {
      cout << "Failure: Error in client IP" << endl;
      return;
    }

    if (connect(downSock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("FAILURE: Can't connect");
      return;
    }

    string download_req_str = "get`" + relative_path + "`";

    int len = send(downSock, download_req_str.c_str(), strlen(download_req_str.c_str()), 0);

    bool success = true;

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
 * This method is used to register a file on the server
 * It only creates and packs the command to a string format and passes it to
 * the sendMessage method, which sends the command to the server.
 *
 * @param filesharepath = The path of the file which is shared
 *
 */
void nodeClient::registerFile(string fileSharePath)
{
  string cmd_name = "share";
  string command = cmd_name + "`" + fileSharePath + "`" + alias;
  string reply = sendMessage(command);

  if (reply.find("True") != -1)
    cout << "SUCCESS:FILE_SHARED" << endl;
}

/**
 * This method is used to remove a file from the server.
 * Only creates the commands, packs it and uses sendMessage
 *
 * @param filesharepath = path of file which is to be removed
 *
 */
void nodeClient::deregisterFile(string fileSharePath)
{
  cout << "Deregistering file " << fileSharePath << endl;
  string cmd_name = "del";
  string command_str = cmd_name + "`" + fileSharePath + "`" + alias;
  string reply = sendMessage(command_str);

  if (reply.find("True") != -1)
    cout << "SUCCESS:FILE_REMOVED" << endl;
  else
    cout << reply << endl;
}

/**
 * This method is used to execute a shell command on a remote client
 *
 * @param rpc_alias = Alias of the client on which we want to execute the
 * command
 *
 * @param shell_command = shell command to execute
 *
 * It gets the ip of the client and then sends a exec request to the client on
 * its c_down_port.
 *
 * Prints out whatever the other client returns.
 *
 */
void nodeClient::exec_command(string rpc_alias, string shell_command)
{
  // first we need to get the client details from trackr
  string command = "ip`" + rpc_alias + "`" + rpc_alias;
  string client_detail_str = sendMessage(command);

  vector<string> client_details;
  tokenize(client_detail_str, client_details, ":");

  if (client_details[0] == "Fail")
  {
    cout << "FAILURE: Client Not Found On Server" << endl;
    return;
  }

  string ip = client_details[0];
  int down_port = stoi(client_details[1]);

  int downSock = socket(AF_INET, SOCK_STREAM, 0);

  if (downSock >= 0)
  {
    // make the socket non blocking, by having a timeout
    const int trueValue = 1;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    (void) setsockopt(downSock, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));
    (void) setsockopt(downSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));

    // this is the struct details on which we will try to connect
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(down_port);

    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
    {
      cout << "Failure: Error in client IP" << endl;
      return;
    }

    if (connect(downSock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("FAILURE: Can't connect");
      return;
    }

    string cmd_name = "exec";
    string shell_command_str = cmd_name + "`" + shell_command + "`" + rpc_alias;

    int len = send(downSock, shell_command_str.c_str(), strlen(shell_command_str.c_str()), 0);

    char recvBuff[1000000];
    memset(recvBuff,0, sizeof(recvBuff));

    int oplen = recv(downSock, recvBuff, sizeof(recvBuff), 0);

    if (oplen == 0)
      cout << "FAILURE: COMMAND EXECUTION FAIL" << endl;
    else
    {
      string s(recvBuff);
      cout << s << endl;
    }
  }
  else
    cout << "Socket failure" << endl;
}


/**
 * This method is used to search for results on the server.
 * It creates the command string packs it and the sendsMesssage to server.
 *
 * @param file_name = Filename to be shared
 */
void nodeClient::searchFile(string file_name)
{
  string cmd_name = "search";
  string command = cmd_name + "`" + file_name + "`" + alias;
  string result = sendMessage(command);

  vector<string> res;
  tokenize(result, res, "`");
  search_results = res;

  int len = search_results.size();

  if (len == 0)
  {
    cout << "FOUND:0" << endl;
    haveSearchResults = false;
  }
  else
  {
    haveSearchResults = true;
    cout << "FOUND:" << len << endl;
    for (int i = 0; i < len; i++)
      printf("[%d] %s\n", i + 1, search_results[i].c_str());
  }
}

/**
 * This method is used to send a heartbeat message to the sever.
 */
void nodeClient::sendPulse(void)
{
  string cmd_name = "heartbeat";
  string client_details = ip + ":" + to_string(port) + ":" + to_string(down_port);

  string command = cmd_name + "`" + client_details + "`" + alias;

  string result = sendMessage(command);
}

/***
 * Server code for client, start listening on the download port
 *
 * This method is executed as a thread. Starts listening on the given client
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
  serv_addr.sin_port = htons(down_port); // port number to listen on

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
    thread t(&nodeClient::handleClient, this, connfd);
    t.detach();
  }
}

/***
 * This method is handler for client side server.
 * It parses the given client command and then executes the respective method.
 *
 * @param connfd = The connected socket descriptor
 */
void nodeClient::handleClient(int connfd)
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
   {
     printf("Error, couldn't open file [%s] to send!\n", full_file_path.c_str());
   }
  }
  if (cmd == EXEC)
  {
    string shell_command_to_execute = tokens[1];
    string result = execute_shell_command(shell_command_to_execute.c_str());
    int sent = send(connfd, result.c_str(), strlen(result.c_str()), 0);
    close(connfd);
  }
}

/**
 * This method is used to execute a shell command on the client
 * It is called when a execc request comes for the client
 *
 * Returns the result of command. The errors are redirected to NULL
 *
 * @param cmd = The char string for the command to be executed;
 */
string nodeClient::execute_shell_command(const char* cmd)
{
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));

  string cmd_res = "";

  string no_err_cmd(cmd);

  no_err_cmd += " 2>/dev/null";

  FILE* fp = popen(no_err_cmd.c_str(), "r");

  while (!feof(fp))
  {
    if (fgets(buffer, 1024, fp) != NULL)
    {
      string this_b(buffer);
      cmd_res += this_b;
    }
    memset(buffer, 0, sizeof(buffer));
  }

  return cmd_res;
}

/**
 * This method sends out a simple message to the server.
 * Server details are fetched from the object members
 *
 * @param message - The message to send to the server
 */
string nodeClient::sendMessage(string message)
{
  int client_sockfd = 0;

  struct sockaddr_in client_sockaddr;
  char recvBuff[100000];

  // intialize the socket struct and buffers
  memset(&client_sockaddr, '0', sizeof(client_sockaddr));
  memset(recvBuff, 0, sizeof(recvBuff));

  // create a socket on listen file descriptor
  client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // ERROR HANDLING
  if (client_sockfd == -1) // if socket creation fails
  {
    string msg = (string)__FUNCTION__ + " ERROR: SOCKET CREATION ";
    perror(msg.c_str());
    string err = strerror(errno);
    blackbox -> record(msg + err);
    cout << "Please try again" << endl;
    return "Fail";
  }

  // intialize the client sockaddr struct. We need this because bind
  client_sockaddr.sin_family = AF_INET; // defines the family of socket, internet family used
  client_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY); // which IP to listen on // TODO replace by inet_addr(ip);
  client_sockaddr.sin_port = htons(port); // port number to listen on

  // bind the socket now to the given client port now
  // res int will track the reutrn result of various calls
  int res = bind(client_sockfd, (struct sockaddr*)&client_sockaddr, sizeof(client_sockaddr));
  // if the bind call fails
  if (res == -1)
  {
    string msg = (string)__FUNCTION__ + " ERROR: SOCKET BINDING";
    perror(msg.c_str());
    string err = strerror(errno);
    blackbox -> record(msg + err);
    cout << "Cannot bind. Please try again." << endl;
    return "Fail";
  }

  struct sockaddr_in serv_sockaddr;
  memset(&serv_sockaddr, '0', sizeof(serv_sockaddr));

  serv_sockaddr.sin_family = AF_INET;
  serv_sockaddr.sin_port   = htons(server_port);

  res = inet_pton(AF_INET, server_ip.c_str(), &serv_sockaddr.sin_addr);
  // ERROR HANDLING CODE
  if (res == 0)
  {
    string msg = "Can't convert server inet_pton";
    perror(msg.c_str());
    blackbox -> record(msg);
    cout << "Exiting"  << endl;
    exit(1);
  }
  if (res == -1)
  {
    perror("Unknown error: ");
    string error = strerror(errno);
    blackbox -> record("Unknown error: " + error);
    cout << "Exiting" << endl;
    exit(1);
  }

  res = connect(client_sockfd, (struct sockaddr*)&serv_sockaddr, sizeof(serv_sockaddr));
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

  res = send(client_sockfd, message.c_str(), message.length(), 0);
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

  int i = 0;
  int bytes_read;
  do
  {
    char c;
    bytes_read = recv(client_sockfd, &c, sizeof(char), 0);
    recvBuff[i] = c;
    i++;
  } while (bytes_read != 0);

  string s(recvBuff);

  close(client_sockfd);
  return s;
}
