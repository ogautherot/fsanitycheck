/* ***************************************************************************
* Configuration file support library
*
* (C) Olivier Gautherot, 2024
* ************************************************************************* */

#include <cstdlib>
#include <fstream>
#include <cctype>

#include <unistd.h>

#include "include/initfile.h"


using namespace std;

/**
 * Constructor
*/
InitFile::InitFile()
{
  char name[128];

  gethostname(name, sizeof(name));

  // Default file name and path, that can be overridden by the command line
  initfilename = ".fsanitycheck";
  initfilepath = ".:";
  initfilepath += getenvvar("HOME");
  configLoaded = false;
  hostname = getenvvar("HOSTNAME");
  ostype = getenvvar("OSTYPE");
  initfileline = 1;
  doCrypt = false;
  
  // Database connection parameters
  host = "localhost";
  port = 5432;
  dbname = "fsanitycheck";
  username = getenv("USER");
  connectTimeout = 20;
  clientEncoding = "UTF8";
  applicationName = "fsanitycheck";

  systemtype = checksystemtype(getenv("PATH"));
}

/**
 * Destructor
*/
InitFile::~InitFile()
{
  //
}

/**
 * Getter: Database host field
*/
const char *InitFile::getHost(void) {
  return host.c_str();
}

/**
 * Getter: Database user name field
*/
const char *InitFile::getUser(void)
{
  return username.c_str();
}

/**
 * Getter: database port field
*/
const char *InitFile::getPort(char *buf, int len)
{
  return itoa(port, buf, 10);
}

/**
 * Getter: Database name
*/
const char *InitFile::getDbName(void)
{
  return dbname.c_str();
}

/**
 * Getter: Database connection timeout
*/
const char *InitFile::getConnectTimeout(char *buf, int len)
{
  return itoa(connectTimeout, buf, 10);
}

/**
 * Getter: Database client encoding
*/
const char *InitFile::getClientEncoding(void)
{
  return clientEncoding.c_str();
}

/**
 * Getter: Application name field for database connection.
*/
const char *InitFile::getApplicationName(void)
{
  return applicationName.c_str();
}

/**
 * Getter: Database connection string
 * 
 * @return Connection string
*/
string InitFile::getConnInfo(void)
{
  char portBuf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  char timeoutBuf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  string ret;
  ret.resize(160);

  getPort(portBuf, sizeof(portBuf));
  getConnectTimeout(timeoutBuf, sizeof(timeoutBuf));
  ret = "host=" + host + " port=" + portBuf +
        " dbname=" + dbname + " user=" + username +
        " connect_timeout=" + timeoutBuf +
        " client_encoding=" + clientEncoding +
        " application_name=" + applicationName;
  return ret;
}

/** 
 * Extract fields from path string. Separator can be either ':' or ';'.
 * 
 * @return Vector of strings.
*/
PathList *InitFile::splitPath(const char *list, const char sep)
{
  PathList *ret = new PathList;

  while (0 != *list) {
    const char *pos = list;

    if (sep == *pos) {
      list++;
      continue;
    }

    do {
      pos++;
    } while ((0 != *pos) && (sep != *pos));
    string field(list, pos - list);
    ret->push_back(field);
    list = (0 == *pos) ? pos : pos + 1;
  }

  return ret;
}

/**
 * Parse command line arguments
 * 
 * @param ac    Arguments count.
 * @param av    Array of strings with arguments.
 * 
 * @return Returns 0 upon success.
*/
int InitFile::parseArgs(int ac, char *av[])
{
  PathList *list = splitPath(getenv("PATH"), ':');
  int numfields = list->size();
  progname = av[0];

  // First pass to search for a config file
  for (int i = 1; i < ac; i++)  {
    const char *arg = av[i];

    if ('-' == arg[0])  {
      if ('c' == arg[1])  {
        if (i + 1 < ac) {
          configLoaded = (0 == parseFile(av[i + 1]));
          break;
        }
      }
    }
  }
  if (!configLoaded)  {
    configLoaded = (0 == parseFile(".fsanitycheck"));
    if (!configLoaded)  {
      string homename(getenv("HOME"));
      homename += "/.fsanitycheck";
      configLoaded = (0 == parseFile(homename.c_str()));
    }
  }

  for (int i = 1; i < ac; i++) {
    const char *arg = av[i];

    if ('-' == arg[0])  {
      switch (arg[1])  {
        case 'h':
          help();
          break;

        case 'c': // Ignore config file
          i++;
          break;

        case 'x':
          doCrypt = true;
          break;

        default:
          break;
      }
    } else {
      scanPath.push_back(arg);
    }
  }
  return 0;
}

/**
 * Print help message.
*/
void InitFile::help(void)
{
  cerr << "Usage: " << progname << " [-c config file] [-h] <pathlist>\n";

  exit (0);
}

int InitFile::processAssignment(string &Key, string &Value)
{
  int ret = 0;
  if (Key.compare("host") == 0) {
    host = Value;
  } else if (Key.compare("port") == 0) {
    port = stoi(Value);
  } else if (Key.compare("dbname") == 0) {
    dbname = Value;
  } else if (Key.compare("user") == 0) {
    username = Value;
  } else if (Key.compare("timeout") == 0) {
    connectTimeout = stoi(Value);
  } else {
    cout << "Bad key: " << Key << ", value: " << Value << endl;
    ret = -1;
  }
  return ret;
}

int InitFile::parseLine(ifstream &ifs)
{
  int c;
  ParserStateEnum State = LeadingSpaceState;
  string Keyname;
  string Value;
  string Section;
  bool KeyValueAssignment = false;

  do
  {
    c = ifs.get();
    if (c == -1)  {
      return -1;
    }

    if (c == '#') {
      State = CommentState;
    }

    switch (State)  {
      case LeadingSpaceState:
        if (!isspace(c)) {
          if (c == '[') {
            State = SectionState;
          } else if (isalpha(c))  {
            State = KeynameState;
            Keyname.push_back(c);
          }
        }
        break;

      case CommentState:
        if ((c == '\n') || (c == '\r')) {
          State = EndOfLine;
        }
        break;

      case SectionState:
        if (isalnum(c)) {
          Section.push_back(c);
        } else if (c == ']')  {
          State = SectionTrailingState;
        } else {
          cerr << "Line " << initfileline << " Error: Bad section name\n";
          exit (1);
        }
        break;

      case SectionTrailingState:
        if (!isspace(c)) {
          cerr << "Line " << initfileline << " Error: Trailing text on section definition\n";
          exit (1);
        } else if ((c == '\r') || (c == '\n'))  {
          State = EndOfLine;
        }
        break;

      case KeynameState:
        if (isalnum(c)) {
          Keyname.push_back(c);
        } else if (c == '=') {
          State = ValueLeadingState;
          KeyValueAssignment = true;
        } else if (isspace(c)) {
          State = KeynameTrailingState;
        } else {
          cerr << "Line " << initfileline << " Error: Bad key name\n";
          exit (1);
        }
        break;

      case KeynameTrailingState:
        if (!isspace(c)) {
          if (c == '=') {
            State = ValueLeadingState;
            KeyValueAssignment = true;
          } else {
            cerr << "Line " << initfileline << " Error: Bad key name\n";
            exit (1);
          }
        }
        break;

      case ValueLeadingState:
        if (!isspace(c))  {
          if (c == '"') {
            State = ValueStringState;            
          } else {
            Value.push_back(c);
            State = ValueState;
          }
        }
        break;

      case ValueState:
        if (isgraph(c)) {
          Value.push_back(c);
        } else if (isspace(c)) {
          State = EndOfLine;
        }
        break;

      case ValueStringState:
        if ((c == '\r') || (c == '\n')) {
          cerr << "Line " << initfileline << " Error: Unended string definition\n";
          exit (1);
        } else if (c == '"')  {
          State = ValueState;
        } else {
          Value.push_back(c);
        }
        break;

      case ValueTrailingState:
      case EndOfLine:
        if (!isgraph(c))  {
          cerr << "Line " << initfileline << " Error: Trailing text on key/value definition\n";
          exit (1);
        }
        break;
    };
  } while ((c != '\n') && (c != '\r'));
  if (c == '\n') {
    initfileline++;
  }

  if (KeyValueAssignment) {
    processAssignment(Keyname, Value);
  }

  cout << "Section: " << Section << ", Key " << Keyname << " -> " << Value << endl;
  return 0;
}

/**
 * Parse configuration file
 * 
 * @param name  File name, ideally with absolute path
 * 
 * @return Returns 0 if the file could be read, or -1 if an error happened.
*/
int InitFile::parseFile(const char *name)
{
  ifstream ifs;

  ifs.open(name);
  if (!ifs.fail())  {
    while (parseLine(ifs) >= 0);

    ifs.close();
    return 0;
  }
  return -1;
}

