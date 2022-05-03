#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	// if (argc != 3) {
	// 	usage();
	// }
	
	// copy_dir(argv[1], argv[2]);

	// return 0;
	   char *str1= "This is ", *str2= "programiz.com";

   // concatenates str1 and str2
   // the resultant string is stored in str1.
   strcat(str1, str2);

   puts(str1);
   puts(str2);

   return 0;
}