#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

/* make sure to use syserror() when a system call fails. see common.h */

void
usage()
{
	fprintf(stderr, "Usage: cpr srcdir dstdir\n");
	exit(1);
}

char* concat2(const char *s1, const char *s2)
{
    int strlen1 = strlen(s1);
    int strlen2 = strlen(s2);
    char *result = malloc(strlen1 + strlen2 + 2); 
    memcpy(result, s1, strlen1);
	*(result+strlen1)='/';
    memcpy(result + strlen1+1, s2, strlen2 + 1);
    return result;
}

void copy_file(char *file1, char *file2){
	int fd_read;
	fd_read = open(file1, O_RDONLY);
	if (fd_read < 0) {
    	syserror(open, file1);
    }

	struct stat stats;
    stat(file1, &stats);

	int fd_write;
	fd_write = open(file2, O_CREAT|O_WRONLY|O_TRUNC, 0777);
	if (fd_write<0){
		syserror(open, file1);
	}

	int bufferSize = 50;
	int charCount;
	char buffer[bufferSize];
	while ((charCount = read(fd_read, buffer, bufferSize)) != 0)
    {
        write(fd_write, buffer, charCount);
    }
	chmod(file2, stats.st_mode);
	close(fd_read);
	close(fd_write);
}

void copy_dir(char *dir1, char *dir2){
	DIR *dr1 = opendir(dir1);
	if (dr1 == NULL){
		syserror(open, dir1);
	}

	struct stat dr1_stats;
	stat(dir1, &dr1_stats);
	mkdir(dir2, 0777);

	struct dirent *dirent;
	while((dirent = readdir(dr1)) != NULL){
		if (strcmp(dirent->d_name, "..") != 0 && strcmp(dirent->d_name, ".") != 0){
			struct stat entity_stats;
			char* dir1FullPath = concat2(dir1, dirent->d_name);
			char* dir2FullPath = concat2(dir2, dirent->d_name);
			stat(dir1FullPath, &entity_stats);
			if (S_ISREG(entity_stats.st_mode)){
				copy_file(dir1FullPath, dir2FullPath);
			}
			else if(S_ISDIR(entity_stats.st_mode)){
				copy_dir(dir1FullPath, dir2FullPath);
			}
		}
	}
	chmod(dir2, dr1_stats.st_mode);
	closedir(dr1);
}

int
main(int argc, char *argv[])
{
	if (argc != 3) {
		usage();
	}
	struct stat check_exists;
	int exist = stat(argv[2], &check_exists);
	if (exist==0){
		printf("mkdir: %s: File exists", argv[2]);
		return 0;
	}
	copy_dir(argv[1], argv[2]);

	return 0;
}