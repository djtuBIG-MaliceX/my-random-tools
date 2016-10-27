
#include <stdio.h>

int main(int argc, char **argv)
{
   FILE *fp = fopen("bgaregga_0-test.vgm", "a");
   
   char i = (char)(0x80 & 0xFF);
   
   while (i != 0)
   {
      printf("Writing OKIM6295 Sample 0x%X Playback...\n", (i-0x80)&0xFF);
      
      fputc(0x61, fp);
      fputc(0xFF, fp);
      fputc(0xFF, fp);
   
      fputc(0xB8, fp);
      fputc(0x00, fp);
      fputc(0x78, fp);
      
      fputc(0xB8, fp);
      fputc(0x00, fp);
      fputc(i & 0xFF, fp);
      
      fputc(0xB8, fp);
      fputc(0x00, fp);
      fputc(0x10, fp);
      
      i++;
   }
   
   fputc(0x66, fp);
   
   fclose(fp);
   
   return 0;
}