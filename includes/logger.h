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
#define EXEC 4
#define SEARCH 5
#define IP 6
#define PULSE 7

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

void tokenize(string str, vector<string>& tokens, const string& delimiters = " ");
void sanitize(string&, char); // remove a char from string
int interpret_command(string); // interpret a command string
string getEnv(const string& var); // getEnvironment variable
#endif /* ifndef logger */
