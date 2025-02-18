/**
 * @file    BOARD.c
 * 
 * Generic driver set up and clean up for functional testing for the UCSC Nucleo
 * I/O Shield connected to a Nucleo STM32F411RE development kit.
 *
 * @author  HARE Lab
 * @author  jLab
 * @author  Adam Korycki
 *
 * @date    18 Dec 2024
 */

#include <Board.h>
#include <leds.h>


/*  PROTOTYPES  */
static int8_t Nucleo_ConfigSysClk(void);
static int8_t Nucleo_ConfigClks(void);
static int8_t Nucleo_ConfigPins(void);
static int8_t Nucleo_Init(void);
static int8_t Serial_ConfigPins(void);
static int8_t Serial_Begin(void);
static int8_t Serial_Init(void);
void Error_Handler(void);


/*  MODULE-LEVEL DEFINITIONS, MACROS    */
static uint8_t initStatus = FALSE;

// Define UART2 handler and setup printf() functionality.
UART_HandleTypeDef huart2;
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

GETCHAR_PROTOTYPE
{
    uint8_t ch = 0;

    // Clear the Overrun flag just before receiving the first character.
    __HAL_UART_CLEAR_OREFLAG(&huart2);

    // Wait for reception of a character on the USART RX line and echo this
    // character on console.
    HAL_UART_Receive(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

// Defaults.
#define SERIAL_BAUDRATE ((uint32_t) 115200)


/*  FUNCTIONS   */
// Clock setup.
/** Nucleo_ConfigSysClk()
 *
 * System clock configuration:
 *  + Clock used = PLL
 *  + f_target = 48MHz
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
static int8_t Nucleo_ConfigSysClk(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure the main internal regulator output voltage.
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Initializes the RCC Oscillators according to the specified parameters
    // in the RCC_OscInitTypeDef structure.
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    // Initializes the CPU, AHB and APB buses clocks.
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

    return SUCCESS;
}

/** Nucleo_ConfigClks()
 *
 * Set up all system clock and all GPIO clocks.
 * @note    Enabling clocks multiple times will not cause errors.
 * 
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
static int8_t Nucleo_ConfigClks(void)
{
    Nucleo_ConfigSysClk();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    return SUCCESS;
}


// Serial setup.
/** Serial_ConfigPins()
 *
 * Configure pins for use with serial communications using UART2.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
static int8_t Serial_ConfigPins(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    // Set up the UART2 bus.
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    return SUCCESS;
}

/** Serial_Begin()
 *
 * Open the serial connection between the Nucleo and a PC over USB.
 * 
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
static int8_t Serial_Begin(void)
{
    // Initialize UART2 for serial communications over USB.
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }

    return SUCCESS;
}

/** Serial_Init()
 *
 * Configure USB-UART2 @ 115200 baud.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
static int8_t Serial_Init(void)
{
    Serial_ConfigPins();
    Serial_Begin();
    return SUCCESS;
}

// Onboard Nucleo functionality.
/** Nucleo_ConfigPins()
 *
 * Configure pins for use by Nucleo peripherals.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
static int8_t Nucleo_ConfigPins(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    // PA5  -> Status LED.
    // PC13 -> "USER" (Big and Blue) Button.
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    return SUCCESS;
}

/** Nucleo_Init()
 *
 * Configure onboard Nucleo pins, i.e. the status LED and B^3 for usage.
 * 
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
static int8_t Nucleo_Init(void)
{
    Nucleo_ConfigClks();
    Nucleo_ConfigPins();

    // Set initial state of Status LED to OFF.
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    return SUCCESS;
}

/** BOARD_Init()
 *
 * 1. Initialize the HAL framework.
 * 2. Configure the clocks and pins used by our Nucleo kit's setup for serial
 * communications, onboard LED statuses, and resetting with the Big Blue Button.
 * 3. Initialize LEDs.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
int8_t BOARD_Init() {
    if (initStatus == FALSE)
    {
        HAL_Init();
        if (Nucleo_Init() == ERROR)
        {
            return ERROR;
        }
        if (Serial_Init() == ERROR)
        {
            return ERROR;
        }
        LEDS_Init();
        initStatus = TRUE;
    }
    return SUCCESS;
}

/** BOARD_End()
 *
 * Shuts down all peripherals; turns all pins into input.
 *
 * @return  initStatus  (int8_t)    [TRUE, FALSE]
 *
 * @todo    Set all ports to be digital inputs.
 */
int8_t BOARD_End(void)
{
    initStatus = FALSE;
    // Kill all interrupts.
    __disable_irq();

    return initStatus;
}

/** Error_Handler()
 * 
 * Enter a state of blinking the status LED on-board the Nucleo, indefinitely.
 */
void Error_Handler(void)
{
    __disable_irq();
    static int halfBlinkPeriod = 500000;
    static int timer = 0;
    while (1)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        while (timer < halfBlinkPeriod)
        {
            timer++;
        }
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        timer = 0;
        while (timer < halfBlinkPeriod)
        {
            timer++;
        }
        timer = 0;
    }
}

