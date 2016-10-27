/*
 * Binpack - AFLB (ED4/BVT)
 * 
 * For "Eiyuu Densetsu IV / Brandish VT for NEC PC-9801"
 *
 * (C) 2014 James Alan Nguyen
 * http://www.codingchords.com
 */

#include <stdio.h>
#include <stdlib.h>

#define HEADER_SIZE (0x10)
#define ALIGN_FACTOR (0x20)

#pragma pack(1)

int printUsage(char *exe)
{
	printf("Binpack - AFLB format\n"
		"Usage:\n"
		"%s num_bindata out_filename\n",
		exe
	);
	return EXIT_SUCCESS;
}

int failExit(char *s)
{
	fprintf(stderr, "%s\n", s);
	return EXIT_FAILURE;
}

int runProgram(int argc, char **argv)
{
	/*Assuming arguments are correct here*/
	FILE  *inFile       = NULL,
	      *outFile      = fopen(argv[2], "wb");
	char **arr_bindata  = NULL;
	size_t num_bindata  = strtol(argv[1], NULL, 10), 
	       i            = 0,
			 j            = 0,
	      *size_bindata = NULL,
			*size_paddata = NULL;
	short startData     = 0,
	      offsetVal     = 0/*, headerPadFlag = 0*/;
			
	/*Ensure startData offset address is aligned by 0x20 (32 bytes)
	  Failure to do so will result in game not starting at all*/
	for(i = 0;
		((HEADER_SIZE + ((num_bindata+i)<<1)) % ALIGN_FACTOR);
		++i)
		/*headerPadFlag = ~0*/;
	
	startData = (HEADER_SIZE + ( (((short)num_bindata)+i)<<1) );
	offsetVal = (startData+(i<<1)) / ALIGN_FACTOR;
	
	printf("num_bindata = %d, pad = %d, startData = 0x%X, offsetVal = 0x%X\n", 
		(int)num_bindata,	(int)i, (int)startData, (int)offsetVal*ALIGN_FACTOR
	);
	
	arr_bindata  = calloc(sizeof(char*), num_bindata);
	size_bindata = calloc(sizeof(size_t), num_bindata);
	size_paddata = calloc(sizeof(size_t), num_bindata);
	
	if (arr_bindata == NULL || size_bindata == NULL || 
	    outFile == NULL || size_paddata == NULL)
		return failExit("Memory allocation fail\n");
	
	/*Read all relevantly named files in current directory to memory*/
	for (i = 0; i < num_bindata; ++i)
	{
		/*Filename format: "000", "001", "002" etc.*/
		char name_binID[4]; /*yeah magic numbers, dealwithit.jpg*/
		
		sprintf(name_binID, "%03lu", (unsigned long)i);
		inFile = fopen(name_binID, "rb");
		
		/*User is responsible for counting correctly (should be +1 the final
			filename*/
		if (inFile == NULL)
		{
			for (i = 0; i < num_bindata; ++i) 
				free(arr_bindata[i]);
			
			free(arr_bindata);
			free(size_bindata);
			free(size_paddata);
		
			fclose(outFile);
			return failExit("Cannot open file\n");
		}
		
		/*Get bin size*/
		fseek(inFile, 0, SEEK_END);
		size_bindata[i] = ftell(inFile);
		rewind(inFile);
		
		/*Read to memory*/
		arr_bindata[i] = malloc(size_bindata[i]);
		fread(arr_bindata[i], 1, size_bindata[i], inFile);
		fclose(inFile);
		inFile = NULL;
	}
	
	/*Write headerdata*/
	/*Note: header offsets need to be divisible by 0x20 for alignment to
		fit WORD space*/
	
	/* -- start header -- */
	fprintf(outFile, "AFLB DAT"); /*File header*/
	fputc(0x1A, outFile);
	fputc(0x00, outFile);
	fwrite(&num_bindata, sizeof(char), 1, outFile);
	for(i=0; i<5; ++i) fputc(0x00, outFile);
	/* -- end header -- */
	
	/*Write data offsets, divided by ALIGN_FACTOR*/
	fwrite(&offsetVal, sizeof(short), 1, outFile);
	for (i = 0; i < num_bindata; ++i)
	{
		/*If data is not aligned to 32-byte blocks, add padding*/
		while ((size_bindata[i] + size_paddata[i]) % ALIGN_FACTOR)
			++size_paddata[i];
		
		offsetVal += (size_bindata[i] / ALIGN_FACTOR);
		fwrite(&offsetVal, sizeof(short), 1, outFile);
	}
	
	/*If too many headers found, terminate*/
	if (ftell(outFile) > startData)
	{
		for (i = 0; i < num_bindata; ++i) 
			free(arr_bindata[i]);
		
		free(arr_bindata);
		free(size_bindata);
		free(size_paddata);
	
		fclose(outFile);
		return failExit("ERROR: Header written inside binary data region");
	}
	/*Pad zeroes until data region*/
	while (ftell(outFile) < startData)
		fputc(0x00, outFile);
	
	/*Write data*/
	for (i = 0; i < num_bindata; ++i)
	{
		for (j = 0; j < (size_bindata[i] + size_paddata[i]); ++j)
		{
			(j < size_bindata[i]) ?
				fputc(arr_bindata[i][j], outFile) : 
			   fputc(0xFF, outFile);
		}
		
		free(arr_bindata[i]);
	}
	
	free(arr_bindata);
	free(size_bindata);
	free(size_paddata);
	
	fclose(outFile);
	
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{	
	return (argc != 3) ? printUsage(argv[0]) : runProgram(argc, argv);
}