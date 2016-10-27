/*
 *  Generic binary file packer
 *  Made for Ys II Special
 *  
 *  2013 James Alan Nguyen (dj.tuBIG/MaliceX)
 *  Do whatever the hell you want with it. :)
 *
 *  Compile with gcc -Wall -ansi -pedantic
 *
 *  http://www.codingchords.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>

#pragma pack(1)

#define FILENAME_LEN 256
#define OFFSET_SIZE 4
#define OFFSET_ENTRY_SIZE 8
#define OUT_CACHE "..out.cache"
#define OUT_HDR "..out.hdr"
/*#define BUFFER_SIZE 65536*/

#if _WIN32
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif

/* 
 * Function name: printUsage
 * Args: fname - Filename of the executable
 * Description: Self-explanatory.
 */
void printUsage(char *fname)
{
   printf("\nUsage:\n"
      "   %s <input directory> <output file>\n",
      fname
   );
}

/* 
 * Function name: runProgram
 * Args: in_fname - Input filename from command line argument 1
 *       out_dir - Output directory path from command line argument 2
 *
 * Description: Main application function
 *
 * Note: Default output file is out.bin
 */
void runProgram(char *in_dir, char *out_fname)
{
   FILE *in_file, *out_cachefile, *out_header, *out_file;
   DIR *dir_ptr;
   struct dirent *dp;
   long in_fsize, out_fsize = 0;
   char in_fname[FILENAME_LEN],
        *file_buffer, offset_buffer[OFFSET_ENTRY_SIZE];
   
   assert((out_header = fopen(OUT_HDR, "wb")) != NULL);
   assert((out_cachefile = fopen(OUT_CACHE, "wb")) != NULL);
   assert((out_file = fopen(out_fname, "wb")) != NULL);
   assert((dir_ptr = opendir(in_dir)) != NULL);
   
   while((dp = readdir(dir_ptr)) != NULL)
   {
      /*Ignore hidden files/directories*/
      if(dp->d_name[0] == '.')
         continue;
      
      sprintf(in_fname, "%s/%s", in_dir, dp->d_name);
      
      if((in_file = fopen(in_fname, "rb")) == NULL)
         continue;
      
      fseek(in_file, 0, SEEK_END);
      in_fsize = ftell(in_file);
      out_fsize += in_size;
      rewind(in_file);
      
      printf("Packing %s (%l bytes)\n", dp->d_name, in_fsize);
      assert((file_buffer = malloc(in_fsize)) != NULL);
      
      /*Write header*/
      memcpy(offset_buffer, );
      
      fread(file_buffer, in_fsize, 1, in_file);
      fwrite(file_buffer, in_fsize, 1, out_cachefile);
      fclose(in_file);
   }
   
   
   
   closedir(dir_ptr);
   fclose(out_file);
}

int main(int argc, char *argv[])
{
   switch(argc)
   {
      case 3:
         runProgram(argv[1], argv[2]);
         break;
      
      case 2:
         runProgram(argv[1], "out.bin");
         break;
      
      case 1:
      default:
         printUsage(argv[0]);
         break;
   }
   
   return EXIT_SUCCESS;
}