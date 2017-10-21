#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;

int main(void)
{
  string log_location;
  string base_folder_location;

  string base_folder_prompt = "Base Folder:";

  cout << base_folder_prompt << " ";
  cin >> base_folder_location;

  // create a output file stream
  // for reading. If file exists, truncate the contents
  ofstream conf;
  conf.open("conf", ios::out | ios::trunc);

  conf << base_folder_prompt << base_folder_location << endl;

  return 0;
}
