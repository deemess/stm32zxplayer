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

int cursor=0;
int dirfiles = 0;
int dirpos=0;
FATFS fileSystem;
char dirpath[256];
char currentFile[256];
int currentFileIsDir=0;
int currentFileIsParentDir=0;

void drawFileMenu() {
	FRESULT res;
	DIR dir; //Directory object
	FILINFO fno; //File information
	int nfile, ndir;
	int line=0;
	char buff[260];
	int skip=dirpos;

	ssd1306_Fill(Black);
	ssd1306_DrawRectangle(0,0,127,63, White);

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
	dirfiles = nfile + ndir;
	ssd1306_UpdateScreen();
}


void cursor_down() {
	if(cursor+1 < dirfiles) {
		cursor++;
	} else {
		cursor = 0;
		dirpos = 0;
	}
	if(dirpos < cursor-5) {
		dirpos = cursor-5;
	}
	drawFileMenu();
}

void cursor_enter() {
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
		drawFileMenu();
	}
}


void app_init() {
	ssd1306_Init();
//  ssd1306_TestAll();
	ssd1306_Fill(Black);
	ssd1306_UpdateScreen();
	LEDON;
	/*FRESULT res = */ f_mount(&fileSystem, SDPath, 1);
	LEDOFF;

	strcpy(dirpath, "/");
	drawFileMenu();
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



