/*
 *  Generic binary file unpacker
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

#define FILENAME_LEN 256
#define OFFSET_SIZE 4
#define OFFSET_ENTRY_SIZE 8
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
      "   %s <input file> <output directory>\n",
      fname
   );
}

/* 
 * Function name: unpackBinaryFile
 * Args: in_file - input file pointer
 *       out_dirname - output directory (must exist or program will fail)
 *       cur_entry - counter ID for the data being extracted
 *
 * Description: Routine for extracting the next data segment.
 *
 * Note: Assumes an offset header structure as follows:
 *       <4 bytes start offset> <4 bytes data size> (repeat until end location)
 */
void unpackBinaryFile(FILE *in_file, char *out_dirname, int cur_entry)
{
   FILE *out_file;
   char out_fname[FILENAME_LEN], *file_buffer;
   int entry_offset, entry_size/*, written = 0*/;
   
   /*Append counter as filename suffix (probably on extension)*/
   sprintf(out_fname, "%s[%04d]", out_dirname, cur_entry);
   
   assert((out_file = fopen(out_fname, "wb")) != NULL);
   
   /*Read header*/
   fseek(in_file, (OFFSET_ENTRY_SIZE*cur_entry), SEEK_SET);
   fread(&entry_offset, OFFSET_SIZE, 1, in_file);
   fread(&entry_size, OFFSET_SIZE, 1, in_file);
   
   /*printf("Extracting: %s (%d bytes)\n", out_fname, entry_size);*/
   printf("[%d] - O:0x%04x, S:0x%04x\n", cur_entry, entry_offset, entry_size);
   assert((file_buffer = malloc(entry_size)) != NULL);
   
   /*Extract contents*/
   fseek(in_file, entry_offset, SEEK_SET);
   fread(file_buffer, entry_size, 1, in_file);
   fwrite(file_buffer, entry_size, 1, out_file);
   
   /* TODO buffered method
   assert((file_buffer = calloc(1, BUFFER_SIZE)) != NULL);
   while(written < entry_size)
   {
      fread(file_buffer, (entry_size > BUFFER_SIZE)
      ? BUFFER_SIZE : entry_size, 1, in_file);
   }
   
   free(file_buffer);
   */
   
   fclose(out_file);
}

/* 
 * Function name: getNumEntries
 * Args: in_file - input file pointer
 *
 * Description: Count the number of known entries based on the offset header
 *              information.
 *
 * Note: Routine assumes first offset information defines the location that
 *       separates the offsets from the binary data.
 */
int getNumEntries(FILE *in_file)
{
   char bin_cache[OFFSET_SIZE];
   int *offset_cache, num_entries;
   
   /*Get end of offset boundary, defined by first offset*/
   fseek(in_file, 0, SEEK_SET);
   fread(bin_cache, 1, OFFSET_SIZE, in_file);
   offset_cache = (int*)&bin_cache;
   num_entries = (*offset_cache) / OFFSET_ENTRY_SIZE;
   
   printf("Offset list ends at: 0x%x\n", (*offset_cache));
   printf("Number of entries: %d\n", num_entries-1);
   
   return num_entries;
}

/* 
 * Function name: runProgram
 * Args: in_fname - Input filename from command line argument 1
 *       out_dir - Output directory path from command line argument 2
 *
 * Description: Main application function
 *
 * Note: Default output directory is ./outs (must exist)
 */
void runProgram(char *in_fname, char *out_dir)
{
   FILE *in_file;
   char out_dirname[FILENAME_LEN];
   int cur_entry, end_entry;
   
   /*Create output filename prefix template*/
   (out_dir[strlen(out_dir)-1] == DIR_SEP)
         ? sprintf(out_dirname, "%s%s", out_dir, in_fname)
         : sprintf(out_dirname, "%s%c%s", out_dir, DIR_SEP, in_fname);
   
   assert((in_file = fopen(in_fname, "rb")) != NULL);
   
   end_entry = getNumEntries(in_file);
   
   /*Ignore data size for first offset*/
   fseek(in_file, OFFSET_SIZE, SEEK_CUR);
   
   for(cur_entry = 1; cur_entry < end_entry; ++cur_entry)
      unpackBinaryFile(in_file, out_dirname, cur_entry);
   
   fclose(in_file);
}

int main(int argc, char *argv[])
{
   switch(argc)
   {
      case 3:
         runProgram(argv[1], argv[2]);
         break;
      
      case 2:
         runProgram(argv[1], "outs");
         break;
      
      case 1:
      default:
         printUsage(argv[0]);
         break;
   }
   
   return EXIT_SUCCESS;
}