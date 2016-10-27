/*
 * VGM YM2149-based SSG -> NULL
 *   for use with songs that have annoying sfx embedded.
 *
 * (C) 2014 James Alan Nguyen
 * http://www.codingchords.com
 *
 * use at your peril
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(1)

#define DEF_BUF_SIZE (80)

int printUsage(char *execName)
{
   printf(
      "VGM (AY) converter"
      "Usage: %s file.vgm\n",
      execName
   );

   /*TODO: support reverse direction conversion.  YM2203/YM2608 is 4x faster than AY3*/

   return EXIT_SUCCESS;
}

int run(char *fileName, int argc, char **argv)
{
   char nameCopy[DEF_BUF_SIZE];/*= "converted.vgm";*/
   char *fileBuffer = NULL, *curPos = NULL, *bufEnd = NULL;
   FILE *inFile = NULL, *outFile = NULL;
   size_t fileSize = 0;
   
   /*Do not allow converting .vgz (TODO gzip integration?)*/
   if (strcmp(".vgz", &(fileName[strlen(fileName)-4])) == 0)
   {
      perror(".vgz not supported\n");
      return EXIT_FAILURE;
   }
   
   strncpy(nameCopy, fileName, DEF_BUF_SIZE);
   strncpy(&(nameCopy[strlen(fileName)-4]), "-ay.vgm", DEF_BUF_SIZE); 
   
   inFile  = fopen(fileName, "rb"),
   outFile = fopen(nameCopy, "wb");
   
   if (inFile == NULL || outFile == NULL)
   {
      perror("File cannot be opened.\n");
      return EXIT_FAILURE;
   }
   
   /*Get file size*/
   fseek(inFile, 0, SEEK_END);
   fileSize = ftell(inFile);
   fseek(inFile, 0, SEEK_SET);

   /*Init file in memory*/
   fileBuffer = calloc(sizeof(char), fileSize);
   if (fileBuffer == NULL)
   {
      perror("Cannot initialize file buffer.\n");
      return EXIT_FAILURE;
   }

   /*Copy contents*/
   /*TODO: ASSUMING VGM 1.70+ HEADER*/
   fread(fileBuffer, 1, fileSize, inFile);
   fclose(inFile);

   /*Enable AY clock w/ default frequency*/
   /*fileBuffer[0x75] = 0x78;
   fileBuffer[0x76] = 0x1E;*/
   curPos = &(fileBuffer[0xE0]);
   bufEnd = &(fileBuffer[fileSize-1]);

   /*Exhaustively read each VGM command*/
   while (curPos <= bufEnd)
   {
      size_t ptrInc;

      switch(curPos[0])
      {
      case 0x55: /*YM2203*/
      case 0x56: /*YM2608*/
         ptrInc = 3;
         /*Change YM2203/YM2608 SSG commands to AY-3-8910*/
         if (curPos[1] >= (char)0x00 && curPos[1] <= (char)0x0F)
         {
				char filterMe = 0;
				/*Channel filter or all*/
				if (argc > 2)
					switch (argv[2][0])
					{
						case '0':
							if (curPos[1] == 0x00 || curPos[1] == 0x01 || curPos[1] == 0x08)
								filterMe++;
							break;
						case '1':
							if (curPos[1] == 0x02 || curPos[1] == 0x03 || curPos[1] == 0x09)
								filterMe++;
							break;
						case '2':
							if (curPos[1] == 0x04 || curPos[1] == 0x05 || curPos[1] == 0x0A)
								filterMe++;
							break;
						default:
							filterMe++;
					}
				else
					filterMe++;
				
				if (!filterMe)
					break;
					
            /*printf("DEBUG: CONVERTED ADDR 0x%X\r", (curPos-fileBuffer));*/
            curPos[0] = 0x61;
				curPos[1] = 0x00;
				curPos[2] = 0x00; /*Lazy*/
         }
         break;

      case 0x62: /*1-byte Wait 60Hz shortcut*/
      case 0x63: /*1-byte Wait 50Hz shortcut*/
         ptrInc = 1;
         break;
      
      case 0x57: /*YM2608 - extended commands*/
      case 0x61: /*3-byte Wait*/
         ptrInc = 3;
         break;

      case 0x66: /*End of sound data*/
         ptrInc = 1;
         curPos = (bufEnd);
         break;
      
      case 0x67: /*Data Block*/
         printf("DEBUG: Curr addr 0x%X\n", curPos-fileBuffer);
         ptrInc = ((*(int*)(curPos+3))); /*32-bit offset*/
         printf("DEBUG: Offset Data Block 0x%X\n", ptrInc);
         curPos += ptrInc + 0x07;
         printf("DEBUG: New addr 0x%X\n", curPos-fileBuffer);
         ptrInc = 0;
         break;
      
      default:
         if (curPos[0] & 0x70) /*1-byte Wait*/
            ptrInc = 1;
			else
				ptrInc = 3;
         break;
      }
      
      /*Increment based on prefix command*/
      curPos += ptrInc;
   }

   /*Write to file*/
   fwrite(fileBuffer, 1, fileSize, outFile);
   fclose(outFile);

   return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
   if (argc < 2)
      return printUsage(argv[0]);

   return run(argv[1], argc, argv);
}