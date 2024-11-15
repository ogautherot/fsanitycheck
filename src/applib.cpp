/* ***************************************************************************
* Generic library
*
* (C) Olivier Gautherot, 2024
* ************************************************************************* */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

//#include <cryptopp/sha.h>
#include <openssl/md5.h>

#include <cctype>
#include <iostream>
#include <fstream>

#include "include/applib.h"

using namespace std;

#define BLK_SIZE  4096

/**
 * Remove the directory part of the path
 * 
 * @param name  File name
 * 
 * @return The file name as listed in the directory
*/
const char *filename(const char *name)
{
  const char *ret = name;

  if (NULL != name) {
    while (0 != *name)  {
      if ('/' == *name) {
        char next = name[1];
        if ((0 != next) && ('/' != next) && ('\\' != next)) {
          ret = name + 1;
        }
      }
      name++;
    }
  }

  return ret;
}

/**
 * Convert a number from integer format to ASCII.
 * 
 * @param value   Value to convert.
 * @param str     Output buffer, provided by the caller.
 * @param base    Conversion base, between 2 and 16.
 * 
 * @return        Pointer to the output buffer.
*/
const char *itoa(int value, char *str, int base)
{
  const char *ret = str;
  const char digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };

  if (0 == value) {
    str[0] = '0';
    str[1] = 0;
  } else {
    // Retrieve text in reverse order (easy to get)
    char *swap1 = str, *swap2;
    while (0 != value)  {
      *str = digits[value % base];
      value /= base;
      str++;
    }

    // ... and swap order
    *str = 0;
    swap2 = str - 1;
    while (swap2 > swap1) {
      char c = *swap1;
      *swap1 = *swap2;
      *swap2 = c;
      swap1++, swap2--;
    }
  }
  return ret;
}

/**
 * Count the number of occurences of the given pattern.
 * 
 * @param str   
 * @param c
 * 
 * @return      Number of occurences of the pattern.
*/
int strcnt(const char *str, char c)
{
  int cnt = 0;

  if (str)  {
    while ('0' != *str) {
      if (c == *str) {
        cnt++;
      }
      str++;
    }
  }
  return cnt;
}

/**
 * @param path  Executable search path.
 * 
 * @return System type, SYSTEM_IS_POSIX or SYSTEM_IS_WINDOWS
*/
SystemType checksystemtype(const char *path)
{
  int slash = 0, backslash = 0;
  int colon = 0, semicolon = 0;
  int isposix = 0, iswindows = 0;
  const char *winpathstart = path;
  const char *pos = path;

  if (path) {
    while (0 != *pos) {
      switch (*pos) {
        case '/':
          slash++;
          break;
        case '\\':
          backslash++;
          break;
        case ':':
          if ((1 != (pos - winpathstart)) || (!isalpha(*winpathstart))) {
            colon++;
          }
          break;
        case ';':
          semicolon++;
          winpathstart = pos + 1;
          break;
      }
      pos++;
    }
    if (0 < colon) {
      isposix++;
      iswindows--;
    }
    if (0 < semicolon) {
      iswindows++;
      if (0 == colon) {
        isposix--;
      }
    }
    if (0 < slash)  {
      isposix++;
      iswindows--;
    }
    if (0 < backslash) {
      iswindows++;
      isposix--;
    }
  }
  return (isposix > iswindows) ? SYSTEM_IS_POSIX :
          (isposix < iswindows) ? SYSTEM_IS_WINDOWS :
          SYSTEM_IS_UNKNOWN;
}

string getenvvar(const char *name)
{
  const char *val = getenv(name);

  if (NULL == val)  {
    val = "";
  }
  return string(val);
}

const char *getEntryType(int mode)
{
  const char sBlockDevice[] = "block device";
  const char sCharDevice[] = "character device";
  const char sDirectory[] = "directory";
  const char sFifo[] = "FIFO";
  const char sSoftLink[] = "soft link";
  const char sRegularFile[] = "regular file";
  const char sSocket[] = "socket";
  const char sUndefinedType[] = "undefined type";

  return (S_ISBLK(mode)) ? sBlockDevice :
          (S_ISCHR(mode)) ? sCharDevice :
          (S_ISDIR(mode)) ? sDirectory :
          (S_ISFIFO(mode)) ? sFifo :
          (S_ISLNK(mode)) ? sSoftLink :
          (S_ISREG(mode)) ? sRegularFile :
          (S_ISSOCK(mode)) ? sSocket :
          sUndefinedType;
}

FileReg::FileReg(const char *host, const char *path, const char *name, 
                uint64_t len, uint64_t diskusage, uint64_t mtime, uint64_t mtime_nsec,
                bool do_crypt)
{
  string fullpath = string(path) + "/" + name;

  Host = host;
  Path = path;
  Name = name;
  StatLen = len;
  DiskUsage = diskusage;
  RealLen = 0;
  Hash = "";
  Mtime = mtime;
  MtimeNsec = mtime_nsec;

  if (do_crypt) {
    ifstream fs(fullpath.c_str(), ios::binary);
    MD5_CTX ctx;
    uint8_t md[16] = { 0, };
    char buf[BLK_SIZE];

    MD5_Init(&ctx);
    do
    {
      len = fs.readsome(buf, BLK_SIZE);
      if (0 < len) {
        memset(buf + len, 0, BLK_SIZE - len);
        MD5_Update(&ctx, buf, len);
        RealLen += len;
      }
    } while (0 < len);

    MD5_Final(md, &ctx);
    fs.close();
    SetHash(md);
  }
  Dump();
}

const char      *FileReg::GetHost()
{
  return Host.c_str();
}

const char      *FileReg::GetPath()
{
  return Path.c_str();
}

const char      *FileReg::GetName()
{
  return Name.c_str();
}

uint64_t        FileReg::GetStatLen()
{
  return StatLen;
}

uint64_t        FileReg::GetRealLen()
{
  return RealLen;
}

uint64_t        FileReg::GetDiskUsage()
{
  return DiskUsage;
}

const char      *FileReg::GetHash()
{
  return Hash.c_str();
}

uint64_t        FileReg::GetMtime()
{
  return Mtime;
}

uint64_t        FileReg::GetMtimeNsec()
{
  return MtimeNsec;
}


void	FileReg::SetHash(uint8_t *hash)
{
  const char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  for (int i = 0; i < 16; i++) {
    uint8_t byte = hash[i];
    Hash.push_back(digits[byte >> 4]);
    Hash.push_back(digits[byte & 0x0f]);
  }
}

void	FileReg::Dump(void)
{
  cout << "Host: " << Host << "\tPath: " << Path << "\tFile: " << Name 
      << "\tStatlen: " << StatLen << "\tReallen: " << RealLen << "\tDisk usage: " 
      << DiskUsage << "\tHash: " << Hash << "\n";
}


