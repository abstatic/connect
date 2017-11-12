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
#define FIND_SUCCESSOR 8
#define GET_SUCCESSOR 9
#define FIND_CPF 10
#define FIND_PFS 11
#define NOTIFY 12
#define GET_FILE_TABLE 13
#define FIX 14
#define PING 15
#define U_PRED 16
#define GET_PREDECESSOR 17
#define U_FIN 18
#define R_NODE 19
#define U_SUCC 20
#define ADD 21
#define VIEW 22
#define TRANSFER 23
#define ADDF 24
#define U_SSUCC 25

#define LEN 8
#define LOCAL_IP_ADDRESS "127.0.0.1"
#define KEY_SPACE 16
#define KEY_SIZE 4
#define KEEP_ALIVE 5 // inseconds

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
  string ip;
  int port;  
  uint32_t node_id; // unsigned int for storing the 32 bit node id
};

struct file_details
{
  string ip;
  int port;
  string path;
  uint32_t file_key;
};

struct ft_struct
{
  pair<int, int> interval;
  int successor;
  node_details* s_d;
};

void tokenize(string str, vector<string>& tokens, const string& delimiters = " ");
void sanitize(string&, char); // remove a char from string
int interpret_command(string); // interpret a command string
string getEnv(const string& var); // getEnvironment variable
int hex2dec(string);
string GetHexRepresentation(const unsigned char *, size_t);
bool is_between(uint32_t key, uint32_t a, uint32_t b);
bool is_equal(node_details*, node_details*);
void printNode(node_details*);

extern string base_loc;

#endif /* ifndef logger */
