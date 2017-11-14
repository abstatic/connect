/**
 * Filename: "connect.cpp"
 *
 * Description: This is the main function which instantiates a connect node
 *
 * Author: Abhishek Shrivastava <abhishek.shrivastava.ts@gmail.com>
 **/
#include "../../includes/nodeClient.h"
#include "../../includes/logger.h"

using namespace std;

string base_loc;

int main(int argc, const char *argv[])
{
  string home_path = getenv("HOME");
  base_loc = home_path + "/connect";
  // eg. ./connect my_ip my_port friend_ip friend_port
  if (argc < 3)
  {
    cout << "Usage: " << argv[0] << " my_ip my_port [friend_ip] [friend_port]" << endl;
    return 1;
  }

  string c_ip, f_ip;
  int c_port, f_port;

  c_ip     = argv[1];
  c_port   = stoi(argv[2]);

  if (argc == 5)
  {
    f_ip     = argv[3];
    f_port   = stoi(argv[4]);
  }
  else
  {
    f_ip = "";
    f_port = 0;
  }

  nodeClient connect_node(c_ip, c_port, f_ip, f_port);

  // navigate to the base directory
  chdir(base_loc.c_str());

  // start listening thread
  // thread t(&nodeClient::startListen, connect_node);
  // t.detach();

  thread ka(&nodeClient::keep_alive, &connect_node);
  ka.detach();

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
          if (tokens[0][4] == '[')
          {
            connect_node.blackbox -> record("Get command for: " + tokens[0]  + " outfile  " + tokens[1]);

            for (auto k : tokens)
              cout << k << endl;

            if (connect_node.haveSearchResults)
            {
              string hit_n   = tokens[0];
              string outfile = tokens[1];

              sanitize(hit_n, '[');
              sanitize(hit_n, ']');
              sanitize(hit_n, 'p');
              sanitize(hit_n, 'u');
              sanitize(hit_n, 'l');
              sanitize(hit_n, 'l');

              int hit_no = stoi(hit_n);

              connect_node.downloadFile(hit_no, outfile);
            }
            else
              cout << "No search results exist. Try again." << endl;
          }
          else
          {
            if (tokens.size() != 2)
            {
              cout << "FAILURE: Invalid Parameters" << endl;
              break;
            }

            string filepath = tokens[1];

            sanitize(filepath, '"');
            sanitize(filepath, '\\');

            connect_node.blackbox -> record("Get command on path " + filepath);
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

          connect_node.blackbox -> record("Registering file: " + file_share_path + " on network");
          connect_node.registerFile(file_share_path);
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

          connect_node.blackbox -> record("Deregistering file: " + file_share_path + " on network");
          connect_node.deregisterFile(file_share_path);
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

          connect_node.blackbox -> record("Searching for file: " + file_name + " on server");
          connect_node.searchFile(file_name);
        }
        break;
      case EXIT:
        {
          connect_node.bye();
          exit(0);
        }
        break;
      case SHOW:
        {
          cout << "Successor " << connect_node.successor -> node_id << endl;

          if (connect_node.predecessor == NULL)
            cout << "Predecessor NULL" << endl;
          else
            cout << "Predecessor " << connect_node.predecessor -> node_id << endl;
          printNode(connect_node.self);
          cout << "Second successor: ";
          printNode(connect_node.second_successor);
        }
        break;
      case STABLIZE:
        {
          connect_node.stabilize();
          cout << "STABLIZE" << endl;
        }
        break;
      case FIX:
        {
          connect_node.fix_fingers();
          cout << "FIX FINGERS" << endl;
        }
        break;
      case VIEW:
        {
          for (auto i : connect_node.my_filetable)
          {
            cout << "KEY: " << i.first << endl;
            for (auto j : i.second)
            {
                cout << "IP: " <<  j.ip << " " << j.port << " " << j.path << endl;
            }
            cout << "--------" << endl;
          }
        }
        break;
      default:
        cout << "Unknown Command" << endl;
    }
  }

  cout << "Bye Bye" << endl;

  return 0;
}
