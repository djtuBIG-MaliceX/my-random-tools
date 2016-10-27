/*
 * 2015 James Alan Nguyen
 * http://www.codingchords.com
 *
 * VGM F-num + clock converter (for OPN)
 *  (this code version is for YM2612 target only)
 *
 * Basically converts all necessary F-num commands such that
 * they are correctly in tune according to the new clock speed.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

typedef struct fnum_work_t
{
	long double fnote;  // fundamental frequency
	uint8_t B;     // 3-bit octave block,  FNum MSB D5-D3.  upper 3 bits of FNum2 
	uint16_t FNum; // 11-bit, where D10-D8 are lower 3 bits of FNum2
	               // and D7-D0 are FNum1 in its entirety.
} fnum_work_t;

static uint8_t *workVgmData;
static uint64_t vgmSize, oldMasterClock, newMasterClock, byteCount;

uint16_t calcFNum(fnum_work_t * fnumWork, uint64_t clk)
{
	// 2^20 = 1048576 = (1<<20)
	uint64_t newFNum = (uint64_t)(roundl(((long double)(144 * fnumWork->fnote * (1<<20)) / clk) / pow(2, fnumWork->B-1)));
	
	// Repeat the calculation if the FNum exceeds 11 bits by increasing the B counter.
	/*if (newFNum == (1<<11))
	{
		printf("DEBUG: ERROR!\n");
	}*/
	
	if (newFNum >= (1<<11) && (fnumWork->B < 0x7))
	{
		fnumWork->B++;
		return calcFNum(fnumWork, clk);
		
	} else if (newFNum >= (1<<11) && (fnumWork->B >= 0x7))
	{
		newFNum = 0x7FF;
		fnumWork->B = 0x7; // in case wrapped
	}
	
	fnumWork->FNum = newFNum;
	
	return fnumWork->FNum;
}

double calcFNote(fnum_work_t * fnumWork, uint64_t clk)
{
	long double newFNote = (fnumWork->FNum * pow(2, fnumWork->B-1) * clk) / (long double)(144 * (1<<20) /*powl(2,20)*/);
	
	fnumWork->fnote = newFNote;
	
	return fnumWork->fnote;
}

void readFNumData(void)
{
	// F-num commands:  
	//   prepare = D5-D3 (octave block),  D2-D0 (F-Num MSB)
	//   set     = D7-D0 (F-Num LSB)
	
	//     prep   set
	//     0xA4 - 0xA0 (ch 0)
	//     0xA5 - 0xA1 (ch 1)
	//     0xA6 - 0xA2 (ch 2)
	//     	0xAC - 0xA8 (ch 2 op 0)
	//       0xAD - 0xA9 (ch 2 op 1)
	//       0xAE - 0xAA (ch 2 op 2)
	//       (use 0xA6 for ch 2 op 3)
	//     ...below require chip 0x52 to be 0x53 but this isn't too
	//        important. :)  (HAX MY ANUS!)
	//     0x1A4 - 0x1A0 (ch 3)
	//     0x1A5 - 0x1A1 (ch 4)
	//     0x1A6 - 0x1A2 (ch 5)
	//     
	//  interesting: Zai Metajo music data attempts to set values
	//               on nonexistent channels/registers. perhaps
	//               these could be used on YM2151?
	//   eg:  0x43, 0x47, 0x4B,    (operator data?)
	//        0xAC - 0xA8          (F-Num???)
	//        0xAD - 0xA9          (F-Num???)
	//        0xAE - 0xAA          (F-Num???)
	
	uint8_t *prepareLoc, *setLoc;
	uint16_t fnum1, fnum1Reg, fnum2=0xFF;
	uint64_t byteCountOffset;
	fnum_work_t oldVal, newVal;
	bool isFound = false;
	
	//printf("DEBUG: WRITE!\n");
	
	memset(&oldVal, 0, sizeof(fnum_work_t));
	memset(&newVal, 0, sizeof(fnum_work_t));
	
	// If command found, mark current position
	prepareLoc = &(workVgmData[byteCount+2]);
	
	// Retrieve F-Num1 (prepare) and F-Num2 (set) values
	fnum1Reg = workVgmData[byteCount+1];
	fnum1 = workVgmData[byteCount+2];
	
	// 3 bytes forward is next command
	byteCountOffset = 3;
	
	while ((byteCount+byteCountOffset)<vgmSize)
	{
		uint8_t increment = 3;
		
		switch (workVgmData[byteCount+byteCountOffset])
		{
			case 0x55: // YM2203
			case 0x56: // YM2608
			case 0x52: // YM2612
			{
				uint8_t cmd = (workVgmData[byteCount+byteCountOffset+1]);
				isFound = 
					(fnum1Reg==0xA4 && cmd==0xA0) || // ch0
					(fnum1Reg==0xA5 && cmd==0xA1) || // ch1
					(fnum1Reg==0xA6 && cmd==0xA2) || // ch2
					(fnum1Reg==0xAC && cmd==0xA8) || // ch2op0
					(fnum1Reg==0xAD && cmd==0xA9) || // ch2op1
					(fnum1Reg==0xAE && cmd==0xAA)    // ch2op2
					;
				
				if (isFound)
				{
					fnum2 = workVgmData[byteCount+byteCountOffset+2];
					setLoc = &(workVgmData[byteCount+byteCountOffset+2]);
				}
				
				increment = 3;
				break;
			}
			case 0x57: // YM2608 block 2
			case 0x53: // YM2612 block 2
			{
				uint8_t cmd = (workVgmData[byteCount+byteCountOffset+1]);
				isFound = 
					(fnum1Reg==0xA4 && cmd==0xA0) || // ch4
					(fnum1Reg==0xA5 && cmd==0xA1) || // ch5
					(fnum1Reg==0xA6 && cmd==0xA2)    // ch6
					;
				
				if (isFound)
				{
					fnum2 = workVgmData[byteCount+byteCountOffset+2];
					setLoc = &(workVgmData[byteCount+byteCountOffset+2]);
				}
				
				increment = 3;
				break;
			}
			
			case 0x61:
				increment = 3;
				break;
				
			case 0x62:
				increment = 1;
				break;
			
			case 0x66: /*End of sound data*/
				return;
			
			case 0x67: /*Data Block*/
				increment = (*((uint32_t*)&workVgmData[byteCount+byteCountOffset+3]));
				byteCountOffset += increment + 0x07;
				increment = 0;
				break;

			case 0x94: // DAC CTRL
			case 0x4f: // SN PSG
			case 0x50: // SN PSG
				increment = 2;
				break;
			case 0x95: // DAC CTRL
				increment = 5;
				break;
			
			// waits and all others
			default:
			{
				uint8_t cmd = workVgmData[byteCount+byteCountOffset];
				increment = 
					((cmd>=0x70&&cmd<=0x8F) || cmd==0x62 || cmd==0x63) ? 1 : // single byte wait
					3;
				break;
			}
		}
		
		//printf("   IN!: %lu  vs  %lu         [%X %X %X]\n", 
		//(unsigned long)(byteCount+byteCountOffset), (unsigned long)vgmSize,
		//workVgmData[byteCount+byteCountOffset], workVgmData[byteCount+byteCountOffset+1], workVgmData[byteCount+byteCountOffset+2]);
		
		if (isFound) 
			break;
		
		byteCountOffset += increment;
	}
	
	// Populate fnum_work_t variable with values obtained.
	oldVal.FNum = ((fnum1&0x7)<<8)|(fnum2&0xFF);
	oldVal.B    = (fnum1>>3)&0x7;
	
	/*
	if (oldVal.FNum >= (1<<11))
	{
		printf("oldFNote overflow %u (B=%u)\n", (unsigned int)oldVal.FNum, (unsigned int)oldVal.B);
	}
	*/
	// Calculate f-note from fnum_work_t
	calcFNote(&oldVal, oldMasterClock);
	
	// Create new fnum_work_t with new clock and fnote found to
	newVal.B = oldVal.B;
	newVal.fnote = oldVal.fnote;
	
	// calculate new F-Num
	calcFNum(&newVal, newMasterClock);
	
	// Write FNum1 and FNum2 values at respective locations
	(*prepareLoc) = ((newVal.B&0x7)<<3)|((newVal.FNum>>8)&0x7);
	(*setLoc)     = (newVal.FNum&0xFF);
	
	// continue
}

int main(int argc, char *argv[])
{
	// args:
	//   [1] - vgm file (for now OPNB-compatible only)
	//   [2] - new clock (eg: 7670453)
	//   [3] - (optional) output filename
	//   [4] - -opna -opn -opnb (select which clock to update) default: -opnb
	
	// Open vgm file and scan to memory (close vgm file afterward).
	FILE *inVgm = fopen(argv[1], "rb"),
	     *outVgm = fopen((argc>=4)?argv[3]:"out.vgm", "wb");
	bool endOfVgm = false;
	
	if (inVgm == NULL || outVgm == NULL)
	{
		perror("ERROR: Cannot open VGM.\n");
		return EXIT_FAILURE;
	}
	
	fseek(inVgm, 0, SEEK_END);
	vgmSize = ftell(inVgm);
	workVgmData = calloc(vgmSize, sizeof(uint8_t));
	
	if (workVgmData == NULL)
	{
		perror("ERROR: Cannot allocate memory\n");
		fclose(inVgm);
		fclose(outVgm);
		return EXIT_FAILURE;
	}
	
	rewind(inVgm);
	fread(workVgmData, sizeof(uint8_t), vgmSize, inVgm);
	fclose(inVgm);
	
	// Collect YM2612 master clock value on vgm.
	// (could be changed to support YM2203 or YM2608 etc.)
	//   YM2612 - 0x2C
	//   YM2608 - 0x48
	//   YM2203 - 0x44  // TODO does not work correctly!
	// NOTE:  THIS WILL _NOT_ CHANGE SSG!
	if (argc>=5)
	{
			
		if (strcmp("-opna", argv[4])==0)
		{
			oldMasterClock = (*(uint32_t*)&(workVgmData[0x48]));
			newMasterClock = strtoul(argv[2], NULL, 0);
			(*(uint32_t*)&(workVgmData[0x48])) = (uint32_t)newMasterClock;
			
		} else if (strcmp("-opn", argv[4])==0)
		{
			oldMasterClock = (*(uint32_t*)&(workVgmData[0x44]));
			newMasterClock = strtoul(argv[2], NULL, 0);
			(*(uint32_t*)&(workVgmData[0x44])) = (uint32_t)newMasterClock;
		}
		else //if (strcmp("-opnb", argv[5])==0)
		{
			oldMasterClock = (*(uint32_t*)&(workVgmData[0x2C]));
			newMasterClock = strtoul(argv[2], NULL, 0);
			(*(uint32_t*)&(workVgmData[0x2C])) = (uint32_t)newMasterClock;
		}
	}		
	
	else // default to OPNB
	{
		oldMasterClock = (*(uint32_t*)&(workVgmData[0x2C]));
		newMasterClock = strtoul(argv[2], NULL, 0);
		(*(uint32_t*)&(workVgmData[0x2C])) = (uint32_t)newMasterClock;
	}
	
	
	
	// scan-and-write vgm until encounter F-Num prepare register
	// note: chip command for ym2612 = 0x52 0xnn 0xnn
	byteCount = 0x34+workVgmData[0x34];
	
	while (byteCount<vgmSize && !endOfVgm)
	{
		uint8_t increment = 3;
		
		switch(workVgmData[byteCount])
		{
			case 0x55: // YM2203
			case 0x56: // YM2608
			case 0x52: // YM2612
			{
				switch (workVgmData[byteCount+1])
				{
					case 0xA4: // ch0
					case 0xA5: // ch1
					case 0xA6: // ch2
					case 0xAC: // ch2op0
					case 0xAD: // ch2op1
					case 0xAE: // ch2op2
					{
						readFNumData();
						break;
					}
				}
				increment = 3;
				break;
			}
			case 0x57: // YM2608 block 2
			case 0x53: // YM2612 block 2
			{
				switch (workVgmData[byteCount+1])
				{
					case 0xA4: // ch4
					case 0xA5: // ch5
					case 0xA6: // ch6
					{
						readFNumData();
						increment = 3;
						break;
					}
				}
				break;
			}
			
			case 0x61:
				increment = 3;
				break;
				
			case 0x62:
				increment = 1;
				break;
			
			case 0x66: /*End of sound data*/
				//byteCount = ~0;
				endOfVgm = true;
				break;
			
			case 0x67: /*Data Block*/
				increment = (*((uint32_t*)&workVgmData[byteCount+3]));
				byteCount += increment + 0x07;
				increment = 0;
				break;
			case 0x94: // DAC CTRL
			case 0x4f: // SN PSG
			case 0x50: // SN PSG
				increment = 2;
				break;
			case 0x95: // DAC CTRL
				increment = 5;
				break;
			// waits and all others
			default:
			{
				uint8_t cmd = workVgmData[byteCount];
				increment = 
					((cmd>=0x70&&cmd<=0x8F) || cmd==0x62 || cmd==0x63) ? 1 : // single byte wait
					(cmd==0x4F || cmd==0x50)  ? 2 : // SN76489 PSG
					3;
				break;
			}
		}
		
		//printf("Test: %lu  vs  %lu         [%X %X %X]\n", 
		//(unsigned long)byteCount, (unsigned long)vgmSize,
		//workVgmData[byteCount], workVgmData[byteCount+1], workVgmData[byteCount+2]);
		
		byteCount += increment;
	}
	
	// On end, write memory contents to new vgm file
	fwrite(workVgmData, sizeof(uint8_t), vgmSize, outVgm);
	fclose(outVgm);
	
	return EXIT_SUCCESS;
}