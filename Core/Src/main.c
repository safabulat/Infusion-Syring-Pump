/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32g0xx_hal.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "math.h"
#include "bitmap.h"
#include "asciiFont.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#ifdef __GNUC__						//Printf function
  /* With GCC, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
	
	#ifdef __GNUC__
#define GETCHAR_PROTOTYPE int __io_getchar (void)
#else
#define GETCHAR_PROTOTYPE int fgetc(FILE * f)
#endif /* __GNUC__ */


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//*************************Vals
uint16_t Count=0;
uint32_t TIM_CLOCK=500000;
uint16_t Frequency=0;
uint16_t Arr=0;
uint16_t Psc=0;
uint32_t abcdsw=0;
float mili_to_height=0;
int open_tims=0;
	
uint16_t milliLiter, timSec, direcT = 0;
char mode[10];
//*************************UART
uint16_t UARTmenuBuff[1]={'0'};
uint8_t UARTmenuScale=0;
int UARTSetmenu=0;
uint8_t UARTSetmenuScale=0;

int UARTmenu=3;
int UARTtime, UARTdistance, UARTdirection, UARTSelection;
char syringe[10];
int selection=0;
float Syringe_volume=0;
float Syringe_radius, Syringe_height;
#define M_PI 3.14159265358979323846
//*************************GLCD
uint8_t startRow, startCol, endRow, endCol; // coordinates of the dirty rectangle
uint8_t numRows = 64;
uint8_t numCols = 128;
uint8_t Graphic_Check = 0;
uint8_t image[1024];
int cursor = 0;
int line, line_buffer = 0;
int main_menu ,sub_menu_1, sub_menu_2, sub_menu_3 = 0;
int row_tracer = 0;
int GLCDtime, GLCDdistance, GLCDdirection;
int GLCD_active = 0;
int GLCD_pump_active = 0;
//*************************Encoder
int encoder_pos = 0;
int encoder_button = 0;
int encoder_buffer = 0;
int encoder_last_post = 0;

int encoder_pos_right =0;
int encoder_pos_left =0;
//*************************Functions
//******************glcd functions***************************************//
void SendByteSPI(uint8_t byte)
{//Spi bridge
	HAL_SPI_Transmit(&hspi2, &byte, 1,0xFFFF);
}

void ST7920_SendCmd (uint8_t cmd)
{//Commands
	HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);  // PUll the CS high
	SendByteSPI(0xf8+(0<<1));  // send the SYNC + RS(0)
	SendByteSPI(cmd&0xf0);  // send the higher nibble first
	SendByteSPI((cmd<<4)&0xf0);  // send the lower nibble
	HAL_Delay(1);
	HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);  // PUll the CS LOW
}

void ST7920_SendData (uint8_t data)
{//Actual data
	HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);  // PUll the CS high
	SendByteSPI(0xf8+(1<<1));  // send the SYNC + RS(1)
	SendByteSPI(data&0xf0);  // send the higher nibble first
	SendByteSPI((data<<4)&0xf0);  // send the lower nibble
	HAL_Delay(1);
	HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);  // PUll the CS LOW
}

void ST7920_SendString(int row, int col, char* string)
{//For text mode: 4 row 20 char string
	switch (row)
	{
		case 0:
			col |= 0x80;
			break;
		case 1:
			col |= 0x90;
			break;
		case 2:
			col |= 0x88;
			break;
		case 3:
			col |= 0x98;
			break;
		default:
			col |= 0x80;
			break;
	}
	ST7920_SendCmd(col);

	while (*string){	
		ST7920_SendData(*string++);
    }
}
void ST7920_SendInteger(int row, int col, int integer)
{//For text mode: 4 row 20 char integer
	switch (row)
	{
		case 0:
			col |= 0x80;
			break;
		case 1:
			col |= 0x90;
			break;
		case 2:
			col |= 0x88;
			break;
		case 3:
			col |= 0x98;
			break;
		default:
			col |= 0x80;
			break;
	}
	ST7920_SendCmd(col);

		for(int q =0; q<sizeof(integer)-1; q++){		
		char numStr[sizeof(integer)-1];
		sprintf(numStr,"%d", integer);
		ST7920_SendData(numStr[q]);
    }	
}
void ST7920_GraphicMode (int enable)   // 1-enable, 0-disable
{//1: Graphic mode, 0: Text mode
	if (enable == 1)
	{
		ST7920_SendCmd(0x30);  // 8 bit mode
		HAL_Delay (1);
		ST7920_SendCmd(0x34);  // switch to Extended instructions
		HAL_Delay (1);
		ST7920_SendCmd(0x36);  // enable graphics
		HAL_Delay (1);
		Graphic_Check = 1;  // update the variable
	}

	else if (enable == 0)
	{
		ST7920_SendCmd(0x30);  // 8 bit mode
		HAL_Delay (1);
		Graphic_Check = 0;  // update the variable
	}
}

void ST7920_Init (void)
{//Initialization
	HAL_GPIO_WritePin(SPI_RST_GPIO_Port, SPI_RST_Pin, GPIO_PIN_RESET);  // RESET=0
	HAL_Delay(10);   // wait for 10ms
	HAL_GPIO_WritePin(SPI_RST_GPIO_Port, SPI_RST_Pin, GPIO_PIN_SET);  // RESET=1

	HAL_Delay(50);   //wait for >40 ms


	ST7920_SendCmd(0x30);  // 8bit mode
	HAL_Delay(15);  //  >100us delay

	ST7920_SendCmd(0x30);  // 8bit mode
	HAL_Delay(5);  // >37us delay

	ST7920_SendCmd(0x08);  // D=0, C=0, B=0
	HAL_Delay(15);  // >100us delay

	ST7920_SendCmd(0x01);  // clear screen
	HAL_Delay(12);  // >10 ms delay


	ST7920_SendCmd(0x06);  // cursor increment right no shift
	HAL_Delay(1);  // 1ms delay

	ST7920_SendCmd(0x0C);  // D=1, C=0, B=0
    HAL_Delay(1);  // 1ms delay

	ST7920_SendCmd(0x02);  // return to home
	HAL_Delay(1);  // 1ms delay
}

void ST7920_Clear()
{//Clear
	if (Graphic_Check == 1)  // if the graphic mode is set
	{
		uint8_t x, y;
		for(y = 0; y < 64; y++)
		{
			if(y < 32)
			{
				ST7920_SendCmd(0x80 | y);
				ST7920_SendCmd(0x80);
			}
			else
			{
				ST7920_SendCmd(0x80 | (y-32));
				ST7920_SendCmd(0x88);
			}
			for(x = 0; x < 8; x++)
			{
				ST7920_SendData(0);
				ST7920_SendData(0);
			}
		}
	}

	else
	{
		ST7920_SendCmd(0x01);   // clear the display using command
		HAL_Delay(2); // delay >1.6 ms
	}
}

void ST7920_DrawBitmap(const unsigned char* graphic)
{
	uint8_t x, y;
	for(y = 0; y < 64; y++)
	{
		if(y < 32)
		{
			for(x = 0; x < 8; x++)							// Draws top half of the screen.
			{												// In extended instruction mode, vertical and horizontal coordinates must be specified before sending data in.
				ST7920_SendCmd(0x80 | y);				// Vertical coordinate of the screen is specified first. (0-31)
				ST7920_SendCmd(0x80 | x);				// Then horizontal coordinate of the screen is specified. (0-8)
				ST7920_SendData(graphic[2*x + 16*y]);		// Data to the upper byte is sent to the coordinate.
				ST7920_SendData(graphic[2*x+1 + 16*y]);	// Data to the lower byte is sent to the coordinate.
			}
		}
		else
		{
			for(x = 0; x < 8; x++)							// Draws bottom half of the screen.
			{												// Actions performed as same as the upper half screen.
				ST7920_SendCmd(0x80 | (y-32));			// Vertical coordinate must be scaled back to 0-31 as it is dealing with another half of the screen.
				ST7920_SendCmd(0x88 | x);
				ST7920_SendData(graphic[2*x + 16*y]);
				ST7920_SendData(graphic[2*x+1 + 16*y]);
			}
		}

	}
}

//******************encoder functions************************************//
void encoder_movements(){
//if(cursor == 1){
	if((HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port,ENC_BTN_Pin) == 0) || (line_buffer == 2)){
		while(HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port,ENC_BTN_Pin) == 0);
//		if(encoder_button == 0){
		if(encoder_button == 1){
			if(line == 1){
				main_menu = 1;
			}
			else if(line == 2){
				main_menu = 2;
			}
			else{
				main_menu = 0;
				encoder_button=0;
			}
		}
		if((encoder_button == 2) && (main_menu == 3)){
			if(line == 0){
				sub_menu_1 = 0;
				main_menu = 0;
			}			
			else if(line == 1){
				sub_menu_1 = 1;
				main_menu = 0;
			}
			else if(line == 2){
				sub_menu_1 = 2;
				main_menu = 0;
			}
			else if(line == 3){
				sub_menu_1 = 3;
				main_menu = 0;

			}
			else{
				encoder_button=1;	
				main_menu = 1;
			}
		}	
		if((encoder_button == 3) && (sub_menu_1 == 5)){
			if(line == 1){
				sub_menu_2 = 0;
				main_menu = 0;
			}
			else if(line == 2){
				sub_menu_2 = 1;
				main_menu = 0;
			}
			else if(line == 3){
				sub_menu_1 = 0;
				sub_menu_2 = 0;
				encoder_button = 1;
				main_menu = 1;
			}
			else{
				encoder_button=2;
				sub_menu_1 = 0;
				main_menu = 0;				
			}		
		}
		else if((encoder_button == 3) && (sub_menu_1 == 6)){
			if(line == 2){
				sub_menu_2 = 2;
				main_menu = 0;
				line_buffer = 2;
			}
			else if(line == 3){
				sub_menu_1 = 0;
				sub_menu_2 = 0;
				encoder_button = 1;
				main_menu = 1;
			}			
			else{
				encoder_button=2;
				sub_menu_1 = 1;
				main_menu = 0;				
			}		
		}
		else if((encoder_button == 3) && (sub_menu_1 == 7)){
			if(line == 2){
				sub_menu_2 = 3;
				main_menu = 0;
				line_buffer = 2;
			}
			else if(line == 3){
				sub_menu_1 = 0;
				sub_menu_2 = 0;
				encoder_button = 1;
				main_menu = 1;
			}			
			else{
				encoder_button=2;
				sub_menu_1 = 2;
				main_menu = 0;				
			}				
		}
		else if((encoder_button == 3) && (sub_menu_1 == 8)){
			if(line == 2){
				sub_menu_2 = 4;
				main_menu = 0;
				line_buffer = 2;
			}
			else if(line == 3){
				sub_menu_1 = 4;
				sub_menu_2 = 0;
				encoder_button = 2;
				main_menu = 0;
			}			
			else{
				encoder_button=2;
				sub_menu_1 = 3;
				main_menu = 0;				
			}				
		}
		if((encoder_button == 4) && (sub_menu_2 == 5)){
			if(line == 1){
				sub_menu_3 = 0;////////////////////////activate the selection
				main_menu = 0;
			}			
			else if(line == 2){
				sub_menu_3 = 1;////////////////////////activate the selection
				main_menu = 0;
			}
			else if(line == 3){
				encoder_button=1;	
				main_menu = 1;
			}
			else{
				encoder_button=3;	
				sub_menu_2 = 0;
			}			
		}
		else if((encoder_button == 4) && (sub_menu_2 == 6)){
			if(line == 1){
				sub_menu_1 = 0;////////////////////////activate the selection cursor
				main_menu = 0;
			}			
			else if(line == 2){
				sub_menu_1 = 1;////////////////////////activate the selection cursor
				main_menu = 0;
			}
			else if(line == 3){
				encoder_button=1;	
				main_menu = 1;
			}
			else{
				encoder_button=3;	
				sub_menu_2 = 1;
			}			
		}
		else if((encoder_button == 4) && (sub_menu_2 == 7)){
			if(line_buffer == 2){
				ST7920_SendInteger(2, 4, line); //////////buradan devam
				HAL_Delay(1);
				main_menu = 0;
				encoder_button = 3;
				sub_menu_2 = 7;
				line_buffer = 2;
			}
			else if(line == 3){
				encoder_button=1;	
				main_menu = 1;
			}
			else{
				encoder_button=3;	
				sub_menu_2 = 2;
			}				
		}
		else if((encoder_button == 4) && (sub_menu_2 == 8)){
			if(line_buffer == 2){
				ST7920_SendInteger(2, 4, line); //////////buradan devam
				HAL_Delay(1);
				main_menu = 0;
				encoder_button = 3;
				sub_menu_2 = 8;
				line_buffer = 2;
			}
			else if(line == 3){
				encoder_button=1;	
				main_menu = 1;
			}
			else{
				encoder_button=3;	
				sub_menu_2 = 3;
			}			
		}
		else if((encoder_button == 4) && (sub_menu_2 == 9)){
			if(line_buffer == 2){
				ST7920_SendInteger(2, 4, line); //////////buradan devam
				HAL_Delay(1);
				main_menu = 0;
				encoder_button = 3;
				sub_menu_2 = 9;
				line_buffer = 2;
			}
			else if(line == 3){
				encoder_button=2;	
				sub_menu_1 =3;
				main_menu = 1;
			}
			else{
				encoder_button=3;	
				sub_menu_2 = 4;
			}			
		}		
		encoder_button++;
		if((HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port,ENC_BTN_Pin) == 0) && (line_buffer == 2)){
			line_buffer = 0;
			row_tracer = 0;
		}
//		else if(encoder_button == 1){
//			encoder_button = 0;
//		}
////////////////////////////////////
		encoder_buffer = HAL_GPIO_ReadPin(ENC_2_GPIO_Port, ENC_2_Pin);
		if(encoder_buffer != encoder_last_post){ //rotating
		
			if(HAL_GPIO_ReadPin(ENC_1_GPIO_Port, ENC_1_Pin) != encoder_buffer){ //counter clockwise			
				encoder_pos--;
				encoder_pos_left = 1;
				encoder_pos_right =0;
				
			}
			else{
				encoder_pos++;//clockwise
				encoder_pos_left = 0;
				encoder_pos_right =1;
			}	
		}
		encoder_last_post = encoder_buffer;		
////////////////////////////////		
	}
	
	else{
		
		encoder_buffer = HAL_GPIO_ReadPin(ENC_2_GPIO_Port, ENC_2_Pin);
		if(encoder_buffer != encoder_last_post){ //rotating
		
			if(HAL_GPIO_ReadPin(ENC_1_GPIO_Port, ENC_1_Pin) != encoder_buffer){ //counter clockwise			
				encoder_pos--;
				encoder_pos_left = 1;
				encoder_pos_right =0;
				
			}
			else{
				encoder_pos++;//clockwise
				encoder_pos_left = 0;
				encoder_pos_right =1;
			}	
		}
		encoder_last_post = encoder_buffer;
	}
//}
//else if((HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port,ENC_BTN_Pin) == 0) && (cursor == 0)){
//	cursor = 1;
//}
}


//******************fusion functions***************************************//
void SetFrequency(uint16_t Freq){ 
	
	Arr=(TIM_CLOCK/Freq);
	TIM3->CCMR2 = TIM_OCMODE_TOGGLE;
	if(Arr > 65535){
	Psc = (TIM_CLOCK/(Freq*65536))-1;
	Arr = 65535;
	TIM3->ARR = Arr;
	TIM3->CCR3 = (Arr*5)/10;
		
	TIM3->PSC=Psc;
	}
	else{
	
	TIM3->PSC = 0;
	TIM3->ARR = Arr;
	TIM3->CCR3 = (Arr*5)/10;	
	}
}

void fusion(uint8_t mL, uint8_t sec, uint8_t dir){ // 1. Volume, 2. Time, 3. Mode
	int freq=0; 
	mili_to_height= ((float)mL)/((float)M_PI*Syringe_radius*Syringe_radius);
	// 0.2cm = 1 revolution = 3200*(2/10) pulse = 640 pulse
	if(dir == 1){
		HAL_GPIO_WritePin(DIRR_GPIO_Port, DIRR_Pin, GPIO_PIN_SET);	
		strcpy(mode, "Re-Load");
		if(open_tims == 0){
			HAL_TIM_Base_Start_IT(&htim2);
			HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_3);
			open_tims++;
		}
		while( Count < sec+1){
			freq= round((mili_to_height*640)/sec);
			SetFrequency(freq);
		}
		HAL_TIM_Base_Stop_IT(&htim2);
		HAL_TIM_OC_Stop(&htim3, TIM_CHANNEL_3);
		Count=0;
		open_tims=0;
		TIM2->EGR |= (1U <<0);
	}
	else if(dir == 2){
		HAL_GPIO_WritePin(DIRR_GPIO_Port, DIRR_Pin, GPIO_PIN_RESET);
		strcpy(mode, "Inject");
	if(open_tims == 0){
			HAL_TIM_Base_Start_IT(&htim2);
			HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_3);
			open_tims++;
		}
		while( Count < sec+1){
			freq= round((mili_to_height*640)/sec);
			SetFrequency(freq);
		}
		HAL_TIM_Base_Stop_IT(&htim2);
		HAL_TIM_OC_Stop(&htim3, TIM_CHANNEL_3);
		Count=0;
		open_tims=0;
		TIM2->EGR |= (1U <<0);
		
	}
	else{
		HAL_TIM_Base_Stop_IT(&htim2);
		HAL_TIM_OC_Stop(&htim3, TIM_CHANNEL_3);
		Count=0;
		open_tims=0;
		TIM2->EGR |= (1U <<0);
	}
}

void setSyringe(int Selection){
	switch(Selection){
		case 0:{
			Syringe_volume= ((float)M_PI*Syringe_radius*Syringe_radius)*(Syringe_height);
			break;
		}
		case 1:{
			Syringe_volume=100;
			Syringe_radius=(float)1.45673124079;
			Syringe_height=15;
			break;
		}
		case 2:{
			Syringe_volume=50;
			Syringe_radius=(float)1.41047395887;
			Syringe_height=8;
			break;
		}
		case 3:{
			Syringe_volume=25;
			Syringe_radius=0.40;
			Syringe_height=4;
			break;
		}
		case 4:{
			Syringe_volume=5;
			Syringe_radius=(float)0.62304316706;
			Syringe_height=4.1;
			break;
		}
		case 5:{
			Syringe_volume=2;
			Syringe_radius=0.40;
			Syringe_height=4;
			break;
		}
		default:{
			Syringe_volume=50;
			Syringe_radius=(float)1.41047395887;
			Syringe_height=8;
			break;
		}
	}
}


//******************menu functions*****************************************//
//**********************************************GLCD
void GLCDmenu(){
//	if(HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port,ENC_BTN_Pin) == 0){cursor =1;}
	encoder_movements();

		
		if((encoder_button == 1) && (main_menu == 0)){//standby
			ST7920_Clear();
			HAL_Delay(1);

			ST7920_SendString(0,0,"......MENU......");
			ST7920_SendString(1,0,"Program!        "); //encoder_button = 2   , main menu = 1
			ST7920_SendString(2,0,"Kill!           "); //encoder_button = 2   , main menu = 2
			if(GLCD_pump_active == 1){
				ST7920_SendString(3,0,"status: active !"); //encoder_button = dont_care
				HAL_Delay(GLCDtime*1000);		
			}
			else{
				ST7920_SendString(3,0,"status: deactive"); //encoder_button = dont_care
			}
			ST7920_SendCmd(0x80);
			HAL_Delay(1);
			
			sub_menu_1=0;
			sub_menu_2=0;
			sub_menu_3=0;
			main_menu =3;
//			GLCD_pump_active = 0;
		}
		if((encoder_button == 2) && (main_menu == 1)){//program menu
			ST7920_Clear();
			HAL_Delay(1);
			
			ST7920_SendString(0,0,"Select Syringe"); //go to sub_menu1 , 0
			ST7920_SendString(1,0,"Set Volume");		 //go to sub_menu1 , 1
			ST7920_SendString(2,0,"Set Time");			 //go to sub_menu1 , 2
			ST7920_SendString(3,0,"Done.");					 //go to sub_menu1 , 3
			ST7920_SendCmd(0x80);
			HAL_Delay(1);
			line=0;
			main_menu =3;
		}	
		if((encoder_button == 2) && (main_menu == 2)){//kill menu
			ST7920_Clear();
			HAL_Delay(1);
			
			ST7920_SendString(0,0,"Emergancy Stop !");
			ST7920_SendString(1,0,"Pressed.       !");
			ST7920_SendString(2,0,"Process KILLED.!");
			ST7920_SendString(3,0,"Time left:      ");
			ST7920_SendCmd(0x80);
			HAL_Delay(3000);
			line=0;
			encoder_button=1;
			main_menu=0;
			sub_menu_1=0;
//			ST7920_SendCmd(0xC);
			HAL_Delay(5);
		}			
		if((encoder_button == 3) && (sub_menu_1 == 0)){//Select Syringe
			ST7920_Clear();
			HAL_Delay(1);

			ST7920_SendString(0,0,"Syringe:        ");
			ST7920_SendString(1,0,"Make a custom?  "); //go to sub_menu_2 , 0 
			ST7920_SendString(2,0,"Use a Premade?  "); //go to sub_menu_2 , 1 
			ST7920_SendString(3,0,"Done.           "); //turn back
			ST7920_SendCmd(0x80);
			HAL_Delay(1);
			line=0;
			sub_menu_1 = 5;
		}
		if((encoder_button == 3) && (sub_menu_1 == 1)){//Select Volume
			ST7920_Clear();
			HAL_Delay(1);
			ST7920_SendString(0,0,"Volume:         ");
			ST7920_SendString(1,0,"Max: 5 mL       ");
			ST7920_SendString(2,0,"Enter:          "); //go to sub_menu_2 , 2
			ST7920_SendString(3,0,"Done.           "); //turn back
			ST7920_SendCmd(0x80);
			HAL_Delay(1);
			line=0;
			sub_menu_1 = 6;
		}
		if((encoder_button == 3) && (sub_menu_1 == 2)){//Set Time
			ST7920_Clear();
			HAL_Delay(1);
			ST7920_SendString(0,0,"Time:           ");
			ST7920_SendString(1,0,"Max: 10000000000");
			ST7920_SendString(2,0,"Enter:          "); //go to sub_menu_2 , 3
			ST7920_SendString(3,0,"Done.           "); //turn back
			ST7920_SendCmd(0x80);
			HAL_Delay(1);
			line=0;
			sub_menu_1 = 7;
		}		
		if((encoder_button == 3) && (sub_menu_1 == 3)){//Pump Programed
			ST7920_Clear();
			HAL_Delay(1);
			ST7920_SendString(0,0,"Pump Programed !");
			ST7920_SendString(1,0,"Select a mode  !");
			ST7920_SendString(2,0,"Mode:           "); // go to sub_menu_2 , 4
			if(GLCDdirection == 1){
				ST7920_SendString(2,4,"Inject");
			}
			else if(GLCDdirection == 2){
				ST7920_SendString(2,4,"Re-Load");
			}
			ST7920_SendString(3,0,"Done.           "); // go pump. sub_menu_1 , 4
			ST7920_SendCmd(0x80);
			HAL_Delay(1);
			line=0;
			sub_menu_1 = 8;
		}			
		if((encoder_button == 3) && (sub_menu_1 == 4)){//Pump Active
			ST7920_Clear();
			HAL_Delay(1);
			ST7920_SendCmd(0xC);
			HAL_Delay(5);
			ST7920_SendString(0,0,"Pump is Active !");
			ST7920_SendString(1,0,"Vol:");
			ST7920_SendInteger(1,4,GLCDdistance);
			ST7920_SendString(2,0,"Time:"); // go to sub_menu_2 , 4
			ST7920_SendInteger(2,4,GLCDtime);
			ST7920_SendString(3,0,"Mode:"); // go pump.
			
			if(GLCDdirection == 1){
				ST7920_SendString(3,4,"Inject");
			}
			else if(GLCDdirection == 2){
				ST7920_SendString(3,4,"Re-Load");
			}
//			ST7920_SendCmd(0x80);
			HAL_Delay(1000);
			ST7920_Clear();
			HAL_Delay(1);

			line=0;
			encoder_button=1;
			main_menu=0;
			sub_menu_1=0;
			sub_menu_2=0;
			HAL_Delay(5);
			GLCD_pump_active = 1;
			milliLiter = GLCDdistance;
			timSec = GLCDtime;
			direcT = GLCDdirection;
			
			ST7920_SendString(0,0,"......MENU......");
			ST7920_SendString(1,0,"Program!        "); //encoder_button = 2   , main menu = 1
			ST7920_SendString(2,0,"Kill!           "); //encoder_button = 2   , main menu = 2
			if(GLCD_pump_active == 1){
				ST7920_SendString(3,0,"status: active !"); //encoder_button = dont_care
			}
			else{
				ST7920_SendString(3,0,"status: deactive"); //encoder_button = dont_care
			}

		}	
		if((encoder_button == 4) && (sub_menu_2 == 0)){//Create a syringe
			ST7920_Clear();
			HAL_Delay(1);

			ST7920_SendString(0,0,"Create a Syringe");
			ST7920_SendString(1,0,"Height:         "); //go to sub_menu_3 , 0 
			ST7920_SendString(2,0,"Radius:         "); //go to sub_menu_1 , 1 
			ST7920_SendString(3,0,"Done.           "); //go back
			ST7920_SendCmd(0x80);
			HAL_Delay(1);
			line=0;
			sub_menu_2 = 5;			
		}		
		if((encoder_button == 4) && (sub_menu_2 == 1)){//Syrince entrance
			ST7920_Clear();
			HAL_Delay(1);

			ST7920_SendString(0,0,"Select a Syringe");
			ST7920_SendString(1,0,"Default, 2, 5   "); //set selected
			ST7920_SendString(2,0,"  25, 50, 100   "); //set selected 
			ST7920_SendString(3,0,"Done.           "); //go back
			ST7920_SendCmd(0x80);
			HAL_Delay(1);
			line=0;
			sub_menu_2 = 6;			
		}	
		if((encoder_button == 4) && (sub_menu_2 == 2)){//Volume entrance
			if(line_buffer == 2){
				row_tracer = 1;
			}
			ST7920_Clear();			
			HAL_Delay(1);
			ST7920_SendString(0,0,"Volume:         ");
			ST7920_SendString(1,0,"Max: selected?  ");
			ST7920_SendString(2,0,"Enter:          "); //select
			ST7920_SendInteger(2,4, line);
			GLCDdistance = line;
			ST7920_SendString(3,0,"Done.           "); //go back
			HAL_Delay(1);
			sub_menu_2 = 7;			
		}	
		if((encoder_button == 4) && (sub_menu_2 == 3)){//Time entrance
			if(line_buffer == 2){
				row_tracer = 1;
			}
			ST7920_Clear();
			HAL_Delay(1);
			ST7920_SendString(0,0,"Time:           ");
			ST7920_SendString(1,0,"Max: 10000000000");	
			ST7920_SendString(2,0,"Enter:"); //select
			ST7920_SendInteger(2,4, line);
			GLCDtime = line;
			ST7920_SendString(3,0,"Done.           "); //go back	
			HAL_Delay(1);		
			sub_menu_2 = 8;			
		}	
		if((encoder_button == 4) && (sub_menu_2 == 4)){//Select Mode
			if(line_buffer == 2){
				row_tracer = 1;
			}
			ST7920_Clear();
			HAL_Delay(1);
			ST7920_SendString(0,0,"Select pump mode");
			ST7920_SendString(1,0,"Infusion/Re-Load");
			ST7920_SendString(2,0,"Mode:           "); // select
			ST7920_SendInteger(2,4, line);
			GLCDdirection = line;
			ST7920_SendString(3,0,"Done.           "); // go back
			HAL_Delay(1);
			sub_menu_2 = 9;			
		}		

		if(cursor == 0){ // && (encoder_button == 1))
			cursor = 1;
			ST7920_SendCmd(0xF);
			HAL_Delay(5);
		}
		else if(cursor == 1){ // && (encoder_button == 0))
			cursor = 0;
			ST7920_SendCmd(0xC);
			HAL_Delay(5);
		}
//	}	
	if(encoder_pos_right == 1){
		line++;
		encoder_pos_right=0;
		if(row_tracer == 0){
			if(line == 0){ST7920_SendCmd(0x80);}
			else if(line == 1){ST7920_SendCmd(0x90);}
			else if(line == 2){ST7920_SendCmd(0x88);}
			else if(line == 3){ST7920_SendCmd(0x98);}
			if(line > 3.5){line=0; ST7920_SendCmd(0x80);}
			encoder_pos_right = 0;
		}
	}
	else if(encoder_pos_left == 1){
		line--;
		encoder_pos_left = 0;
		if(row_tracer == 0){
			if(line == 0){ST7920_SendCmd(0x80);}
			else if(line == 1){ST7920_SendCmd(0x90);}
			else if(line == 2){ST7920_SendCmd(0x88);}
			else if(line == 3){ST7920_SendCmd(0x98);}
			if(line < -0.5){line=3; ST7920_SendCmd(0x98);}
			encoder_pos_left = 0;
		}
	}

}
//**********************************************UART
void UARTupMenu(){
		if(UARTmenu==1){
			//***For entering the SetMenu(s)*****...gets UARTSetmenu input but doesn't wait........
			printf("\n\n\rFusion Set Menu:\n\r0.selectSyringe\n\r1.setVolume\n\r2.setTime\n\r3.setDirection\n\r4.Exit\n\t");
			memset(&UARTSetmenu, '\0', sizeof(UARTSetmenu));
			HAL_UART_Receive(&huart1, (uint8_t *)&UARTSetmenuScale, 1, 0xFFFF);
			UARTSetmenu=UARTSetmenuScale-48; 
			//*************************************************************************************
			if(UARTSetmenu==0)//Set Syringe
			{

				//***Menu for setting the Volume (Milli liter)********************

				printf("\n\rSelect Syringe Model: \n");
				printf("\n\r0- Custom\n\r1- 100mL\n\r2- 50mL\n\r3- 25mL\n\r4- 5mL\n\r5- 2mL\n\rSelection: ");			
				scanf("%d",&UARTSelection);
				selection=UARTSelection;
				if(!((selection==0)||(selection==1)||(selection==2)||(selection==3)||(selection==4)||(selection==5))){
					printf("\n\rInvalid Selection...\n\r");	
					printf("\n\rSyringe selected 50mL by default.\n\r");					
				}
				if(selection == 0){
					printf("\n\rEnter radius (in centimeter): ");
					scanf("%f",&Syringe_radius);
					printf("\n\rRadius set to %.2f cm." , Syringe_radius);
					
					printf("\n\rEnter height (in centimeter): ");
					scanf("%f",&Syringe_height);
					printf("\n\rRadius set to %.2f cm." , Syringe_height);
					
					printf("\n\rCustom Syringe is created.\n\r");
					
				}
				if(selection == 0){ strcpy(syringe, "Custom Syringe");}
				else if(selection == 1){ strcpy(syringe, "100 mL Syringe"); }
				else if(selection == 2){ strcpy(syringe, "50 mL Syringe");}
				else if(selection == 3){ strcpy(syringe, "25 mL Syringe");}
				else if(selection == 4){ strcpy(syringe, "5 mL Syringe");}
				else if(selection == 5){ strcpy(syringe, "2 mL Syringe");}
				else{strcpy(syringe, "50mL by default");}
				
				setSyringe(selection);
				printf("\n\rSyringe model set to: %s\n\r\t %.4f mL; with %.2f cm radius and %.2f cm height\n\r",
				syringe, Syringe_volume, Syringe_radius, Syringe_height);	//Prints set volume
				
				UARTSetmenu++;																//Goes to the next menu automatically 
			}
			
			if(UARTSetmenu==1)//Set Volume
			{
				//***Menu for setting the Volume (Milli liter)********************
				volume_enter:
				printf("\n\rEnter Volume: ");		
				scanf("%d",&UARTdistance);
				milliLiter = UARTdistance;
				if(milliLiter > Syringe_volume){
					printf("\n\rYou can't enter volume greater then Syringes volume...\n\r");	
					goto volume_enter;
				}
				printf("\nVolume set to: %d Milliliter.\n",milliLiter);	//Prints set volume
				UARTSetmenu++;																//Goes to the next menu automatically 
			}
			
			if(UARTSetmenu==2)//Set Distance
			{
				//***Menu for setting the distance (milimeter)*****************
				time_enter:
				printf("\n\rEnter Time: ");		
				scanf("%d",&UARTtime);
				timSec = UARTtime;
				if(0 >= timSec){
					printf("\n\rYou can't enter Invalid time...\n\r");	
					goto time_enter;
				}
				printf("\nTime set to: %d Seconds.\n",timSec);	//Prints set time and distance
				UARTSetmenu++;																//Goes to the next menu automatically 
			}
			if(UARTSetmenu==3)//Set Direction
			{
				//***Menu for setting the direction (CW - CCW)*****************
				direction_enter:
				printf("\n\rEnter Direction 1(injection) or 2(re-load): ");		
				scanf("%d",&UARTdirection);
				direcT = UARTdirection;
				if( !((direcT == 1) || (direcT == 2)) ){
					printf("\n\rYou can't enter Invalid direction...\n\r");	
					goto direction_enter;
				}
				
				if(direcT == 1){ strcpy(mode, "Inject"); }
				else if(direcT == 2){ strcpy(mode, "Re-Load");}
				else { strcpy(mode, "Error");}
				printf("\nDirection set to: %s\n", mode);	//Prints set time and distance
				//Prints the fusion to the screen:
				printf("\n\rFusion: ON, Volume: %d mL, Time: %d sec, Mode: %s\n\r", milliLiter, timSec, mode);

				UARTSetmenu++;																//Goes to the next menu automatically 
			}
			if(UARTSetmenu==4)//Exit
			{
				//***Exit Menu for making the Clock ON********************
				memset(&UARTSetmenu, '\0', sizeof(UARTSetmenu));				
				UARTmenu=0;
				printf("\n\rExit Settings, Fusion:ON\n\n\r");
			}

		}
		else if(UARTmenu==2){ //Emergancy Stop
			printf("\n\r\t!!! Emergancy Stop: ON !!!\n\r");
			HAL_TIM_OC_Stop(&htim3, TIM_CHANNEL_1);
			HAL_TIM_Base_Stop(&htim2);
			UARTmenu=0;
		} 
		else{
		//If anything comes except UARTmenu tabs, it becomes 0, and fusion continues..
		UARTmenu=0;
		}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
	encoder_last_post = HAL_GPIO_ReadPin(ENC_2_GPIO_Port, ENC_2_Pin);		//encoder start!
	ST7920_Init();
	HAL_Delay(2); // delay >1.6 ms
//******Fusion Stuff*****************************************************************************************//
	setSyringe(4);
//	HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_3);
	GLCD_active = 0;
	HAL_GPIO_WritePin(DIRR_GPIO_Port, DIRR_Pin, GPIO_PIN_SET);	//set direction to Pull
	HAL_GPIO_WritePin(MS_PINS_GPIO_Port, MS_PINS_Pin, GPIO_PIN_RESET);	//set direction to Pull
	printf("\n\rInfusion pump is gettin ready.\n\r");
	ST7920_SendString(0,0,"  Infusion Pump ");
	ST7920_SendString(1,0,"       Is       ");
	ST7920_SendString(2,0,"  Getting Ready ");
	ST7920_SendString(3,0,"       !!       ");
	HAL_Delay(1000);
	fusion(0,0,0);
	printf("\rPump is ready..!\n");
	ST7920_Clear();
	ST7920_SendString(0,0,"  Infusion Pump ");
	ST7920_SendString(1,0,"       Is       ");
	ST7920_SendString(2,0,"      Ready     ");
	ST7920_SendString(3,0,"Press the button");
	HAL_Delay(100);
//	starto:
//	jumpback:
//	printf("\rEnter Menu Key: ");
//	if(GLCD_active == 0){
//		memset(&UARTSetmenu, '\0', sizeof(UARTSetmenu));

//		HAL_UART_Receive(&huart1, (uint8_t *)&UARTmenuScale, 1, 0xFFFF);
//		UARTmenu=UARTmenuScale-48; 
//		
//		if (UARTmenu == 1 || UARTmenu == 2){ goto jump; }
//		else{	printf("\n\rInvalid selection.\n\r"); goto starto; }
//	}
//	else{
//		goto while_glcd;
//		goto jump;
//	}

//******Fusion Stuff END*****************************************************************************************//
//	fusion(3, 10, 1);	

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//		while_glcd:
		GLCDmenu();
		if((UARTmenu == 0) || (GLCD_pump_active == 1)){
			fusion(milliLiter, timSec, direcT);		// fusion
			HAL_Delay(GLCDtime*1000);
			GLCD_pump_active = 0;
			
//			goto jumpback;
		}

//		GLCDmenu();	
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


//******Fusion Stuff******************************************//
		}
//	jump:
//			if(GLCD_active == 0){
//			UARTupMenu();	
//			}
//			else{
//			GLCDmenu();
//			}
//		if((UARTmenu == 0) || (GLCD_pump_active == 1)){
//			fusion(milliLiter, timSec, direcT);		// fusion
//			goto jumpback;
//		}
	
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV16;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV16;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the peripherals clocks
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 500-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_FORCED_ACTIVE;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SPI_CS_Pin|SPI_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, MS_PINS_Pin|DIRR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : SPI_CS_Pin SPI_RST_Pin */
  GPIO_InitStruct.Pin = SPI_CS_Pin|SPI_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PF2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : MS_PINS_Pin DIRR_Pin */
  GPIO_InitStruct.Pin = MS_PINS_Pin|DIRR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : Led_Pin */
  GPIO_InitStruct.Pin = Led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Led_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ENC_2_Pin ENC_BTN_Pin ENC_1_Pin */
  GPIO_InitStruct.Pin = ENC_2_Pin|ENC_BTN_Pin|ENC_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

}

/* USER CODE BEGIN 4 */
PUTCHAR_PROTOTYPE									//printf def to uart
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

GETCHAR_PROTOTYPE
{
  uint8_t ch = 0;
  // Clear the Overrun flag just before receiving the first character
  __HAL_UART_CLEAR_OREFLAG(&huart1);
 
  HAL_UART_Receive(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
  //HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_TIM_PeriodElapsedCallback could be implemented in the user file
   */
	
	if(htim->Instance == TIM2) // 1 sec
  {
		Count++;
		HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
	}
	
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(GPIO_Pin);
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */

	
	if(!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)){
	//HAL_GPIO_WritePin(GPIOA, STEP_Pin, GPIO_PIN_RESET);
		TIM3->CCR3=0;	
		TIM2->CCR1=0;	
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
