#include "myLib.h"

u16 *videoBuffer = (u16*)0x6000000;

extern const unsigned char fontdata_6x8[12288];


void setPixel(int r, int c, u16 color) 
{ 
	videoBuffer[OFFSET(r,c,240)] = color;
}

void drawRect(int row, int col, int width, int height, volatile u16 color)
{
	int r;
	for(r=0; r<height; r++)
	{
		DMA[3].src = &color;
		DMA[3].dst = videoBuffer + OFFSET(row+r, col, 240);
		DMA[3].cnt = DMA_ON | DMA_SOURCE_FIXED | width;
	}
}

void drawHollowRect(int r, int c, int width, int height, u16 color) 
{ 
	for(int row = r; row<r+height; row++)
	{
		setPixel(row, c, color);
		setPixel(row, c+width-1, color);
	}
	for(int col = c+1; col<c+width; col++)
	{
		setPixel(r,col, color);
		setPixel(r+height-1, col, color);
	}
}

void drawChar(int row, int col, char ch, unsigned short color)
{
	int r,c;
	for(r=0; r<8; r++)
	{
		for(c=0; c<6; c++)
		{
			if(fontdata_6x8[OFFSET(r, c, 6)+ch*48])
			{
				setPixel(row+r, col+c, color);
			}
		}
	}
}

void drawString(int row, int col, char *str, unsigned short color)
{
	while(*str)
	{
		drawChar(row, col, *str++, color);
		col += 6;	
	}
}


void waitForVblank()
{
	while(SCANLINECOUNTER > 160);
	while(SCANLINECOUNTER < 160);
}

void setDMA(int channel, void *source, void *destination, u32 control)
{
	DMA[channel].src = source;
	DMA[channel].dst = destination;
	DMA[channel].cnt = DMA_ON | control;
}

void drawImage3(int row, int col, int width, int height, const u16 *image) 
{ 
	int r;
	for(r=0; r<height; r++)
	{
 	DMA[3].src = image + OFFSET(r,0,width);
	DMA[3].dst = videoBuffer + OFFSET(row+r,col,240);
	DMA[3].cnt = DMA_ON | width;
	}
}

void drawSubBackground3(int row, int col, int width, int height, const u16 *image)
{
	int r;
	for(r=row; r<row+height; r++)
	{
 	DMA[3].src = image + OFFSET(r,col,240);
	DMA[3].dst = videoBuffer + OFFSET(r,col,240);
	DMA[3].cnt = DMA_ON | width;
	}	
}

void drawSubImage3(int row, int col, int width, int height, const u16 *image)
{
	int r;
	for(r=0; r<height; r++)
	{
 	DMA[3].src = image + OFFSET(r,0,240);
	DMA[3].dst = videoBuffer + OFFSET(row+r,col,240);
	DMA[3].cnt = DMA_ON | width;
	}	
}


void fillScreen(const u16 *image)
{
	DMA[3].src = image;
	DMA[3].dst = videoBuffer;
	DMA[3].cnt = DMA_ON | 240*160;
}

void fillScreenColor(volatile u16 color)
{
	DMA[3].src = &color;
	DMA[3].dst = videoBuffer;
	DMA[3].cnt = DMA_ON | 160*240 | DMA_SOURCE_FIXED;
}

void wait4dma()
{
	while(DMA[3].cnt & DMA_ON);
}
