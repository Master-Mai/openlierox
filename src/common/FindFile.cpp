/////////////////////////////////////////
//
//   OpenLieroX
//
//   Auxiliary Software class library
//
//   based on the work of JasonB
//   enhanced by Dark Charlie and Albert Zeyer
//
//   code under LGPL
//
/////////////////////////////////////////


// File finding routines
// Created 30/9/01
// By Jason Boettcher


#include "defs.h"
#include "LieroX.h"

#ifdef WIN32
	#include <shlobj.h>
#else
	#include <pwd.h>
#endif

// TODO: this only checks, if it is a file
// if this info is enough for use, rename the function
// if it is not enough, extend it
bool CanReadFile(const std::string& f, bool absolute) {
	static std::string abs_f;
	if(absolute) {
		if(!GetExactFileName(f, abs_f)) return false;
	} else
		if((abs_f = GetFullFileName(f)) == "") return false;

	// HINT: this should also work on WIN32, as we have _stat here
	// (see the #include and #define in defs.h)
	struct stat s;
	if(stat(abs_f.c_str(), &s) != 0 || !S_ISREG(s.st_mode)) {
		// it's not stat-able or not a reg file
		return false;
	}

	// it's stat-able and a file
	return true;
}

/*

	Drives

*/

////////////////////
//
drive_list GetDrives(void)
{
static drive_list list;
list.clear();
#ifdef WIN32
	static char drives[34];
	int len = GetLogicalDriveStrings(sizeof(drives),drives); // Get the list of drives
	drive_t tmp;
	if (len)  {
		for (register int i=0; i<len; i+=strnlen(&drives[i],4)+1)  {
			// Create the name (for example: C:\)
			tmp.name = &drives[i];
			// Get the type
			tmp.type = GetDriveType((LPCTSTR)tmp.name.c_str());
			// Add to the list
			list.push_back(tmp);
		}
	}


#else
	// there are not any drives on Linux/Unix/MacOSX/...
	// it's only windows which uses this crazy drive-letters
	
	// perhaps not the best way
	// home-dir of user is in other applications the default
	// but it's always possible to read most other stuff
	// and it's not uncommon that a user hase a shared dir like /mp3s
	drive_t tmp;
	tmp.name = "/";
	tmp.type = 0;
	list.push_back(tmp);
	
	// we could communicate with dbus and ask it for all connected
	// and mounted hardware-stuff
#endif
 
	return list;
}


#ifndef WIN32

// TODO: use std::string!
// used by unix-GetExactFileName
int GetNextName(const char* fullname, const char** seperators, char* nextname)
{
	int pos;
	int i;

	for(pos = 0; fullname[pos] != '\0'; pos++)
	{
		for(i = 0; seperators[i] != NULL; i++)
			if(strncasecmp(&fullname[pos], seperators[i], strlen(seperators[i])) == 0)
			{
				nextname[pos] = '\0';
				return pos + strlen(seperators[i]);
			}

		nextname[pos] = fullname[pos];
	}

	nextname[pos] = '\0';
	return 0;
}


// TODO: use std::string!
// used by unix-GetExactFileName
int CaseInsFindFile(const char* dir, const char* searchname, char* filename)
{
	if(strcmp(searchname, "") == 0)
	{
		strcpy(filename, "");
		return true;
	}

	DIR* dirhandle;
	dirhandle = opendir((strcmp(dir, "") == 0) ? "." : dir);
	if(dirhandle == 0) return false;

	dirent* direntry;
	while((direntry = readdir(dirhandle)))
	{
		if(strcasecmp(direntry->d_name, searchname) == 0)
		{
			strcpy(filename, direntry->d_name);
			closedir(dirhandle);
			return true;
		}
	}

	closedir(dirhandle);
	return false;
}


// does case insensitive search for file
bool GetExactFileName(const std::string& abs_searchname, std::string& filename)
{
	if(abs_searchname.size() == 0) {
		filename = "";
		return false;
	}

	std::string sname = abs_searchname;
	ReplaceFileVariables(sname);

	// TODO: ouhhhhh, this has to be redone... ! (use std::string)
	const char* seps[] = {"\\", "/", (char*)NULL};
	static char nextname[512]; strcpy(nextname, "");
	static char nextexactname[512]; strcpy(nextexactname, "");
	filename = "";
	int pos = 0;
	int npos = 0;
	do {
		pos += npos;
		if(npos > 0) filename += "/";

		npos = GetNextName(&sname.c_str()[pos], seps, nextname);

		if(!CaseInsFindFile(filename.c_str(), nextname, nextexactname))
		{
			filename += &sname.c_str()[pos];
			return false;
		}

		filename += nextexactname;

	} while(npos > 0);

	return true;
}

#endif // not WIN32


searchpathlist	basesearchpaths;
void InitBaseSearchPaths() {
	// TODO: it would be nice to have also Mac OS X conversions
	basesearchpaths.clear();
#ifndef WIN32
	AddToFileList(&basesearchpaths, "${HOME}/.OpenLieroX");
	AddToFileList(&basesearchpaths, ".");
	AddToFileList(&basesearchpaths, SYSTEM_DATA_DIR"/OpenLieroX"); // no use of ${SYSTEM_DATA}, because it is uncommon and could cause confusion to the user
#else // Win32
	AddToFileList(&basesearchpaths, "${HOME}/OpenLieroX");
	AddToFileList(&basesearchpaths, ".");
	AddToFileList(&basesearchpaths, "${BIN}");
#endif
}

void CreateRecDir(const std::string& abs_filename, bool last_is_dir) {
	static std::string tmp;
	std::string::const_iterator f = abs_filename.begin();
	for(tmp = ""; f != abs_filename.end(); f++) {
		if(*f == '\\' || *f == '/')
			mkdir(tmp.c_str(), 0777);
		tmp += *f;
	}
	if(last_is_dir)
		mkdir(tmp.c_str(), 0777);
}

std::string GetFirstSearchPath() {
	if(tLXOptions->tSearchPaths.size() > 0)
		return tLXOptions->tSearchPaths.front();
	else if(basesearchpaths.size() > 0)
		return basesearchpaths.front();
	else
		return GetHomeDir();
}



	class CheckSearchpathForFile { public: 
		const std::string& filename;
		std::string* result;
		std::string* searchpath;
		CheckSearchpathForFile(const std::string& f, std::string* r, std::string* s) : filename(f), result(r), searchpath(s) {}
		inline bool operator() (const std::string& spath) {
			std::string tmp = spath + filename;
			if(GetExactFileName(tmp, *result)) {
				// we got here, if the file exists
				if(searchpath) *searchpath = spath;
				return false; // stop checking next searchpaths
			}
	
			// go to the next searchpath
			return true;
		}
	};

std::string GetFullFileName(const std::string& path, std::string* searchpath) {
	static std::string fname;
	static std::string tmp;

	if(searchpath) *searchpath = "";

	if(path == "")
		return GetFirstSearchPath();

	fname = "";
	// this also do lastly a check for an absolute filename
	ForEachSearchpath(CheckSearchpathForFile(path, &fname, searchpath));
	return fname;
}

std::string GetWriteFullFileName(const std::string& path, bool create_nes_dirs) {
	static std::string tmp;
	static std::string fname;

	// get the dir, where we should write into
	if(tLXOptions->tSearchPaths.size() == 0 && basesearchpaths.size() == 0) {
		printf("ERROR: we want to write somewhere, but don't know where => we are writing to your temp-dir now...\n");
		tmp = GetTempDir() + "/" + path;
	} else {
		GetExactFileName(GetFirstSearchPath(), tmp);

		CreateRecDir(tmp);
		if(!CanWriteToDir(tmp)) {
			printf("ERROR: we cannot write to %s => we are writing to your temp-dir now...\n", tmp.c_str());
			tmp = GetTempDir();
		}

		tmp += "/";
		tmp += path;
	}

	GetExactFileName(tmp, fname);
	if(create_nes_dirs) CreateRecDir(fname, false);
	return tmp;
}

FILE *OpenGameFile(const std::string& path, const char *mode) {
	if(path.size() == 0)
		return NULL;

	std::string fullfn = GetFullFileName(path);

	bool write_mode = strchr(mode, 'w') != 0;
	bool append_mode = strchr(mode, 'a') != 0;
	if(write_mode || append_mode) {
		std::string writefullname = GetWriteFullFileName(path, true);
		if(append_mode && fullfn.size()>0) { // check, if we should copy the file
			if(CanReadFile(fullfn,true)) { // we can read the file
				// GetWriteFullFileName ensures an exact filename,
				// so no case insensitive check is needed here
				if(fullfn != writefullname) {
					// it is not the file, we would write to, so copy it to the wanted destination
					FileCopy(fullfn, writefullname);
				}
			}
		}
		//printf("opening file for writing (mode %s): %s\n", mode, writefullname);
		return fopen(writefullname.c_str(), mode);
	}

	if(fullfn.size() != 0) {
		//printf("open file for reading (mode %s): %s\n", mode, fullfn);
		return fopen(fullfn.c_str(), mode);
	}

	return NULL;
}


void AddToFileList(searchpathlist* l, const std::string& f) {
	if(!FileListIncludes(l, f)) l->push_back(f);
}

void removeEndingSlashes(std::string& s) {	
	for(
		int i = s.size() - 1;
		i > 0 && (s[i] == '\\' || s[i] == '/');
		i--
	)
		s.erase(i);
}

/////////////////
// Returns true, if the list contains the path
bool FileListIncludes(const searchpathlist* l, const std::string& f) {
	static std::string tmp1;
	static std::string tmp2;
	tmp1 = f;
	removeEndingSlashes(tmp1);
	ReplaceFileVariables(tmp1);
	
	// Go through the list, checking each item
	for(searchpathlist::const_iterator i = l->begin(); i != l->end(); i++) {
		tmp2 = *i;
		removeEndingSlashes(tmp2);
		ReplaceFileVariables(tmp2);
		if(stringcasecmp(tmp1, tmp2) == 0)
			return true;		
	}
	
	return false;
}

std::string GetHomeDir() {
#ifndef WIN32
	char* home = getenv("HOME");
	if(home == NULL || home[0] == '\0') {
		passwd* userinfo = getpwuid(getuid());
		if(userinfo)
			return userinfo->pw_dir;
		return ""; // both failed, very strange system...
	}
	return home;	
#else
	static char tmp[1024];
	if (!SHGetSpecialFolderPath(NULL,tmp,CSIDL_PERSONAL,FALSE))  {
		// TODO: get dynamicaly another possible path
		// the following is only a workaround!
		return "C:\\OpenLieroX";
	}
	return tmp;
#endif
}


std::string GetSystemDataDir() {
#ifndef WIN32
	return SYSTEM_DATA_DIR;
#else
	// windows don't have such dir, don't it?
	// or should we return windows/system32 (which is not exactly intended here)?
	return "";
#endif
}

std::string GetBinaryDir() {
	return binary_dir;
}

std::string GetTempDir() {
#ifndef WIN32
	return "/tmp"; // year, it's so simple :)
#else
	static char buf[256] = "";
	if(buf[0] == '\0') { // only do this once
		GetTempPath(sizeof(buf),buf);
		fix_markend(buf);
	}
	return buf;
#endif
}

void ReplaceFileVariables(std::string& filename) {
	if(filename.compare(0,2,"~/")==0
	|| filename.compare(0,2,"~\\")==0
	|| filename == "~") {
		filename.erase(0,1);
		filename.insert(0,GetHomeDir());
	}
	replace(filename, "${HOME}", GetHomeDir());
	replace(filename, "${SYSTEM_DATA}", GetSystemDataDir());
	replace(filename, "${BIN}", GetBinaryDir());
}

bool FileCopy(const std::string& src, const std::string& dest) {
	static char tmp[2048];

	printf("FileCopy: %s -> %s\n", src.c_str(), dest.c_str());
	FILE* src_f = fopen(src.c_str(), "r");
	if(!src_f) {
		printf("FileCopy: ERROR: cannot open source\n");
		return false;
	}
	FILE* dest_f = fopen(dest.c_str(), "w");
	if(!dest_f) {
		fclose(src_f);
		printf("  ERROR: cannot open destination\n");
		return false;
	}

	printf("  ");
	bool success = true;
	unsigned short count = 0;
	size_t len = 0;
	while((len = fread(tmp, 1, sizeof(tmp), src_f)) > 0) {
		if(count == 0) printf("."); count++; count %= 20;
		if(len != fwrite(tmp, 1, len, dest_f)) {
			printf("  ERROR: problem while writing\n");
			success = false;
			break;
		}
		if(len != sizeof(tmp)) break;
	}
	if(success) {
		success = feof(src_f) != 0;
		if(!success) printf("  ERROR: problem while reading\n");
	}

	fclose(src_f);
	fclose(dest_f);
	if(success)	printf("  success :)\n");
	return success;
}

bool CanWriteToDir(const std::string& dir) {
	// TODO: we have to make this a lot better!
	std::string fname = dir + "/.some_stupid_temp_file";
	FILE* fp = fopen(fname.c_str(), "w");
	if(fp) {
		fclose(fp);
		remove(fname.c_str());
		return true;
	}
	return false;
}
