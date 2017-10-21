/**
 * Filename: "logger.cpp"
 *
 * Description: This is implementation of the logging class
 *
 * Also contains some helper methods
 *
 * Author: Abhishek Shrivastava <abhishek.shrivastava.ts@gmail.com>
 **/
#include "../includes/logger.h"

using namespace std;

/**
 * Constructor for the logger
 */
Logger::Logger(string logfile_path)
{
  log_location = "None";
  log_level = 0;
  log_file = logfile_path;

  log_fp.open(log_file, ios::out | ios::app);
}

/**
 * This is the log reccording function. TIMESTAMP MESSAGE
 */
void Logger::record(std::string message)
{
  time_t t = time(0);
  struct tm* now = localtime(&t);
  log_fp <<(now -> tm_year + 1900) << '-'
         <<(now -> tm_mon + 1) << '-'
         << now -> tm_mday << ' '
         << now -> tm_hour << ':'
         << now -> tm_min << ':'
         << now -> tm_sec;
  log_fp << " " << message << std::endl;
}


/**
 * Destructor for the logger
 */
Logger::~Logger(void)
{
  log_fp.close();
}

/**
 * This method is used to tokenize a string with a delimiter provided
 *
 * @param str - The string to tokenize
 * @param tokens - The reference to the vector which will store the tokens
 * @param delimiters - The string of delimitters at which we will break
 */
void tokenize(string str, vector<string>& tokens, const string& delimiters)
{
  // skip the initial delimiters
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);

  // find the first non delimiter
  string::size_type pos = str.find_first_of(delimiters,lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    // Found a token, add it to the vector
    tokens.push_back(str.substr(lastPos, pos - lastPos));

    // Skip delimiters, on to the next token we go
    lastPos = str.find_first_not_of(delimiters, pos);

    // Find next "non-delimiter" position of next delimiter
    pos = str.find_first_of(delimiters, lastPos);
  }
}

/**
 * This method is used to sanitize a string by removing a particular
 * character
 */
void sanitize(string& str, char c)
{
  str.erase(remove(str.begin(), str.end(), c), str.end());
}

/**
 * This method is used to map a command string to a DEFINED command int value
 *
 * @param cmd_str - the command to match
 *
 */
int interpret_command(string cmd_str)
{
  if (cmd_str == "get")
    return GET;
  else if (cmd_str == "share")
    return SHARE;
  else if (cmd_str == "del")
    return DEL;
  else if (cmd_str == "exec")
    return EXEC;
  else if (cmd_str == "search")
    return SEARCH;
  else if (cmd_str == "ip")
    return IP;
  else if (cmd_str == "heartbeat")
    return PULSE;
  else if (cmd_str.find("get[") == 0)
    return GET;
  else
    return 0;
}

/**
 * This method is used to find a environment variable
 */
string GetEnv( const string & var ) {
     const char * val = getenv( var.c_str() );
     if ( val == 0 ) {
         return "";
     }
     else {
         return val;
     }
}
