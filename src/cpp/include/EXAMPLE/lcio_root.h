#ifndef LCIO_ROOT
#define LCIO_ROOT

#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

extern "C"  {
  FILE*         root_fopen(const char *filepath, const char* mode);
  int           root_fclose(FILE* fd);
  int           root_access(const char *nam, int mode);
  long long int root_fseek64(FILE *stream, long long int offset, int how);
  long          root_fseek(FILE *stream, long offset, int whence);
  long long int root_ftell64(FILE* stream);
  long          root_ftell(FILE* stream);
  size_t        root_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
  size_t        root_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
  int           root_fflush(FILE *stream);
  int           root_stat(const char* path, struct stat* buf);
}

#define dc_fflush root_fflush
#define dc_fclose root_fclose
#define dc_fopen  root_fopen
#define dc_fread  root_fread
#define dc_fwrite root_fwrite
#define dc_ftell  root_ftell
#define dc_fseek  root_fseek
#define dc_stat   root_stat

#endif // LCIO_ROOT

