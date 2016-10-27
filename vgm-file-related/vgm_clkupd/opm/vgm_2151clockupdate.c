/*
 * 2015 James Alan Nguyen
 * http://www.codingchords.com
 *
 * VGM F-num + clock converter (for OPM)
 *  (this code version is for YM2151 target only)
 *
 * Basically converts all necessary key code/key fraction commands such that
 * they are correctly in tune according to the new clock speed.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <tgmath.h>
#include <string.h>

/*typedef struct fnum_work_t
{
	long double fnote;  // fundamental frequency
	uint8_t B;     // 3-bit octave block,  FNum MSB D5-D3.  upper 3 bits of FNum2 
	uint16_t FNum; // 11-bit, where D10-D8 are lower 3 bits of FNum2
	               // and D7-D0 are FNum1 in its entirety.
} fnum_work_t;
*/
	
static uint8_t *workVgmData;
static uint64_t vgmSize, oldMasterClock, newMasterClock, byteCount;

static int16_t 
	lastKCode[8] = {0,0,0,0,0,0,0,0},
	lastKFrac[8] = {0,0,0,0,0,0,0,0};

void readKeyData(uint8_t * reg, uint8_t ch)
{
	// KeyCode/KeyFrac commands:  
	//   KC  = D6-D4 (octave block),  D3-D0 (Note)
	//   KF  = D7-D2 (1.6cent tune)
	//		
	//		0x54 (YM2151)
	//				 code  frac
	//		ch 0 - 0x28, 0x30
	//		ch 1 - 0x29, 0x31
	//		ch 2 - 0x2A, 0x32
	//		ch 3 - 0x2B, 0x33
	//		ch 4 - 0x2C, 0x34
	//		ch 5 - 0x2D, 0x35
	//		ch 6 - 0x2E, 0x36
	//		ch 7 - 0x2F, 0x37
	
	//uint8_t *regLoc = &workVgmData[byteCount];
	int16_t kcode, kfrac, koct, knote;
	//int64_t clkfactorCnt = 0;
	uint64_t byteCountOffset = 3, waitCnt = 0;
	long double kworking/*, kwork_upperbound, kwork_lowerbound, 
					kwork_ubchk, kwork_lbchk*/;

	// Construct constant from two components in the order of
	//   note, cents
	
	// lookahead for register pair in same "sample" to account for note calculation at
	// the respective time slice
	//printf("DBG: *reg == %X\n", *reg);
	
	if ((*reg) >= 0x28 && (*reg) <= 0x2F)
	{
		bool breakme = false;
		uint64_t increment = 3, 
				  cmd = 0x00, kStartReg = (*reg);
		
		//printf("DBG: *reg == %X\n", *reg);
		
		while ((byteCount+byteCountOffset > vgmSize) && !breakme)
		{
			cmd = workVgmData[byteCount+byteCountOffset+1];
			
			if (workVgmData[byteCount+byteCountOffset] == 0x54) switch (cmd)
			{
				case 0x30: // ch0
				case 0x31: // ch1
				case 0x32: // ch2
				case 0x33: // ch3
				case 0x34: // ch4
				case 0x35: // ch5
				case 0x36: // ch6
				case 0x37: // ch7
					breakme = 
						((kStartReg==0x28 && cmd==0x30) || // ch1
						 (kStartReg==0x29 && cmd==0x31) || // ch2
						 (kStartReg==0x2A && cmd==0x32) || // ch3
						 (kStartReg==0x2B && cmd==0x33) || // ch4
						 (kStartReg==0x2C && cmd==0x34) || // ch5
						 (kStartReg==0x2D && cmd==0x35) || // ch6
						 (kStartReg==0x2E && cmd==0x36) || // ch7
						 (kStartReg==0x2F && cmd==0x37));  // ch8
						
					if (breakme)
					{
						//printf("DEBUG: break with KCode first %x\n", kStartReg);
						lastKFrac[ch] = workVgmData[byteCount+byteCountOffset+2];
					}
					break;
			} else switch (workVgmData[byteCount+byteCountOffset])
			{
				case 0x61: // wait
				case 0x66:
				case 0x62:
				case 0x63:
					increment = 3;
					breakme = (++waitCnt>=2);
					break;
				case 0x70:
				case 0x71:
				case 0x72:
				case 0x73:
				case 0x74:
				case 0x75:
				case 0x76:
				case 0x77:
				case 0x78:
				case 0x79:
				case 0x7A:
				case 0x7B:
				case 0x7C:
				case 0x7D:
				case 0x7E:
				case 0x7F:
				case 0x80: // YM2612 addr write-wait
				case 0x81:
				case 0x82:
				case 0x83:
				case 0x84:
				case 0x85:
				case 0x86:
				case 0x87:
				case 0x88:
				case 0x89:
				case 0x8A:
				case 0x8B:
				case 0x8C:
				case 0x8D:
				case 0x8E:
				case 0x8F:
					increment = 1;
					breakme = (++waitCnt>=2);
					break;
				case 0x94: // DAC CTRL
				case 0x4f: // SN PSG
				case 0x50: // SN PSG
					increment = 2;
					break;
				case 0x95: // DAC CTRL
					increment = 5;
					break;
				case 0x67: //Data Block
					increment = (*((uint32_t*)&workVgmData[byteCount+byteCountOffset+3]));
					byteCountOffset += increment + 0x07;
					increment = 0;
					break;
				default:
					increment = 3;
					break;
			}
			
			byteCountOffset += increment;
		}
	} else if ((*reg) >= 0x30 && (*reg) <= 0x37)
	{
		bool breakme = false;
		uint64_t increment = 3, cmd = 0x00, kStartReg = (*reg);
		
		//printf("DBG: *reg == %X\n", *reg);
		
		while ((byteCount+byteCountOffset < vgmSize) && !breakme)
		{
			if (workVgmData[byteCount+byteCountOffset] == 0x54) switch (workVgmData[byteCount+byteCountOffset+1])
			{
				case 0x28: // ch0
				case 0x29: // ch1
				case 0x2A: // ch2
				case 0x2B: // ch3
				case 0x2C: // ch4
				case 0x2D: // ch5
				case 0x2E: // ch6
				case 0x2F: // ch7
					breakme = 
						((kStartReg==0x30 && cmd==0x28) || // ch1
						 (kStartReg==0x31 && cmd==0x29) || // ch2
						 (kStartReg==0x32 && cmd==0x2A) || // ch3
						 (kStartReg==0x33 && cmd==0x2B) || // ch4
						 (kStartReg==0x34 && cmd==0x2C) || // ch5
						 (kStartReg==0x35 && cmd==0x2D) || // ch6
						 (kStartReg==0x36 && cmd==0x2E) || // ch7
						 (kStartReg==0x37 && cmd==0x2F));  // ch8
						 
					if (breakme)
					{
						//printf("DEBUG: break with KFrac first %X\n", kStartReg);
						lastKCode[ch] = workVgmData[byteCount+byteCountOffset+2];
					}
					break;
			} else switch (workVgmData[byteCount+byteCountOffset])
			{
				case 0x61:
				case 0x66:
				case 0x62:
				case 0x63: 
					increment = 3;
					breakme = (++waitCnt>=2);
					break;
				case 0x70:
				case 0x71:
				case 0x72:
				case 0x73:
				case 0x74:
				case 0x75:
				case 0x76:
				case 0x77:
				case 0x78:
				case 0x79:
				case 0x7A:
				case 0x7B:
				case 0x7C:
				case 0x7D:
				case 0x7E:
				case 0x7F:
				case 0x80: // YM2612 addr write-wait
				case 0x81:
				case 0x82:
				case 0x83:
				case 0x84:
				case 0x85:
				case 0x86:
				case 0x87:
				case 0x88:
				case 0x89:
				case 0x8A:
				case 0x8B:
				case 0x8C:
				case 0x8D:
				case 0x8E:
				case 0x8F:
					increment = 1;
					breakme = (++waitCnt>=2);
					break;
				case 0x94: // DAC CTRL
				case 0x4f: // SN PSG
				case 0x50: // SN PSG
					increment = 2;
					break;
				case 0x95: // DAC CTRL
					increment = 5;
					break;
				case 0x67: //Data Block
					increment = (*((uint32_t*)&workVgmData[byteCount+byteCountOffset+3]));
					byteCountOffset += increment + 0x07;
					increment = 0;
					break;
			}
			byteCountOffset += increment;
		}
	}
	
	kcode = lastKCode[ch];
	kfrac = lastKFrac[ch];
	
	//printf("DEBUG=kcode:%X, kfrac:%x\n", kcode, kfrac);
	
	koct = (kcode>>4);
	knote = (kcode&0xF);
	
	kworking = log2l((long double)newMasterClock/oldMasterClock)*1200;
	
	// Rounding needed due to imprecision on kfrac register
	//     method 1  - nearest 1.5625 cents - improved but doesn't work well for some clock ratios
	/*
	kwork_lowerbound = (100.0/64.0) * floorl((long double)(64.0/100.0) * kworking);
	kwork_upperbound = (100.0/64.0) * floorl((long double)(64.0/100.0) * kworking + (100.0/64.0));
	
	kwork_lbchk = fabsl(kworking) / fabsl(kwork_lowerbound);
	kwork_ubchk = fabsl(kwork_upperbound) / fabsl(kworking);
	
	kworking = (kwork_lbchk > kwork_ubchk) ? kwork_lowerbound : kwork_upperbound;
	//printf("DEBUG: lower=%lf, upper=%lf, %s\n", kwork_lbchk, kwork_ubchk, (kwork_lbchk<kwork_ubchk)?"lower":"upper");
	*/
	
	//    method 2  - nearest next clock ratio with respect to log_2((12+n)/12) (or -n) (not working correctly)
	/*
	do 
	{
		//clkfactorCnt = (newMasterClock >= oldMasterClock) ? 1 : -1;
		
		if (newMasterClock >= oldMasterClock)
		{
			kwork_upperbound = log2l(((long double)oldMasterClock * (12.0*64.0 + (++clkfactorCnt))/(12.0*64.0))/oldMasterClock)*1200;
			kwork_lowerbound = log2l(((long double)oldMasterClock * (12.0*64.0 + (clkfactorCnt - 1))/(12.0*64.0))/oldMasterClock)*1200;
		} else
		{
			kwork_lowerbound = log2l(((long double)oldMasterClock * (12.0*64.0 + (--clkfactorCnt))/(12.0*64.0))/oldMasterClock)*1200;
			kwork_upperbound = log2l(((long double)oldMasterClock * (12.0*64.0 + (clkfactorCnt + 1))/(12.0*64.0))/oldMasterClock)*1200;
		}
		
		//printf("DEBUG: lb=%lf, kw=%lf, ub=%lf\n", kwork_lowerbound, kworking, kwork_upperbound);
	} while ((newMasterClock >= oldMasterClock) ? kwork_upperbound < kworking : kwork_lowerbound >= kworking);
	
	printf("DEBUG: lb=%lf, kw=%lf, ub=%lf\n", kwork_lowerbound, kworking, kwork_upperbound);
	
	kwork_lbchk = fabsl(kworking) / fabsl(kwork_lowerbound);
	kwork_ubchk = fabsl(kwork_upperbound) / fabsl(kworking);
	
	//kworking = (kwork_lbchk > kwork_ubchk) ? kwork_lowerbound : kwork_upperbound;
	kworking = kwork_lowerbound;
	
	printf("DEBUG: lower=%lf, upper=%lf, %s\n", kwork_lbchk, kwork_ubchk, (kwork_lbchk<kwork_ubchk)?"lower":"upper");
	*/
	//printf("DEBUG=koct:%X, knote:%x, kworking:%f\n", koct, knote, kworking);
	
	// set octave
	//koct = koct - log2(kworking);
	
	koct -= (int16_t)(kworking/1200);
	
	if (kworking >= 1200)
	{
		while (kworking >= 1200) kworking -= 1200;
	} else if (kworking <= -1200)
	{
		while (kworking <= -1200) kworking += 1200;
	}
	
	// pitch range clip to avoid wrapping
	
	if (koct > 0x7)
	{
		koct = 0x7;
		knote = 0xF;
		kfrac = 0xFC;
	} else if (koct < 0)
	{
		koct = 0;
		knote = 0;
		kfrac = 0;
	} else
	{
		//Key Code (Note+Oct) component
		/*This is what the spec suggests if using standard clock
			0000 C#
			0001 D
			0010 D#/Eb
			0011 
			0100 E
			0101 F
			0110 F#/Gb
			0111 
			1000 G
			1001 G#/Ab
			1010 A - 440
			1011 
			1100 A#/Bb
			1101 B
			1110 C
			1111
			*/
		
		//printf("DEBUG: koct=%X knote=%X, kfrac=%X, kworking=%f\n", koct, knote, kfrac, kworking);
		
		if (kworking < 0)
		{
			while (kworking <= -100)
			{
				kworking += 100;
				
				knote += (((knote+1)&3)==3)? 2 : 1;
				
				if (knote > 0xE)
				{
					++koct;
					knote = 0x0;
					
					if (koct > 0x7)
					{
						koct = 0x7;
						knote = 0xF;
						kfrac = 0xFC;
						break;
					}
				}
			}
		
			while (kworking <= (-50.0/64.0))
			{
				kworking += (100.0/64.0); //(5.0/3.0);  //or 1.6?
				
				kfrac+=4;
				
				if (kfrac > 0xFC)
				{
					kfrac = 0;
					//kfrac -= 0x100;
					
					knote += (((knote+1)&3)==3)? 2 : 1;
					
					if (knote > 0xE)
					{
						++koct;
						knote = 0x0;
						
						if (koct > 0x7)
						{
							koct = 0x7;
							knote = 0xF;
							kfrac = 0xFC;
							break;
						}
					}
				}
			}
			
		} else if (kworking > 0)
		{
			while (kworking >= 100)
			{
				kworking -= 100;
				knote -= (((knote-1)&3)==3)? 2 : 1;
			
				if (knote < 0x0)
				{
					--koct;
					knote = 0xE;
					
					if (koct < 0)
					{
						koct = 0;
						knote = 0;
						kfrac = 0;
						break;
					}
				}
			}
		
			while (kworking >= (50.0/64.0))
			{
				kworking -= (100.0/64.0); //(5.0/3.0);  //or 1.6?
				
				kfrac-=4;
			
				if (kfrac < 0x0)
				{
					knote -= (((knote-1)&3)==3)? 2 : 1;
					kfrac = 0xFC;
					//kfrac += 0x100;
					
					if (knote < 0x0)
					{
						--koct;
						knote = 0xE;
						
						if (koct < 0x0)
						{
							koct = 0;
							knote = 0;
							kfrac = 0;
							break;
						}
					}
				}
			}
		}
	}

	kcode = (koct<<4)|knote;
	
	// Write KCode and KFrac values at respective locations
	//printf("DEBUG=kcode:%X, kfrac:%x\n", kcode, kfrac);
	
	//printf("DEBUG=koct:%X, knote:%x, kworking:%f\n", koct, knote, kworking);
	
	switch(*reg)
	{
		case 0x28: // ch0
		case 0x29: // ch1
		case 0x2A: // ch2
		case 0x2B: // ch3
		case 0x2C: // ch4
		case 0x2D: // ch5
		case 0x2E: // ch6
		case 0x2F: // ch7
		{
			//(*(regLoc+1)) = kcode;
			workVgmData[byteCount+2] = kcode;
			break;
		}
		
		case 0x30: // ch0
		case 0x31: // ch1
		case 0x32: // ch2
		case 0x33: // ch3
		case 0x34: // ch4
		case 0x35: // ch5
		case 0x36: // ch6
		case 0x37: // ch7
		{
			//(*(regLoc+1)) = kfrac;
			workVgmData[byteCount+2] = kfrac;
			break;
		}
	}
	
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
	//   YM2203 - 0x44
	//   YM2151 - 0x30
	// NOTE:  THIS WILL _NOT_ CHANGE SSG!
	/*if (argc>=5)
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
	
	else */// default to OPM
	//{
	oldMasterClock = (*(uint32_t*)&(workVgmData[0x30]));
	newMasterClock = strtoul(argv[2], NULL, 0);
	(*(uint32_t*)&(workVgmData[0x30])) = (uint32_t)newMasterClock;
	//}
	
	
	// scan-and-write vgm until encounter F-Num prepare register
	// note: chip command for ym2612 = 0x52 0xnn 0xnn
	byteCount = 0x34+workVgmData[0x34];
	
	while (byteCount<vgmSize && !endOfVgm)
	{
		uint64_t increment = 3;
		
		switch(workVgmData[byteCount])
		{
		/*
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
		*/
			case 0x54: // YM2151
			{
				switch (workVgmData[byteCount+1])
				{
					case 0x28: // ch0
					case 0x29: // ch1
					case 0x2A: // ch2
					case 0x2B: // ch3
					case 0x2C: // ch4
					case 0x2D: // ch5
					case 0x2E: // ch6
					case 0x2F: // ch7
					{
						uint8_t ch = workVgmData[byteCount+1] - 0x28;
						lastKCode[ch] = workVgmData[byteCount+2];
						//printf("DEBUG: %X %X %X\n", workVgmData[byteCount], workVgmData[byteCount+1], workVgmData[byteCount+2]);
						readKeyData(&workVgmData[byteCount+1], ch);
						break;
					}
					
					case 0x30: // ch0
					case 0x31: // ch1
					case 0x32: // ch2
					case 0x33: // ch3
					case 0x34: // ch4
					case 0x35: // ch5
					case 0x36: // ch6
					case 0x37: // ch7
					{
						uint8_t ch = workVgmData[byteCount+1] - 0x30;
						lastKFrac[ch] = workVgmData[byteCount+2];
						//printf("DEBUG: %X %X %X\n", workVgmData[byteCount], workVgmData[byteCount+1], workVgmData[byteCount+2]);
						readKeyData(&workVgmData[byteCount+1], ch);
						break;
					}
				}
				increment = 3;
				break;
			}
		
			case 0x61:
				increment = 3;
				break;
				
			case 0x62:
			case 0x63: /* single byte wait*/
			case 0x70:
			case 0x71:
			case 0x72:
			case 0x73:
			case 0x74:
			case 0x75:
			case 0x76:
			case 0x77:
			case 0x78:
			case 0x79:
			case 0x7A:
			case 0x7B:
			case 0x7C:
			case 0x7D:
			case 0x7E:
			case 0x7F:
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
				//uint8_t cmd = workVgmData[byteCount];
				increment = 3;
				break;
			}
		}

//		printf("DEBUG-ADDR: 0x%X, %d, endofvgm=%d\n", byteCount, increment, endOfVgm);
		
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