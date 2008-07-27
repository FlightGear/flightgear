#ifndef _PARSE_FILE_H_
#define _PARSE_FILE_H_

#include <simgear/compiler.h>

#include <string>
#include <list>
#include <fstream>

using std::list;
using std::string;
using std::getline;
using std::ifstream;

#define DELIMITERS " \t"
#define COMMENT "#"

#define MAXLINE 400   // Max size of the line of the input file

typedef list<string> stack; //list to contain the input file "command_lines"

class ParseFile
{
        private:
                
                stack commands;
                ifstream file;
                void readFile();

        public:

                ParseFile() {}
                ParseFile(const string fileName);
                ~ParseFile();

                
                void removeComments(string& inputLine);
                string getToken(string inputLine, int tokenNo);
                void storeCommands(string inputLine);
                stack getCommands();
};

#endif  // _PARSE_FILE_H_
