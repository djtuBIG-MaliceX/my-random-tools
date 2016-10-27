/*
 * Bin shift + repack
 *  For SImpsons arcade DOS
 * 
 * (C) 2013 James Alan Nguyen
 * http://www.codingchords.com
 *
 * Expected format:
 *  0x4 bytes - Number of data blobs
 *  (0x4 * number of data blobs) bytes - Corresponding data sizes
 *  Remaining bytes - Raw binary data
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#pragma pack(1)

static FILE *fin, *fout;
static uint8_t **num_blobs, *blob_order;
static size_t num_entries = 0, i, *blob_size;
//static bool is_saving = false;

void printUsage(void)
{
   printf("Usage: <binfile>\n");
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
      free(blob_size);
      free(blob_order);
      
      printUsage();
   }
}

int main(int argc, char *argv[])
{  
   if (argc != 2) printUsage();
   if ((fin = fopen(argv[1], "rb")) == NULL)
   {
      printf("Cannot read file.\n");
      printUsage();
   }
   
   printf("Opening file %s...\n", argv[1]);
   checkPos(fin);
   
   // Read number of data sizes (these are not offsets)
   fread(&num_entries, /*sizeof(uint32_t)*/4, 1, fin);
   checkPos(fin);
   
   /* HEADER READ */
   
   // Allocate references into memory
   num_blobs = calloc(sizeof(void*), num_entries);
   blob_size = calloc(sizeof(size_t), num_entries);
   blob_order = calloc(1/*sizeof(uint8_t)*/, num_entries);
   
   for (i = 0; i < num_entries; ++i)
   {
      // Read data sizes with respect ot each data size
      printf("Blob %d size: 0x%X bytes\n", (int)i, 
         (int)fread(&blob_size[i], 1, 4/*sizeof(uint32_t)*/, fin));
   
      checkPos(fin);
      
      // Preallocate space to hold data
      num_blobs[i] = calloc(1/*sizeof(uint8_t)*/, blob_size[i]);
      
      // Preset data order (to be changed by user later)
      blob_order[i] = i;
   }
   
   assert(!feof(fin));
   
   /* DATA READ */
   
   for (i = 0; i < num_entries; ++i)
   {
      assert(!feof(fin));
      // Read in each block of data to memory
      printf("Reading blob %d (0x%X bytes read, expecting 0x%X)...\n", (int)i, 
         (int)fread(num_blobs[i], 1, blob_size[i], fin), (int)blob_size[i]);
         
      checkPos(fin);
      
   }
   //checkPos(fin);
   
   //fileCheck(fin, false);
   fclose(fin);
   
   /* WHAT DO YOU WANT TO DO? */
   
   // Set 2nd to 1st, 1st to last etc.
   // eg: 0 1 2 3 4 -> 1 2 3 4 0
   for (i = 0; i < num_entries; ++i)
      blob_order[i]++;
   blob_order[num_entries-1] = 0;
   
   
   if ((fout = fopen("out.bin", "wb")) == NULL)
   {
      perror("Cannot write out.bin.  Ensure you have write permissions!\n");
      exit(EXIT_FAILURE);
   }
   
   // Write number of entries
   fwrite(&(num_entries), 4/*sizeof(uint32_t)*/, 1, fout);
   
   // Write reordered data sizes
   for (i = 0; i < num_entries; ++i)
   {
      printf("Writing size %d (0x%X) sizeof(uint32_t) = 0x%X...\n",
         (int)blob_order[i], (int)blob_size[i],
         (int)fwrite(&blob_size[blob_order[i]], 4/*sizeof(uint32_t)*/, 1, fout));
      
   }
   
   
   // Write data blobs in same order
   for (i = 0; i < num_entries; ++i)
   {
      printf("Writing blob %d (0x%X bytes)...\n", (int)blob_order[i],
         (int)fwrite(num_blobs[blob_order[i]], blob_size[blob_order[i]], 1, fout));
   }
   
   printf("out.bin written successfully.");
   
   // Deallocate
   
   fclose(fout);
   for (i = 0; i < num_entries; ++i)
      free(num_blobs[i]);
      
   free(num_blobs);
   free(blob_size);
   free(blob_order);
   
   return EXIT_SUCCESS;
}