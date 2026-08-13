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

# 📁 Project Structure
```bash
astri_project/
├── astri_project.ioc                  # STM32CubeMX project configuration
├── astri_project.launch               # STM32CubeIDE debug/run launch config
├── .cproject / .project               # STM32CubeIDE build & project metadata
├── STM32G474RETX_FLASH.ld             # Linker script (flash)
├── STM32G474RETX_RAM.ld               # Linker script (RAM)
├── *.tflite-...-code.zip              # Raw ST Edge AI generator output (reference only)
├── Core/
│   ├── Inc/                           # Application headers
│   │   ├── STAI/                      # ⭐ ST Edge AI runtime headers (ai_*.h, layers_*.h, lite_*.h, stai.h, ...)
│   │   ├── network/                   # ⭐ ST Edge AI generated model headers (network.h, network_data.h, network_details.h)
│   │   ├── PDM2PCM/                   # PDM microphone filter headers
│   │   ├── main.h
│   │   ├── stm32g4xx_it.h
│   │   └── arm_math.h                 # CMSIS-DSP math wrapper
│   ├── Src/                           # Application sources
│   │   ├── network/                   # ⭐ ST Edge AI generated model sources (network.c, network_data.c, network_c_info.json)
│   │   ├── PDM2PCM/
│   │   ├── main.c                     # App entry: DSP pipeline + aiInit()/aiRun()
│   │   ├── stm32g4xx_it.c
│   │   └── system_stm32g4xx.c
│   ├── Lib/                           # Static libraries (linker search path)
│   │   ├── libNetworkRuntime1201_CM4_GCC.a   # ⭐ ST Edge AI runtime library
│   │   ├── libarm_cortexM4lf_math.a          # CMSIS-DSP math library
│   │   └── libPDMFilter_CM4_GCC_wc32.a       # PDM filter library
│   └── Startup/                       # startup_stm32g474retx.s
├── Drivers/                           # STM32Cube HAL + CMSIS device headers/sources
├── Middlewares/
│   └── Third_Party/ARM_CMSIS/CMSIS/   # CMSIS core + CMSIS-DSP (Include, Lib/GCC)
└── Debug/                             # Build output (generated, not committed)
```
> ⭐ marks folders that are populated by the ST Edge AI code generator. See below.

# 🛠️ ST Edge AI Output Placement
The embedded model is generated with the **ST Edge AI** toolchain
([ST Edge AI Developer Cloud](https://stm32ai-cs.st.com/)) and is delivered as a zip archive
(e.g. `best_arc_fault_131_stm32.tflite-NUCLEO-G474RE-code.zip`). The archive contains a `network*` model
fileset, an `Inc/` folder with the runtime headers, and a `Lib/NetworkRuntime<...>.a` precompiled runtime library.
Copy the generated artifacts into the folders below **exactly** as follows:

| ST Edge AI output (zip)              | Destination in this project                | Content                                   |
|--------------------------------------|--------------------------------------------|-------------------------------------------|
| `Inc/*.h`                            | `Core/Inc/STAI/`                           | Runtime headers (`stai.h`, `ai_*.h`, `layers_*.h`, `lite_*.h`, `core_*.h`, ...) |
| `Lib/NetworkRuntime<ver>_CM4_GCC.a`  | `Core/Lib/libNetworkRuntime<ver>_CM4_GCC.a`| Precompiled ST Edge AI runtime library     |
| `network.h`, `network_data.h`, `network_details.h` | `Core/Inc/network/`          | Generated model interface & metadata       |
| `network.c`, `network_data.c`, `network_c_info.json` | `Core/Src/network/`       | Generated model graph & weights            |
| `network_generate_report.txt`, `LICENSE*`, `stdout/stderr.txt` | *(root, reference only)* | Reports / licensing – not compiled |

Notes:
1. The `network.h` includes are `"stai.h"` / `"layers.h"`, so the `STAI/` and `network/` folders **must** stay
   in `Core/Inc` — these are already wired into the build via the include paths in `.cproject`
   (`../Core/Inc/STAI`, `../Core/Inc/network`).
2. `Core/Lib` is already configured as the linker search path (`-L ../Core/Lib`) and the runtime
   library is linked by name, so only the matching `libNetworkRuntime<ver>_CM4_GCC.a` must be copied.
3. Regenerating / re-exporting the model **only overwrites** `Core/Inc/network`, `Core/Src/network`,
   `Core/Inc/STAI`, and `Core/Lib/libNetworkRuntime*.a`. Application code in `main.c`
   (e.g. `quantize_input_256`, `aiInit()`, `aiRun()`) is **not** touched and keeps working, as the
   `STAI_NETWORK_*` macros from `network.h` drive buffer allocation.
4. Model input here is a quantized `STAI_FORMAT_S8` spectrum of 131 samples → 1 output (arc-fault classifier),
   produced by `quantize_input()` from the normalized FFT magnitude bins.

# ⚙️ Toolchain: STM32CubeIDE
Refer to <b><a>https://stedgeai-dc.st.com/assets/embedded-docs/embedded_client_stai_api.html#ref_quick_usage_code</a></b> for the ST Edge AI runtime usage guide.



