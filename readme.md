# AImer - AI-Assisted Aiming Learning Project Based on Computer Vision Object Detection

<div align="center">
  <a href="LICENSE"><img alt="License" src="./assets/license.svg"></a>
 <br>
  <img alt="C++" src="./assets/c++.svg">
  <img alt="CMake" src="./assets/cmake.svg">
  <img alt="Platform" src="./assets/platform.svg">
</div>

<div align="center">
  <a href="readme-zh.md">简体中文</a>
</div>

## Disclaimer

- Educational Use Only: Do not use in online games to preserve fair play.
- Risk Warning: Using this in online games may result in account bans. Users assume full responsibility for any
  consequences.
- Commercial Use Prohibited: Please refer to the [LICENSE](LICENSE) for details.
- Fair Play: We urge players to uphold a fair competitive environment.

## Introduction

**_AImer_** is an aim-assist project based on computer vision object detection,
currently applicable to the game ***Valorant, CS2, Global Strike***.

### Performance

#### OpenVINO (CPU Inference)

- The latest **_AImer_** update introduces IR models.
  The new model is based on YOLO object detection with the
  backbone replaced by MobileNetV4 which is more lightweight.

- The new model is deployed using OpenVINO for CPU inference,
  making it suitable for devices without NVIDIA GPUs.
  It also avoids consuming GPU resources needed for game rendering.

- This model trades a small amount of precision for higher inference FPS.

#### ONNX Runtime (NVIDIA GPU CUDA-Accelerated Inference)

- The YOLO-based model uses ONNX Runtime for inference,
  with CUDA acceleration available on NVIDIA GPUs for higher inference FPS.

- Compared to MobileNetV4, the original DarkNet backbone offers higher precision.
  However, GPU-accelerated inference resulting in lower FPS when rendering the game.
  Actual inference FPS depends on your hardware.

> **Note:** Maxing out inference FPS can backfire. If it's too fast
> for the recoil recovery, the script will over-correct and pull
> your crosshair down, causing you to whiff. To fix this,
> comment out the auto-fire code in `autoAim.cpp` and handle
> the shooting yourself. (auto-fire is disabled by default).

### Program Logic

- **Screen Capture**: Uses `DXGI` to capture a 640×640 region from the center of the screen.

- **Object Detection**: YOLO-based object detection models, exported as ONNX / IR models, and inferred using ONNX
  Runtime or OpenVINO.

- **Mouse Control**: Uses [IbInputSimulator](https://github.com/Chaoses-Ib/IbInputSimulator) to simulate mouse input.
  Applies an adsorption mode for fine-tuning when targets are near the crosshair, and directly computes mouse input for
  instant "one-frame flicks" at longer ranges.

### Compatibility

1. The target platform for this project is `Windows 11` with `NVIDIA` GPUs for accelerated inference. GPUs from other
   manufacturers are currently unsupported. Other Windows versions have not been tested.

2. This project simulates a Logitech mouse (see [Dependencies Installation](#dependencies-installation)),
   with a screen resolution of 2560×1440, capturing the center 640×640 region of the screen.
    - Default sensitivity may vary depending on the simulated mouse driver
    - The FOV for screen capture may differ across resolutions

3. Currently supported games and recommended settings

   | Launch Name | Game           | Mouse Sensitivity            | V-Sync |
   |-------------|----------------|------------------------------|--------|
   | valorant    | Valorant       | 0.1                          | Off    |
   | cs2         | CS2            | 1.0                          | Off    |
   | ssjj        | Global Strike  | 10, disable mouse smoothing  | On     |
   > **Notes:**
   > - Different sensitivities may not scale ideally under the same simulated input.
       If performance doesn't meet expectations, try the settings above
   > - Different game engines handle mouse input differently.
       If the crosshair wobbles near the target, try enabling/disabling V-Sync

4. Currently supported detection classes

   | Launch Name | Game           | Class 0       | Class 1          | Class 2    | Class 3        |
   |-------------|----------------|---------------|------------------|------------|----------------|
   | valorant    | Valorant       | head          | enemy            | -          | -              |
   | cs2         | CS2            | ct            | ct_head          | t          | t_head         |
   | ssjj        | Global Strike  | ct            | ct_head          | t          | t_head         |

5. Custom Games
   - All game configurations are loaded from [games.yaml](games.yaml).
     You can edit this file to add support for other games. 
   - You can use your own models (currently only supports YOLO-detect). 
     Make sure the target order matches your model's class categories. 
   - Other games and configurations have not been tested. For adaptation requests, please contact the author
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

#### 1. Launch from the Launcher UI

1. Simply double-click AImer.exe or run it without any arguments to open the Launcher window as shown below.
2. Select the game, target, and mouse sensitivity (in-game sensitivity) from the dropdown menus.
3. Choose the model file and weight file (optional) for the target.
4. Click the **_Launch AImer_** button to start.

|                                    |                                              |                                                  |
|------------------------------------|----------------------------------------------|--------------------------------------------------|
| ![EUI-NEO.png](assets/EUI-NEO.png) | ![GameDropdown.png](assets/GameDropdown.png) | ![TargetDropdown.png](assets/TargetDropdown.png) |

#### 2. Launch from the Command Line
The launch command help is as follows:

```bash
Usage: AImer.exe [OPTIONS]
Options:
  -h,--help                            Print this help message and exit
  -n,--name TEXT REQUIRED              Game name (valorant|cs2|ssjj)
  -t,--target TEXT REQUIRED            Target name (e.g. head, enemy, ct, t)
  -s,--sensitivity FLOAT REQUIRED      Mouse sensitivity
  -m,--model TEXT:FILE REQUIRED        Model path (.xml/.onnx)
  -w,--weights TEXT:FILE               Weights path (.bin), use OpenVINO when provided
```

- Launch command:
  `AImer.exe -n <game_name> -t <target_name> -s <in_game_sensitivity> -m <model_path> -w <weights_path (optional)>`
- Game names and target names are listed in the [Compatibility](#compatibility) table.
- The weights path is optional:
    - With `-w` specified: uses OpenVINO (CPU) inference
    - Without `-w`: uses ONNX Runtime (NVIDIA GPU CUDA acceleration) inference
- Example: `AImer.exe -n valorant -t head -s 0.1 -m ../models/valorant-bot.onnx`
    - Game: Valorant
    - Target: head
    - Sensitivity: 0.1
    - Model: `../models/valorant-bot.onnx`
    - No weights file, using NVIDIA GPU inference

## Build and Run (Optional)

If you want to modify the source code or compile it yourself:

Prerequisites: [CMake](https://cmake.org/download/), [Visual Studio](https://visualstudio.microsoft.com/vs/), [OpenCV](https://opencv.org/releases/), [CUDNN](https://developer.nvidia.com/cudnn)

```powershell 
# Clone the repository and submodules
git clone https://github.com/HeHaoyang1124/AImer.git
cd AImer
git submodule update --init --recursive

mkdir build
cd build

# Configure
cmake `
-DOpenCV_DIR=path/to/OpenCV `
-DCUDNN_LIB_DIR=Path/to/CUDNN/xxx/lib/xxx/x64 `
-DOpenVINO_DIR=path/to/OpenVINO ..

# Build
cmake --build . --config Release

# Run
..\bin\Release\AImer.exe `
-n valorant -t head -s 0.1 `
-m ..\models\valorant\valorant-bot.xml `
-w ..\models\valorant\valorant-bot.bin
# Or simply run ../AImer.exe to launch the Launcher UI.
```