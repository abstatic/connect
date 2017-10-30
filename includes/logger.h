#ifndef logger
#define logger 
/*! \class logger
 *  \brief The logger class is used to handle loggind for Anakata
 *
 *  This class defines various levels of logging for the application after
 *  reading from a configuration file
 */
#include "base_conf.h"

#define GET 1
#define SHARE 2
#define DEL 3
#define EXIT 4
#define SEARCH 5
#define SHOW 6
#define STABLIZE 7

#define LEN 4

using namespace std;

class Logger
{
public:
  Logger(string);
  virtual ~Logger();
  void record(std::string message);

protected:
  std::string log_location; /*!< Default location for the log file */
  std::string log_file;
  int log_level;
  std::ofstream log_fp;
};

struct node_details
{
  int port;
  string ip;
  int node_id;
};

struct file_details
{
  int port;
  string ip;
  string path;
};

struct ft_struct
{
  pair<int, int> interval;
  int successor;
  node_details s_d;
};

void tokenize(string str, vector<string>& tokens, const string& delimiters = " ");
void sanitize(string&, char); // remove a char from string
int interpret_command(string); // interpret a command string
string getEnv(const string& var); // getEnvironment variable
int hex2dec(string);
string GetHexRepresentation(const unsigned char *, size_t);

string base_loc = "~/connect_files";

#endif /* ifndef logger */
