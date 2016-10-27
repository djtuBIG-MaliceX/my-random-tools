/*
 *  Location 57 (intro) replace
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

#pragma pack(1)

#define FILENAME_LEN 256
#define OFFSET_SIZE 4
#define OFFSET_ENTRY_SIZE 8
#define TEMPLATE_SIZE 0x1D1

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
   printf(
      "\nLittle hax for playing different music during Ys II Special intro"
      "\nUsage:\n"
      "   %s <input file> <output file>\n",
      fname
   );
}

long getFileSize(FILE *fp)
{
   long file_size;
   
   if(fp==NULL)
      return -1;
   
   rewind(fp);
   fseek(fp, 0, SEEK_END);
   file_size = ftell(fp);
   rewind(fp);
   
   return file_size;
}

void writePadding(FILE *out_file)
{
   int i;
   
   for(i = 58; i < 66; ++i)
   {
      /*fputc(0xD0, out_file);
      fputc(0x01, out_file);*/
      fputc(0x10, out_file);
      fputc(0x02, out_file);
      fputc(0x00, out_file);
      fputc(0x00, out_file);
      fputc(0x01, out_file);
      fputc(0x00, out_file);
      fputc(0x00, out_file);
      fputc(0x00, out_file);
   }
   
   fputc(0x30, out_file);
}

void writeHeader(FILE *out_file)
{
   int i;
   
   for(i = 0; i < 57; ++i)
   {
      /*fputc(0xD0, out_file);
      fputc(0x01, out_file);*/
      fputc(0x10, out_file);
      fputc(0x02, out_file);
      fputc(0x00, out_file);
      fputc(0x00, out_file);
      fputc(0x01, out_file);
      fputc(0x00, out_file);
      fputc(0x00, out_file);
      fputc(0x00, out_file);
   }
   
   /*for(i = 0; i < 0x1C0; ++i)
      fputc(0xFF, out_file);*/
      
   /*fputc(0xD1, out_file);
   fputc(0x01, out_file);*/
   fputc(0x11, out_file);
   fputc(0x02, out_file);
   fputc(0x00, out_file);
   fputc(0x00, out_file);
}

/* 
 * Function name: runProgram
 * Args: in_fname - Input filename from command line argument 1
 *       out_fname - Output filename path from command line argument 2
 *
 * Description: Main application function
 *
 */
void runProgram(char *in_fname, char *out_fname)
{
   FILE *in_file, *out_file;
   int in_fsize;
   char *file_buffer;
   
   assert((in_file = fopen(in_fname, "rb")) != NULL);
   assert((out_file = fopen(out_fname, "wb")) != NULL);
   
   in_fsize = (int)getFileSize(in_file);
   
   assert((file_buffer = malloc(in_fsize)) != NULL);
   
   writeHeader(out_file);
   fwrite(&in_fsize, sizeof(int), 1, out_file);
   writePadding(out_file);
   
   fread(file_buffer, in_fsize, 1, in_file);
   fwrite(file_buffer, in_fsize, 1, out_file);
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