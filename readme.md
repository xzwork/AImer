# AImer - AI-Assisted Aiming Learning Project Based on Computer Vision Object Detection

## Disclaimer

- Educational Use Only: Do not use in online games to preserve fair play.
- Risk Warning: Using this in online games may result in account bans. Users assume full responsibility for any
  consequences.
- Fair Play: We urge players to uphold a fair competitive environment.

## Introduction

**_AImer_** is an aim-assist project based on computer vision object detection,
currently applicable to the game ***Valorant, CS2, Global Strike***.

### Performance

- GPU-accelerated inference competes with the game for computational resources.
  The actual inference efficiency depends on your hardware performance.

NOTE:  Maxing out inference FPS can backfire. If it's too fast for the recoil recovery,
the script will over-correct and pull your crosshair down, causing you to whiff.
To fix this, comment out the auto-fire code in `autoAim.cpp` and handle the shooting yourself.

### Program Logic

- **Screen Capture**: Uses `DXGI` to capture a $640\times640$ region from the center of the screen.

- **Object Detection**: Trained based on `yolo11n`, exported as an ONNX model, and inferred using `onnxruntime`.

- **Mouse Control**: Uses [IbInputSimulator](https://github.com/Chaoses-Ib/IbInputSimulator) to simulate mouse input.
    - Linear fine-tuning for close targets
    - Angular projection for large turns

### Compatibility

1. The target platform for this project is `Windows 11` with `NVIDIA` GPUs for accelerated inference.
   GPUs from other manufacturers are currently unsupported. Other Windows versions have not been tested.

2. This project simulates a Logitech mouse (see [Dependencies-Installation](#dependencies-installation)),
   with a screen resolution of $2560\times 1440$, capturing the center $640\times640$ region of the screen.
   **Notes:**
    - Default sensitivity may vary depending on the simulated mouse driver
    - The FOV for screen capture may differ across resolutions

3. Currently supported games and recommended settings are as follows

   | # | Game          | Mouse Sensitivity           | V-Sync |
   |---|---------------|-----------------------------|--------|
   | 0 | Valorant      | 0.1                         | Off    |
   | 1 | CS2           | 1.0                         | Off    |
   | 2 | Global Strike | 10, disable mouse smoothing | On     |

   **Notes:**
    - Different sensitivities may not scale ideally under the same simulated input.
      If performance doesn't meet expectations, try the settings above
    - Different game engines handle mouse input differently.
      If the crosshair wobbles near the target, try enabling/disabling V-Sync

4. Currently supported models

   | # | Game          | Model             | Class 0 | Class 1 | Class 2 | Class 3 |
   |---|---------------|-------------------|---------|---------|---------|---------|
   | 0 | Valorant      | valorant-bot.onnx | Head    | Body    | -       | -       |
   | 1 | CS2           | cs2.onnx          | CT Body | CT Head | T Body  | T Head  |
   | 2 | Global Strike | ssjj.onnx         | CT Body | CT Head | T Body  | T Head  |

Other games and configurations have not been tested. For adaptation requests, please contact the author
at [hehaoyang1124@outlook.com](mailto:hehaoyang1124@outlook.com).

## Usage

### Dependencies Installation

- You must install the [NVIDIA Graphics Driver](https://www.nvidia.cn/geforce/drivers/) to
  utilize [CUDA Toolkit](https://developer.nvidia.com/cuda/toolkit) and [CUDNN](https://developer.nvidia.com/cudnn) for
  inference acceleration.
- A mouse driver is required to simulate mouse input; otherwise, the program will fallback to `Send Input`, which is
  likely filtered out by most anti-cheat systems.
    - Simply
      install [Logitech Gaming Software v9.02.65](https://github.com/Chaoses-Ib/IbLogiSoftExt/releases/download/v0.1/LGS.v9.02.65_x64.exe).
    - If the driver fails to load, you may need
      to [disable Memory Integrity](https://support.microsoft.com/en-us/windows/a-driver-can-t-load-on-this-device-8eea34e5-ff4b-16ec-870d-61a4a43b3dd5).
    - For more driver options, see [IbInputSimulator](https://github.com/Chaoses-Ib/IbInputSimulator).

### Launching the Program
- Launch command: `AImer.exe <game_id> <target_class> <in_game_sensitivity> <model_path>`
- Game ID, target class, and model names are listed in the [Compatibility](#compatibility) table
- Example: `AImer.exe 0 0 0.2 "../models/valorant-bot.onnx"` means playing Valorant,
  aiming at `head`, with sensitivity of 0.2, and model path at `../models/valorant-bot.onnx`
- The above parameters are for the original code. Feel free to modify as needed

## Build and Run (Optional)

```powershell 
# Run the following commands in PowerShell
mkdir build
cd build

# Configure
cmake `
-DOpenCV_DIR=path/to/OpenCV `
-DCUDNN_LIB_DIR=Path/to/CUDNN/xxx/lib/xxx/x64 ..

# Build
cmake --build . --config Release

# Run
.\Release\AImer.exe <path/to/model.onnx> <sensitivity>
```