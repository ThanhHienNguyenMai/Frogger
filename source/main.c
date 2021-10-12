#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "framebuffer.h"
#include "initGPIO.h"
#include <wiringPi.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "startScreen1.h"
#include "startScreen2.h"
#include "gameScreen1.h"
#include "game1.h"
#include "game2.h"
#include "game3.h"
#include "game4.h"
#include "pauseScreen1.h"
#include "pauseScreen2.h"
#include "frog.h"
#include "winScreen.h"
#include "loseScreen.h"
#include "enemy1.h"
#include "enemy2.h"
#include "enemy3.h"
#include "enemy4.h"
#include "enemy5.h"
#include "enemy6.h"
#include "enemy7.h"
#include "enemy8.h"
#include "valueP1.h"
#include "valueP2.h"
#include "valueP3.h"
#include "valueP4.h"
#include "gameInfoBar.h"
#include "n0.h"
#include "n1.h"
#include "n2.h"
#include "n3.h"
#include "n4.h"
#include "n5.h"
#include "n6.h"
#include "n7.h"
#include "n8.h"
#include "n9.h"
#include "score.h"

//Authors: Thanh Hien Nguyen-Mai & Lauren Mayes
//UCID: 30038243 & 30062361
//Date: April 22, 2021
//Course: CPSC 359
//Project Part 2
//Desc:Frogger Game

#define GPLEV0 13
#define GPSET0 7 
#define GPCLR0 10
#define INP_GPIO(p) *(gpio + (p / 10)) &= ~(7<<((p % 10) * 3))
#define OUT_GPIO(p) *(gpio + (p / 10)) |= (1<<((p % 10) * 3))

//button structure
//name is a list of chars to represent the name of the button
//unpressed is an int that represents if the button is unpressed or not
//1 means unpressed and 0 means pressed
struct buttons {
    char name[13];
    int unpressed;
};

//options structure
//name is a list of chars to represent the name of teh options
//selected is an int that represents if the option is currently being hovered
//pressed is an int that represents if the option has been chosen
struct options {
    char name[10];
    int selected;
    int pressed;
};

//info structure
//currScreen is an int representing the current screen being displayed
//numPressed is an int representing the current button being pressed by the user
//score is an int representing the current score of the user
//lives is an int representing the current amount of lives of the user
//timeLeft is an unsigned int representing the amount of timeLeft before the game ends
//moves is an unsigned int representing the amount of moves the user has left
//win is is an int represent if the game has been won (1 = win)
//lose is an int representing if the game has been lost (1 = lose)
//end is an int representing if the user wants the game to end (1 = end)
//xfrog is an int representing the x position of the frog
//yfrog is an int representing the y position of the frog
//collision is an int representing if the frog has collided with an object
//screen is an int representing the current game level the user is on
//justChanged is an int representing if the level has just changed
//reset is an int representing if the game variables need to be reset
//valueP is an int representing if the value pack has been found
//timeTillVP is an int representing the time left before a value pack appears
struct info {
    int currScreen;
    int numPressed;
    int score;
    int lives;
    int timeLeft;
    int moves;    
    int win;
    int lose;
    int end;
    int xFrog;
    int yFrog;
    int collison;
    int screen;
    int justChanged;
    int reset;
    int valueP;
    int timeTillVP;
};

//levelVar struct
//pGame is the pointer to the a game background
//gameHeight is an int representing the height of the background
//gameWidth is an int representing the width of the background
//pEnemy1 is the pointer to the enemy1 on that level
//e1Height is an int representing the height of the enemy1 on that level
//e1Width is an int representing the width of the enemy1 on that level
//pEnemy2 is the pointer to the enemy2 on that level
//e2Height is an int representing the height of the enemy2 on that level
//e2Width is an int representing the width of the enemy2 on that level
//pValue is the pointer to the value pack
//vHeight is the height of the value pack
//vWidth is the width of the value pack
struct levelVar {
    short int *pGame;
    int gameHeight;
    int gameWidth;
    short int *pEnemy1;
    int e1Height;
    int e1Width;
    short int *pEnemy2;
    int e2Height;
    int e2Width;
    short int *pValue;
    int vHeight;
    int vWidth;
};

struct fbs framebufferstruct;

//pixel structure
typedef struct {
	short int color;
	int x, y;
} Pixel;

void drawPixel(Pixel *pixel);

//draws an image to the screen pixel by pixel
//@param height is the height of the image
//@param width is the width of the image
//@param xShift is the shift on the x plane of the image start
//@param yShift is the shift on the y plane of the image start
//@param image is the pointer to the image to be displayed 
void drawImage(int height, int width, int xShift, int yShift, short int* image){
    /* initialize a pixel */
    Pixel *pixel;
    pixel = malloc(sizeof(Pixel));
    int i=0;	
    for (int y = 0; y < height; y++)//the image height
    {
	for (int x = 0; x < width; x++) //is image width
	{	                      
	    pixel->color = image[i]; 
	    pixel->x = x + xShift;
	    pixel->y = y + yShift;
	
	    drawPixel(pixel);
	    i++;
	}
    }
    /* free pixel's allocated memory */
    free(pixel);
    pixel = NULL;
}

//draws a pixel to the screen
//@param *pixel a pointer to the pixel to be printed
void drawPixel(Pixel *pixel){
    long int location =(pixel->x +framebufferstruct.xOff) * (framebufferstruct.bits/8) +
	(pixel->y+framebufferstruct.yOff) * framebufferstruct.lineLength;
    *((unsigned short int*)(framebufferstruct.fptr + location)) = pixel->color;
}

//draws a number to the screen
//@param number the int to display
//@param xShift an int representing where the x coor of the leading value should be
//@param yShift an int representing where the y coor of the leading value should be
void drawNum(int number, int xShift, int yShift){
    int amount = 0;
    int x;
    int temp = number;
    short int *pNum;
    int numHeight;
    int numWidth;
    
    //finds the amount of digits in the number
    while(temp != 0){
	temp = temp / 10;
	amount++;
    }

    temp = number;
    
    //puts each digit into array
   for(int i = 0; i < amount; i++){
       //finds the appropriate image for the number
	switch(temp % 10){
	    case 0:
		pNum = (short int *) n0.pixel_data;
		numHeight = n0.height;
		numWidth = n0.width;
		break;
	    case 1:
		pNum = (short int *) n1.pixel_data;
		numHeight = n1.height;
		numWidth = n1.width;
		break;
	    case 2:
		pNum = (short int *) n2.pixel_data;
		numHeight = n2.height;
		numWidth = n2.width;
		break;
	    case 3:
		pNum = (short int *) n3.pixel_data;
		numHeight = n3.height;
		numWidth = n3.width;
		break;
	    case 4:
		pNum = (short int *) n4.pixel_data;
		numHeight = n4.height;
		numWidth = n4.width;
		break;
	    case 5:
		pNum = (short int *) n5.pixel_data;
		numHeight = n5.height;
		numWidth = n5.width;
		break;
	    case 6:
		pNum = (short int *) n6.pixel_data;
		numHeight = n6.height;
		numWidth = n6.width;
		break;
	    case 7:
		pNum = (short int *) n7.pixel_data;
		numHeight = n7.height;
		numWidth = n7.width;
		break;
	    case 8:
		pNum = (short int *) n8.pixel_data;
		numHeight = n8.height;
		numWidth = n8.width;
		break;
	    case 9:
		pNum = (short int *) n9.pixel_data;
		numHeight = n9.height;
		numWidth = n9.width;
		break;
	}
	//offsets each digit
	x = xShift + (20 * (amount - i));
	drawImage(numHeight, numWidth, x, yShift, pNum);
	temp = temp/10;
    }
}

//Initializes the GPIO line using a line number and function code
//@param gpio the gpio pointer
//@param lineNum the line number
//@param funcCode the function code
void Init_GPIO(unsigned int *gpio, int lineNum, int funcCode){
    if (funcCode == 000) {
	INP_GPIO(lineNum);
    }else if(funcCode == 001) {
	INP_GPIO(lineNum);
	OUT_GPIO(lineNum);
    }
}

//Waits for a time interval, passed as a paramter
//@param timeInterval the amount of time to wait
void Wait(int timeInterval){
    delayMicroseconds(timeInterval);
}

//Writes a bit to the GPIO latch line
//@param gpio the gpio pointer
void Write_Latch(unsigned int *gpio){
    gpio[GPSET0] = 1 << 9;
    Wait(12);
    gpio[GPCLR0] = 1 << 9;
}

//writes to the GPIO Clock Line
//@param gpio the gpio pointer
//@param funcCode the function code
void Write_Clock(unsigned int *gpio, int funcCode){
    if (funcCode == 000) {
	gpio[GPCLR0] = 1 << 11;
    }else{
	gpio[GPSET0] = 1 << 11;
    }
}

//Reads a bit from the GPIO data line
//@param gpio the gpio pointer
//@return v the bit read
int Read_Data(unsigned int *gpio){
   int v = (gpio[GPLEV0] >> 10) & 1;
   
   return v;
}

//Reads the input from the SNES controller and returns the code of a press button in a register
//@param gpio the gpio pointer
//@param arrButtons the array of button structures
//@param numButtons the size of arrButtons
void Read_SNES(unsigned int *gpio, struct buttons *arrButtons, int numButtons){
    
    //loop going through each of the buttons
    for (int i = 1; i < numButtons; i++){
	Wait(6);
	Write_Clock(gpio, 000);
	Wait(6);
	arrButtons[i].unpressed = Read_Data(gpio);
	Write_Clock(gpio, 001);
    }
}

//Checks to see if a button has been pressed
//@param arrButtons the array of button structures
//@param numButtons the size of arrButtons
//@param prevPressed an array where each index represents a button and the value at that index represents if a button was pressed (pressed = 1)
//@param gameInfo a structure of the gameInfo
//@return pressedButton an int representing if a button was pressed
int checkButtonPressed(struct buttons *arrButtons, int numButtons, int *prevPressed, struct info *gameInfo){
    int pressed = 0;
    
    //for loop to look for the pressed button and print when a button is pressed
    for (int i = 1; i < numButtons; i++){
	//makes sure to only print pressed buttons that weren't printed before
	if (arrButtons[i].unpressed == 0 && prevPressed[i] == 0){
	    //printf("the button being pressed is %s \n", arrButtons[i].name);
	    gameInfo -> numPressed = i;
	    prevPressed[i] = 1;
	    pressed = 1;
	}else if (arrButtons[i].unpressed == 1) {
	    //sets all the unpressed buttons to unpressed in prevPressed array
	    prevPressed[i] = 0;
	}
    }
    
    delay(75);
    
    return pressed;    
}

//The read controller function that updates the current button being pressed by the user
//@param gameInfo a structure of the gameInfo
//@param gpio the gpio pointer
int readController(struct info *gameInfo, unsigned int *gpio){
    int numButtons = 17; 
    int buttonPressed = 0;
    
    //Creating the array of struct of buttons
    struct buttons arrButtons[] = { {"", 1}, {"B", 1}, {"Y", 1}, {"Select", 1}, {"Start", 1}, {"Joy-pad UP", 1}, {"Joy-pad DOWN", 1} , {"Joy-pad LEFT", 1} , 
       {"Joy-pad RIGHT", 1}, {"A", 1}, {"X", 1} , {"Left", 1}, {"Right", 1}, {"13", 1}, {"14", 1}, {"15", 1}, {"16", 1}};
       
    //Creating an array where each index represents a button and the value at that index represents if a button is pressed (pressed = 1)
    int prevPressed[numButtons];
    
    //traverses the entire array and initializes all the values to 0
    for (int i = 0; i < numButtons; i++){
	prevPressed[i] = 0;
    }
    
    //Initializing 
    Init_GPIO(gpio, 9, 001);
    Init_GPIO(gpio, 11, 001);
    Init_GPIO(gpio, 10, 000);
    
    //infinite loop that checks the buttons' status and prints a message if any of them were pressed
    while(1){
	if (buttonPressed == 1){
	    //printf("Please press button... \n");
	}
	Write_Latch(gpio);
	Read_SNES(gpio, arrButtons, numButtons);
	//Check if the start button was pressed and ends the program if it has been
        if(gameInfo -> numPressed != 0){
	    //printf("Button Pressed...\n");
	    break;
	}
	buttonPressed = checkButtonPressed(arrButtons, numButtons, prevPressed, gameInfo);
   }

    return 0;
}

//The thread responsible for continously reading controller inputs
//@param ptr is a pointer to the gameInfo struct
void *controller(void *ptr){
    struct info *gameInfo = ptr;
    
    //printf("Controller made\n");
    unsigned int *gpio = getGPIOPtr();
    
    while(1){
	readController(gameInfo, gpio);
    }
}

//The thread that manages the start screen
//@param ptr is a pointer to the gameInfo struct
void *startScreen(void *ptr){
    int id = 1;
    int shown = 0;
    struct info *gameInfo = ptr;
    
    //The array of options for the start screen
    struct options arrStartOptions[] = {{"Start Game", 1, 0}, {"Quit Game", 0, 0}};
    
    //Initializes values needed to draw start screens
    short int *start1=(short int *) startScreen1.pixel_data;
    int height1 = startScreen1.height;
    int width1 = startScreen1.width;
    
    short int *start2=(short int *) startScreen2.pixel_data;
    int height2 = startScreen2.height;
    int width2 = startScreen2.width;
    
    while(1){
	while (gameInfo -> currScreen != id){
	    //waits until turn starts
	    shown = 0;
	}
	
	//if the screen was not previously been shown
	if (shown == 0){
	    delay(200);
	    drawImage(height1, width1,0,0,start1);
	    
	    //reset options
	    arrStartOptions[0].selected = 1;
	    arrStartOptions[0].pressed = 0;
	    arrStartOptions[1].selected = 0;
	    arrStartOptions[1].pressed = 0;
	    
	    shown = 1;
	};
	
	//if up or down was pressed
	if(gameInfo -> numPressed == 5 || gameInfo -> numPressed == 6){
	    if(arrStartOptions[0].selected == 1){
		arrStartOptions[0].selected = 0;
		arrStartOptions[1].selected = 1;
		drawImage(height2, width2,0,0,start2);
	    } else {
		arrStartOptions[0].selected = 1;
		arrStartOptions[1].selected = 0;
		drawImage(height1, width1,0,0,start1);
	    }
	    gameInfo -> numPressed = 0;
	}//if the user presses a on the controller
	else if(gameInfo -> numPressed == 9){
	    if(arrStartOptions[0].selected == 1){
		arrStartOptions[0].pressed = 1;
	    } else {
		arrStartOptions[1].pressed = 1;
	    }
	}else if(gameInfo -> numPressed == 0){
	    //do nothing
	}else{
	    gameInfo -> numPressed = 0;
	}
	
	//if the player chooses to start the game, change the current screen to the game screen
	if(arrStartOptions[0].pressed == 1){
	    arrStartOptions[0].pressed = 0;
	    //manually resetting button number pressed
	    gameInfo -> numPressed = 0;
	    gameInfo -> reset = 1;
	}else if(arrStartOptions[1].pressed == 1){
	    arrStartOptions[1].pressed = 0;
	    gameInfo -> end = 1;
	    gameInfo -> currScreen = 0;
	}
    }
}

//function to update the timeLeft
//@return the new time
time_t updateTime(struct info *gameInfo, time_t currTime){
    if(currTime < time(0)){
	gameInfo -> timeLeft = gameInfo -> timeLeft - 1;
    }
    
    return time(0);
}

//The thread that checks the gameInfo variables
//@param ptr is a pointer to the gameInfo struct
void *gameScreen(void *ptr){
    int id = 2;
    int shown = 0;
    int initial = 1;
    int scoreConstant = 5;
    struct info *gameInfo = ptr;
    time_t seconds = time(0);
    int screenTracker = 0;
    int moved = 0;
    
    //Initializes values needed to draw game screen
    short int *pGameScreen1=(short int *) gameScreen1.pixel_data;
    int height3 = gameScreen1.height;
    int width3 = gameScreen1.width;
    
    while(1){
	while (gameInfo -> currScreen != id){
	    //waits until turn starts
	    shown = 0;
	}
	
	//display game screen again when entering the gameScreen only
	if (shown == 0){
	    delay(100);
	    shown = 1;
	}
	
	//only happens the first time they start the game
	if(initial == 1){   
	    //waiting for the user to press start
	    drawImage(height3, width3,0,0, pGameScreen1);
	    if(gameInfo -> numPressed == 4){
		gameInfo -> justChanged = 1;

		//draw setup
		gameInfo -> screen = 1;
		initial = 0;
	    }else if(gameInfo -> numPressed == 0){
		//do nothing
	    }else{
		gameInfo -> numPressed = 0;
	    }

	} else{
	    
	    //manually reseting numPressed
	    gameInfo -> numPressed = 0;
	    
	    //recalc time left
	    seconds = updateTime(gameInfo, seconds);
	    
	    //recalc score
	    gameInfo -> score = (gameInfo -> moves + gameInfo -> timeLeft + gameInfo -> lives) * scoreConstant;
	    
	    //end game if time runs out, no moves left or out of lives
	    if (gameInfo -> timeLeft == 0 || gameInfo -> moves == 0 || gameInfo -> lives == 0){
		gameInfo -> lose = 1;
	    }
	    
	    //frog controls
	    if(gameInfo -> numPressed == 5){
		if(gameInfo -> yFrog >= 32){
		    // move up
		    gameInfo -> yFrog -= 32;
		    gameInfo -> moves -= 1;
		    moved = 1;
		}
	    }
	    else if(gameInfo -> numPressed == 6){
		if(gameInfo -> yFrog < 640){
		    // move down
		    gameInfo -> yFrog += 32;
		    gameInfo -> moves -= 1;
		    moved = 1;
		}
	    }
	    else if(gameInfo -> numPressed == 7){
		if(gameInfo -> xFrog > 16){
		    // move left
		    gameInfo -> xFrog -= 32;
		    gameInfo -> moves -= 1;
		}
	    }
	    else if(gameInfo -> numPressed == 8){
		if(gameInfo -> xFrog < 1232){
		    // move right
		    gameInfo -> xFrog += 32;
		    gameInfo -> moves -= 1;
		}
	    }
	    
	    //game screen controls
	    if(gameInfo -> screen == 1){
		//cant go back

		//move forawrd to screen 2
		if(gameInfo -> yFrog <= 31 && screenTracker == 0 && moved == 1) {
		    gameInfo -> screen = 2;
		    gameInfo -> justChanged = 1;
		    gameInfo -> yFrog = 640;
		    screenTracker = 1;
		    moved = 0;
		}
	    }
	    else if(gameInfo -> screen == 2){
		//move back to 1
		if(gameInfo -> yFrog >= 640 && screenTracker == 0 && moved == 1){
		    gameInfo -> screen = 1;
		    gameInfo -> justChanged = 1;
		    gameInfo -> yFrog = 0;
		    screenTracker = 1;
		    moved = 0;
		}
		//move forward to 3
		if(gameInfo -> yFrog <= 31 && screenTracker == 0 && moved == 1) {
		    gameInfo -> screen = 3;
		    gameInfo -> justChanged = 1;
		    gameInfo -> yFrog = 640;
		    screenTracker = 1;
		    moved = 0;
		}
	    }
	    else if(gameInfo -> screen == 3){
		//move back to screen 2
		if(gameInfo -> yFrog >= 640 && screenTracker == 0 && moved == 1){
		    gameInfo -> screen = 2;
		    gameInfo -> justChanged = 1;
		    gameInfo -> yFrog = 0;
		    screenTracker = 1;
		    moved = 0;
		}
		//move forward to screen 4
		if(gameInfo -> yFrog <= 31 && screenTracker == 0 && moved == 1) {
		    gameInfo -> screen = 4;
		    gameInfo -> justChanged = 1;
		    gameInfo -> yFrog = 640;
		    screenTracker = 1;
		    moved = 0;
		}
	    }
	    else if(gameInfo -> screen == 4){
		//move back to screen 3
		if(gameInfo -> yFrog >= 640 && screenTracker == 0 && moved == 1){
		    gameInfo -> screen = 3;
		    gameInfo -> justChanged = 1;
		    gameInfo -> yFrog = 0;
		    screenTracker = 1;
		    moved = 0;
		}
		//win
		if(gameInfo -> yFrog <= 31 && screenTracker == 0) {
		    // go to win screen
		    gameInfo -> win = 1;  
		}
	    }
	    screenTracker = 0;
	    moved = 0;
	    
	    //collision handler
	    if(gameInfo -> collison == 1){ 
		gameInfo -> lives -= 1;
		gameInfo -> collison = 0;
		gameInfo -> yFrog = 640;
	    }
	    
	    //if the player chooses to pause the game, change the current screen to the pause screen
	    if(gameInfo -> numPressed == 4){
		//manually resetting button number pressed
		gameInfo -> numPressed = 0;
		gameInfo -> currScreen = 3;
	    }else if(gameInfo -> numPressed == 0){
		//do nothing
	    }else{
		gameInfo -> numPressed = 0;
	    }
	}
    }
}

//The thread that manages the pause window 
//@param ptr is a pointer to the gameInfo struct
void *pauseScreen(void *ptr){
    int id = 3;
    int shown = 0;
    struct info *gameInfo = ptr;
    
    //Initializes values needed to draw pause screens
    short int *pPause1=(short int *) pauseScreen1.pixel_data;
    int height5 = pauseScreen1.height;
    int width5 = pauseScreen1.width;
    
    short int *pPause2=(short int *) pauseScreen2.pixel_data;
    int height6 = pauseScreen2.height;
    int width6 = pauseScreen2.width;
    
    //The array of options for the pause screen
    struct options arrPauseOptions[] = {{"Restart", 1, 0}, {"Quit", 0, 0}};
    
    while(1){
	while (gameInfo -> currScreen != id){
	    //waits until turn starts
	    shown = 0;
	}
	if (shown == 0){
	    delay(120);
	    drawImage(height5, width5,290,110,pPause1);
	    
	    //reset options
	    arrPauseOptions[0].selected = 1;
	    arrPauseOptions[0].pressed = 0;
	    arrPauseOptions[1].selected = 0;
	    arrPauseOptions[1].pressed = 0;
	    
	    shown = 1;
	};
	
	//If the user presses up or down on the joy pad
	if(gameInfo -> numPressed == 5 || gameInfo -> numPressed == 6){
	    if(arrPauseOptions[0].selected == 1){
		arrPauseOptions[0].selected = 0;
		arrPauseOptions[1].selected = 1;
		drawImage(height6, width6,290,110,pPause2);
	    }
	    else{
		arrPauseOptions[0].selected = 1;
		arrPauseOptions[1].selected = 0;
		drawImage(height5, width5,290,110,pPause1);
	    }
	    //manually resetting button number pressed
	    gameInfo -> numPressed = 0;
	}
	//if the user presses a on the controller
	else if(gameInfo -> numPressed == 9){
	    if(arrPauseOptions[0].selected == 1){
		arrPauseOptions[0].pressed = 1;
	    } else {
		arrPauseOptions[1].pressed = 1;
	    }
	    //manually resetting button number pressed
	    gameInfo -> numPressed = 0;
	}else if(gameInfo -> numPressed == 4){
	    //manually resetting button number pressed
	    gameInfo -> numPressed = 0;
	    gameInfo -> currScreen = 2;
	}else if(gameInfo -> numPressed == 0){
	    //do nothing
	}else{
	    gameInfo -> numPressed = 0;
	}
	
	if(arrPauseOptions[0].pressed == 1){
	    //tell program to restart the game
	    gameInfo -> reset = 1;
	}
	    
	else if(arrPauseOptions[1].pressed == 1){
	    arrPauseOptions[1].pressed = 0;
	    gameInfo -> currScreen = 1;
	    //manually resetting button number pressed
	    gameInfo -> numPressed = 0;
	}
    }
}

//function to update the timeTimeVP
//@return the new time
time_t updateTimeTillVP(struct info *gameInfo, time_t currTime){
    if(currTime < time(0)){
	gameInfo -> timeTillVP = gameInfo -> timeTillVP - 1;
    }
    
    return time(0);
}

//The thread that manages the creation and positions of the non playable characters, frog, and game background
//@param ptr is a pointer to the gameInfo struct
void *gameAssets(void *ptr){
    time_t seconds = time(0);
    int randX, randY;
    int valuePShown = 0;
    //possible lanes the value pack can appear
    int valuePLane[] = {2,4,6,8,10,12,14,16,18};
    //lanes for the enemy
    int enemyLane[] = {1,3,5,7,9,11,13,15,17,19}; //between 1 and 19
    int enemyPosition[10][3];

    struct info *gameInfo = ptr;
    
    //calling srand to seed the value
    srand(time(0));
    
    //initializing enemy position randomly
    for (int i = 0; i < 10; i++){
	for(int j = 0; j < 3; j++){
	    enemyPosition[i][j] = rand() % 39 + 1;
	}
    }
    
    //frog
    short int *pFrog = (short int *) frogImage.pixel_data;
    int frogHeight = frogImage.height;
    int frogWidth = frogImage.width;
    
    //game information bar
    short int *pGIB = (short int *) gameInfoBar.pixel_data;
    int gIBHeight = gameInfoBar.height;
    int gIBWidth = gameInfoBar.width;
    
    //Creating the array of struct of levelVar
    //first left empty to match with with the screen value
    struct levelVar level[] = { {0,0,0,0,0,0,0,0,0},
	{(short int *) game1.pixel_data, game1.height, game1.width, (short int *) enemy1.pixel_data, enemy1.height, enemy1.width, (short int *) enemy2.pixel_data, enemy2.height, enemy2.width, 
	    (short int *) valueP1.pixel_data, valueP1.height, valueP1.width},
	{(short int *) game2.pixel_data, game2.height, game2.width, (short int *) enemy3.pixel_data, enemy3.height, enemy3.width, (short int *) enemy4.pixel_data, enemy4.height, enemy4.width,
	    (short int *) valueP2.pixel_data, valueP2.height, valueP2.width},
	{(short int *) game3.pixel_data, game3.height, game3.width, (short int *) enemy5.pixel_data, enemy5.height, enemy5.width, (short int *) enemy6.pixel_data, enemy6.height, enemy6.width,
	    (short int *) valueP3.pixel_data, valueP3.height, valueP3.width},
	{(short int *) game4.pixel_data, game4.height, game4.width, (short int *) enemy7.pixel_data, enemy7.height, enemy7.width, (short int *) enemy8.pixel_data, enemy8.height, enemy8.width, 
	    (short int *) valueP4.pixel_data, valueP4.height, valueP4.width}};
  
    while(1){
	while (gameInfo -> currScreen != 2){
	    //waits until turn starts
	}
	delay(100);

	if(gameInfo -> screen != 0){
	    drawImage(level[gameInfo -> screen].gameHeight, level[gameInfo -> screen].gameWidth, 0 ,0, level[gameInfo -> screen].pGame); //background
	    drawImage(gIBHeight, gIBWidth, 0, 672, pGIB); //gameInfoBAr
	    drawNum(gameInfo -> score, 215, 683); //score
	    drawNum(gameInfo -> lives, 540, 683); //lives
	    drawNum(gameInfo -> timeLeft, 850, 683); //timer
	    drawNum(gameInfo -> moves, 1190, 683); //moves
	    drawImage(frogHeight, frogWidth, gameInfo -> xFrog, gameInfo -> yFrog, pFrog); //frog
		    
	    for(int i = 0; i < 10; i++){
		for(int j = 0; j < 3; j++){
		    //enemy that moves left to right
		    if( i % 2 == 0){
			drawImage(level[gameInfo -> screen].e1Height, level[gameInfo -> screen].e1Width, enemyPosition[i][j] * 32, enemyLane[i] * 32, level[gameInfo -> screen].pEnemy1); //enemy 1
			if(gameInfo -> yFrog == enemyLane[i]*32){
			    if((gameInfo -> xFrog - 16) == enemyPosition[i][j]*32 || (gameInfo -> xFrog + 16) == enemyPosition[i][j]*32){
				gameInfo -> collison = 1;
			    }
			}
    
			if(enemyPosition[i][j] >= 39){
			    enemyPosition[i][j] = 0;
			}else{
			    enemyPosition[i][j] += 1; //move right
			}
		    }
		
		    //enemy that moves right to left
		    if( i % 2 == 1){
			drawImage(level[gameInfo -> screen].e2Height, level[gameInfo -> screen].e2Width, enemyPosition[i][j] * 32, enemyLane[i] * 32, level[gameInfo -> screen].pEnemy2); //enemy 2
			if(gameInfo -> yFrog == enemyLane[i]*32){
			    if((gameInfo -> xFrog - 16) == enemyPosition[i][j]*32 || (gameInfo -> xFrog + 16) == enemyPosition[i][j]*32){
				gameInfo -> collison = 1;
			    }
			}
			if(enemyPosition[i][j] == 0){
			    enemyPosition[i][j] = 39;
			}else{
			    enemyPosition[i][j] -= 1; //move left
			}
		    }
		}
	    }
	    
	    //if the level just changed, restart the timer
	    if(gameInfo -> justChanged == 1){
		gameInfo -> timeTillVP = 30;
		gameInfo -> justChanged = 0;
		valuePShown = 0; //not shown
		
		//changing position of enemies
		for (int i = 0; i < 10; i++){
		    for(int j = 0; j < 3; j++){
			enemyPosition[i][j] = rand() % 39 + 1;
		    }
		}
	    }else{
		seconds = updateTimeTillVP(gameInfo, seconds);
		if(gameInfo -> timeTillVP <= 0 && valuePShown == 0){
		    randX = rand() % 39;
		    //random number between 0 and 8 which will randomly select a lane for the value pack to appear
		    randY = valuePLane[rand() % 8];
		    valuePShown = 1; //shown but not gotten 
		}
		
		//draw the value pack to the screen if it has appeared
		if(valuePShown == 1){
		    drawImage(level[gameInfo -> screen].vHeight, level[gameInfo -> screen].vWidth, (randX * 32)+16 , randY * 32, level[gameInfo -> screen].pValue);
		    //if the frog collides with the value pack
		    if(gameInfo -> xFrog == (randX*32) + 16 && gameInfo -> yFrog == randY*32){
			valuePShown = 2; //has been consumed by the user
			if(gameInfo -> screen == 1){ //level 1, value pack increases life count
			    gameInfo -> lives = gameInfo -> lives + 1;
			}else if(gameInfo -> screen == 2){ //level 2, value pack increases amount of moves
			    gameInfo -> moves = gameInfo -> moves + 1;
			}else if(gameInfo -> screen == 3){ //level 3, value pack causes user to instantly win
			    gameInfo -> win = 1;
			}else{//level 4, value pack causes user to instantly lose
			    gameInfo -> lose = 1;
			}
		    }
		}
	    }
	}
    }
}


//The thread that manages the creation of all other threads
//@param pthreadid the id of the pthread
void *management(void *pthreadid){
    //initialises the game inforamation structure
    //currScreen, numPressed, score, lives, timeLeft, moves, win, lose, end, xFrog, yFrog, collison, screen, justChanged, reset, valueP, timeTillVP
    struct info gameInfo = {1, 0, 0, 4, 100, 100, 0, 0, 0, 624, 640, 0, 0, 0, 0, 0, 30};
    
    short int *pLoseScreen =(short int *) loseScreen.pixel_data;
    int height2 = loseScreen.height;
    int width2 = loseScreen.width;
    
    short int *pWinScreen =(short int *) winScreen.pixel_data;
    int height1 = winScreen.height;
    int width1 = winScreen.width;

    pthread_t tid1, tid2, tid3, tid4, tid5;
    pthread_attr_t attr;
    
    //initializies a thread attributes
    pthread_attr_init(&attr);
    
    //creates the screen threads
    pthread_create(&tid1, &attr, startScreen, (void *)&gameInfo);
    pthread_create(&tid2, &attr, gameScreen, (void *)&gameInfo);
    pthread_create(&tid3, &attr, pauseScreen, (void *)&gameInfo);
    pthread_create(&tid4, &attr, controller, (void *)&gameInfo);
    pthread_create(&tid5, &attr, gameAssets, (void *)&gameInfo);

    //start with the start screen
    gameInfo.currScreen = 1;
    while(gameInfo.end == 0){
	//if the player won or lost the game
	if(gameInfo.win != 0 || gameInfo.lose != 0){
	    //cancels old game and npc threads
	    pthread_cancel(tid2);
	    pthread_cancel(tid5);
	    
	    //if game has been won
	    if(gameInfo.win == 1){
		delay(110);
		drawImage(height1, width1, 0 , 0, pWinScreen);
		
		//reset win flag
		gameInfo.win = 0;
		
	    //if game lost
	    }else if(gameInfo.lose == 1){
		delay(110);
		drawImage(height2, width2, 0 , 0, pLoseScreen);
		
		//reset lose flag
		gameInfo.lose = 0;
	    }
	    
	    drawImage(score.height, score.width, 450 , 40, (short int *) score.pixel_data);
	    drawNum(gameInfo.score, 700, 45);
	    
	    //manually resetting button pressed
	    gameInfo.numPressed = 0;
	
	    while(gameInfo.numPressed == 0){
		//wait for a button to be pressed
	    }
	    
	    //manually resetting button pressed
	    gameInfo.numPressed = 0;
	    
	    //change the game back to the start screen
	    gameInfo.currScreen = 1;
	    
	    //creates new gameScreen and npc
	    pthread_create(&tid2, &attr, gameScreen, (void *)&gameInfo);
	    pthread_create(&tid5, &attr, gameAssets, (void *)&gameInfo);
	}
	
	//resets the game
	if(gameInfo.reset == 1){
	    //cancels old game and npc threads
	    pthread_cancel(tid2);
	    pthread_cancel(tid5);
	    
	    //resetting game values
	    gameInfo.currScreen = 2;
	    gameInfo.numPressed = 0;
	    gameInfo.score = 0;
	    gameInfo.lives = 4;
	    gameInfo.timeLeft = 100;
	    gameInfo.moves = 100;
	    gameInfo.win = 0;
	    gameInfo.lose = 0;
	    gameInfo.end = 0;
	    gameInfo.xFrog = 624;
	    gameInfo.yFrog = 640; 
	    gameInfo.collison = 0;
	    gameInfo.screen = 0; 
	    gameInfo.justChanged = 0;  
	    gameInfo.reset = 0;
	    gameInfo.valueP = 0;
	    gameInfo.timeTillVP = 30;
	    
	    //creates new gameScreen and npc
	    pthread_create(&tid2, &attr, gameScreen, (void *)&gameInfo);
	    pthread_create(&tid5, &attr, gameAssets, (void *)&gameInfo);
	}
    }
    
    system("clear");
    pthread_exit(0);
}

//The main function that creates the main thread
int main(int argc, char *argv[]){
    framebufferstruct = initFbInfo();
    pthread_t mid;
    pthread_attr_t attr;
    
    //initializes a thread attribute
    pthread_attr_init(&attr);
    pthread_create(&mid, &attr, management, NULL);
    pthread_join(mid, NULL);
    
    return 0;
}
