/**
 *
 */

#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;


#include <postgresql/libpq-fe.h>

//char *progname;

PGconn *pg;

/** Open the database connection. It sets the global variable pg.
 *
 * @param connstr	Database connection string
 * @return Returns 0 upon success, or -1 upon error.
 */
int OpenConnection(char *connstr)
{
    pg = PQconnectdb(connstr);
    if (pg == 0)    {
        return -1;
    }
    return 0;
}

void BeginTrx()
{
    const char qry[] = "begin";
    PGresult *res = PQexecParams(pg, qry, 0, NULL, NULL, NULL, NULL, 0);

    if (res != NULL)    {
        ExecStatusType stat = PQresultStatus(res);
        if ((stat == PGRES_TUPLES_OK) || (stat == PGRES_COMMAND_OK))    {
        }
        PQclear(res);
    }
}

void CommitTrx()
{
    const char qry[] = "commit";
    PGresult *res = PQexecParams(pg, qry, 0, NULL, NULL, NULL, NULL, 0);

    if (res != NULL)    {
        ExecStatusType stat = PQresultStatus(res);
        if ((stat == PGRES_TUPLES_OK) || (stat == PGRES_COMMAND_OK))    {
        }
        PQclear(res);
    }
}

int64_t GetStatLen(string &path, int64_t &t_sec, int64_t &t_nsec, int64_t &disk_usage)
{
    struct stat sb;
    int64_t ret = -1;

    t_sec = t_nsec = 0;
    if (lstat(path.c_str(), &sb) == 0)  {
        if (S_IFREG == (sb.st_mode & S_IFMT))   {
            ret = sb.st_size;
            t_sec = sb.st_mtim.tv_sec;
            t_nsec = sb.st_mtim.tv_nsec;
            disk_usage = sb.st_blocks * 512;
        } else {
            ret = -3;
        }
    }
    return ret;  
}

/**
 *
 */
int PatchStatLen(int64_t idx_files, int64_t statlen, int64_t &t_sec, int64_t &t_nsec, int64_t &disk_usage)
{
    const char qry[] =
        "update files set statlen = $2, reallen = $3, mtime = $4, "
        "mtime_nsec = $5, disk_usage = $6 where idx_files = $1";
    string sidx = to_string(idx_files);
    string slen = to_string(statlen);
    string sreallen = to_string(-2);
    string smtime = to_string(t_sec);
    string smtime_nsec = to_string(t_nsec);
    string sdisk_usage = to_string(disk_usage);

    const char *args[] = { sidx.c_str(), slen.c_str(), sreallen.c_str(), smtime.c_str(), smtime_nsec.c_str(), sdisk_usage.c_str() };
    PGresult *res = PQexecParams(pg, qry, 6, NULL, args, NULL, NULL, 0);

    if (res != NULL)    {
        ExecStatusType stat = PQresultStatus(res);
        if ((stat != PGRES_TUPLES_OK) && (stat != PGRES_COMMAND_OK))    {
            return -1;
        }
        PQclear(res);
    }
    return 0;
}

/**
 *
 */
int GetEntry(int &idx_dirs, int &idx_files, string &path)
{
    string snum = to_string(idx_files);
    const char *args[1] = { snum.c_str() };
    const char qry[] =
        "select f.idx_dirs, f.idx_files, d.path, f.filename, f.statlen "
        "from files f left join dirs d on f.idx_dirs = d.idx_dirs "
        "where f.statlen = -1 and f.idx_files > $1 order by f.idx_files limit 1";
    int64_t statlen;
    PGresult *res = PQexecParams(pg, qry, 1, NULL, args, NULL, NULL, 0);

    idx_files = -1;
    if (res != NULL)    {
        ExecStatusType stat = PQresultStatus(res);
        if ((stat == PGRES_TUPLES_OK) || (stat == PGRES_COMMAND_OK))    {
            int rows = PQntuples(res);

            if (rows > 0)   {
                int db_statlen = atoi(PQgetvalue(res, 0, 4));
                char *db_name = PQgetvalue(res, 0, 3);
                int64_t t_sec, t_nsec;
                int64_t disk_usage = 0;

                idx_dirs = atoi(PQgetvalue(res, 0, 0));
                idx_files = atoi(PQgetvalue(res, 0, 1));
                path = string(PQgetvalue(res, 0, 2)) + "/";
                path += db_name;
                statlen = GetStatLen(path, t_sec, t_nsec, disk_usage);
                // cout << "##> " << path << ": " << statlen << "\n";
                PatchStatLen(idx_files, statlen, t_sec, t_nsec, disk_usage);
            }
        } else {
            cout << "**> Entry " << idx_dirs << "/" << idx_files << " not found\n"; 
        }
        PQclear(res);
    }
    return idx_files;
}


/**
 *
 */
void ScanTable(void)
{
    int idx_dirs = 0, idx_files = 0;
    string path;
    int iteration = 0;

    //BeginTrx();
    while (0 < GetEntry(idx_dirs, idx_files, path))   {
        if (idx_files < 0)  {
            break;
        }
        if (iteration >= 5)    {
            //CommitTrx();
            //BeginTrx();
            iteration = 0;
        }
        iteration++;
    }
    //CommitTrx();
}

/**
 *
 */
int main(int ac, char *av[]) {
    //
    char connstr[] = "dbname=fsanitycheck2 user=olivier host=localhost port=5432";

    if (OpenConnection(connstr) == 0) {
        //BeginTrx();
        ScanTable();
        //CommitTrx();
    }
    return 0;
}

