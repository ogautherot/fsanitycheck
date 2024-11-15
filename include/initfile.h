/* ***************************************************************************
* Configuration file support library
*
* (C) Olivier Gautherot, 2024
* ************************************************************************* */

/**
 * 
*/

#ifndef __INITFILE_H__
#define __INITFILE_H__

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
//#include <cctype>

#include "applib.h"


using namespace std;

typedef vector<string> PathList;


/**
 * The class Initfile gathers the configuration parameters coming from the
 * command line and the configuration file.
*/
class InitFile {
public:
  PathList  scanPath;

  /**
   * Constructor
  */
  InitFile();
  ~InitFile();

  int parseArgs(int ac, char *av[]);
  void  help(void);

  string getConnInfo(void);
  string &getHostname(void) { return hostname; }

  // Getters
  const char  *getHost(void);
  const char  *getPort(char *buf, int len);
  const char  *getDbName(void);
  const char  *getUser(void);
  const char  *getConnectTimeout(char *buf, int len);
  const char  *getClientEncoding(void);
  const char  *getApplicationName(void);

private:
  string    initfilename;
  string    initfilepath;
  int       initfileline;
  bool      configLoaded;
  string    progname;
  string    hostname;
  string    ostype;
  bool      doCrypt;

  SystemType systemtype;

  string  host;
  int     port;
  string  dbname;
  string  username;
  int     connectTimeout;
  string  clientEncoding;   // UTF8
  string  applicationName;

  PathList *splitPath(const char *list, const char sep);
  const char *getConfigFileName();
  int parseFile(const char *name);
  int parseLine(ifstream &ifs);
  int processAssignment(string &Key, string &Value);

};

enum ParserStateEnum {
  LeadingSpaceState = 1,
  CommentState,
  SectionState,
  SectionTrailingState,
  KeynameState,
  KeynameTrailingState,
  ValueLeadingState,
  ValueState,
  ValueStringState,
  ValueTrailingState,
  EndOfLine,
};


#endif // __INITFILE_H__
