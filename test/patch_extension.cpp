/**
 *
 */

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>

#include <openssl/sha.h>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;


#include <postgresql/libpq-fe.h>

#define FILE_BUFFER_SIZE    32768
#define SIGNATURE_SIZE      68

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

/**
 *
 */
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

/**
 *
 */
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

/** Patch field dirname in table
 * @param idx_dirs  Index of the row
 * @param dirname   Directory name
 * @return Returns 0
 */
int PatchDirname(int64_t idx_dirs, string &dirname)
{
    const char qry[] = "update dirs set dirname = $2 where idx_dirs = $1 returning idx_dirs;";
    //const char qry[] = "select 1;";
    string snum = to_string(idx_dirs);
    const char *args[] = { snum.c_str(), dirname.c_str() };

    PGresult *res = PQexecParams(pg, qry, 2, NULL, args, NULL, NULL, 0);
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
int PatchExtension(int64_t idx_files, string &extension)
{
    const char qry[] = "update files set ext = $2 where idx_files = $1 returning idx_files;";
    //const char qry[] = "select 1;";
    string sidx = to_string(idx_files);
    const char *args[] = { sidx.c_str(), extension.c_str() };
    PGresult *res = PQexecParams(pg, qry, 2, NULL, args, NULL, NULL, 0);

    if (res != NULL)    {
        ExecStatusType stat = PQresultStatus(res);
        if ((stat != PGRES_TUPLES_OK) && (stat != PGRES_COMMAND_OK))    {
            return -1;
        }
        PQclear(res);
    }
    return 0;
}

/** Get the index and path of the next row
 * @param idx_dirs  Index of the "dirs" table (OUT)
 * @param idx_files Index of the next entry in the "files" table (INOUT)
 * @param path      Full path of the file entry (OUT)
 * @return          Returns the index of the "files" table, or -1 if the endo of the table is reached
 */
int GetDirEntry(int64_t &idx_dirs, string &path, string &dirname)
{
    string snum = to_string(idx_dirs);
    const char *args[1] = { snum.c_str() };
    const char qry[] =
        "select idx_dirs, path "
        "from dirs "
        "where idx_dirs > $1 order by idx_dirs limit 1";
    PGresult *res = PQexecParams(pg, qry, 1, NULL, args, NULL, NULL, 0);

    idx_dirs = -1;
    dirname = string("");
    if (res != NULL)    {
        ExecStatusType stat = PQresultStatus(res);
        if ((stat == PGRES_TUPLES_OK) || (stat == PGRES_COMMAND_OK))    {
            int rows = PQntuples(res);

            if (rows > 0)   {
                dirname = string(basename(PQgetvalue(res, 0, 1)));

                idx_dirs = atoi(PQgetvalue(res, 0, 0));
                //cout << "##> " << idx_files << ": " << filename << " -> " << ext <<  "\n";
            }
        } else {
            cout << "**> Entry " << idx_dirs << " not found\n"; 
        }
        PQclear(res);
    }
    return idx_dirs;
}

/** Get the index and path of the next row
 * @param idx_dirs  Index of the "dirs" table (OUT)
 * @param idx_files Index of the next entry in the "files" table (INOUT)
 * @param path      Full path of the file entry (OUT)
 * @return          Returns the index of the "files" table, or -1 if the endo of the table is reached
 */
int GetFilesEntry(int64_t &idx_files, string &filename, string &ext)
{
    string snum = to_string(idx_files);
    const char *args[1] = { snum.c_str() };
    const char qry[] =
        "select f.idx_files, f.filename "
        "from files f "
        "where f.idx_files > $1 order by f.idx_files limit 1";
    PGresult *res = PQexecParams(pg, qry, 1, NULL, args, NULL, NULL, 0);

    idx_files = -1;
    ext = string("");
    if (res != NULL)    {
        ExecStatusType stat = PQresultStatus(res);
        if ((stat == PGRES_TUPLES_OK) || (stat == PGRES_COMMAND_OK))    {
            int rows = PQntuples(res);

            if (rows > 0)   {
                char *ptr = PQgetvalue(res, 0, 1);
                char *sep = (char *) "";
                int len;

                filename = string(ptr);
                idx_files = atoi(PQgetvalue(res, 0, 0));
                while (0 != *ptr)   {
                    if ('.' == *ptr)    {
                        sep = ptr + 1;
                    }
                    ptr++;
                }
                ext = string(sep);
                len = ext.length();
                for (int i = 0; i < len; i++)   {
                    ext[i] = tolower(ext[i]);
                }
                //cout << "##> " << idx_files << ": " << filename << " -> " << ext <<  "\n";
            }
        } else {
            cout << "**> Entry " << idx_files << " not found\n"; 
        }
        PQclear(res);
    }
    return idx_files;
}


/** Scan the table one row at a time. 
 *
 */
void AddExtensionsToFiles(void)
{
    int64_t idx_files = 0;
    string filename, extension;
    int iteration = 0;

    //BeginTrx();
    while (0 < GetFilesEntry(idx_files, filename, extension))   {
        if (idx_files < 0)  {
            break;
        }
        PatchExtension(idx_files, extension);
        if (iteration >= 5)    {
            //CommitTrx();
            //BeginTrx();
            iteration = 0;
        }
        iteration++;
    }
    //CommitTrx();
}

/** Scan the table one row at a time. 
 *
 */
void AddDirnameToDirs(void)
{
    int64_t idx_dirs = 0;
    string path, dirname;
    int iteration = 0;

    //BeginTrx();
    while (0 < GetDirEntry(idx_dirs, path, dirname))   {
        if (idx_dirs < 0)  {
            break;
        }
        PatchDirname(idx_dirs, dirname);
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
        //AddExtensionsToFiles();
        AddDirnameToDirs();
    }
    return 0;
}

