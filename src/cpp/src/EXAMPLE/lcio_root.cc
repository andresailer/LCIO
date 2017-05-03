#include <lcio_root.h>

#include <cctype>
#include <cstring>
#include <cerrno>
#ifdef _WIN32
#include <io.h>
static const int S_IRWXU = (S_IREAD|S_IWRITE);
#else
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#define O_BINARY 0
#define EXPORT
#endif

#define LCIO_ROOT_DEBUG 0

#include <TUrl.h>
#include <TFile.h>
#include <TSystem.h>

#include <iostream>

using namespace std;


#if LCIO_ROOT_DEBUG>0
static void info(TFile* f, const char* fmt, ...)   {
  size_t len = 0;
  char text[1024];
  text[0] = 0;
  if ( f ) {
    len = ::snprintf(text,sizeof(text),"%s: Size:%d Pos:%d ",f->GetName(),f->GetSize(),f->GetRelOffset());
  }
  if ( fmt ) {
    va_list args;
    va_start(args,fmt);
    len += ::vsnprintf(text+len,sizeof(text)-len,fmt,args);
    va_end(args);
    text[len]='\n';
    text[len+1]=0;
  }
  ::printf(text);
}
#else
static void info(TFile*, const char* , ...)   {
}
#endif

extern "C" FILE* root_fopen(const char *filepath, const char* mode) {
  TFile* f = 0;
  TUrl url(filepath);
  TString opts="filetype=raw", proto, spec, tmp=url.GetOptions();

  if ( tmp.Length()>0 ) {
    opts += "&";
    opts += url.GetOptions();
  }
  url.SetOptions(opts);
  proto = url.GetProtocol();
  if ( proto == "file" || proto == "http" ) {
    spec = filepath;
    spec += "?filetype=raw";
  }
  else {
    spec = url.GetUrl();
  } 
  int flags = 0;
  if ( strchr(mode,'r') || strchr(mode,'R') ) flags |= O_RDONLY;
  if ( strchr(mode,'w') || strchr(mode,'W') ) flags |= O_WRONLY;
  if ( strchr(mode,'a') || strchr(mode,'A') ) flags  = O_APPEND|O_WRONLY;
  if ( strchr(mode,'b') || strchr(mode,'B') ) flags |= O_BINARY;
  if ( strchr(mode,'+') ) flags |= O_APPEND|O_WRONLY;
  else if ( (flags&O_WRONLY) != 0 && (flags&O_APPEND)==0 ) flags |= O_CREAT;

  if ( (flags&O_CREAT) == O_CREAT ) {
    f = TFile::Open(spec,"RECREATE","LCIO Root file",0);
  }
  else if ( (flags&O_WRONLY) == O_WRONLY || (flags&O_APPEND) == O_APPEND ) {
    f = TFile::Open(spec,"UPDATE","LCIO Root file",0);
  }
  else if ( (flags&O_RDONLY)==O_RDONLY ) {
    f = TFile::Open(spec);
  }
  if ( f && !f->IsZombie() ) {
    if ( (flags&O_APPEND) == O_APPEND )
      f->Seek(0,TFile::kEnd);
    else
      f->Seek(0,TFile::kBeg);
    if ( LCIO_ROOT_DEBUG>0 ) info(f,"OPEN");
    if ( LCIO_ROOT_DEBUG>2 ) info(f,"        :OPTS:%s PROTOCOL:%s SPECS:%s TOpts:%s",
				  url.GetOptions(),url.GetProtocol(),(const char*)spec,f->GetOption());
    return (FILE*)((void*)f);
  }
  if ( LCIO_ROOT_DEBUG>0 ) info(0,"FAILED OPEN URL:%s OPTS:%s PROTOCOL:%s SPECS:%s",
	 url.GetUrl(),url.GetOptions(),url.GetProtocol(),(const char*)spec);
  return 0;
}

extern "C" int root_fclose(FILE* fd) {
  if ( fd ) {
    TFile* f = (TFile*)fd;
    if ( f ) {
      if ( LCIO_ROOT_DEBUG>0 ) info(f,"CLOSE Read calls:%d Bytes:%d Write Wytes:%d",
	   f->GetReadCalls(),f->GetFileBytesRead(),
	   f->GetFileBytesWritten());
      if ( !f->IsZombie() ) f->Close();
      delete f;
    }
    return 0;
  }
  errno = EBADF;
  return -1;
}

extern "C" int root_access(const char *nam, int mode)   {
  return kFALSE==gSystem->AccessPathName(nam, (mode&S_IWRITE) != 0 ? kWritePermission : kReadPermission) ? 0 : -1;
}

extern "C" long long int root_fseek64(FILE *stream, long long int offset, int how)  {
  if ( stream ) {
    Long64_t off;
    TFile* f = (TFile*)stream;
    switch(how) {
    case SEEK_SET:
      if ( LCIO_ROOT_DEBUG>2 ) info(f,"SEEK_SET(%d)",offset);
#if 0
      if ( offset < 0 )  {
	offset = 0;
      }
      else if ( offset > f->GetSize() ) {
	errno = EFBIG;
	return -1;
      }
#endif
      f->Seek(offset,TFile::kBeg);
      return 0;
    case SEEK_CUR:
      if ( LCIO_ROOT_DEBUG>2 ) info(f,"SEEK_CUR(%d)",offset);
      off = offset+f->GetRelOffset();
      if ( off < 0 ) {
	offset = -1 * f->GetRelOffset();
      }
      else if ( off > f->GetSize() ) {
	errno = EFBIG;
	return -1;
      }
      f->Seek(offset,TFile::kCur);
      return 0;
    case SEEK_END:
      if ( LCIO_ROOT_DEBUG>2 ) info(f,"SEEK_END(%d)",offset);
      off = offset+f->GetSize();
      if ( off < 0 )  {
	offset = -1 * f->GetSize();
      }
      else if ( off > f->GetSize() ) {
	errno = EFBIG;
	return -1;
      }
      f->Seek(offset,TFile::kEnd);
      return 0;
    default:
      if ( LCIO_ROOT_DEBUG>0 ) info(f,"SEEK_UNKNOWN(%d,%d)",offset,how);
      errno = EINVAL;
      return -1;
    }
  }
  errno = EBADF;
  return -1;
}

extern "C" long root_fseek(FILE *stream, long offset, int how)  {
  return root_fseek64(stream,offset,how);
}

extern "C" long long int root_ftell64(FILE* stream) {
  if ( stream ) {
    TFile* f = (TFile*)stream;
    return f->GetRelOffset();
  }
  errno = EBADF;
  return -1;
}

extern "C" long root_ftell(FILE* stream) {
  return root_ftell64(stream);
}

extern "C" size_t root_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)   {
  if ( stream ) {
    size_t len = size*nmemb;
    TFile* f = (TFile*)stream;
    size_t offset = f->GetRelOffset();
    if ( f->GetBytesRead()+len > size_t(f->GetSize()) ) {
      if ( LCIO_ROOT_DEBUG>0 ) info(f,"FAILED: READ(%d)",len);
      return 0;
    }
    if ( LCIO_ROOT_DEBUG>2 ) info(f,"READ(%d)",len);
    if ( 0 == f->ReadBuffer((char*)ptr,len) )
      return nmemb;
    f->Seek(offset,TFile::kBeg);
    if ( 0 == f->ReadBuffer((char*)ptr,len) ) {
      std::cout << f->GetName() << " Recovered from READ FAILURE len=" << len << " fsize=" 
		<< f->GetSize() << " offset=" << offset << std::endl;
      return nmemb;
    }
    return 0;
  }
  errno = EBADF;
  return 0;
}

extern "C" size_t root_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)  {
  if ( stream ) {
    size_t len = size*nmemb;
    TFile* f = (TFile*)stream;
    if ( LCIO_ROOT_DEBUG>2 ) info(f,"WRITE(%d)",len);
    if ( 0 == f->WriteBuffer((const char*)ptr,size*nmemb) ) 
      return nmemb;
    return 0;
  }
  errno = EBADF;
  return 0;
}

extern "C" int root_fflush(FILE *stream)  {
  if ( stream ) {
    TFile* f = (TFile*)stream;
    f->Flush();
    return 0;
  }
  errno = EBADF;
  return -1;
}

extern "C" int root_stat64(const char* path, struct stat* buf) {
  if ( buf && path ) {
    FileStat_t st;
    int res = gSystem->GetPathInfo(path,st);
    if ( 0 == res ) {
      ::memset(buf,0,sizeof(struct stat));
#if 0
      dev_t     st_dev;     /* ID of device containing file */
      ino_t     st_ino;     /* inode number */
      mode_t    st_mode;    /* protection */
      nlink_t   st_nlink;   /* number of hard links */
      uid_t     st_uid;     /* user ID of owner */
      gid_t     st_gid;     /* group ID of owner */
      dev_t     st_rdev;    /* device ID (if special file) */
      off_t     st_size;    /* total size, in bytes */
      blksize_t st_blksize; /* blocksize for filesystem I/O */
      blkcnt_t  st_blocks;  /* number of blocks allocated */
      time_t    st_atime;   /* time of last access */
      time_t    st_mtime;   /* time of last modification */
      time_t    st_ctime;   /* time of last status change */
#endif
      buf->st_dev   = st.fDev;
      buf->st_ino   = st.fIno;
      buf->st_mode  = st.fMode;
      buf->st_uid   = st.fUid;
      buf->st_gid   = st.fGid;
      buf->st_size  = st.fSize;
      buf->st_mtime = st.fMtime;
      return 0;
    }
  }
  return -1;
}

extern "C" int root_stat(const char* path, struct stat* buf) {
  return root_stat64(path,buf);
}

/*
 *    Anonymous namespace
 */
namespace   {

  /** @class PosixIO 
    *
    *
    * @author:  M.Frank
    * @version: 1.0
    */
  class PosixIO  {
  public:
    PosixIO()  {  ::memset(this,0,sizeof(PosixIO));   }
    int           (*stat)     (const char*, struct stat *statbuf);
    int           (*stat64)   (const char*, struct stat64 *statbuf);
    FILE*         (*fopen)    (const char *, const char *);
    int           (*fclose)   (FILE*);
    size_t        (*fwrite)   (const void*, size_t, size_t, FILE*);
    size_t        (*fread)    (void*, size_t, size_t, FILE*);
    long          (*ftell)    (FILE*);
    long long int (*ftell64)  (FILE*);
    long          (*fseek)    (FILE *, long int, int);
    long long int (*fseek64)  (FILE*, long long int, int);
    int           (*fflush)   (FILE*);
  };
}

#if 0
PosixIO* POSIX_ROOT()  {
  typedef PosixIO _IO;
  static _IO p;
  if ( 0 == p.fopen )  {
    memset(&p,0,sizeof(p));
    p.fopen     = root_fopen;
    p.fclose    = root_fclose;
    p.fwrite    = root_fwrite;
    p.fread     = root_fread;
    p.fseek     = root_fseek;
    p.ftell     = root_ftell;
    p.fseek64   = root_fseek64;
    p.ftell64   = root_ftell64;
    p.fflush    = root_fflush;
    p.stat      = 0;
    p.stat64    = 0;
  }
  return &p;
}

static PosixIO* io = POSIX_ROOT();
#endif

