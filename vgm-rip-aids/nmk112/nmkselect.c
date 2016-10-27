/*
 * NMK112 bank switcher for .vgm 
 *
 * (C) 2014 James Alan Nguyen
 * http://www.codingchords.com
 *
 * Utility for nominating 4 sample ROM banks to be used as part
 * of the .vgm logged output from MAME VGM Mod 0.152.
 *
 * This one's for Armed Police Batrider.  Note that the resultant output
 * may require more tweaking due to mid-song bank-switching.
 * This utility only assigns sample bank at the BEGINNING of
 * song playback.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define OKIM_BANKSIZ (0x10000)
#define OKIM_BANKNUM (0x4)
#define OKIM_HEADSIZ (0x100)
#define OKIM_SROMSIZ (0x100000)

int printUsage()
{
   printf("Usage: [input VGM file] [OKIM6295 sample ROM] "
          "[Bank1(0-F)] [Bank2(0-F)] [Bank3(0-F)] [Bank4(0-F)]"
          "[output VGM file]\n");
   return EXIT_SUCCESS;
}

int loadError()
{
   printf("Error: failed to load one of the files.\n");
   
   return EXIT_FAILURE;
}

int processError(int error)
{
   switch(error)
   {
      case 0:
         return EXIT_SUCCESS;
      case 1:
         return loadError();
      default:
         printf("General error.\n");
         return EXIT_FAILURE;
   }
}

int runProgram(int argc, char **argv)
{
   FILE *in_vgm = fopen(argv[1], "rb"),
        *in_rom = fopen(argv[2], "rb"),
        *out_vgm = fopen(argv[7], "wb");
        
   int error = 0;
   
   if (in_vgm == NULL || in_rom == NULL || out_vgm == NULL)
      error++;
   
   if (!error)
   {
      uint8_t
         bank[OKIM_BANKNUM][OKIM_BANKSIZ], /*Note: 0x100-byte header = sample offsets*/
         bank_id[OKIM_BANKNUM],
         file_buffer;
         
      memset(bank, 0, OKIM_BANKNUM*OKIM_BANKSIZ);
      memset(bank_id, 0, OKIM_BANKNUM);
      
      /* Read in selected banks from sample ROM */
      for (uint8_t i = 0; i < OKIM_BANKNUM; ++i)
      {
         bank_id[i] = (uint8_t)strtoul(argv[3+i], NULL, 16);
         fseek(in_rom, bank_id[i]*OKIM_BANKSIZ, SEEK_SET);
         fread(bank[i], OKIM_BANKSIZ, 1, in_rom);
      }
      
      /* Write bank switch offsets */
      for (uint8_t i = 0; i < OKIM_BANKNUM-1; ++i)
      {
         memcpy(&(bank[0][(i+1)*OKIM_HEADSIZ]), 
                &(bank[i+1][(i+1)*OKIM_HEADSIZ]), OKIM_HEADSIZ);
      }
      
      /* Write new VGM */
      
      /*copy 0x0 - 0xCE*/
      fseek(in_vgm, 0, SEEK_SET);
      while (ftell(in_vgm) < 0xCF)
      {
         fread(&file_buffer, sizeof(uint8_t), 1, in_vgm);
         fwrite(&file_buffer, sizeof(uint8_t), 1, out_vgm);
      }
      
      /*write bank[0] - bank[3] at 0xCF onwards*/
      fwrite(bank, OKIM_BANKNUM*OKIM_BANKSIZ, 1, out_vgm);
      
      /*pad 0xFF until 0x1400CE*/
      while(ftell(out_vgm) < 0x2800DE)
         fputc(0xFF, out_vgm);
      
      /*copy 0x2800DE - EOF */
      fseek(in_vgm, 0x2800DE, SEEK_SET);
      while (!feof(in_vgm))
      {
         //printf("DEBUG: 0x%X vs 0x1400CF\n", ftell(in_vgm));
         fread(&file_buffer, sizeof(uint8_t), 1, in_vgm);
         fputc(file_buffer, out_vgm);
      }
   }
   
   (in_vgm != NULL) ? fclose(in_vgm) : 0;
   (in_rom != NULL) ? fclose(in_rom) : 0;
   (out_vgm != NULL) ? fclose(out_vgm) : 0;
   
   return processError(error);
}


int main(int argc, char **argv)
{
   if (argc != 8) return printUsage();
   
   return runProgram(argc, argv);
}