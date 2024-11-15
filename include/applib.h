/* ***************************************************************************
* Generic library
*
* (C) Olivier Gautherot, 2024
* ************************************************************************* */

#ifndef __APPLIB_H__
#define __APPLIB_H__

#include <string>


#ifdef __cplusplus
extern "C" {
#endif

#include <string>

#include <sys/stat.h>
#include <stdlib.h>

using namespace std;


typedef enum SystemType {
  SYSTEM_IS_UNKNOWN = 0,
  SYSTEM_IS_POSIX,
  SYSTEM_IS_WINDOWS
} SystemType;

const char *filename(const char *name);
const char *itoa(int value, char *str, int base);
int strcnt(const char *str, char c);
SystemType checksystemtype(const char *path);
string getenvvar(const char *name);
const char *getEntryType(int mode);

#ifdef __cplusplus
}
#endif

class FileReg {
	public:
		FileReg(const char *host, const char *path, const char *name, uint64_t len, 
				uint64_t diskusage, uint64_t mtime, uint64_t mtime_nsec,
				bool do_crypt = false);

		const char 	*GetHost();
		const char 	*GetPath();
		const char 	*GetName();
		uint64_t	GetStatLen();
		uint64_t	GetRealLen();
		uint64_t	GetDiskUsage();
		const char 	*GetHash();
		uint64_t 	GetMtime();
		uint64_t  	GetMtimeNsec();

	private:
		string 		Host;
		string 		Path;
		string 		Name;
		uint64_t	StatLen;
		uint64_t	RealLen;
		uint64_t	DiskUsage;
		string		Hash;
		uint64_t	Mtime;
		uint64_t 	MtimeNsec;

		void		SetStatLen(uint64_t len);
		void		SetRealLen(uint64_t len);
		void		SetHash(uint8_t *hash);

		void		Dump(void);
};

#endif // __APPLIB_H__
