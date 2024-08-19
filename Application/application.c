/*
 * application.c
 *
 *  Created on: Aug 14, 2024
 *      Author: Dmitry
 */

#include "application.h"
#include "main.h"
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "ssd1306_fonts.h"
#include <stdio.h>
#include <string.h>
#include "fatfs.h"

extern TIM_HandleTypeDef htim1;

int cursor=0;
int dirfiles = 0;
int dirpos=0;
FATFS fileSystem;
char dirpath[256];
char currentFile[256];
int currentFileIsDir=0;
int currentFileIsParentDir=0;
int menu = 0; //0 - File menu, 1 - play menu, 2 - playing

char buff[260];

void drawFileMenu() {
	FRESULT res;
	DIR dir; //Directory object
	FILINFO fno; //File information
	int nfile, ndir;
	int line=0;

	int skip=dirpos;

	ssd1306_Fill(Black);
	ssd1306_DrawRectangle(0,0,127,63, White);

	LEDON;
	res =  f_mount(&fileSystem, SDPath, 1);
	LEDOFF;
	if(res != FR_OK) {
		ssd1306_SetCursor(3, 3);
		ssd1306_WriteString("DEVICE NOT RDY", Font_7x10, White);
		ssd1306_UpdateScreen();
		return;
	}


	int filepos=0;
	nfile = ndir = 0;
	LEDON;
	res = f_opendir(&dir, dirpath); //Open the directory
	LEDOFF;
	if (res == FR_OK) {
		//ROOT
		if(!(skip-- > 0)) {
			sprintf(buff, "<%s>", dirpath);
			ssd1306_SetCursor(3, (line++)*10+3);
			ssd1306_WriteString(buff, Font_7x10, White);
			//Draw cursor
			if(filepos == cursor) {
				ssd1306_InvertRectangle(1, (line-1)*10+2, 126, (line-1)*10+3+10);
				currentFileIsDir = 1;
				currentFileIsParentDir = 1;
				strcpy(currentFile, fno.fname);
			} else {
				currentFileIsParentDir = 0;
			}
		}
		ndir++;
		filepos++;
		//All files in directory
		for (;; filepos++) {
			LEDON;
			res = f_readdir(&dir, &fno); //Read a directory item
			LEDOFF;
			if (res != FR_OK || fno.fname[0] == 0)
				break; //Error or end of dir
			if (fno.fattrib & AM_DIR) { //Directory
				ndir++;
			} else { //File
				nfile++;
			}
			if(skip-- > 0) {
				continue;
			}
			if (fno.fattrib & AM_DIR) { //Directory
				if(filepos == cursor) {
					currentFileIsDir = 1;
					strcpy(currentFile, fno.fname);
				}
				if(line < 6) {
					sprintf(buff, "[%s]", fno.fname);
					ssd1306_SetCursor(3, (line++)*10+3);
					ssd1306_WriteString(buff, Font_7x10, White);
				}
			} else { //File
				if(filepos == cursor) {
					currentFileIsDir = 0;
					strcpy(currentFile, fno.fname);
				}
				if(line < 6) {
					if(fno.fsize<1024) {
						sprintf(buff, "%s %dB", fno.fname, (int)fno.fsize);
					} else {
						sprintf(buff, "%s %dK", fno.fname, (int)fno.fsize/1024);
					}
					ssd1306_SetCursor(3, (line++)*10+3);
					ssd1306_WriteString(buff, Font_7x10, White);
				}
			}
			//Draw cursor
			if(filepos == cursor) {
				ssd1306_InvertRectangle(1, (line-1)*10+2, 126, (line-1)*10+3+10);
			}
		}
		f_closedir(&dir);
	}

	//unmount
	f_mount(0, SDPath, 0);
	dirfiles = nfile + ndir;
	ssd1306_UpdateScreen();
}

int playMenuCursor=0;
int playMenuItems = 3;

void drawPlayMenu() {
	int line=0;
	ssd1306_Fill(Black);
	ssd1306_DrawRectangle(0,0,127,63, White);
	ssd1306_SetCursor(3, (line++)*10+3); //Draw File name
	ssd1306_WriteString(currentFile, Font_7x10, White);
	ssd1306_Line(0, 10+2, 127, 10+2, White);
	line++; //Skip line
	ssd1306_SetCursor(3, (line++)*10+3); //Draw menu
	ssd1306_WriteString("Play", Font_7x10, White);
	ssd1306_SetCursor(3, (line++)*10+3);
	ssd1306_WriteString("Delete", Font_7x10, White);
	ssd1306_SetCursor(3, (line++)*10+3);
	ssd1306_WriteString("Exit", Font_7x10, White);

	ssd1306_InvertRectangle(1, (playMenuCursor+2)*10+2, 126, (playMenuCursor+2)*10+3+10);
	ssd1306_UpdateScreen();
}



char filePullPath[256];
char fileBuffer[64*1024];
int blockSize;
int blockNumber;

void drawPlaying() {
	int line=0;
	ssd1306_Fill(Black);
	ssd1306_DrawRectangle(0,0,127,63, White);
	ssd1306_SetCursor(3, (line++)*10+3); //Draw File name
	ssd1306_WriteString(currentFile, Font_7x10, White);
	ssd1306_Line(0, 10+2, 127, 10+2, White);
	line++; //Skip line
	ssd1306_SetCursor(3, (line++)*10+3); //Draw menu
	if(blockSize == 0) {
		ssd1306_WriteString("Done", Font_7x10, White);
	} else {
		ssd1306_WriteString("Playing...", Font_7x10, White);
	}
	ssd1306_SetCursor(3, (line)*10+3);
	sprintf(buff, "Block: %d", blockNumber-1);
	ssd1306_WriteString(buff, Font_7x10, White);

	ssd1306_UpdateScreen();
}

void drawMenu() {
	switch(menu) {
	case 0:
		drawFileMenu();
		break;
	case 1:
		drawPlayMenu();
		break;
	case 2:
		drawPlaying();
		break;
	}
}

int readBlock(int blockN) { // 1 - block read, 0 - failed to read block / no block
	FIL fsrc;
	FRESULT res;

	unsigned int br;
	blockSize=0;

	LEDON;
	res =  f_mount(&fileSystem, SDPath, 1);
	LEDOFF;

	if(res != FR_OK) {
		menu = 1; //goto play menu
		//unmount
		f_mount(0, SDPath, 0);
		return 0;
	}

	strcpy(filePullPath, dirpath);
	strcat(filePullPath, "/");
	strcat(filePullPath, currentFile);

	LEDON;
	res = f_open(&fsrc, filePullPath, FA_READ);
	LEDOFF;
	if(res != FR_OK) {
		//unmount
		f_mount(0, SDPath, 0);
		return 0;
	}

	for(int i=0; i<blockN; i++) { //read block
		blockSize = 0;
		LEDON;
		res = f_read(&fsrc, fileBuffer, 2, &br);
		LEDOFF;
		if(res != FR_OK || br == 0) {
			f_close(&fsrc);
			//unmount
			f_mount(0, SDPath, 0);
			return 0;
		}

		blockSize = ((unsigned short*)fileBuffer)[0];
		if(blockSize >= sizeof fileBuffer) {
			f_close(&fsrc);
			//unmount
			f_mount(0, SDPath, 0);
			return 0;
		}

		res = f_read(&fsrc, fileBuffer, blockSize, &br);
		if(res != FR_OK || br == 0) {
			f_close(&fsrc);
			//unmount
			f_mount(0, SDPath, 0);
			return 0;
		}
	}

	f_close(&fsrc);
	//unmount
	f_mount(0, SDPath, 0);

	return 1;
}

int pilotPulses;
int pausePulses;
int blockPos;
int currentPulseType; //0 - pilot, 1 - 1 pulse, 2 - 0 pulse, 3 - start pulse, 4 - end pulse, 5 - pause
int bitn;
int l=0;

void playNextPulse() {
	l=0;

	if(currentPulseType == 0 && pilotPulses-- > 0) { //pilot pulse
		OUT_HI; //pulse begins from HI
		__HAL_TIM_SET_AUTORELOAD(&htim1, 2168-1);
		return;
	} else {
		if(currentPulseType == 0) { //start pulse
			OUT_HI; //pulse begins from HI
			currentPulseType = 3;
			bitn=7;
			__HAL_TIM_SET_AUTORELOAD(&htim1, 735-1);
			return;
		}
		if(currentPulseType == 4) { //after end pulse make pause
			OUT_LOW;
			currentPulseType = 5;
			pausePulses=4000;
			__HAL_TIM_SET_AUTORELOAD(&htim1, 667-1);
			return;
		}
		if(currentPulseType == 5 && pausePulses-- > 0) { //pause pulses
			OUT_LOW;
			__HAL_TIM_SET_AUTORELOAD(&htim1, 1084-1);
			return;
		}
		if(currentPulseType == 5) { //Stop playing
			OUT_LOW;
			if(readBlock(blockNumber++)) { //Play next block
				pilotPulses = 4000;
				blockPos= 0;
				currentPulseType = 0; //pilot
				playNextPulse();
			} else {
				//stay in current menu
				HAL_TIM_Base_Stop_IT(&htim1);
			}
			drawMenu();
			return;
		}

		//Bit data pulses
		OUT_HI; //pulse begins from HI
		if(bitn < 0) {
			bitn = 7;
			blockPos++;
			if(blockPos >= blockSize) { // stop pulse
				currentPulseType = 4;
				__HAL_TIM_SET_AUTORELOAD(&htim1, 1710-1);
				return;
			}
		}
		int bit = BitVal(fileBuffer[blockPos], bitn--);
		if(bit == 1) { // 1 pulse
			currentPulseType = 1;
			__HAL_TIM_SET_AUTORELOAD(&htim1, 1710-1);
		} else { // 0 pulse
			currentPulseType = 2;
			__HAL_TIM_SET_AUTORELOAD(&htim1, 855-1);
		}

	}
}

void app_tim1_update() {
	l++;
	if(l == 1) {
		OUT_LOW;
	} else if(l == 2) {
		playNextPulse();
	}
}

void playBlock() {
	pilotPulses = 4000;
	blockPos= 0;
	currentPulseType = 0; //pilot

	playNextPulse();
	__HAL_TIM_CLEAR_FLAG(&htim1, TIM_SR_UIF); // очищаем флаг чтобы не генерировалось сразу прерывание
	HAL_TIM_Base_Start_IT(&htim1);
//	HAL_TIM_Base_Stop_IT(&htim1);
}

void startPlaying() {
	blockNumber=1;
	if(readBlock(blockNumber++) == 0) {
		menu = 1;
	} else {
		playBlock();
	}
}

void stopPlaying() {
	HAL_TIM_Base_Stop_IT(&htim1);
	OUT_LOW;
}

void cursor_down() {
	if(menu == 0) {
		if(cursor+1 < dirfiles) {
			cursor++;
		} else {
			cursor = 0;
			dirpos = 0;
		}
		if(dirpos < cursor-5) {
			dirpos = cursor-5;
		}
	} else if(menu == 1) {
		if(playMenuCursor+1 < playMenuItems) {
			playMenuCursor++;
		} else {
			playMenuCursor = 0;
		}
	} else if(menu == 2) {
		menu=1;
		stopPlaying();
	}

	drawMenu();
}

void cursor_enter() {
	if(menu == 0) {
		if(currentFileIsDir == 1) {
			if(currentFileIsParentDir == 1) { //Go up
				for(int i=strlen(dirpath)-1; i>0; i--) {
					if(dirpath[i] == '/') {
						dirpath[i] = 0;
						break;
					}
				}

			} else { //Go down to sub folder
				strcat(dirpath, "/");
				strcat(dirpath, currentFile);
			}
			cursor = 0;
			dirpos = 0;
		} else {
			menu = 1;
			playMenuCursor = 0;
		}
	} else if(menu == 1) { //Play menu
		if(playMenuCursor == 0) { //Play
			menu = 2;
			startPlaying();
		} else if(playMenuCursor == 2) { //Exit
			menu = 0;
		}
	}

	drawMenu();
}


void app_init() {
	ssd1306_Init();
//  ssd1306_TestAll();
	ssd1306_Fill(Black);
	ssd1306_UpdateScreen();

	strcpy(dirpath, "/");
	drawFileMenu();

	//HAL_TIM_OC_Start_IT(&htim1, TIM_CHANNEL_1);
//	__HAL_TIM_CLEAR_FLAG(&htim1, TIM_SR_UIF); // очищаем флаг чтобы не генерировалось сразу прерывание
//	HAL_TIM_Base_Start_IT(&htim1);
//	HAL_TIM_Base_Stop_IT(&htim1);

}

int keypressed = 0;
int presstime = 0;

void app_loop() {
	HAL_Delay(100);
	if(KEY_UP && keypressed == 1) {
		if(presstime > 7) {
			cursor_enter();
		} else {
			cursor_down();
		}
		keypressed = 0;
		presstime = 0;
	}
	if(KEY_DOWN && keypressed == 0) {
		keypressed = 1;
	}
	if(KEY_DOWN && keypressed == 1) {
		presstime++;
	}
}



