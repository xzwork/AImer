# AImer


## 声明

- 本项目一切源码仅供学习使用，可自由修改并应用于离线游戏，但不应用于在线游戏而破坏游戏公平性。

- 本项目的程序在线上游戏存在封号风险，因违规竞技造成的后果需要自行承担。
 
- 希望各位玩家热爱游戏、尊重对手、珍视账号，共同维护游戏的公平竞技环境。

## 简介

**_AImer_** 是一个基于计算机视觉目标检测的辅助瞄准项目，目前可用于【无畏契约】进行辅助瞄准。

### 关于性能
- 使用显卡加速推理，会与游戏竞争资源，实际推理帧数与设备性能有关
- ***请确保关闭垂直同步！！！*** 否则辅助瞄准的准星将大幅度晃动，无法按照预期工作
 
推理帧数过高不总是好事，当推理比弹道回正速度快时，程序会自动压镜头，导致连续空枪。

您可以在`autoAim.cpp`中注释掉自动开火，自己把握射击的主动权

### 程序逻辑

- 屏幕截取：使用`DXGI`来抓取屏幕中间 $640\times640$ 的区域

- 目标检测：基于`yolo11n`训练，导出为onnx模型，并使用`onnxruntime`进行推理

- 鼠标控制：使用[IbInputSimulator](https://github.com/Chaoses-Ib/IbInputSimulator)模拟鼠标进行控制。
在目标靠近准星时使用吸附模式进行微调，在远距离时直接计算鼠标输入实现“一帧拉”

### 适配情况
 
1. 本项目的目标平台为`windows 11`，使用`NVIDIA`显卡加速推理。
   其他厂商的显卡尚不可用，其他`windows`版本未进行测试。

2. 本项目目前仅适配【无畏契约】，测试灵敏度为*0.1*，模拟罗技鼠标（见[安装依赖](#安装依赖)），屏幕分辨率为$2560\times 1440$，截取为屏幕中心$640\times640$
    - 不同游戏的视场角可能有所差异
    - 模拟不同鼠标驱动默认灵敏度可能会有所差异
    - 不同灵敏度在相同的模拟输入下未必是理想的倍数关系
     
其他游戏以及不同配置尚未进行测试，如有需要，可联系作者[hehaoyang1124@outlook.com](mailto:hehaoyang1124@outlook.com)进行适配。

### 安装依赖

- 您需要安装[NVIDIA显卡驱动程序](https://www.nvidia.cn/geforce/drivers/)，
以便使用[CUDA Toolkit](https://developer.nvidia.com/cuda/toolkit)，[CUDNN](https://developer.nvidia.com/cudnn)进行推理加速
- 您需要鼠标驱动进行模拟鼠标，否则将退为Send Input，可能被大部分反作弊的游戏过滤掉
  - 简单地说，您可以直接安装[Logitech Gaming Software v9.02.65](https://github.com/Chaoses-Ib/IbLogiSoftExt/releases/download/v0.1/LGS.v9.02.65_x64.exe)
  - 如无法加载驱动程序，需[关闭内存完整性保护](https://support.microsoft.com/en-us/windows/a-driver-can-t-load-on-this-device-8eea34e5-ff4b-16ec-870d-61a4a43b3dd5)
  - 更多驱动，详见[IbInputSimulator](https://github.com/Chaoses-Ib/IbInputSimulator)。

### 构建运行

``` powershell 
# 在powershell中运行如下命令
mkdir build
cd build

# 配置
cmake `
-DOpenCV_DIR=path/to/OpenCV `
-DCUDNN_LIB_DIR=Path/to/CUDNN/xxx/lib/xxx/x64 ..

# 编译
cmake --build . --config Release

# 运行
.\Release\AImer.exe <path/to/model.onnx> <sensitivity>
```