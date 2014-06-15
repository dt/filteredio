#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_set>

#define DYLD_INTERPOSE(_replacment,_replacee) \
  __attribute__((used)) static struct{ const void* replacment; const void* replacee; } _interpose_##_replacee \
  __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacment, (const void*)(unsigned long)&_replacee };

#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define PURPLE "\x1b[35m"
#define BLUE "\x1b[36m"
#define RED "\x1b[31m"
#define RESET "\x1b[0m"

#define DBG(f) f
//#define DBG(f)

using namespace std;

std::map<DIR*, string> fs_openedPaths;
unordered_set<string> fs_allowedPaths;
string fs_prefix;
bool fs_is_setup = false;

void fs_do_setup()
{
  fs_is_setup = true;

  char* whitelist = getenv("FILTERIO_WHITELIST");

  if (!whitelist) {
    DBG(cerr << RED << "No FILTERIO_WHITELIST specified\n";)
    return;
  }

  DBG(cerr<< BLUE << "FILTERIO_WHITELIST: " << whitelist << RESET << "\n";)

  ifstream fp(whitelist);

  if (fp.is_open()) {
    if (!getline(fp,fs_prefix)) {
      DBG(cerr << "Could not read prefix to filter from first line of whitelist\n");
      fp.close();
      return;
    }

    DBG(cerr << BLUE << "Filtered prefix: " << fs_prefix << RESET << "\n");

    string line;
    while (getline(fp,line))
    {
      fs_allowedPaths.insert(line);
      DBG(cerr << BLUE << "\t* " << line << RESET << '\n');
    }
    DBG(cerr << BLUE << "Read " << fs_allowedPaths.size() << " items\n"  << RESET);
    fp.close();
  } else {
    DBG(cerr << RED << "Unable to open file: " << whitelist << "\n" << RESET;)
  }
}

void fs_ensure_setup()
{
  if (!fs_is_setup)
    fs_do_setup();
}

bool fs_isPrefix(const char* str) {
  fs_ensure_setup();
  return strncmp(str, fs_prefix.c_str(), fs_prefix.length()) == 0;
}

bool fs_isAllowedPath(string path)
{
  fs_ensure_setup();

  if (fs_allowedPaths.size() <= 0) {
    DBG(cerr << "allowedPaths empty -- everything is allowed\n");
    return true;
  }

  char buf[PATH_MAX + 1];
  if (!realpath(path.c_str(), buf)) {
    DBG(cerr << "realpath failed, allowing path\n");
    return true;
  }

  if (!fs_isPrefix(buf)) {
    DBG(cerr << "lives outside filtered prefix so always allowed\n");
    return true;
  }

  bool found = fs_allowedPaths.find(path) != fs_allowedPaths.end();

  return found;
 }

bool fs_shouldSkipDirEntry(DIR* dirp, struct dirent *entry, const char* what)
{
  if (!entry)
    return false;

  if (entry->d_type == DT_DIR)
    return false;

  if (fs_openedPaths.count(dirp) <= 0)
    return false;

  string path = fs_openedPaths[dirp] + '/' + string(entry->d_name);

  bool allowed = fs_isAllowedPath(path);

  if (!allowed) {
    (void)what;
    DBG(cerr << YELLOW << what << " skipping " << path << " due to whitelist\n" << RESET ;)
  }

  return !allowed;
}

void fs_recordDirOpened(const char* name, DIR* dirp)
{
  if (!dirp)
    return;

  char buf[PATH_MAX + 1];
  if(realpath(name, buf) && fs_isPrefix(buf)) {
    fs_openedPaths[dirp] = buf;
  }
}

int fs_closedir(DIR *dir)
{
  fs_openedPaths.erase(dir);
  return closedir(dir);
}
DYLD_INTERPOSE(fs_closedir, closedir)

DIR* fs_opendir(const char *name)
{
  DIR* r = opendir(name);
  fs_recordDirOpened(name, r);
  return r;
}
DYLD_INTERPOSE(fs_opendir, opendir)

DIR* fs_opendir2(const char *name, int options)
{
  DIR* r = __opendir2(name, options);
  fs_recordDirOpened(name, r);
  return r;
}
DYLD_INTERPOSE(fs_opendir2, __opendir2)

struct dirent* fs_readdir(DIR *dirp)
{
  struct dirent* r = readdir(dirp);

  if (fs_shouldSkipDirEntry(dirp, r, "readdir")) {
    r = fs_readdir(dirp);
  }

  return r;
}
DYLD_INTERPOSE(fs_readdir, readdir)

int fs_readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
  int r = readdir_r(dirp, entry, result);

  if (fs_shouldSkipDirEntry(dirp, *result, "readdir_r")) {
    r = fs_readdir_r(dirp, entry, result);
  }

  return r;
}
DYLD_INTERPOSE(fs_readdir_r, readdir_r)

bool fs_allowOpen(const char* path, bool isCreate)
{
  char buf[PATH_MAX + 1];
  bool isPrefix = (realpath(path, buf) && fs_isPrefix(buf));

  if (!isPrefix)
    return true;

  if (fs_allowedPaths.size() <= 0) {
    DBG(cerr << "allowedPaths empty -- everything is allowed\n");
    return true;
  }

  if (isCreate) {
    fs_allowedPaths.insert(buf);
    return true;
  }

  return fs_allowedPaths.find(buf) != fs_allowedPaths.end();
}

int fs_open(const char *path, int oflag, ...)
{
  if (fs_allowOpen(path, oflag & O_CREAT)) {
    va_list args;
    va_start(args, oflag);
    int r = open(path, oflag, args);
    va_end(args);
    return r;
  } else {
    DBG(cerr << RED << "open denying read of " << path << "\n";)
    errno = ENOENT;
    return -1;
  }
}
DYLD_INTERPOSE(fs_open, open)

FILE * fs_fopen(const char * path, const char * mode)
{
  if (fs_allowOpen(path, mode[0] != 'r')) {
    return fopen(path, mode);
  } else {
    DBG(cerr << RED << "fopen denying read of " << path << "\n";)
    errno = ENOENT;
    return NULL;
  }
}
DYLD_INTERPOSE(fs_fopen, fopen)

