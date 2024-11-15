/**
 * 
*/

#include <iostream>
#include <cstring>

#include "include/dbconn.h"

DbConn::DbConn(const char *connstr)
{
  conn = PQconnectdb(connstr);
  str = connstr;
  ConnStatusType stat = PQstatus(conn);
  cout << "Connection: " << ((stat == CONNECTION_OK) ? "OK" : "bad") << endl;
}


DbConn::~DbConn()
{
  cout << "Closing DB connection\n";
  PQfinish(conn);
}

static const char *DecodeErr(const PGresult *res)
{
  ExecStatusType stat = PQresultStatus(res);
  const char *err = "unknown";

  switch (stat) {
    case PGRES_EMPTY_QUERY:     err = "empty";  break;
    case PGRES_COMMAND_OK:      return NULL;  // err = "command ok";  break;
    case PGRES_TUPLES_OK:       return NULL;  // err = "tuples ok";  break;
    case PGRES_COPY_OUT:        err = "copy out";  break;
    case PGRES_COPY_IN:         err = "copy in";  break;
    case PGRES_BAD_RESPONSE:    err = "bad response";  break;
    case PGRES_NONFATAL_ERROR:  err = "non-fatal error";  break;
    case PGRES_FATAL_ERROR:     err = "fatal error";  break;
    case PGRES_COPY_BOTH:       err = "copy both";  break;
    case PGRES_SINGLE_TUPLE:    err = "single tuple";  break;
  };
  return err;
}

uint64_t DbConn::appendFile(const char *host, const char *path,
        const char *filename, const uint64_t statlen, const uint64_t reallen,
        const uint64_t disk_usage, const char *hash,
        const uint64_t mtime, const uint64_t mtime_nsec)
{
  PGresult *res;
  uint64_t idxdir = -1;
  string statlen_str = to_string(statlen);
  string reallen_str = to_string(reallen);
  string disk_usage_str = to_string(disk_usage);
  string mtime_str = to_string(mtime);
  string mtime_nsec_str = to_string(mtime_nsec);
  const char *params[] = {
    host, path, filename, statlen_str.c_str(), reallen_str.c_str(),
    disk_usage_str.c_str(), hash, mtime_str.c_str(), mtime_nsec_str.c_str()
  };
  const char *error_str;

  res = PQexecParams(conn,
        "SELECT get_file_idx($1, $2, $3, $4, $5, $6, $7, $8, $9);",
        9, NULL, params, NULL, NULL, 0);

  if (NULL != error_str)  {
    cout << "SQL executed: " << DecodeErr(res) << endl;
  } else if (PQntuples(res) > 0)  {
    idxdir = atoi(PQgetvalue(res, 0, 0));
    //cout << "idxdir = " << idxdir << endl;
  }
  PQclear(res);
  return idxdir;
}

