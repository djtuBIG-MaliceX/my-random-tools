/*
 * Bin shift + repack
 *  For Knights of Xentar / Dragon Knight 3 PC-DOS
 *  Tested working with all MegaTech games (*EMI.VOL)
 * 
 * (C) 2013-2014 James Alan Nguyen
 * http://www.codingchords.com
 *
 * Expected format:
 *  0 to (location at first 0x4 bytes) - fixed array of starting offsets, each 0x4 bytes
 *    ie: 0xC0 entries (192)
 *  Remaining bytes - Raw binary data
 */

#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#pragma pack(1)

static FILE *fin, *fout;
static unsigned char **num_blobs, *blob_order;
static int num_entries = 1, i, *blob_start, *blob_size, cumulative_offset,
       header_bound = ~0;
//static bool is_saving = false;

#define ABSOLUTE_MAX_ENTRIES 0xFFFF
#define MAX_ENTRIES (header_bound/0x4)

void printUsage(void)
{
   printf("Usage: <binfile> <shiftfactor>\n");
   exit(EXIT_SUCCESS);
}

void checkPos(FILE *fin)
{
   printf("DEBUG: ftell(fin) = 0x%X\n", (int)ftell(fin));
}

void fileCheck(FILE *fin, bool checkMode)
{
   if (((checkMode) ? feof(fin) : !feof(fin)))
   {
      printf("File is not a binary packed file of the expected type\n");
      
      // Deallocate
      fclose(fin);
      
      for (i = 0; i < num_entries; ++i)
         free(num_blobs[i]);
      
        
      free(num_blobs);
      free(blob_start);
      free(blob_order);
      free(blob_size);
      
      printUsage();
   }
}

int main(int argc, char *argv[])
{  
   unsigned int eof_loc = ~0;
   char *out_fname = argv[1];
   
   if (argc < 2) printUsage();
   if ((fin = fopen(argv[1], "rb")) == NULL)
   {
      printf("Cannot read file.\n");
      printUsage();
   }
   
   printf("Opening file %s...\n", argv[1]);
   
   //num_entries = 0xC0;
   
   /* HEADER READ */
   
   // Allocate references into memory
   num_blobs = calloc(sizeof(void*), ABSOLUTE_MAX_ENTRIES);
   blob_start = calloc(4, ABSOLUTE_MAX_ENTRIES);
   blob_size = calloc(4, ABSOLUTE_MAX_ENTRIES);
   blob_order = calloc(1/*sizeof(uint8_t)*/, ABSOLUTE_MAX_ENTRIES);
   
   for (i = 0; i < ABSOLUTE_MAX_ENTRIES; ++i)
   {
      int some_size = 0;
      
      // Read data sizes with respect ot each data size
      fread(&blob_start[i], 1, 4/*sizeof(uint32_t)*/, fin);
      
      checkPos(fin);
      
      // Preallocate space to hold data
      if (i > 0)
      {
         some_size = blob_start[i] - blob_start[i-1];
         printf("DEBUG = 0x%X - 0x%X\n", blob_start[i], blob_start[i-1]);
         
         if (blob_start[i] == 0 && blob_start[i-1] != 0)
         {
            int cur_loc = (int)ftell(fin);
            
            // Retain current offset, calculate size from last location to end
            fseek(fin, 0, SEEK_END);
            some_size = ftell(fin) - blob_start[i-1];
            eof_loc = ftell(fin);
            fseek(fin, cur_loc, SEEK_SET);
         }
         
         if (blob_start[i-1] > 0)
         {
            blob_size[i-1] = some_size;
            num_blobs[i-1] = calloc(4/*sizeof(uint8_t)*/, blob_size[i-1]);
         }
         printf("Size: 0x%X\n", (int)blob_size[i-1]);
         
         // Set header boundary based on starting data location
         if (i == 1)
            header_bound = blob_start[0];
         
      }
      
      printf("Blob %d start location: 0x%X ", (int)i, (int)blob_start[i]);
      
      // Preset data order (to be changed by user later)
      blob_order[i] = i;
      
      if (ftell(fin) == header_bound)
      {
         printf("DEBUG: Reached header bound 0x%X\n", (int)header_bound);
         break;
      }
   }
   
   assert(!feof(fin));
   printf("ftell(fin) = 0x%X , header_bound = 0x%X\n", (int)ftell(fin), (int)header_bound);
   assert(ftell(fin) == (size_t)header_bound);
   
   /* DATA READ */
   
   for (i = 0; i < MAX_ENTRIES; ++i, ++num_entries)
   {
      size_t bytes_read = 0;
      
      // Stop writing once reached end of data, and set number of entries
      if (blob_size[i] == 0)
         break;
      
      assert(!feof(fin));
      
      // Read in each block of data to memory
      size_t bytes_to_read = (blob_start[i] < blob_start[i+1]) ? (size_t)((int)blob_start[i+1] - (int)blob_start[i]) : (size_t)~0;
      bytes_read = fread(num_blobs[i], 1, bytes_to_read, fin);
         
      printf("Reading blob %d (0x%X bytes read, expecting 0x%X)...\n", 
         (int)i, (int)bytes_read, (unsigned int)blob_size[i]);
   }
   //checkPos(fin);
   
   //fileCheck(fin, false);
   fclose(fin);
   
   /* WHAT DO YOU WANT TO DO? */
   
   unsigned char shift_factor = (argc==3) ? atoi(argv[2]) : 0;
   
   if (shift_factor >= num_entries)
      shift_factor = 0;
   
   // shift data up by one place, wrapping around.
   // NOTE: shift_factor indicates how many from start to leave alone.
   // eg: 0 1 2 3 4 -> 0 2 3 4 1 (zero stays there; shift_factor = 1
   for (i = shift_factor; i < num_entries-1; ++i)
      blob_order[i]++;  // do not increment EOF loc
   blob_order[num_entries-1] = shift_factor;
   
   //DEBUG
   for (i = 0; i < num_entries; ++i) printf("0x%X ", blob_start[blob_order[i]]);
   printf("\n");
   
   if ((fout = fopen(out_fname, "wb")) == NULL)
   {
      perror("Cannot write out.bin.  Ensure you have write permissions!\n");
      exit(EXIT_FAILURE);
   }
   
   cumulative_offset = 0;
   
   // Write reordered data sizes
   for (i = 0; i < num_entries-1; ++i)
   {
      // Accumulate
      cumulative_offset += (i==0) ? header_bound : blob_size[(int)blob_order[i-1]];
      
      printf("Writing offset %d (0x%X) sizeof(uint32_t) = 0x%X...\n",
         (int)blob_order[i], (int)blob_start[i],
         (int)fwrite(&cumulative_offset, 1, 4/*sizeof(uint32_t)*/, fout));
   }
   
   printf("Writing EOF offset %d (0x%X) sizeof(uint32_t) = 0x%X...\n",
      (int)blob_order[i], (int)eof_loc,
      (int)fwrite(&eof_loc, 1, 4, fout));
   
   // Zero pad to end of header
   while (ftell(fout) != header_bound)
      fputc(0, fout);
   
   
   // Write data blobs in new order
   for (i = 0; i < num_entries; ++i)
   {
      printf("Writing blob %d (0x%X bytes)...\n", (int)blob_order[i],
         (int)fwrite(num_blobs[(int)blob_order[i]], 1, blob_size[(int)blob_order[i]], fout));
   }
   
   printf("%s written successfully.\n", out_fname);
   
   // Deallocate
   
   fclose(fout);
   for (i = 0; i < num_entries; ++i)
      free(num_blobs[i]);
      
   free(num_blobs);
   free(blob_start);
   free(blob_order);
   free(blob_size);
   
   return EXIT_SUCCESS;
}