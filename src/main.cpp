/* ***************************************************************************
* Configuration file support library
*
* (C) Olivier Gautherot, 2024
* ************************************************************************* */


#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "include/initfile.h"
#include "include/applib.h"
#include "include/dbconn.h"

using namespace std;

const char *progname;

char buf[2048];


char *SplitLine(char *s)
{
  char *last = NULL;

  while (0 != *s)	{
    if (*s == '\n')	{
      *s = 0;
      break;
    } else if (*s == '/')	{
      last = s;
    }
    s++;
  }
  *last = 0;
  return last + 1;
}

void DummyParse(DbConn *dbconn)
{
  char *name;
  char host[32];
  gethostname(host, sizeof(host));

  while (NULL != fgets(buf, sizeof(buf), stdin))  {
    name = SplitLine(buf);
    cout << "dir -> " << buf << ", file -> " << name << "\n";  
    dbconn->appendFile(host, buf, name, 0, 0, 0, "", 0, 0);
  }
}


int ParseDir(DbConn *dbconn, string &path, string &host)
{
  DIR *dirp = opendir(path.c_str());

  if (NULL != dirp) {
    struct dirent *ent;
    while (NULL != (ent = readdir(dirp))) {
      struct stat buf;
      if ((0 == strcmp(ent->d_name, ".")) || (0 == strcmp(ent->d_name, "..")))  {
        continue;
      }
      string entryname = path + "/" + ent->d_name;
      if (0 == lstat(entryname.c_str(), &buf)) {
        if (S_ISREG(buf.st_mode)) {
          uint64_t du = buf.st_blksize;

          du *= buf.st_blocks;
          FileReg reg((const char *) "localhost", path.c_str(),
                      (const char *) ent->d_name, (uint64_t) buf.st_size, du,
                      buf.st_mtim.tv_sec, buf.st_mtim.tv_nsec);
          dbconn->appendFile(reg.GetHost(), reg.GetPath(), reg.GetName(),
              reg.GetStatLen(), reg.GetRealLen(), reg.GetDiskUsage(), reg.GetHash(),
              reg.GetMtime(), reg.GetMtimeNsec());
          //cout << "    *** INFO: block size " << buf.st_blksize << ", " << buf.st_blocks << " block(s)\n";
        } else if (S_ISDIR(buf.st_mode)) {
          ParseDir(dbconn, entryname, host);
        } else if (S_ISLNK(buf.st_mode)) {
          //cout << "    link: " << entryname << endl;
        } else  {
          cerr << "      Ignoring " << entryname << ": " << getEntryType(buf.st_mode) << endl;
        }
      }
    }
    closedir(dirp);
    return 0;
  }
  return -1;
}

/**
*************************************************************************** */
void welcome(const char *progname)
{
  cerr << progname << " - Version 0.0.1" << endl;
  cerr << "Build date: " << __DATE__ " " __TIME__ << "\n";
}

/**
*************************************************************************** */
int main(int argc, char *argv[])
{
  progname = argv[0];
  welcome(filename(progname));
  InitFile *initfile = new InitFile;

  initfile->parseArgs(argc, argv);
  DbConn *dbconn = new DbConn(initfile->getConnInfo().c_str());

  cout << "Connection string: " << initfile->getConnInfo().c_str() << endl;

  DummyParse(dbconn);

  //cout << "conninfo: " << initfile->getConnInfo() << endl;
  /// @todo Parse directories
  for (int i = 0; i < initfile->scanPath.size(); i++) {
    cout << "Scanning " << initfile->scanPath.at(i) << endl;
    ParseDir(dbconn, initfile->scanPath.at(i), initfile->getHostname());
  }
  delete dbconn;
}

