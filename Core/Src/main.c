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
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include"u8g2.h"
#include <stdio.h>
#include<math.h>
#include <string.h>
#include <stdlib.h>
#include "uart_console.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char rx_data[1000];
float sampleRate=11.851f,debug_duty=50;
volatile config_t config={
        .isConsoleEnable=1,
        .isPIDPrint=1,
        .isDebugPWM=0
};
uint32_t ADC_Value[10] = {0};
float duty = 0;
float V_cur = 0, V_set = 0;
u8g2_t u8g2;
//buffer starts
uint8_t first[14] = {0};
uint8_t second[14] = {0};
uint8_t resault[14] = {0};
char menu[16][2] = {
        "7", "8", "9", "/",
        "4", "5", "6", "x",
        "1", "2", "3", "-",
        "0", ".", "=", "+"
};
char sign[2];
uint8_t line = 1;
uint8_t model = 1;
uint8_t state = 1;
typedef enum {
    M_input = 1,
    M_output
} Mode;
int isOutput = 0;
//state表：
//  输入  1
//   +    2
//   -    3
//   x    4
//   /    5
//  输出  6
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
int UART_printf(UART_HandleTypeDef *huart, const char *fmt, ...);

uint8_t u8x8_stm32_gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8,
                                  U8X8_UNUSED uint8_t msg,
                                  U8X8_UNUSED uint8_t arg_int,
                                  U8X8_UNUSED void *arg_ptr);

uint8_t u8x8_byte_4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);


char *strcat(char *destination, const char *source);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
typedef struct {
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;
} GPIO_Pin_TypeDef;

GPIO_Pin_TypeDef keyboardRow[] = {
        {KB_R3_GPIO_Port, KB_R3_Pin},
        {KB_R2_GPIO_Port, KB_R2_Pin},
        {KB_R1_GPIO_Port, KB_R1_Pin},
        {KB_R0_GPIO_Port, KB_R0_Pin},
};

GPIO_Pin_TypeDef keyboardCol[] = {
        {KB_C3_GPIO_Port, KB_C3_Pin},
        {KB_C2_GPIO_Port, KB_C2_Pin},
        {KB_C1_GPIO_Port, KB_C1_Pin},
        {KB_C0_GPIO_Port, KB_C0_Pin},
};

uint32_t KeyboardScan(void) {
    uint32_t result = 0;
    for (int row = 0; row < sizeof(keyboardRow) / sizeof(GPIO_Pin_TypeDef); row++) {
        HAL_GPIO_WritePin(keyboardRow[row].GPIOx, keyboardRow[row].GPIO_Pin, GPIO_PIN_SET);
        for (int col = 0; col < sizeof(keyboardCol) / sizeof(GPIO_Pin_TypeDef); col++) {
            result <<= 1;
            result |= HAL_GPIO_ReadPin(keyboardCol[col].GPIOx, keyboardCol[col].GPIO_Pin) == GPIO_PIN_SET;
        }
        HAL_GPIO_WritePin(keyboardRow[row].GPIOx, keyboardRow[row].GPIO_Pin, GPIO_PIN_RESET);
    }

    return result;
}

//u8g2相关内容

void frame(u8g2_t *u8g2, uint8_t Vset_buf[], uint8_t Vout_buf[], float Vset, int model);

void frameutf8(u8g2_t *u8g2, uint8_t first[], uint8_t second[], uint8_t out[]);

uint8_t decode(uint16_t data, uint8_t pos);

void operate(uint8_t *first) {
    uint32_t index = KeyboardScan();

    while (KeyboardScan()) {

    }
    for (int i = 0; i < 16; i++) {
        if (index & (1 << i)) {
            strcat(first, menu[i]);
        }
    }


}


void countmodel(uint8_t *buffer) {
    int i = 0;
    while (buffer[i++]) {
        if (buffer[i] == '.')
            model = _float;
    }
}

int str2value_int(uint8_t *buffer) {
    int len = 0;
    int sum = 0;
    int i = 0;
    while (buffer[i++]) {

        sum = sum * 10 + buffer[i - 1] - '0';

    }
    return sum;
}

void value2str_int(uint8_t *buffer, int value) {
    int m, len = 0;
    if (value >= 0) {
        do {
            m = value % 10;
            len = 0;
            while (buffer[len++]);
            len--;
            for (; len > 0; len--) {
                buffer[len] = buffer[len - 1];
            }
            buffer[0] = m + '0';
            value /= 10;

        } while (value);
    } else {
        value = -value;
        do {
            m = value % 10;
            len = 0;
            while (buffer[len++]);
            len--;
            for (; len > 0; len--) {
                buffer[len] = buffer[len - 1];
            }
            buffer[0] = m + '0';
            value /= 10;

        } while (value);
        len = 0;
        while (buffer[len++]);
        len--;
        for (; len > 0; len--) {
            buffer[len] = buffer[len - 1];
        }
        buffer[0] = '-';

    }

}

//copy de demo
float str2value_float(char *num) {

    double n = 0, sign = 1, scale = 0;
    int subscale = 0, signsubscale = 1;

    if (*num == '-') {
        sign = -1, num++;    /* Has sign? */
    }

    while (*num == '0') {
        num++;
    }

    if (*num >= '1' && *num <= '9') {
        do {
            n = (n * 10.0) + (*num++ - '0');
        } while (*num >= '0' && *num <= '9');    /* Number? */
    }

    if (*num == '.' && num[1] >= '0' && num[1] <= '9') {
        num++;
        do {
            n = (n * 10.0) + (*num++ - '0'), scale--;
        } while (*num >= '0' && *num <= '9');
    }    /* Fractional part? */

    if (*num == 'e' || *num == 'E') {    /* Exponent? */
        num++;
        if (*num == '+') {
            num++;
        } else if (*num == '-') {
            signsubscale = -1, num++;        /* With sign? */
        }

        while (*num >= '0' && *num <= '9') {
            subscale = (subscale * 10) + (*num++ - '0');    /* Number? */
        }
    }

    n = sign * n *
        pow(10.0, (scale + subscale * signsubscale));    /* number = +/- number.fraction * 10^ +/- exponent */

    return n;
}

void value2str_float(char *buffer, float slope)  //浮点型数，存储的字符数组，字符数组的长度
{
    // buffer[0]=' ';
    int n = 13;
    int temp, i, j;
    if (slope < 0) {
        buffer[0] = '-';
        slope = -slope;
    }
    temp = (int) slope;//取整数部�?
    for (i = 0; temp != 0; i++)//计算整数部分的位�?
        temp /= 10;
    temp = (int) slope;

    for (j = i; j >= 0; j--)//将整数部分转换成字符串型
    {
        buffer[j] = temp % 10 + '0';
        temp /= 10;
    }

    buffer[i + 1] = '.';
    slope -= (int) slope;
    for (i = i + 2; i < n - 1; i++)//将小数部分转换成字符串型
    {
        slope *= 10;
        buffer[i] = (int) slope + '0';
        slope -= (int) slope;
    }
    buffer[n - 1] = '\0';
}

int iffloat(uint8_t *buffer) {
    int flag = 1, i = 0;
    while (buffer[i]) {
        if (buffer[i] == '.')
            flag = 0;
        i++;
    }

    return flag;
}

void del_key(uint8_t *buffer) {
    int i = 0;
    while (buffer[i++]);
    i--;
    if (i > 0)
        buffer[i - 1] = 0;
}

int getKeyboardIndex() {

//************uniform model select***********************

    int keyboard = KeyboardScan();

    for (int i = 0; i < 16; i++) {
        if (keyboard & (1 << i)) {
            return i;
        }
    }
//***************************************************************
}

int readline(char *line) {
    uint32_t index = KeyboardScan();

    while (KeyboardScan()) {

    }
    for (int i = 0; i < 16; i++) {
        if (index & (1 << i)) {
            strcat(line, menu[i]);
        }
    }

    if (!HAL_GPIO_ReadPin(del_GPIO_Port, del_Pin)) {
        while (!HAL_GPIO_ReadPin(del_GPIO_Port, del_Pin)) {

        }

        del_key(line);

    }
}

#define CLAMP(x, max, min) ((x < max) ? ((x > min) ? x : min) : max)
typedef struct {
    float A0;       /**< The derived gain, A0 = Kp + Ki + Kd . */
    float A1;       /**< The derived gain, A1 = -Kp - 2Kd. */
    float A2;       /**< The derived gain, A2 = Kd . */
    float state[3]; /**< The state array of length 3. */
    float Kp;       /**< The proportional gain. */
    float Ki;       /**< The integral gain. */
    float Kd;       /**< The derivative gain. */
} PID_t;

volatile PID_t pid_o_Vol = {
        .Kp = 0.5,
        .Ki = 0.005,
        .Kd = 0.00};

void pid_init(PID_t *S) {
    /* Derived coefficient A0 */
    S->A0 = S->Kp + S->Ki + S->Kd;
    /* Derived coefficient A1 */
    S->A1 = (-S->Kp) - ((float) 2.0f * S->Kd);
    /* Derived coefficient A2 */
    S->A2 = S->Kd;
}

float pid_process_o_Vol(PID_t *S, float aim, float current, float out_max, float out_min) {
    pid_init(S);
    float out;
    float in = aim - current;
    /* u[t] = u[t - 1] + A0 * e[t] + A1 * e[t - 1] + A2 * e[t - 2]  */
    out = (S->A0 * in) +
          (S->A1 * S->state[0]) + (S->A2 * S->state[1]) + (S->state[2]);
    /* Update state */
    S->state[1] = S->state[0];
    S->state[0] = in;
    S->state[2] = out;
    out = CLAMP(out, out_max, out_min);
    /* return to application */
    return (out);
}
int cmd_setPID(char *args){
#define  printf(...) UART_printf(&huart1,__VA_ARGS__)
    // paras
    char *arg1 = strtok(NULL, " ");
    if (arg1 == NULL)
    {
        printf("Usage: setPID kp ki kd (Vset)\n");
        return 0;
    }
    float kp = atoff(arg1);

    char *arg2 = strtok(NULL, " ");
    if (arg2 == NULL)
    {
        printf("Usage: setPID kp ki kd (Vset)\n");
        return -1;
    }
    float ki = atoff(arg2);

    char *arg3 = strtok(NULL, " ");
    if (arg3 == NULL)
    {
        printf("Usage: setPID kp ki kd (Vset)\n");
        return -1;
    }

    float kd = atoff(arg3);

    char *arg4 = strtok(NULL, " ");
    if (arg4 == NULL)
    {

    } else{

        float pid_aim = atoff(arg4);
        V_set=pid_aim;
    }


    pid_o_Vol.Kp=kp;
    pid_o_Vol.Kd=kd;
    pid_o_Vol.Ki=ki;
    printf("PID:kp=%f ki=%f kd=%f V_set=%.2f\n",pid_o_Vol.Kp,pid_o_Vol.Ki,pid_o_Vol.Kd,V_set);
    // do with addr&N


    return 0;
}
int cmd_systemctl(char *args){
    // paras
    int func_en=1;
    char *arg1 = strtok(NULL, " ");
    if (arg1 == NULL)
    {
        printf("Usage: systemctl enable/disable <func>\n");
        printf("<func>:\n"
               "    int isPIDPrint;\n"
               "    int isConsoleEnable;\n"
               "    int isDebugPWM;\n"
        );
        return 0;
    }
    //printf(arg1);
    if(strcmp(arg1,"enable")==0){
        func_en=1;
    } else if(strcmp(arg1,"disable")==0){
        func_en=0;
    } else{
        printf("Usage: systemctl enable/disable <func>\n");
        return -1;
    }
    //printf("func_en=%d\n",func_en);
    char *arg2 = strtok(NULL, " ");
    if (arg2 == NULL)
    {
        printf("Usage: systemctl enable/disable <func>\n");
        return -1;
    }

    if(strcmp(arg2,"PIDPrint")==0){
        if(func_en==1){
            config.isPIDPrint=1;
        } else{
            config.isPIDPrint=0;
        }
        return 0;
    }
    if(strcmp(arg2,"DebugPWM")==0){
        if(func_en==1){
            config.isDebugPWM=1;

            char *arg3 = strtok(NULL, " ");
            if (arg3 == NULL)
            {

            } else{
                debug_duty = atoff(arg3);
            }
        } else{
            config.isDebugPWM=0;
        }
        printf("config.isDebugPWM=%s",(config.isDebugPWM)?"true":"false");
        return 0;
    }

    printf("arg %s is not found\n",arg2);
    return -1;

}
int cmd_setSampleRate(char *args){
    // paras
    char *arg1 = strtok(NULL, " ");
    if (arg1 == NULL)
    {
        printf("Usage: setSampleRate <rate> (Vol=Vin*rate)\n");
        return 0;
    }
    float rate = atoff(arg1);
    sampleRate=rate;
    printf("sampleRate=%f\nVol=%.3f*Vin\n",sampleRate,sampleRate);
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    V_cur = (float) ADC_Value[0] * 3.3f / 4096.0f *sampleRate;
    duty = (config.isDebugPWM)?(debug_duty):pid_process_o_Vol(&pid_o_Vol, V_set, V_cur, 95, 1);
    TIM2->CCR1 = (uint32_t)(((float) (TIM2->ARR + 1)) * duty/100.0f - 1.0f);
}
//空闲中断回调函数，参数Size为串口实际接收到数据字节数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance==USART1)
    {
        //把收到的一包数据通过串口回传
        rx_data[Size]=0;
        UART_Console(rx_data);
        HAL_UART_Transmit(&huart1,rx_data,Size,0xff);


        //再次开启空闲中断接收，不然只会接收一次数据
        HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_data,1000);
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
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
    u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi, u8x8_stm32_gpio_and_delay);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    HAL_TIM_Base_Start_IT(&htim2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *) ADC_Value, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    HAL_GPIO_WritePin(check_GPIO_Port, check_Pin, 1);
    HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_data,1000);
    while (1) {
        //HAL_UART_Transmit(&huart1,"hhh",3,HAL_MAX_DELAY);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        int keyboard = getKeyboardIndex();

        fsm_stateDirection:
        if (model == M_input) {
            if (menu[keyboard][0] == '=') {
                while (KeyboardScan()) {

                }
                V_set = str2value_float(first);
                first[0] = 0;
                model = M_output;
                goto display;
            }
        }
        if (model == M_output) {
            if (menu[keyboard][0] == '=') {
                while (KeyboardScan()) {

                }
                first[0] = 0;
                model = M_input;
                goto display;
            }
        }

        fsm_process:
        if (model == M_input)
            readline(first);

        else if (model == M_output) {

        }

//else if(state);


        display:
        if(config.isPIDPrint) {
            UART_printf(&huart1, "V:%f,%f,%.2f\n", V_set, V_cur, duty);
        }
        char V_cur_buf[200];

        static int tik_tok = 0;

        if(tik_tok){
            sprintf(V_cur_buf, "Vout:%.2fV", V_cur);
            tik_tok=0;
        }

        static uint32_t tickstart = 0;
        if ((HAL_GetTick() - tickstart) < 500) {
        } else {
            tickstart = HAL_GetTick();
            tik_tok = 1;
        }


        frame(&u8g2, first, V_cur_buf, V_set, model);


    }
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
#include <stdio.h>
#include <stdarg.h>

int UART_printf(UART_HandleTypeDef *huart, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int length;
    char buffer[128];

    length = vsnprintf(buffer, 128, fmt, ap);

    HAL_UART_Transmit(huart, (uint8_t *) buffer, length, HAL_MAX_DELAY);

    va_end(ap);
    return length;
}

uint8_t u8x8_stm32_gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8,
                                  U8X8_UNUSED uint8_t msg,
                                  U8X8_UNUSED uint8_t arg_int,
                                  U8X8_UNUSED void *arg_ptr) {
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            HAL_Delay(1);
            break;
        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int);
            break;
        case U8X8_MSG_GPIO_DC:
            HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:
            HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, arg_int);
            break;
    }
    return 1;
}

uint8_t u8x8_byte_4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch (msg) {
        case U8X8_MSG_BYTE_SEND:
            HAL_SPI_Transmit(&hspi1, (uint8_t *) arg_ptr, arg_int, HAL_MAX_DELAY);
            break;
        case U8X8_MSG_BYTE_INIT:
            break;
        case U8X8_MSG_BYTE_SET_DC:
            HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, arg_int);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            HAL_GPIO_WritePin(OLED_NSS_GPIO_Port, OLED_NSS_Pin, GPIO_PIN_RESET);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            HAL_GPIO_WritePin(OLED_NSS_GPIO_Port, OLED_NSS_Pin, GPIO_PIN_SET);
            break;
        default:
            return 0;
    }
    return 1;
}


char *strcat(char *destination, const char *source) {
    int i = 0;
    while (destination[i] != '\0')
        i++;
    while (*source != '\0')
        destination[i++] = *(source++);
    *(destination + i) = '\0';
    return (destination);
}

void frameutf8(u8g2_t *u8g2, uint8_t first[], uint8_t second[], uint8_t out[]) {
    int len = strlen(out);
//    u8g2_FirstPage(u8g2);
//    do
//    {
//        u8g2_SetFont(u8g2, u8g2_font_courR12_tf);
//        u8g2_DrawStr(u8g2, 0, 15, first);
//        u8g2_DrawStr(u8g2, 0, 30, second);
//        u8g2_DrawStr(u8g2, 128-len*10, 50, out);
//        u8g2_DrawLine(u8g2,0, 32, 128, 32);
//
//
//    } while (u8g2_NextPage(u8g2));

    u8g2_FirstPage(u8g2);
    u8g2_ClearBuffer(u8g2);
    do {

        u8g2_SetFont(u8g2, u8g2_font_unifont_t_symbols);
        u8g2_DrawUTF8(u8g2, 0, 15, first);
        u8g2_DrawUTF8(u8g2, 0, 30, second);
        u8g2_DrawUTF8(u8g2, 128 - len * 10, 50, out);
        u8g2_DrawLine(u8g2, 0, 32, 128, 32);
    } while (u8g2_NextPage(u8g2));
}

void frame(u8g2_t *u8g2, uint8_t Vset_buf[], uint8_t Vout_buf[], float Vset, int model) {
    static int tik_tok = 0;
    char tmp[200];
    int len = strlen(Vset_buf);
    u8g2_FirstPage(u8g2);
    do {
        u8g2_SetFont(u8g2, u8g2_font_courR12_tf);
        u8g2_DrawStr(u8g2, 0, 15, "Vset:");
        //Vset
        sprintf(tmp, "%.2fV", Vset);
        u8g2_DrawStr(u8g2, 50, 15, tmp);
        u8g2_DrawStr(u8g2, 0, 30, Vout_buf);
        u8g2_DrawStr(u8g2, 128 - len * 10, 50, Vset_buf);
        //u8g2_DrawStr(u8g2, 0, 50, sign);
        if (!tik_tok && model == M_input)
            //u8g2_DrawLine(u8g2, 0, 32, 128, 32);
            u8g2_DrawRFrame(u8g2, 0, 32, 128, 23, 3);


        static uint32_t tickstart = 0;
        if ((HAL_GetTick() - tickstart) < 800) {
        } else {
            tickstart = HAL_GetTick();
            tik_tok = !tik_tok;
        }
    } while (u8g2_NextPage(u8g2));
}

uint8_t decode(uint16_t data, uint8_t pos) {
    return data & (1 << pos);
//    return 1&(data>>(pos));
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
    while (1) {
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
