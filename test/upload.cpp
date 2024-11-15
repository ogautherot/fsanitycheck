/**
 *
 */

#include <cstdint>
#include <cstdlib>
#include <unistd.h>

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
	if (pg == 0)	{
		return -1;
	}
	return 0;
}

/** Add an entry in the "dirs" table. If the path is already in the table,
 * the function returns the index of the existing row.
 *
 * @param host	Host name.
 * @param path	Path entry
 *
 * @return	Returns the index of the row containing the path, or -1 if it couldn't be inserted.
 */
int AddPath(char *host, char *path)
{
	int row = -1;
	char *args[2] = { host, path };
	const char qry[] = "select get_path_idx($1, $2)";
	PGresult *res = PQexecParams(pg, qry, 2, NULL, args, NULL, NULL, 0);
	if (res != NULL)	{
		ExecStatusType stat = PQresultStatus(res);
		if ((stat == PGRES_TUPLES_OK) || (stat == PGRES_COMMAND_OK))	{
			row = atoi(PQgetvalue(res, 0, 0));
			//cout << "**> Found " << row << " (" << path << ")\n";
		} else {
			cout << "**> Unable to insert " << path << "\n";
		}
		PQclear(res);
	}
	return row;
}

/**
 *
 */
int AddFile(char *host, char *path, char *fname)
{
	int row = -1;
	char *args[3] = { host, path, fname };
	const char qry[] = "select get_file_idx($1, $2, $3)";
	PGresult *res = PQexecParams(pg, qry, 3, NULL, args, NULL, NULL, 0);
	if (res != NULL)	{
		ExecStatusType stat = PQresultStatus(res);
		if ((stat == PGRES_TUPLES_OK) || (stat == PGRES_COMMAND_OK))	{
			row = atoi(PQgetvalue(res, 0, 0));
			// cout << "**> Found " << row << " (" << path << " " << fname << ")\n";
		} else {
			cout << "**> Unable to insert file " << path << " " << fname << "\n"; 
		}
		PQclear(res);
	}
	return row;
}

/**
 *
 */
void EnumeratePath(char *host, char *line)
{
	//char *end = line;
	char *ptr = line + 1;
	char *fname = NULL;

	if (*line != '/')	{
		cout << "Absolute path required\n";
	} else {
		do {
			if (*ptr == '/')	{
				fname = ptr + 1;
				*ptr = 0;
				//cout << "==> " << line << "\n";
				//AddPath(host, line);
				*ptr = '/';
			}
			ptr++;
		} while (*ptr);
		//end = ptr;
		fname[-1] = 0;
		AddFile(host, line, fname);
		// cout << "**> File name: " << fname << "\n";
	}
}

/**
 *
 */
void ParseFile(string &name)
{
	fstream fs(name.c_str(), fstream::in | fstream::binary);
	char line[1024];
	char host[32];
	gethostname(host, sizeof(host));

	while(!fs.eof())	{
		fs.getline(line, sizeof(line));
		// cout << "->" << line << "\n";
		EnumeratePath(host, line);
	}
}

/**
 *
 */
void ProcessFiles(vector<string> &filelist)
{
	for (unsigned int i = 0; i < filelist.size(); i++)	{
		string name = filelist.at(i);
		cout << "File name: " << name << "\n";
		ParseFile(name);
	}
}

/**
 *
 */
void ParseArgs(int ac, char *av[], char *&progname, vector<string> &filelist)
{
	(void) progname;
	for (int i = 1; i < ac; i++)	{
		if (av[i][0] == '-')	{
			// Parser option
		} else {
			filelist.push_back(av[i]);
		}
	}
}

/**
 *
 */
int main(int ac, char *av[]) {
    //
	vector <string> filelist;
    char *progname = *av;
	char connstr[] = "dbname=fsanitycheck2";

	ParseArgs(ac, av, progname, filelist);
	if (OpenConnection(connstr) == 0) {
		ProcessFiles(filelist);
	}

    return 0;
}

