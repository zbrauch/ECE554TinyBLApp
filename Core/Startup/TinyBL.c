
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "BLQueue.h"

__weak void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	GPIO_InitTypeDef  GPIO_InitStruct;


	//##-1- Enable peripherals and GPIO Clocks #################################
	// Enable GPIO TX/RX clock
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	// Enable USARTx clock
	__HAL_RCC_USART3_CLK_ENABLE();

	//##-2- Configure peripheral GPIO ##########################################
	// UART TX GPIO pin configuration
	GPIO_InitStruct.Pin       = GPIO_PIN_8;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_PULLUP;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	// UART RX GPIO pin configuration
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

UART_HandleTypeDef UartHandle; //UART handler declaration
void TinyBLInit(void) {
	HAL_Init();

	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 180;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		for(;;);
	}

	/** Activate the Over-Drive mode
	 */
	if (HAL_PWREx_EnableOverDrive() != HAL_OK)
	{
		for(;;);
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		for(;;);
	}


	/*GPIO_InitTypeDef  GPIO_InitStruct;
	//##-1- Enable peripherals and GPIO Clocks #################################
	// Enable GPIO TX/RX clock
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	// Enable USARTx clock
	__HAL_RCC_USART3_CLK_ENABLE();

	//##-2- Configure peripheral GPIO ##########################################
	// UART TX GPIO pin configuration
	GPIO_InitStruct.Pin       = GPIO_PIN_8;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_PULLUP;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	// UART RX GPIO pin configuration
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);*/


	UartHandle.Instance        = USART3;
	UartHandle.Init.BaudRate   = 57600;
	UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	UartHandle.Init.StopBits   = UART_STOPBITS_1;
	UartHandle.Init.Parity     = UART_PARITY_NONE;
	UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
	UartHandle.Init.Mode       = UART_MODE_TX_RX;
	UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;

	if (HAL_UART_Init(&UartHandle) != HAL_OK)
	{
		//Initialization Error
		for(;;);
	}

}


void TinyBLRAMLoader(void) {
	uint8_t uartInBuf[17];
	uint32_t startAddr;
	uint32_t msgCount;
	uint32_t dataRxCount = 0; //number of data messages received by loader
	uint8_t cmdBuf[10];

	//init fancy circular queue
	BLQueue q;
	BLQueueInit(&q);

	asm("nop");
	//wait for an init message
	//init format: "I" + message count (uint32, LE) + start address (uint32, LE) + 'E'
	for(;;) {
		uint16_t count;
		HAL_UARTEx_ReceiveToIdle(&UartHandle, uartInBuf, 17, &count, 0);
		if(count) {
			//Add to input ring buffer
			BLQueueAddArray(&q, uartInBuf, count);
			//process for packets
			uint8_t isMessage = BLQueueExtractMessage(&q, cmdBuf, 'I');
			if(isMessage) {
				msgCount = *((uint32_t*)(cmdBuf+1));
				startAddr = *((uint32_t*)(cmdBuf+5));

				HAL_UART_Transmit(&UartHandle, "Init Received\r\n", 15, HAL_MAX_DELAY);
				break;
			}
		}
	}
	BLQueueInit(&q);

	asm("nop");

	//receive all ramcode data messages
	uint32_t curAddr = startAddr;
	uint32_t recDataMsgCount = 0;
	for(uint32_t i = 0; i < msgCount; i++) {
		for(;;) {
			uint16_t count;
			HAL_UARTEx_ReceiveToIdle(&UartHandle, uartInBuf, 17, &count, 0);
			if(count) {
				//Add to input ring buffer
				BLQueueAddArray(&q, uartInBuf, count);
				//process for packets
				uint8_t isMessage = BLQueueExtractMessage(&q, cmdBuf, 'D');
				if(isMessage) {
					recDataMsgCount++;
					uint8_t *ramAddr = (uint8_t*)curAddr;
					for(uint8_t i = 0; i < 8; i++)
						ramAddr[i] = cmdBuf[i+1];
					curAddr += 8;
					break;
				}
			}
		}
	}

	__asm("nop");

	//wait for second init message
	//init format: "I" + entry address (uint32, little endian) + 4 pad bytes + 'E'
	uint32_t entryAddr;
	for(;;) {
		uint16_t count;
		HAL_UARTEx_ReceiveToIdle(&UartHandle, uartInBuf, 17, &count, 0);
		if(count) {
			//Add to input ring buffer
			BLQueueAddArray(&q, uartInBuf, count);
			//process for packets
			uint8_t isMessage = BLQueueExtractMessage(&q, cmdBuf, 'I');
			if(isMessage) {
				entryAddr = *((uint32_t*)(cmdBuf+1));

				HAL_UART_Transmit(&UartHandle, "Branching to RAM\r\n", 16, HAL_MAX_DELAY);
				break;
			}
		}
	}
	BLQueueInit(&q);

	__asm("nop");

	//The most beautiful line of C ever written
	((void (*)(void)) entryAddr)();

	//temp test echo back messages
	/*for(;;) {
		uint16_t count;
		HAL_UARTEx_ReceiveToIdle(&UartHandle, uartInBuf, 17, &count, 0);
		if(count) {
			//HAL_UART_Transmit(&UartHandle, uartInBuf, count, HAL_MAX_DELAY);
			//Add to input ring buffer
			BLQueueAddArray(&q, uartInBuf, count);

			//process for packets
			uint8_t isMessage = BLQueueExtractMessage(&q, cmdBuf, 'S');
			if(isMessage) {
				HAL_UART_Transmit(&UartHandle, cmdBuf, 10, HAL_MAX_DELAY);
			}
		}
		//HAL_UART_Receive_DMA(&UartHandle, uartInBuf, 0xFF);
		//HAL_UARTEx_ReceiveToIdle_DMA(&UartHandle, uartInBuf, 0xff);
		//HAL_UARTEx_ReceiveToIdle_IT(&UartHandle, uartInBuf, 0xFF);
		//HAL_UART_Receive_IT(&UartHandle, uartInBuf, 1);
		//for(;;);
	}*/
}

//Check digital pin and branches to TinyBL if set
//runs on startup
void TinyBLStartup(void) {

	//TODO: return when digital pin is not set
	//needs digital pin init and read code. We'll configure the pin to pull down for convenience

	TinyBLInit();

	//temp UART test
	uint8_t printout[50] = "Hello from BL!\r\n";
	for(uint8_t i = 0; i < 1; i++) {
		//UART_
		HAL_UART_Transmit(&UartHandle, printout, 17, HAL_MAX_DELAY);
	}

	//proceed to loader
	TinyBLRAMLoader();
}

/*void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
	// check if the callback is called by the USART2 peripheral
	if(huart->Instance == USART3){

	}
}

void HAL_UART_RxCpltCallback (UART_HandleTypeDef * huart)
{
	asm("nop");
	HAL_UART_Receive_IT(&UartHandle, uartInBuf, 1);
}*/
