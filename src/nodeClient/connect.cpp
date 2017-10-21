/**
 * Filename: "katalyn.cpp"
 *
 * Description: This is the main function which instantiates a client in anakata
 *
 * Author: Abhishek Shrivastava <abhishek.shrivastava.ts@gmail.com>
 **/
#include "../../includes/nodeClient.h"
#include "../../includes/logger.h"

using namespace std;

int main(int argc, const char *argv[])
{
  // eg. ./client_20172001 "Bob's Computer" 192.168.1.20 8754 192.168.1.2 8750 14000 ~/Desktop
  if (argc != 8)
  {
    cout << "Usage: " << argv[0] << " client_alias client_ip client_port server_ip server_port downloading_port client_root" << endl;
    return 1;
  }

  string c_alias  = argv[1];
  string c_ip     = argv[2];
  int c_port      = stoi(argv[3]);
  string s_ip     = argv[4];
  int s_port      = stoi(argv[5]);
  int c_down_port = stoi(argv[6]);
  string c_base   = argv[7];

  nodeClient katalyn(c_alias, c_ip, c_port, s_ip, s_port, c_down_port, c_base);

  // navigate to the given base directory
  chdir(c_base.c_str());

  katalyn.sendPulse();

  thread t(&nodeClient::startListen, katalyn);
  t.detach();

  string line;
  while (getline(cin, line))
  {
    if (line == "")
      continue;

    vector<string> tokens;

    // tokenize on '"' and then sanitize as required
    tokenize(line, tokens, "\"");

    // because switch cannot use string
    sanitize(tokens[0], ' ');

    int command = interpret_command(tokens[0]);

    switch(command)
    {
      case GET:
        {
          if (tokens.size() <= 1)
          {
            cout << "FAILURE: Invalid Parameters" << endl;
            break;
          }
          // if we are downloading after a search
          if (tokens[0][3] == '[')
          {
            katalyn.blackbox -> record("Get command for: " + tokens[0]  + " outfile  " + tokens[1]);

            for (auto k : tokens)
              cout << k << endl;

            if (katalyn.haveSearchResults)
            {
              string hit_n   = tokens[0];
              string outfile = tokens[1];

              sanitize(hit_n, '[');
              sanitize(hit_n, ']');
              sanitize(hit_n, 'g');
              sanitize(hit_n, 'e');
              sanitize(hit_n, 't');

              int hit_no = stoi(hit_n);

              katalyn.downloadFile(hit_no, outfile);
            }
            else
              cout << "No search results exist. Try again." << endl;
          }
          else
          {
            if (tokens.size() != 6)
            {
              cout << "FAILURE: Invalid Parameters" << endl;
              break;
            }

            string client_alias  = tokens[1];
            string relative_path = tokens[3];
            string outfile       = tokens[5];

            sanitize(client_alias, '"');
            sanitize(client_alias, '\\');
            sanitize(relative_path, '"');
            sanitize(relative_path, '\\');
            sanitize(outfile, '"');
            sanitize(outfile, '\\');

            katalyn.blackbox -> record("Get command on " + client_alias + " path " + relative_path + " outputfile " + outfile);
            katalyn.downloadFile(client_alias, relative_path, outfile);
          }
        }
        break;
      case SHARE:
        {
          if (tokens.size() != 2)
          {
            cout << "Failure: Invalid Parameters" << endl;
            break;
          }
          string file_share_path = tokens[1];

          sanitize(file_share_path, '\\');
          sanitize(file_share_path, '"');

          katalyn.blackbox -> record("Registering file: " + file_share_path + " on server");
          katalyn.registerFile(file_share_path);
        }
        break;
      case DEL:
        {
          if (tokens.size() != 2)
          {
            cout << "Failure: Invalid Parameters" << endl;
            break;
          }

          string file_share_path = tokens[1];
          sanitize(file_share_path, '\\');

          sanitize(file_share_path, '"');

          katalyn.blackbox -> record("Deregistering file: " + file_share_path + " on server");
          katalyn.deregisterFile(file_share_path);
        }
        break;
      case EXEC:
        {
          if (tokens.size() != 4)
          {
            cout << "FAILURE: Invalid Parameters";
            break;
          }
          string rpc_alias     = tokens[1];
          string shell_command = tokens[3];

          sanitize(rpc_alias, '"');
          sanitize(rpc_alias, '\\');
          sanitize(shell_command, '"');

          katalyn.blackbox -> record("Executing shell command " + shell_command + " on " + rpc_alias);
          katalyn.exec_command(rpc_alias, shell_command);
        }
        break;
      case SEARCH:
        {
          if (tokens.size() != 2)
          {
            cout << "FAILURE: Invalid Parameters" << endl;
            break;
          }
          string file_name = tokens[1];
          sanitize(file_name, '\\');

          sanitize(file_name, '"');

          katalyn.blackbox -> record("Searching for file: " + file_name + " on server");
          katalyn.searchFile(file_name);
        }
        break;
      default:
        cout << "Unknown Command" << endl;
    }
  }

  cout << "Bye Bye" << endl;

  return 0;
}
