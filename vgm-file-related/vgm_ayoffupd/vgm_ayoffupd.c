#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printTitle(void)
{
	printf("VGM 1.70 YM2203/YM2608/YM2610 SSG (AY) offset fixer\n");
}

int printErrorAndExit(char * errString)
{
	fprintf(stderr, "%s\n", errString);
	return EXIT_FAILURE;
}

int printUsage(char * exe)
{
	printf(
		"Usage:\n"
		"%s <filename> <mode>\n"
		"where mode can be as follows:\n"
		"  1 - YM2203\n"
		"  2 - YM2608/2610\n"
		"", exe
	);
	return EXIT_SUCCESS;
}

int updateYMAYOffset(int argc, char **argv)
{
	FILE *vgmFile = fopen(argv[1], "r+b");
	unsigned int vgmVer = 0;
	
	if (vgmFile == NULL || strcmp(".vgz", argv[1]+strlen(argv[1])-4) == 0) 
		return printErrorAndExit("file dun goofed son");
	
	/*Check if v1.70+ vgm file (fail if not)*/
	fseek(vgmFile, 0x08, SEEK_SET);
	fread(&vgmVer, sizeof(short), 1, vgmFile);
	printf("vgmVer read: 0x%X\n", vgmVer);
	
	if (vgmVer < 0x170)
		return printErrorAndExit("vgm file not new enough. bog off");
	
	/*Update value*/
	fseek(vgmFile, 0xCF, SEEK_SET);
	
	switch (atoi(argv[2]))
	{
		case 1: /*YM2203 -> 0x819A*/
			fputc(0x9A, vgmFile); /*sup.*/
			fputc(0x81, vgmFile); /*hax*/
			break;
		case 2: /*YM2608 -> 0x80CD*/
			fputc(0xCD, vgmFile); /*my*/
			fputc(0x80, vgmFile); /*anus*/
			break;
		default:
			return printErrorAndExit("incorrect mode");
	}
	
	fflush(vgmFile);
	fclose(vgmFile);
	
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	printTitle();
	
	return (argc != 3) ?
		printUsage(argv[0]) :
		updateYMAYOffset(argc, argv);
}
