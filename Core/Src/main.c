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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "network.h"
#include "arm_math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/**
 * ADC
 */
#define BLOCK_SIZE  512
#define ADC_BUF_SIZE (BLOCK_SIZE * 2)
#define FFT_BINS     (BLOCK_SIZE / 2)   // half of block size always, each bin represents = sampling frequency/no. of bins


/**
 * UART
 */
#define HEADER_BYTES      2                            // 0xAA, 0xBB
#define PAYLOAD_BYTES     ((FFT_BINS+1) * sizeof(float))   // 256 * 4 = 1024 bytes
#define FOOTER_BYTES      2                            // 0xCC, 0xDD

#define TX_BUF_SIZE       (HEADER_BYTES + PAYLOAD_BYTES + FOOTER_BYTES) // 1028 bytes //originally block size/2


/**
 * DAC
 */
#define DMA_BUFFER_SIZE 64
#define NS  128 // 2x of DMA BUFFER SIZE, no. of points in the sinusoidal wave


/**
 * AI INPUT PRE-PROCESS
 */
#define INPUT_SCALE       (1.0f / 255.0f) // 1/(no. of inputs-1)
#define INPUT_ZERO_POINT  (-128.0f)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

CRC_HandleTypeDef hcrc;

DAC_HandleTypeDef hdac1;
DMA_HandleTypeDef hdma_dac1_ch1;

SAI_HandleTypeDef hsai_BlockA1;
DMA_HandleTypeDef hdma_sai1_a;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */

/*1D CNN*/
/* Global byte buffer to save instantiated C-model network context */
STAI_ALIGNED(STAI_NETWORK_CONTEXT_ALIGNMENT)
static stai_network network_context[STAI_NETWORK_CONTEXT_SIZE] = {0};

/* Global c-array to handle the activations buffer */
STAI_ALIGNED(STAI_NETWORK_ACTIVATION_1_ALIGNMENT)
static uint8_t activations[STAI_NETWORK_ACTIVATION_1_SIZE_BYTES];

/* Array to store the data of the input tensor */
STAI_ALIGNED(STAI_NETWORK_IN_1_ALIGNMENT)
static int8_t in_data[STAI_NETWORK_IN_1_SIZE];
/* or static uint8_t/float in_data[STAI_NETWORK_IN_1_SIZE_BYTES]; */

/* c-array to store the data of the output tensor */
STAI_ALIGNED(STAI_NETWORK_OUT_1_ALIGNMENT)
static int8_t out_data[STAI_NETWORK_OUT_1_SIZE];
/* static uint8_t/float out_data[STAI_NETWORK_OUT_1_SIZE_BYTES]; */

/* Array of pointer to manage the model's input/output tensors */
static stai_ptr stai_input[STAI_NETWORK_IN_NUM];
static stai_ptr stai_output[STAI_NETWORK_OUT_NUM];


/**
 * ADC
 */
volatile uint16_t g_adcBuffer[ADC_BUF_SIZE];
float32_t g_dspInput[BLOCK_SIZE];
float32_t g_fftOutput[BLOCK_SIZE]; // Real/Imaginary pairs (Size must match FFT needs)
float32_t g_fftMag[BLOCK_SIZE/2 + 1];
//float32_t g_hanningWindow[BLOCK_SIZE];

// Threads synchronization variables
volatile uint8_t g_fftReadyFlag = 0;
uint16_t *gp_activeAdcData = NULL; // Points to the stable half buffer



/**
 * FFT
 */
arm_rfft_fast_instance_f32 fftInstance;


/**
 * UART
 */
uint8_t dma_tx_buffer[TX_BUF_SIZE];
volatile uint8_t dma_busy = 0;

/**
 * Analog Watch Dog
 */
uint32_t glow = 0;


/**
 * ADC
 */
uint16_t dma_buffer[2 * DMA_BUFFER_SIZE];

uint32_t Wave_LUT[NS] = {
    2048, 2149, 2250, 2350, 2450, 2549, 2646, 2742, 2837, 2929, 3020, 3108, 3193, 3275, 3355,
    3431, 3504, 3574, 3639, 3701, 3759, 3812, 3861, 3906, 3946, 3982, 4013, 4039, 4060, 4076,
    4087, 4094, 4095, 4091, 4082, 4069, 4050, 4026, 3998, 3965, 3927, 3884, 3837, 3786, 3730,
    3671, 3607, 3539, 3468, 3394, 3316, 3235, 3151, 3064, 2975, 2883, 2790, 2695, 2598, 2500,
    2400, 2300, 2199, 2098, 1997, 1896, 1795, 1695, 1595, 1497, 1400, 1305, 1212, 1120, 1031,
    944, 860, 779, 701, 627, 556, 488, 424, 365, 309, 258, 211, 168, 130, 97,
    69, 45, 26, 13, 4, 0, 1, 8, 19, 35, 56, 82, 113, 149, 189,
    234, 283, 336, 394, 456, 521, 591, 664, 740, 820, 902, 987, 1075, 1166, 1258,
    1353, 1449, 1546, 1645, 1745, 1845, 1946, 2047
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SAI1_Init(void);
static void MX_CRC_Init(void);
static void MX_DAC1_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * START - EDGE AI
 */
int aiInit(void) {
  stai_return_code ret_code;

  /* Initialize runtime library */
  ret_code = stai_runtime_init();
  if (ret_code != STAI_SUCCESS) { return -1;}

  /* Initialize network model context */
  ret_code = stai_network_init(network_context);
  if (ret_code != STAI_SUCCESS) { return -1;}

  /* Set network activations buffers */
  const stai_ptr acts[] = { activations };
  ret_code = stai_network_set_activations(network_context, acts, STAI_NETWORK_ACTIVATIONS_NUM);
  if (ret_code != STAI_SUCCESS) { return -1;}

  return 0;
}

int aiDeinit(void) {
  stai_return_code ret_code;

  /* Deinitialize network model context */
  ret_code = stai_network_deinit(network_context);
  if (ret_code != STAI_SUCCESS) { return -1;}

  /* Deinitialize runtime library */
  ret_code = stai_runtime_deinit();
  if (ret_code != STAI_SUCCESS) { return -1;}

  return 0;
}

/*
 * Run inference
 */
int aiRun(const void *in_data, void *out_data) {
  stai_return_code ret_code;

  /* Set network input/output buffers */
  const stai_ptr inputs_ptr[] = { in_data };
  ret_code = stai_network_set_inputs(network_context, inputs_ptr, STAI_NETWORK_IN_NUM);
  if (ret_code != STAI_SUCCESS) { return -1;}

  const stai_ptr outputs_ptr[] = { out_data };
  ret_code = stai_network_set_outputs(network_context, outputs_ptr, STAI_NETWORK_OUT_NUM);
  if (ret_code != STAI_SUCCESS) { return -1;}


  /* Perform the inference */
  ret_code = stai_network_run(network_context, STAI_MODE_SYNC);
  if (ret_code != STAI_SUCCESS) {
      ret_code = stai_network_get_error(network_context);
      return -1;
  };

  return 0;
}

void quantize_input_256(const float32_t *float_input, int8_t *int8_output) {
    // 1. Calculate inverse scale (1 / scale) to replace division with multiplication
    float32_t inv_scale = 1.0f / INPUT_SCALE;

    // Temporary buffer for intermediate float processing
    float32_t temp_buffer[256];

    // 2. Multiply entire array by inv_scale: temp = float_input * inv_scale
    arm_scale_f32(float_input, inv_scale, temp_buffer, 256);

    // 3. Add zero-point and clip to int8 range
    for (int i = 0; i < 256; i++) {
        // Round to nearest integer
        float32_t rounded = roundf(temp_buffer[i]) + INPUT_ZERO_POINT;

        // Manual clipping to prevent overflow
        if (rounded > 127.0f)  rounded = 127.0f;
        if (rounded < -128.0f) rounded = -128.0f;

        int8_output[i] = (int8_t)rounded;
    }
}

int acquire_and_process_data(void *in_data)
{
	/* fill the inputs of the c-model
	for (int idx=0; idx < AI_NETWORK_IN_NUM; idx++ )
	{
	in_data[idx] = ....
	}
	*/
	quantize_input_256(g_fftMag, in_data);

	return 0;
}

int post_process(void *out_data){
	return 0;
}

/**
 * END - EDGE AI
 */

/**
 * START - FFT DSP
 */

void init_dsp(void) {
    // Initialize the Real FFT instance for a 512-point conversion
    arm_rfft_fast_init_f32(&fftInstance, BLOCK_SIZE);
}

void normalize_array_256_fast(float32_t *array) {
    float32_t min_val, max_val;
    uint32_t min_index, max_index;

    // 1. Find min and max using hardware-accelerated vectors
    arm_min_f32(array, 256, &min_val, &min_index);
    arm_max_f32(array, 256, &max_val, &max_index);

    float32_t range = max_val - min_val;

    if (range > 0.0f) {
        // 2. Subtract min_val from all elements: array[i] = array[i] - min_val
        arm_offset_f32(array, -min_val, array, 256);

        // 3. Multiply by (1 / range) instead of dividing inside a loop
        float32_t inv_range = 1.0f / range;
        arm_scale_f32(array, inv_range, array, 256);
    } else {
        // If all elements are equal, set entire array to 0
        arm_fill_f32(0.0f, array, 256);
    }
}

void process_ultrasonic_data(uint16_t *p_raw_buffer) {
    float32_t dcBias = 0.0f;

    // 1. Convert unsigned 16-bit to float directly (Safe for all ADC bit depths)
    for (int i = 0; i < BLOCK_SIZE; i++) {
        g_dspInput[i] = (float32_t)p_raw_buffer[i];
    }

    // 2. Compute the mean (Average DC Offset) using optimized hardware math
    arm_mean_f32(g_dspInput, BLOCK_SIZE, &dcBias);

    // 3. Subtract the DC offset from the entire vector in one shot
    arm_offset_f32(g_dspInput, -dcBias, g_dspInput, BLOCK_SIZE);

    // 4. Execute CMSIS-DSP Real FFT
    // Note: g_dspInput will be modified/destroyed during execution
    arm_rfft_fast_f32(&fftInstance, g_dspInput, g_fftOutput, 0);

    // 5. Correctly unpack and calculate magnitudes
    // Handle the pure-real DC component
    g_fftMag[0] = fabsf(g_fftOutput[0]);

    // Handle bins 1 to (BLOCK_SIZE/2 - 1)
    arm_cmplx_mag_f32(&g_fftOutput[2], &g_fftMag[1], (BLOCK_SIZE / 2) - 1);

    // Handle the pure-real Nyquist component (stored at the end of the mag array)
    g_fftMag[BLOCK_SIZE / 2] = fabsf(g_fftOutput[1]);

    normalize_array_256_fast(g_fftMag);

  /**
   * PRINT TO PC VIA UART
   */

	// 1. Send a unique Start-of-Frame header (e.g., 2 bytes: 0xAA, 0xBB)
	// This allows the computer to find the exact start of your array
	uint8_t header[2] = {0xAA, 0xBB};
	_write(1, (char *)header, 2);

	// 2. Blast the ENTIRE float array out of memory in a single shot!
	// No loops, no formatting math, no printf overhead.
	uint32_t bytes_to_send = (BLOCK_SIZE / 2) * sizeof(float32_t);
	_write(1, (char *)g_fftMag, bytes_to_send);

	// 3. Send an End-of-Frame footer (e.g., 2 bytes: 0xCC, 0xDD)
	// This acts as a validation check for the PC
	uint8_t footer[2] = {0xCC, 0xDD};
	_write(1, (char *)footer, 2);
}


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    // First half of buffer is full (samples 0 to BLOCK_SIZE-1)
	if (g_fftReadyFlag == 0) {
		gp_activeAdcData = &g_adcBuffer[0]; // Point to the first half
		g_fftReadyFlag = 1;                 // Wake up the main loop math
	} else {
		// Overrun Warning: Main loop didn't finish the last FFT in time!
	}
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    // Second half of buffer is full (samples BLOCK_SIZE to ADC_BUF_SIZE-1)
	if (g_fftReadyFlag == 0) {
		gp_activeAdcData = &g_adcBuffer[BLOCK_SIZE]; // Point to the second half
		g_fftReadyFlag = 1;                          // Wake up the main loop math
	} else {
		// Overrun Warning: Main loop didn't finish the last FFT in time!
	}

}
/**
 * END - FFT DSP
 */


int _write(int file, char *ptr, int len)
{
    // If a previous print is still sending, wait or skip to avoid corrupting data
    // (For real-time FFT, skipping or blocking briefly depends on your requirements)
    while (dma_busy);

    // Cap the length to prevent buffer overflow
    if (len > TX_BUF_SIZE) len = TX_BUF_SIZE;

    // Copy to stable background memory
    memcpy(dma_tx_buffer, ptr, len);
    dma_busy = 1;

    // Transmit instantly in the background via DMA
    if (HAL_UART_Transmit_DMA(&huart2, dma_tx_buffer, len) == HAL_OK)
    {
        return len;
    }

    dma_busy = 0;
    return -1;
}

// This callback fires automatically when the DMA finishes transferring the data
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        dma_busy = 0; // Release the lock so the next _write can run
    }
}

//Increment counter if value into ADC buffer exceeds a threshold
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc)
{
    glow++;
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
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_SAI1_Init();
  MX_CRC_Init();
  MX_DAC1_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  init_dsp();

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adcBuffer, ADC_BUF_SIZE);
  HAL_TIM_Base_Start(&htim2);


  //aiInit();




  /** Test FFT with sinusoidal wave: sinusoidal wave frequency = trigger frequency/NS */
  HAL_TIM_Base_Start_IT(&htim6);

  HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t*) Wave_LUT, NS, DAC_ALIGN_12B_R);


  while (1)
  {
	  /* 1 - Acquire, pre-process and fill the input buffers */
	     // acquire_and_process_data(in_data);

	      /* 2 - Call inference engine */
	      //aiRun(in_data, out_data);

	      /* 3 - Post-process the predictions */
	     //post_process(out_data);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  // Check if a new half-buffer snapshot is safely ready for math processing
	  if (g_fftReadyFlag == 1)
	  {
		  // Execute the intensive math calculations out here in user thread space
		  process_ultrasonic_data(gp_activeAdcData);

		  // Your 256 magnitude spectra profile is now ready in g_fftMag!
		  // Trace your 40kHz spikes or ultrasonic squeaks here.

		  // Reset the flag to unlock the next callback interception
		  g_fftReadyFlag = 0;
	  }
	 
  }
  //aiDeinit();
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV6;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_AnalogWDGConfTypeDef AnalogWDGConfig = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T2_TRGO;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analog WatchDog 1
  */
  AnalogWDGConfig.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
  AnalogWDGConfig.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
  AnalogWDGConfig.Channel = ADC_CHANNEL_15;
  AnalogWDGConfig.ITMode = ENABLE;
  AnalogWDGConfig.HighThreshold = 1000;
  AnalogWDGConfig.LowThreshold = 0;
  AnalogWDGConfig.FilteringConfig = ADC_AWD_FILTERING_NONE;
  if (HAL_ADC_AnalogWDGConfig(&hadc1, &AnalogWDGConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */

  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC;
  sConfig.DAC_DMADoubleDataMode = DISABLE;
  sConfig.DAC_SignedFormat = DISABLE;
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_Trigger2 = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_EXTERNAL;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}

/**
  * @brief SAI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAI1_Init(void)
{

  /* USER CODE BEGIN SAI1_Init 0 */

  /* USER CODE END SAI1_Init 0 */

  /* USER CODE BEGIN SAI1_Init 1 */

  /* USER CODE END SAI1_Init 1 */
  hsai_BlockA1.Instance = SAI1_Block_A;
  hsai_BlockA1.Init.Protocol = SAI_FREE_PROTOCOL;
  hsai_BlockA1.Init.AudioMode = SAI_MODEMASTER_RX;
  hsai_BlockA1.Init.DataSize = SAI_DATASIZE_16;
  hsai_BlockA1.Init.FirstBit = SAI_FIRSTBIT_MSB;
  hsai_BlockA1.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
  hsai_BlockA1.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockA1.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockA1.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockA1.Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;
  hsai_BlockA1.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockA1.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;
  hsai_BlockA1.Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
  hsai_BlockA1.Init.Mckdiv = 14;
  hsai_BlockA1.Init.MonoStereoMode = SAI_MONOMODE;
  hsai_BlockA1.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockA1.Init.PdmInit.Activation = ENABLE;
  hsai_BlockA1.Init.PdmInit.MicPairsNbr = 1;
  hsai_BlockA1.Init.PdmInit.ClockEnable = SAI_PDM_CLOCK1_ENABLE;
  hsai_BlockA1.FrameInit.FrameLength = 16;
  hsai_BlockA1.FrameInit.ActiveFrameLength = 1;
  hsai_BlockA1.FrameInit.FSDefinition = SAI_FS_STARTFRAME;
  hsai_BlockA1.FrameInit.FSPolarity = SAI_FS_ACTIVE_HIGH;
  hsai_BlockA1.FrameInit.FSOffset = SAI_FS_FIRSTBIT;
  hsai_BlockA1.SlotInit.FirstBitOffset = 0;
  hsai_BlockA1.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
  hsai_BlockA1.SlotInit.SlotNumber = 2;
  hsai_BlockA1.SlotInit.SlotActive = 0x00000003;
  if (HAL_SAI_Init(&hsai_BlockA1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAI1_Init 2 */

  /* USER CODE END SAI1_Init 2 */

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
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 679;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 0;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 24;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 6000000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
