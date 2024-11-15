/**
 *
 */

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

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

void sha256_hash_string(uint8_t *hash, char *signature)
{
    char digit[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    for (int i = 0; i < 32; i++)    {
        uint8_t val = *hash++;
        *signature++ = digit[val >> 4];
        *signature++ = digit[val & 0x0f];
    }
}

int SignFile(string &path, int64_t &reallen, char signature[SIGNATURE_SIZE])
{
    fstream fin(path, fstream::in | fstream::binary);
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    static char buffer[FILE_BUFFER_SIZE];
    int bytesRead = 0;
    int64_t length = 0;

    memset(signature, 0, SIGNATURE_SIZE);
    if(!buffer) return ENOMEM;
    //while((bytesRead = fread(buffer, 1, FILE_BUFFER_SIZE, file)))
    do {
        fin.read(buffer, FILE_BUFFER_SIZE);
        bytesRead = fin.gcount();
        SHA256_Update(&sha256, buffer, bytesRead);
        length += bytesRead;
    } while (bytesRead == FILE_BUFFER_SIZE);
    SHA256_Final(hash, &sha256);
    reallen = length;
    sha256_hash_string(hash, signature);
    cout << "Path " << path << ", length " << reallen << ", signature " << signature << "\n";

    fin.close();
    return 0;
}


/**
 *
 */
int PatchSignature(int64_t idx_files, int64_t reallen, char *signature)
{
    const char qry[] =
        "update files set reallen = $2, hash = $3 where idx_files = $1";
    string sidx = to_string(idx_files);
    string slen = to_string(reallen);

    const char *args[] = { sidx.c_str(), slen.c_str(), signature };
    PGresult *res = PQexecParams(pg, qry, 3, NULL, args, NULL, NULL, 0);

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
int GetEntry(int &idx_dirs, int &idx_files, string &path)
{
    string snum = to_string(idx_files);
    const char *args[1] = { snum.c_str() };
    const char qry[] =
        "select f.idx_dirs, f.idx_files, d.path, f.filename, f.statlen "
        "from files f left join dirs d on f.idx_dirs = d.idx_dirs "
        "where f.reallen in (-1,-2) and f.idx_files > $1 order by f.idx_files limit 1";
    PGresult *res = PQexecParams(pg, qry, 1, NULL, args, NULL, NULL, 0);

    idx_files = -1;
    if (res != NULL)    {
        ExecStatusType stat = PQresultStatus(res);
        if ((stat == PGRES_TUPLES_OK) || (stat == PGRES_COMMAND_OK))    {
            int rows = PQntuples(res);

            if (rows > 0)   {
                int db_statlen = atoi(PQgetvalue(res, 0, 4));
                char *db_name = PQgetvalue(res, 0, 3);

                idx_dirs = atoi(PQgetvalue(res, 0, 0));
                idx_files = atoi(PQgetvalue(res, 0, 1));
                path = string(PQgetvalue(res, 0, 2)) + "/";
                path += db_name;
                cout << "##> " << idx_files << ": " << path << "\n";
            }
        } else {
            cout << "**> Entry " << idx_dirs << "/" << idx_files << " not found\n"; 
        }
        PQclear(res);
    }
    return idx_files;
}


/** Scan the table one row at a time. 
 *
 */
void ScanTable(void)
{
    int idx_dirs = 0, idx_files = 0;
    string path;
    int iteration = 0;
    int64_t reallen;
    char signature[SIGNATURE_SIZE];

    //BeginTrx();
    while (0 < GetEntry(idx_dirs, idx_files, path))   {
        SignFile(path, reallen, signature);
        PatchSignature(idx_files, reallen, signature);
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

