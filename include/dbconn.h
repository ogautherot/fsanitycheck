/**
 * 
*/

#ifndef __DB_CONN_H__
#define __DB_CONN_H__

#include <cstdint>

#include <postgresql/libpq-fe.h>

using namespace std;


class DbConn {
public:
  DbConn(const char *connstr);
  ~DbConn();

  uint64_t appendFile(const char *host, const char *path, const char *filename,
        const uint64_t statlen, const uint64_t reallen,
        const uint64_t disk_usage, const char *hash,
        const uint64_t mtime, const uint64_t mtime_nsec);

private:
  PGconn  *conn;
  string str;

};


#endif // __DB_CONN_H__