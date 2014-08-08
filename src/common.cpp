#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_set>
#include <string.h> 
#include <stdarg.h>

#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define PURPLE "\x1b[35m"
#define BLUE "\x1b[36m"
#define RED "\x1b[31m"
#define RESET "\x1b[0m"

#define DBG(f) f
//#define DBG(f)

using namespace std;

class FilteredIo {

  std::map<DIR*, string> openedPaths;
  unordered_set<string> allowedPaths;
  string prefix;
  bool isSetup;

  public:
    void recordDirOpened(const char* name, DIR* dirp);
    void recordDirClosed(DIR* dirp);
    bool isAllowedPath(string path, bool isCreate);
    bool shouldSkipDirEntry(DIR* dirp, int entryType, const char* name);

  private:
    bool isManagedPath(const char* str);
    void setup();
    void ensureSetup();
};

void FilteredIo::setup()
{
  isSetup = true;

  char* whitelist = getenv("FILTERIO_WHITELIST");

  if (!whitelist) {
    DBG(cerr << RED << "No FILTERIO_WHITELIST specified\n";)
    return;
  }

  ifstream fp(whitelist);

  if (fp.is_open()) {
    string rawPrefix;
    if (!getline(fp,rawPrefix)) {
      DBG(cerr << "Could not read prefix to filter from first line of whitelist\n");
      fp.close();
      return;
    }

    char buf[PATH_MAX + 1];

    realpath(rawPrefix.c_str(), buf);

    prefix = buf;
    string line;
    while (getline(fp,line))
    {
      realpath(line.c_str(), buf);
      allowedPaths.insert(buf); // TODO(instead of realpathing all input, avoid realpath everywhere and just use cwd)
    }
    fp.close();
  } else {
    DBG(cerr << RED << "Unable to open file: " << whitelist << "\n" << RESET;)
  }
}

void FilteredIo::ensureSetup()
{
  if (!isSetup) setup();
}

bool FilteredIo::isManagedPath(const char* str)
{
  ensureSetup();
  return strncmp(str, prefix.c_str(), prefix.length()) == 0;
}

bool FilteredIo::isAllowedPath(string raw, bool isCreate)
{
  ensureSetup();

  if (allowedPaths.size() <= 0) {
    return true;
  }

  char path[PATH_MAX + 1];
  if (!realpath(raw.c_str(), path)) {
    return true;
  }

  if (!isManagedPath(path)) {
    return true;
  }

  if (isCreate) {
    allowedPaths.insert(path);
    return true;
  }

  return allowedPaths.find(path) != allowedPaths.end();
}

bool FilteredIo::shouldSkipDirEntry(DIR* dirp, int entryType, const char* name)
{
  if (entryType == DT_DIR) return false;
  if (openedPaths.count(dirp) <= 0) return false;

  string path = openedPaths[dirp] + '/' + string(name);
  bool skip = !isAllowedPath(path, false);

  return skip;
}

void FilteredIo::recordDirOpened(const char* name, DIR* dirp)
{
  if (!dirp) return;

  char buf[PATH_MAX + 1];
  if(realpath(name, buf) && isManagedPath(buf)) {
    openedPaths[dirp] = buf;
  }
}

void FilteredIo::recordDirClosed(DIR* dirp)
{
  if (!dirp) return;
  openedPaths.erase(dirp);
}
