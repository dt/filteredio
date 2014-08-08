#include "common.cpp"

#define DYLD_INTERPOSE(_replacment,_replacee) \
  __attribute__((used)) static struct{ const void* replacment; const void* replacee; } _interpose_##_replacee \
  __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacment, (const void*)(unsigned long)&_replacee };

FilteredIo filteredio;

int filteredio_closedir(DIR *dir)
{
  filteredio.recordDirClosed(dir);
  return closedir(dir);
}
DYLD_INTERPOSE(filteredio_closedir, closedir)

DIR* filteredio_opendir(const char *name)
{
  DIR* r = opendir(name);
  filteredio.recordDirOpened(name, r);
  return r;
}
DYLD_INTERPOSE(filteredio_opendir, opendir)

DIR* filteredio_opendir2(const char *name, int options)
{
  DIR* r = __opendir2(name, options);
  filteredio.recordDirOpened(name, r);
  return r;
}
DYLD_INTERPOSE(filteredio_opendir2, __opendir2)

struct dirent* filteredio_readdir(DIR *dirp)
{
  struct dirent* r = readdir(dirp);

  if (r && filteredio.shouldSkipDirEntry(dirp, r->d_type, r->d_name)) {
    r = filteredio_readdir(dirp);
  }

  return r;
}
DYLD_INTERPOSE(filteredio_readdir, readdir)

int filteredio_readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
  int r = readdir_r(dirp, entry, result);

  if (*result && filteredio.shouldSkipDirEntry(dirp, (*result)->d_type, (*result)->d_name)) {
    r = filteredio_readdir_r(dirp, entry, result);
  }

  return r;
}
DYLD_INTERPOSE(filteredio_readdir_r, readdir_r)

int filteredio_open(const char *path, int oflag, ...)
{
  if (filteredio.isAllowedPath(path, oflag & O_CREAT)) {
    va_list args;
    va_start(args, oflag);
    int r = open(path, oflag, args);
    va_end(args);
    return r;
  } else {
    errno = ENOENT;
    return -1;
  }
}
DYLD_INTERPOSE(filteredio_open, open)

FILE * filteredio_fopen(const char * path, const char * mode)
{
  if (filteredio.isAllowedPath(path, mode[0] != 'r')) {
    return fopen(path, mode);
  } else {
    errno = ENOENT;
    return NULL;
  }
}
DYLD_INTERPOSE(filteredio_fopen, fopen)

