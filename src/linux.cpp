#include "common.cpp"
#include <dlfcn.h>
#include <fts.h>

int (*orig_closedir)(DIR *) = NULL;
DIR *(*orig_opendir)(const char*) = NULL;
dirent* (*orig_readdir)(DIR*) = NULL;
dirent64* (*orig_readdir64)(DIR *) = NULL;
int (*orig_readdir_r)(DIR*, struct dirent*, struct dirent**) = NULL;
int (*orig_readdir64_r)(DIR*, struct dirent64*, struct dirent64**) = NULL;
int (*orig_open)(const char*, int, ...) = NULL;
FILE *(*orig_fopen)(const char*, const char*) = NULL;

FilteredIo filteredio;

int closedir(DIR *dir)
{
  if (!orig_closedir)
    orig_closedir = (int (*)(DIR*))dlsym(RTLD_NEXT, "closedir");
  filteredio.recordDirClosed(dir);
  return orig_closedir(dir);
}

DIR* opendir(const char *name)
{
  if (!orig_opendir)
    orig_opendir = (DIR* (*)(const char*))dlsym(RTLD_NEXT, "opendir");
  DIR* r = orig_opendir(name);
  filteredio.recordDirOpened(name, r);
  return r;
}

struct dirent* readdir(DIR *dirp)
{
  
  if (!orig_readdir)
    orig_readdir = (dirent* (*)(DIR*))dlsym(RTLD_NEXT, "readdir");

  struct dirent* r = orig_readdir(dirp);

  if (r && filteredio.shouldSkipDirEntry(dirp, r->d_type, r->d_name)) {
    r = readdir(dirp);
  }

  return r;
}

struct dirent64* readdir64(DIR *dirp)
{
  
  if (!orig_readdir64)
    orig_readdir64 = (dirent64* (*)(DIR*))dlsym(RTLD_NEXT, "readdir64");

  struct dirent64* r = orig_readdir64(dirp);

  if (r && filteredio.shouldSkipDirEntry(dirp, r->d_type, r->d_name)) {
    r = readdir64(dirp);
  }

  return r;
}

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
  if (!orig_readdir_r)
    orig_readdir_r = (int (*)(DIR*, dirent*, dirent**))dlsym(RTLD_NEXT, "readdir_r");

  int r = orig_readdir_r(dirp, entry, result);


  if (*result && filteredio.shouldSkipDirEntry(dirp, (*result)->d_type, (*result)->d_name)) {
    r = readdir_r(dirp, entry, result);
  }

  return r;
}

int readdir64_r(DIR *dirp, struct dirent64 *entry, struct dirent64 **result)
{
  if (!orig_readdir64_r)
    orig_readdir64_r = (int (*)(DIR*, dirent64*, dirent64**))dlsym(RTLD_NEXT, "readdir64_r");

  int r = orig_readdir64_r(dirp, entry, result);
  if (*result && filteredio.shouldSkipDirEntry(dirp, (*result)->d_type, (*result)->d_name)) {
    r = readdir64_r(dirp, entry, result);
  }

  return r;
}

int open(const char *path, int oflag, ...)
{
  if (!orig_open)
    orig_open = (int (*)(const char*, int, ...))dlsym(RTLD_NEXT, "open");

  if (filteredio.isAllowedPath(path, oflag & O_CREAT)) {
    va_list args;
    va_start(args, oflag);
    int r = orig_open(path, oflag, args);
    va_end(args);
    return r;
  } else {
    errno = ENOENT;
    return -1;
  }
}

FILE * fopen(const char * path, const char * mode)
{
  if (!orig_fopen)
    orig_fopen = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen");

  if (filteredio.isAllowedPath(path, mode[0] != 'r')) {
    return orig_fopen(path, mode);
  } else {
    errno = ENOENT;
    return NULL;
  }
}
