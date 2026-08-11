# 📌 Architecture Overview
---
```bash
                          ┌────────────────────────┐
                          │     Timer 6 (TIM6)     │
                          └───────────┬────────────┘
                                      │
                                      ▼
┌──────────────────┐      ┌────────────────────────┐
│  DAC1 + DMA      ├─────►│  Ultrasonic Generator  │ (Hardware Loopback / Test Setup)
└──────────────────┘      └────────────────────────┘
                                      │
                                      ▼
┌──────────────────┐      ┌────────────────────────┐
│  Timer 2 (TIM2)  ├─────►│     ADC1 + DMA         │ ( Ping-Pong / Double Buffer )
└──────────────────┘      └───────────┬────────────┘
                                      │
                                      ▼
                          ┌────────────────────────┐
                          │ HAL ADC Callbacks      │ (Half-Cplt / Conv-Cplt Flags)
                          └───────────┬────────────┘
                                      │
                                      ▼
                          ┌────────────────────────┐
                          │ ARM CMSIS-DSP FFT      │ (RFFT 512-point -> 256 Magnitude Bins)
                          └───────────┬────────────┘
                                      │
               ┌──────────────────────┴──────────────────────┐
               ▼                                             ▼
┌─────────────────────────────┐               ┌─────────────────────────────┐
│    ST Edge AI Inference     │               │        UART DMA Tx          │
│ (Quantization -> Model Run) │               │   (Framed Binary Stream)    │
└─────────────────────────────┘               └─────────────────────────────┘
```

# 🚀 Key Features
  * <b>Zero-Copy Double Buffering:</b> Uses DMA circular mode with ```HAL_ADC_ConvHalfCpltCallback``` and ```HAL_ADC_ConvCpltCallback``` to maintain constant ADC sampling without dropped frames.

* <b>Hardware-Accelerated DSP:</b> Uses ```arm_rfft_fast_f32``` (ARM   CMSIS-DSP) to compute real-time 512-point FFTs and derive 256 magnitude bins.

* <b>Non-Blocking Serial Streaming:</b> Custom non-blocking ```_write()``` implementation via USART2 DMA to transmit real-time binary FFT frames with dedicated header/footer delimitation.

* <b>ST Edge AI Ready:</b> Includes quantization helpers (```quantize_input_256```) to scale normalized float spectrum data to INT8 format for direct neural network inference using the ```stai_network``` C library.

* <b>Signal Generation Capabilities:</b> Integrated DAC with DMA-driven 128-point lookup table (LUT) sine wave generation triggered by TIM6 for loopback testing and validation.

# 📂 Code Structure & Implementation Details
1. ```init_dsp()```: Initializes ARM RFFT instances.
2. ```process_ultrasonic_data()```:
   1. Converts raw ADC values to float.
   2. Subtracts DC offset dynamically using ```arm_mean_f32```.
   3. Executes Real FFT (```arm_rfft_fast_f32```).Calculates complex magnitudes (arm_cmplx_mag_f32).
   4. Normalizes array to $0.0$ to $1.0$.Sends payload over UART DMA.
   5. ```quantize_input()```: Converts floating-point FFT magnitudes to INT8 for ST Edge AI model input tensors.
3. ```aiInit() / aiRun()```: Handles model activation allocation and synchronization with ST Edge AI runtime.

# ⚙️Toolchain: STM32CubeIDE

# 🛠️ ST Edge AI: <br>
1. ```Core/Inc/STAI``` folder contains the header source files for the ST Edge AI library at ```Core/Lib/libNetworkRuntime... .a``` <br>
2. ```Core/Inc/network & Core/Src/network``` contains the optimized embedded ML model's data
3. Use <b><a>https://stm32ai-cs.st.com/</a></b> to get the output to be put into these folders
4. Refer to <b><a>https://stedgeai-dc.st.com/assets/embedded-docs/embedded_client_stai_api.html#ref_quick_usage_code</a></b> for usage guide.



