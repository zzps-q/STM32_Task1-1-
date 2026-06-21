/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
// RGB模式：0=绿常亮  1=红常亮  2=绿呼吸
uint8_t rgb_mode = 0;

// 蜂鸣模式：0=常响  1=静默  2=间断响
uint8_t beep_mode = 0;

// 按键状态
uint8_t key1_short = 0;  // 按键1短按标志
uint8_t key2_long  = 0;  // 按键2长按标志

// 呼吸灯用
uint16_t pwm_cnt   = 0;   // PWM计数
uint16_t pwm_cmp   = 0;   // PWM比较值
uint8_t  breath_dir = 1;  // 呼吸方向 1=渐亮 0=渐暗
// 计时用
uint32_t tick = 0;
uint32_t breath_tick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief  按键扫描函数（带软件消抖，区分短按和长按）
 * @retval 0=无按键  1=短按  2=长按
 */
uint8_t KEY_Scan(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    // 按键按下（高电平，因为下拉输入，按下接3.3V）
    if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_SET)
    {
        HAL_Delay(10);  // 消抖10ms
        if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_SET)
        {
            // 等待松手，同时计时判断长按
            uint32_t press_time = 0;
            while (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_SET)
            {
                HAL_Delay(10);
                press_time++;
                if (press_time > 50)  // 50*10ms = 500ms 以上算长按
                {
                    // 还按着，返回长按
                    // 等松手再返回
                    while (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_SET)
                    {
                        HAL_Delay(10);
                    }
                    return 2;  // 长按
                }
            }
            return 1;  // 短按
        }
    }
    return 0;  // 无按键
}
/**
 * @brief  RGB灯控制
 * @param  mode: 0=绿常亮  1=红常亮  2=绿呼吸
 */
void RGB_Control(uint8_t mode)
{
    switch (mode)
    {
        case 0:  // 绿常亮
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);  // 绿灯亮
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // 红灯灭
            break;

        case 1:  // 红常亮
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET); // 绿灯灭
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);   // 红灯亮
            break;

        case 2:  // 绿呼吸（在SysTick中断里更新亮度，这里先不做，主循环里处理）
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // 红灯灭
            break;

        default:
            break;
    }
}

/**
 * @brief  蜂鸣器控制
 * @param  mode: 0=常响  1=静默  2=间断响
 * @param  tick: 系统滴答计数，用于间断响计时
 */
void BEEP_Control(uint8_t mode, uint32_t tick)
{
    switch (mode)
    {
        case 0:  // 常响
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
            break;

        case 1:  // 静默
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
            break;

        case 2:  // 间断响（500ms响，500ms停）
            if (tick % 1000 < 500)
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
            else
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
            break;

        default:
            break;
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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
   tick = HAL_GetTick();  // 获取当前系统时间（ms）

    // ====== 按键扫描 ======
    uint8_t key1 = KEY_Scan(GPIOA, GPIO_PIN_3);
    uint8_t key2 = KEY_Scan(GPIOA, GPIO_PIN_4);

    // 按键1短按：切换RGB模式和蜂鸣模式
    if (key1 == 1)
    {
      rgb_mode++;
      if (rgb_mode > 2) rgb_mode = 0;

      beep_mode++;
      if (beep_mode > 2) beep_mode = 0;
    }

    // 按键2长按：绿灯常亮（松手后恢复原模式）
    if (key2 == 2)
    {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    }

    // ====== RGB灯控制 ======
    if (rgb_mode == 2)  // 呼吸灯模式
    {
      // 每10ms调整一次亮度
      if (tick - breath_tick >= 10)
      {
        breath_tick = tick;
        if (breath_dir)  // 渐亮
        {
          pwm_cmp++;
          if (pwm_cmp >= 50) breath_dir = 0;
        }
        else  // 渐暗
        {
          pwm_cmp--;
          if (pwm_cmp == 0) breath_dir = 1;
        }
      }

      // 软件PWM输出
      pwm_cnt++;
      if (pwm_cnt < pwm_cmp)
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
      else
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    }
    else
    {
      RGB_Control(rgb_mode);
    }

    // ====== 蜂鸣器控制 ======
    BEEP_Control(beep_mode, tick);

    HAL_Delay(1);  // 1ms延时，配合呼吸灯PWM
      /* USER CODE END 3 */
  }

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
#ifdef USE_FULL_ASSERT
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
