#include "myLib.h"

#include "startScreen.h"
#include "failScreen.h"
#include "winScreen.h"
#include "b1.h"
#include "b2.h"
#include "b3.h"
#include "b4.h"
#include "b5.h"
#include "b6.h"
#include "b7.h"
#include "b8.h"
#include "block.h"
#include "empty.h"
#include "flagged.h"
#include "marked.h"
#include "mine.h"
#include "minered.h"
#include "minewrong.h"

#define MINE 9

enum {
STARTSCREEN, DIFFSELECT, GAME, LOSE, WIN
};

enum {
EASY, MEDIUM, HARD
};

enum {
NONE, FLAGGED, MARKED
};

typedef struct{
	int row;
	int col;
	int value;
	int hidden;
	int flag;
}block;

u32 held;

block *board;

int boardRows, boardCols, boardMines, boardX, boardY;
int remainingSpaces;
int rng = 0;

char strBuffer[40];

int state = STARTSCREEN;

void startScreen();
void diffSelect();
int game();
void lose();
void win();

void drawDifficulty();

int generateBlocks(int rows, int cols, int numMines);
void generateBoard(int y, int x, int rows, int cols, int unitWidth, int unitHeight, const u16 image[]);
void generateRegularBoard(int y, int x, int rows, int cols);

int drawBlock(int row, int col, int size);
void highlightBlock(int row, int col, int size);
void flagBlock(int row, int col, int size);
int revealBlock(int row, int col, int size);

void revealBoardRegular();
void revealFullBoardRegular(int y, int x, int rows, int cols);

void updateTimeText(int row, int col, int width, int height, const u16 *image, char *str, u16 color);

void freezeTransition(int button, int newState);

int main(void)
{
	int exit = FALSE;
	REG_DISPCNT = MODE3 | BG2_ENABLE;
	while(!exit)
	{
		switch(state)
		{
			case STARTSCREEN:
				startScreen();
				break;
			case DIFFSELECT:
				diffSelect();
				break;
			case GAME:
				if(!game())
				{
					exit = TRUE;
				}
				break;
			case LOSE:
				lose();
				break;
			case WIN:
				win();
				break;
		}
		held = ~BUTTONS;
	}
}

void startScreen()
{
	fillScreen(startScreenBitmap);

	sprintf(strBuffer,"PRESS A TO START");
	drawString(72,72,strBuffer, WHITE);

	while(state == STARTSCREEN)
	{
		rng++;
		if KEY_DOWN_NOW(BUTTON_A)
		{
			WAIT4FULLPUSH(BUTTON_A);
			drawSubBackground3(72,72,96,8,startScreenBitmap);
			state = DIFFSELECT;
		}
		held = ~BUTTONS;
	}
}

void diffSelect()
{
	int diff = 0;
	int cursorPos = 0;
	drawDifficulty();
	drawRect(60+18*cursorPos,86,4,4,WHITE);
	while(state == DIFFSELECT)
	{
		if(KEY_DOWN_NOW(BUTTON_A))
		{
			WAIT4FULLPUSH(BUTTON_A);
			state = GAME;
			fillScreen(startScreenBitmap);
			switch(diff)
			{
				case EASY:
					boardRows = 4;
					boardCols = 8;
					boardMines = 5;
					break;
				case MEDIUM:
					boardRows = 6;
					boardCols = 10;
					boardMines = 10;
					break;
				case HARD:
					boardRows = 8;
					boardCols = 13;
					boardMines = 19;
					break;
			}
			boardX = 120-boardCols*8;
			boardY = 22;
		}
		if(KEY_DOWN_NOW(BUTTON_UP))
		{
			WAIT4FULLPUSH(BUTTON_UP);
			diff--;
			drawSubBackground3(60,86,4,54,startScreenBitmap);
		}
		if(KEY_DOWN_NOW(BUTTON_DOWN))
		{
			WAIT4FULLPUSH(BUTTON_DOWN);
			diff++;
			drawSubBackground3(60,86,4,54,startScreenBitmap);
			if(diff<0)
			{
				diff += 3;
			}
			else
			{
				cursorPos = diff % 3;
			}
			drawRect(60+18*cursorPos,86,4,4,WHITE);
		}
		if(KEY_DOWN_NOW(BUTTON_SELECT))
		{
			WAIT4FULLPUSH(BUTTON_SELECT);
			state = STARTSCREEN;
		}
		if(diff<0)
		{
			diff += 3;
		}
		cursorPos = diff % 3;
		drawRect(60+18*cursorPos,86,4,4,WHITE);
		held = ~BUTTONS;
	}
}

void drawDifficulty()
{
	sprintf(strBuffer, "SELECT A DIFFICULTY");
	drawString(40,63,strBuffer,WHITE);

	drawString(58,95,"EASY",WHITE);
	drawString(76,95,"MEDIUM",WHITE);
	drawString(94,95,"HARD",WHITE);
}

void lose()
{
	fillScreen(failScreenBitmap);

	sprintf(strBuffer,"YOU FAILED...");
	drawString(72,84,strBuffer, WHITE);

	while(KEY_DOWN_NOW(BUTTON_A));
	
	while(state == LOSE)
	{
		if(KEY_DOWN_NOW(BUTTON_A))
		{
			WAIT4FULLPUSH(BUTTON_A);
			state = STARTSCREEN;
		}
		else if(KEY_DOWN_NOW(BUTTON_SELECT))
		{
			WAIT4FULLPUSH(BUTTON_SELECT);
			state = STARTSCREEN;
		}
	}
}

void win(int time)
{
	fillScreen(winScreenBitmap);

	sprintf(strBuffer,"CONGRATULATIONS!!");
	drawString(56,69,strBuffer, WHITE);
	sprintf(strBuffer,"YOU DID IT!!");
	drawString(72,84,strBuffer, WHITE);	

	while(KEY_DOWN_NOW(BUTTON_A));

	while(state == WIN)
	{
		if(KEY_DOWN_NOW(BUTTON_A))
		{
			WAIT4FULLPUSH(BUTTON_A);
			state = STARTSCREEN;
		}
		else if(KEY_DOWN_NOW(BUTTON_SELECT))
		{
			WAIT4FULLPUSH(BUTTON_SELECT);
			state = STARTSCREEN;
		}
	}
}

int game(){
	int	currRow = 0;
	int currCol = 0;
	int oldRow = 0;
	int oldCol = 0;

	int time = 0;

	int blockSize = 16;

	remainingSpaces = boardRows*boardCols - boardMines;

	generateRegularBoard(boardY,boardX,boardRows,boardCols);
	if(!generateBlocks(boardRows,boardCols,boardMines))
	{
		return 0;
	}

	while(state == GAME)
	{

		sprintf(strBuffer,"TIME: %i", time/60);
		updateTimeText(8,100,60,8,startScreenBitmap,strBuffer,WHITE);
	
		highlightBlock(currRow,currCol,blockSize);
		if(oldRow != currRow || oldCol != currCol)
		{
			drawBlock(oldRow, oldCol, blockSize);
			oldRow = currRow;
			oldCol = currCol;
		}

		if KEY_PRESSED(BUTTON_A)
		{
			if(!revealBlock(currRow,currCol,blockSize))
			{
				sprintf(strBuffer,"YOU LOSE.");
				drawString(8,8,strBuffer, WHITE);
				freezeTransition(BUTTON_A, LOSE);
			}
		}
		else if KEY_PRESSED(BUTTON_B)
		{
			flagBlock(currRow,currCol,blockSize);
		}
		else if KEY_PRESSED(BUTTON_SELECT)
		{
			state = STARTSCREEN;
		}
		else if KEY_PRESSED(BUTTON_UP)
		{
			currRow = (currRow - 1) % boardRows;
		}
		else if KEY_PRESSED(BUTTON_DOWN)
		{
			currRow = (currRow + 1) % boardRows;
		}
		else if KEY_PRESSED(BUTTON_LEFT)
		{
			currCol = (currCol - 1) % boardCols;
		}
		else if KEY_PRESSED(BUTTON_RIGHT)
		{
			currCol = (currCol + 1) % boardCols;
		}
		if (currRow < 0)
		{
			currRow = currRow + boardRows;
		}
		if (currCol < 0)
		{
			currCol = currCol + boardCols;
		}
		held = ~BUTTONS;
		waitForVblank();
		time++;
		if(remainingSpaces <=0)
		{
			sprintf(strBuffer,"YOU WIN!!");
			drawString(8,8,strBuffer, WHITE);
			freezeTransition(BUTTON_A, WIN);
			win(time/60);
		}
	}
	return 1;
}

void freezeTransition(int button, int newState)
{
	WAIT4FULLPUSH(button);
	held = ~BUTTONS;
	int freeze = TRUE;
	while(freeze)
	{
		if KEY_DOWN_NOW(button)
		{
			state = newState;
			freeze = FALSE;
		}
		else if KEY_DOWN_NOW(BUTTON_SELECT)
		{
			state = STARTSCREEN;
			freeze = FALSE;
		}
	}
}

void highlightBlock(int row, int col, int size)
{
	int x, y;
	x = boardX + col*size;
	y = boardY + row*size;
	drawHollowRect(y,x,size,size,GREEN);
	drawHollowRect(y+1,x+1,size-2,size-2,GREEN);
}

int drawBlock(int row, int col, int size)
{
	const u16 *image = NULL;
	block b = board[OFFSET(row,col,boardCols)];
	if(b.hidden == TRUE)
	{
		switch(b.flag)
		{
			case NONE:
				image = blockBitmap;
				break;
			case FLAGGED:
				image = flaggedBitmap;
				break;
			case MARKED:
				image = markedBitmap;
				break;
		}
	}
	else
	{
		switch(b.value)
		{
			case 0:
				image = emptyBitmap;
				break;
			case 1:
				image = b1Bitmap;
				break;
			case 2:
				image = b2Bitmap;
				break;
			case 3:
				image = b3Bitmap;
				break;
			case 4:
				image = b4Bitmap;
				break;
			case 5:
				image = b5Bitmap;
				break;
			case 6:
				image = b6Bitmap;
				break;
			case 7:
				image = b7Bitmap;
				break;
			case 8:
				image = b8Bitmap;
				break;
			case 9:
				image = mineredBitmap;
				drawImage3(boardY+row*size, boardX+col*size, size, size, image);
				return 0;
		}
	}

	drawImage3(boardY+row*size, boardX+col*size, size, size, image);
	return 1;
}

void flagBlock(int row, int col, int size)
{
	block *b = board + OFFSET(row,col,boardCols);
	if(b->hidden)
	{
	b->flag = (b->flag + 1) % 3;
	drawBlock(row,col,size);
	}
}

int revealBlock(int row, int col, int size)
{
	block *b = board + OFFSET(row,col,boardCols);
	if((b->flag != FLAGGED) && (b->hidden == TRUE))
	{
		b->hidden = FALSE;
		remainingSpaces--;
		if(b->value == MINE)
		{
			revealBoardRegular();
			remainingSpaces++;
		}
		else if(b->value == 0)
		{
			int i,j,newRow,newCol;
			for(i=-1;i<=1;i++)
			{
				for(j=-1;j<=1;j++)
				{
					newRow = row + i;
					newCol = col + j;
					if(!(i==0 && j==0) && (newRow<boardRows) && (newCol<boardCols) && (newRow>=0) && (newCol>=0))
					{
						revealBlock(newRow,newCol,size);
					}
				}
			}
		}
	}
	return drawBlock(row,col,size);
}

void generateRegularBoard(int y, int x, int rows, int cols)
{
	generateBoard(y,x,rows,cols,16,16,blockBitmap);
}


void generateBoard(int y, int x, int rows, int cols, int unitWidth, int unitHeight, const u16 image[])
{
	int r, c;
	for(r = 0; r<rows; r++)
	{
		for(c = 0; c<cols; c++)
		{
			drawImage3(y+r*unitHeight, x+c*unitWidth, unitWidth, unitHeight, image);
		}
	}
}

void revealBoardRegular()
{
	int unitHeight = 16;
	int unitWidth = 16;
	int r,c;
	block *b;
	for(r=0;r<boardRows;r++)
	{
		for(c=0;c<boardCols;c++)
		{
			b = board + OFFSET(r,c,boardCols);
			if(b->value == MINE && b->flag != FLAGGED)
			{
					drawImage3(boardY+r*unitHeight, boardX+c*unitWidth, unitWidth, unitHeight, mineBitmap);
			}else if(b->flag == FLAGGED && b->value != MINE)
			{
					drawImage3(boardY+r*unitHeight, boardX+c*unitWidth, unitWidth, unitHeight, minewrongBitmap);
			}
		}
	}
}

void revealFullBoardRegular(int y, int x, int rows, int cols)
{
	int unitHeight = 16;
	int unitWidth = 16;
	int r,c,xcoord,ycoord;
	block *b;
	for(r=0;r<rows;r++)
	{
		for(c=0;c<cols;c++)
		{	
			xcoord = x+c*unitWidth;
			ycoord = y+r*unitHeight;
			b = board + OFFSET(r,c,cols);
			if(b->value == MINE && b->flag != FLAGGED)
			{
				drawImage3(ycoord, xcoord, unitWidth, unitHeight, mineBitmap);
			}else if(b->flag == FLAGGED && b->value != MINE)
			{
				drawImage3(ycoord, xcoord, unitWidth, unitHeight, minewrongBitmap);
			}else
			{
				switch(b->value)
				{
					case 0:
						drawImage3(ycoord,xcoord,unitWidth,unitHeight,emptyBitmap);
						break;
					case 1:
						drawImage3(ycoord,xcoord,unitWidth,unitHeight,b1Bitmap);
						break;
					case 2:
						drawImage3(ycoord,xcoord,unitWidth,unitHeight,b2Bitmap);
						break;
					case 3:
						drawImage3(ycoord,xcoord,unitWidth,unitHeight,b3Bitmap);
						break;
					case 4:
						drawImage3(ycoord,xcoord,unitWidth,unitHeight,b4Bitmap);
						break;
					case 5:
						drawImage3(ycoord,xcoord,unitWidth,unitHeight,b5Bitmap);
						break;
					case 6:
						drawImage3(ycoord,xcoord,unitWidth,unitHeight,b6Bitmap);
						break;
					case 7:
						drawImage3(ycoord,xcoord,unitWidth,unitHeight,b7Bitmap);
						break;
					case 8:
						drawImage3(ycoord,xcoord,unitWidth,unitHeight,b8Bitmap);
						break;
				}
			}
		}
	}
}


int generateBlocks(int rows, int cols, int numMines)
{
	int numBlocks = rows*cols;
	board = (block *)malloc(numBlocks*sizeof(block));
	if((NULL == board) || (numMines>(numBlocks)))
	{
		return 0;
	}

	//instantiate structs
	int r,c;
	for(r=0; r<rows;r++)
	{
		for(c=0; c<cols;c++)
		{
			board[OFFSET(r,c,cols)].row = r;
			board[OFFSET(r,c,cols)].col = c;
			board[OFFSET(r,c,cols)].value = 0;
			board[OFFSET(r,c,cols)].hidden = TRUE;
			board[OFFSET(r,c,cols)].flag = NONE;
		}
	}


	//generate mines
	srand(rng);
	int i;
	for(i=0; i<numMines; i++)
	{
		int r = rand() % numBlocks;
		while(MINE == board[r].value)
		{
			r++;
			r = r % numBlocks;
		}
		board[r].value = MINE;
	}

	remainingSpaces = numBlocks - numMines;
	
	//fill in numbers
	int m,n;
	for(r=0; r<rows;r++)
	{
		for(c=0; c<cols;c++)
		{
			if(board[OFFSET(r,c,cols)].value == MINE)
			{
				for(n=-1; n<=1; n++)
				{
					for(m=-1; m<=1; m++)
					{
						if(!(n==0 && m==0) && (r+n<rows) && (c+m<cols) && (r+n>=0) && (c+m>=0))
						{
							
							if(board[OFFSET(r+n,c+m,cols)].value != MINE)
							{
								board[OFFSET(r+n,c+m,cols)].value++;
							}
						}
					}
				}
			}
		}
	}

	return 1;
}


void updateTimeText(int row, int col, int width, int height, const u16 *image, char *str, u16 color)
{
	drawSubBackground3(row,col,width,height, image);
	drawString(row,col,str,color);
}
