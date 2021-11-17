/*
 * VGM YM3812 (OPL2) -> YMF278B (OPL4)
 *
 * (C) 2016 James Alan Nguyen
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
      "VGM OPL2->OPL4 converter"
      "Usage: %s file.vgm\n",
      execName
   );

   /*TODO: support reverse direction conversion.*/

   return EXIT_SUCCESS;
}

int run(char *fileName)
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
   strncpy(&(nameCopy[strlen(fileName)-4]), "-ym2612.vgm", DEF_BUF_SIZE); 
   
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

   /*Copy clock frequency for YM3812 to YMF278B*/
   /*fileBuffer[0x75] = 0x78;
   fileBuffer[0x76] = 0x1E;*/
	//memcpy(&fileBuffer[0x60], &fileBuffer[0x50], 4);
	//memset(&fileBuffer[0x60], (int)((*(int*)(fileBuffer+0x50))*9.4617613), 4);
	//memset(&fileBuffer[0x60], (*(int*)(fileBuffer+0x50)), 4);
	memset(&fileBuffer[0x60], 0x204CC00, 4); // hardcoded clock TODO
	memset(&fileBuffer[0x50], 0, 4);
	memset(&fileBuffer[0x5C], 0, 4);
	
	// write header of new file from 0x00 to 0x80
	
	// keep note of the new file write size - every command will need 1 extra byte
	// since it needs to define port numbers for the second byte.
	fwrite(fileBuffer, 1, 0xC0, outFile);
	
   curPos = &(fileBuffer[0xC0]);
   bufEnd = &(fileBuffer[fileSize-1]);
	
   /*Exhaustively read each VGM command*/
   while (curPos <= bufEnd)
   {
      size_t ptrInc;

      switch(curPos[0])
      {
      case 0x5A: /*YM3812*/
	  case 0x5E: // YMF262 Port 0
	  case 0x5F: // YMF262 Port 1
		fputc(0xD0, outFile);
		fputc((curPos[0]==0x5A || curPos[0]==0x5E)?0x00:0x01, outFile); // YM3812 - only port 0 is used? TODO
		fputc(curPos[1], outFile);
		fputc(curPos[2], outFile);
		ptrInc=3;
		break;
		
      case 0x62: /*1-byte Wait 60Hz shortcut*/
      case 0x63: /*1-byte Wait 50Hz shortcut*/
         ptrInc = 1;
		 fputc(curPos[0], outFile);
         break;
      
      case 0x61: /*3-byte Wait*/
         ptrInc = 3;
		 fwrite(curPos, 1, 3, outFile);
         break;

      case 0x66: /*End of sound data*/
         ptrInc = 1;
         curPos = (bufEnd);
		 fputc(0x66, outFile);
         break;
      
      case 0x67: /*Data Block*/
		// let's forget about it for now - most OPL2 vgms don't have one of these
         printf("DEBUG: Curr addr 0x%X\n", curPos-fileBuffer);
         ptrInc = ((*(int*)(curPos+3))); /*32-bit offset*/
         printf("DEBUG: Offset Data Block 0x%X\n", ptrInc);
         curPos += ptrInc + 0x07;
         printf("DEBUG: New addr 0x%X\n", curPos-fileBuffer);
         ptrInc = 0;
         break;
      
      default:
         if (curPos[0] & 0x70) /*1-byte Wait*/
		 {
            ptrInc = 1;
			fputc(curPos[0], outFile);
		 }
			else
			{
				ptrInc = 3;
				fwrite(curPos, 1, 3, outFile);
			}
         break;
      }
      
      /*Increment based on prefix command*/
      curPos += ptrInc;
   }

   /*Write to file*/
   //fwrite(fileBuffer, 1, fileSize, outFile);
   fclose(outFile);

   return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
   if (argc != 2)
      return printUsage(argv[0]);

   return run(argv[1]);
}