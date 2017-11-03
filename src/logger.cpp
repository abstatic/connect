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
  if (cmd_str == "pull")
    return GET;
  else if (cmd_str == "push")
    return SHARE;
  else if (cmd_str == "remove")
    return DEL;
  else if (cmd_str == "search")
    return SEARCH;
  else if (cmd_str == "show")
    return SHOW;
  else if (cmd_str == "stablize")
    return STABLIZE;
  else if (cmd_str == "exit")
    return EXIT;
  else if (cmd_str == "find_successor")
    return FIND_SUCCESSOR;
  else if (cmd_str == "get_successor")
    return GET_SUCCESSOR;
  else if (cmd_str == "find_cpf")
    return CPF;
  else if (cmd_str.find("pull[") == 0)
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

string GetHexRepresentation(const unsigned char * Bytes, size_t Length)
{
    std::ostringstream os;
    os.fill('0');
    os<<std::hex;
    for(const unsigned char * ptr=Bytes;ptr<Bytes+Length;ptr++)
        os<<std::setw(2)<<(unsigned int)*ptr;
    return os.str();
}

/**
 * Converts given hex char array to equivalent decimal number depending on the
 * value of M
 */
int hex2dec(string hex)
{
    string hexstr;
    int length = 4;
    const int base = 16;     // Base of Hexadecimal Number
    int decnum = 0;
    int i;

    // Now Find Hexadecimal Number
    hexstr = hex;
    for (i = 0; i < length; i++)
    {
      // Compare *hexstr with ASCII values
      if (hexstr[i] >= 48 && hexstr[i] <= 57)   // is *hexstr Between 0-9
      {
          decnum += (((int)(hexstr[i])) - 48) * pow(base, length - i - 1);
      }
      else if ((hexstr[i] >= 65 && hexstr[i] <= 70))   // is *hexstr Between A-F
      {
          decnum += (((int)(hexstr[i])) - 55) * pow(base, length - i - 1);
      }
      else if (hexstr[i] >= 97 && hexstr[i] <= 102)   // is *hexstr Between a-f
      {
          decnum += (((int)(hexstr[i])) - 87) * pow(base, length - i - 1);
      }
      else
      {
        cout << endl << hexstr[i] << endl;
          cout<<"Invalid Hexadecimal Number \n";
      }
    }
    return decnum;
}
