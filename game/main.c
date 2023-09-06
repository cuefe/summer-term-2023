#include "../inc/STC15F2K60S2.H"        //必须。
#include "../inc/sys.H"                 //必须。
#include "../inc/displayer.H"
#include "../inc/key.H"
#include "../inc/adc.h"
#include "../inc/M24C02.h"
#include "../inc/beep.H"

code unsigned long SysClock=11059200;         //必须。定义系统工作时钟频率(Hz)，用户必须修改成与实际工作频率（下载时选择的）一致
#ifdef _displayer_H_                          //显示模块选用时必须。（数码管显示译码表，用戶可修改、增加等） 
code char decode_table[]= {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x00,0x08,0x40,0x01, 0x41, 0x48,
                           /* 序号:   0   1    2	   3    4	    5    6	  7   8	   9	 10	   11		12   13    14     15     */
                           /* 显示:   0   1    2    3    4     5    6    7   8    9  (无)   下-  中-  上-  上中-  中下-   */
                           0x3f|0x80,0x06|0x80,0x5b|0x80,0x4f|0x80,0x66|0x80,0x6d|0x80,0x7d|0x80,0x07|0x80,0x7f|0x80,0x6f|0x80,0x71,0x77,0x06,0x38,0x79,0x3f
                          };
/* 带小数点     0         1         2         3         4         5         6         7         8         9        */
#endif

#define uint unsigned int
#define uchar unsigned char
#define BLOCKNUM 20

uint rand = 0;
uint score = 0;
uint maxscore = 0;
uint sttime = 0;
uint edtime = 0;
uint totaltime = 0;
uint mi = 0;
uchar mode = 0;
uchar t = 0;
uchar st = 0;
uchar ed = 0;
uchar query1 = 0;
uchar query2 = 0;
uchar block = BLOCKNUM;
uchar NVM_addr1 = 0x00;
uchar NVM_addr2 = 0x01;
uchar NVM_addr3 = 0x02;
uchar NVM_addr4 = 0x03;
uchar NVM_data1 = 0;
uchar NVM_data2 = 0;
uchar NVM_data3 = 0;
uchar NVM_data4 = 0;

char c[8] = {12,13,11,12,13,11,12,13};

/*
低8度 131 147 165 175 196 220 247
中8度 262 294 330 349 392 440 494
高8度 523 587 659 698 784 880 988
*/

code uint xxx[28] = {262, 262, 392, 392, 440, 440, 392,
                     349, 349, 330, 330, 293, 294, 262,
                     392, 392, 349, 349, 330, 330, 294,
                     392, 392, 349, 349, 330, 330, 294
                    };
code uint lzlh[32] = {262, 294, 330, 262, 262, 294, 330, 262,
                      330, 349, 392, 330, 349, 392,
                      392, 440, 392, 349, 330, 262, 392, 440, 392, 349, 330, 262,
                      262, 196, 262, 262, 196, 262
                     };

uint *music = xxx;
uchar musicid = 0;
uchar musicsize = 28;

void Delay10ms() {		//@11.0592MHz
	unsigned char i, j;
	i = 108;
	j = 145;
	do {
		while (--j);
	} while (--i);
}

void Move() {
	int i;
	SetBeep(music[mi], 20);
	mi = (mi + 1) % musicsize;
	t = rand % 3 + 11;
	for(i = 7; i > 0; i--) {
		c[i] = c[i - 1];
	}
	c[0] = t;
	Seg7Print(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
}

void UpdateMaxScore() {
	NVM_data1 = M24C02_Read(NVM_addr1);
	NVM_data2 = M24C02_Read(NVM_addr2);
	if(score > (NVM_data2 << 8) + NVM_data1) {
		M24C02_Write(NVM_addr1, score % 256);
		Delay10ms();
		M24C02_Write(NVM_addr2, score / 256);
	}
}

void ReadMaxScore() {
	NVM_data1 = M24C02_Read(NVM_addr1);
	NVM_data2 = M24C02_Read(NVM_addr2);
	maxscore = (NVM_data2 << 8) + NVM_data1;
	Seg7Print(10, 10, 10, 10, maxscore / 1000 % 10, maxscore / 100 % 10, maxscore / 10 % 10, maxscore % 10);
}

void UpdateMinTime() {
	NVM_data3 = M24C02_Read(NVM_addr3);
	NVM_data4 = M24C02_Read(NVM_addr4);
	if(totaltime < NVM_data3 * 1000 + NVM_data4 * 10) {
		M24C02_Write(NVM_addr3, totaltime / 1000);
		Delay10ms();
		M24C02_Write(NVM_addr4, totaltime % 1000 / 10);
	}
}

void ReadMinTime() {
	NVM_data3 = M24C02_Read(NVM_addr3);
	NVM_data4 = M24C02_Read(NVM_addr4);
	Seg7Print(10, 10, 10, 10, NVM_data3 / 10, NVM_data3 % 10 + 16, NVM_data4 / 10, NVM_data4 % 10);
}
void mykey_callback() {
	if(st == 0) {
		if(GetKeyAct(enumKey1) == enumKeyPress) {
			M24C02_Write(NVM_addr1, 0);
			Delay10ms();
			M24C02_Write(NVM_addr2, 0);
		}
		if(GetKeyAct(enumKey2) == enumKeyPress) {
			M24C02_Write(NVM_addr3, 99);
			Delay10ms();
			M24C02_Write(NVM_addr4, 99);
		}
	}
	if(st == 1 && ed == 0 && mode == 0) {
		if(GetKeyAct(enumKey1) == enumKeyPress) {
			if(c[7] == 13) {
				Move();
				score++;
			} else {
				Seg7Print(10, 10, 10, 10, score / 1000 % 10, score / 100 % 10, score / 10 % 10, score % 10);
				UpdateMaxScore();
				ed = 1;
			}
		} else if(GetKeyAct(enumKey2) == enumKeyPress) {
			if(c[7] == 12) {
				Move();
				score++;
			} else {
				Seg7Print(10, 10, 10, 10, score / 1000 % 10, score / 100 % 10, score / 10 % 10, score % 10);
				UpdateMaxScore();
				ed = 1;
			}
		}
	}
	if(st == 1 && ed == 0 && mode == 1) {
		if(GetKeyAct(enumKey1) == enumKeyPress && block > 0) {
			if(c[7] == 13) {
				Move();
				block--;
			} else {
				Seg7Print(10, 10, 26, 27, 28, 29, 30, 31);
				ed = 1;
				block = BLOCKNUM;
			}
		}
		if(GetKeyAct(enumKey2) == enumKeyPress && block > 0) {
			if(c[7] == 12) {
				Move();
				block--;
			} else {
				Seg7Print(10, 10, 26, 27, 28, 29, 30, 31);
				ed = 1;
				block = BLOCKNUM;
			}
		}
	}
}

void mynav_callback() {
	if(st == 0) {
		if(GetAdcNavAct(enumAdcNavKeyUp) == enumKeyPress) {
			query1 = 0;
			query2 = 0;
			if(mode == 0) mode = 1;
		} else if(GetAdcNavAct(enumAdcNavKeyDown) == enumKeyPress) {
			query1 = 0;
			query2 = 0;
			if(mode == 1) mode = 0;
		} else if(GetAdcNavAct(enumAdcNavKeyLeft) == enumKeyPress) {
			query1 = 1;
			query2 = 0;
			ReadMaxScore();
		} else if(GetAdcNavAct(enumAdcNavKeyRight) == enumKeyPress) {
			query1 = 0;
			query2 = 1;
			ReadMinTime();
		} else if(GetAdcNavAct(enumAdcNavKeyCenter) == enumKeyPress) {
			st = 1;
			sttime = rand;
			Seg7Print(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
		} else if(GetAdcNavAct(enumAdcNavKey3) == enumKeyPress) {
			musicid = (musicid + 1) % 2;
			if(musicid == 0) {
				music = xxx;
				musicsize = 28;
			} else if(musicid == 1) {
				music = lzlh;
				musicsize = 32;
			}
		}
	}
	if(st == 1 && ed == 0 && mode == 0) {
		if(GetAdcNavAct(enumAdcNavKey3) == enumKeyPress) {
			if(c[7] == 11) {
				Move();
				score++;
			} else {
				Seg7Print(10, 10, 10, 10, score / 1000 % 10, score / 100 % 10, score / 10 % 10, score % 10);
				UpdateMaxScore();
				ed = 1;
			}
		}
	}
	if(st == 1 && ed == 0 && mode == 1) {
		if(GetAdcNavAct(enumAdcNavKey3) == enumKeyPress) {
			if(c[7] == 11) {
				Move();
				block--;
			} else {
				Seg7Print(10, 10, 26, 27, 28, 29, 30, 31);
				ed = 1;
				block = BLOCKNUM;
			}
		}
	}
	if(ed == 1) {
		if(GetAdcNavAct(enumAdcNavKeyCenter) == enumKeyPress) {
			score = 0;
			st = 0;
			ed = 0;
			query1 = 0;
			query2 = 0;
			mi = 0;
		}
	}
}

void my10mS_callback() {
	rand++;
	if(!st && !query1 && !query2) {
		Seg7Print(10, 10, 10, 10, 10, 10, musicid, mode);
	}
	if(block == 0) {
		ed = 1;
		query2 = 1;
		edtime = rand;
		totaltime = (edtime - sttime) * 10;
		Seg7Print(10, 10, 10, 10, totaltime / 10000 % 10, totaltime / 1000 % 10 + 16, totaltime / 100 % 10, totaltime / 10 % 10);
		UpdateMinTime();
		block = BLOCKNUM;
	}
}

void main() {
	DisplayerInit();
	KeyInit();
	AdcInit(ADCincEXT);
	BeepInit();

	SetDisplayerArea(0, 7);
	LedPrint(0);

	SetEventCallBack(enumEventNav, mynav_callback);
	SetEventCallBack(enumEventKey, mykey_callback);
	SetEventCallBack(enumEventSys10mS, my10mS_callback);

	MySTC_Init();
	while(1) {
		MySTC_OS();
	}
}