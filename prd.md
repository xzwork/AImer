基于现有 AImer 项目进行改造，运行环境：

- Windows 11
- NVIDIA RTX 3060 Ti
- 3840×2160 4K 显示器
- `apex_v11s_640.onnx`
- ONNX Runtime + CUDA

模型已确认：

```text
YOLO11s Detect
Input:  [1,3,640,640]
Output: [1,8,8400]
4 个类别
FP32
Static Shape
NMS=False
```

类别定义固定为：

```text
class 0 = 敌人
class 1 = 我方
class 2 = 倒地
class 3 = 负标签
```

**只有 class 0 参与辅助瞄准。**

class 1、2、3 全部忽略，不需要做复杂的类别优先级、白名单或多目标类别配置。

### 需要完成

1. **增加 Apex 配置**
   - 在 `games.yaml` 增加 Apex。
   - FOV、游戏灵敏度、Aim FOV 等参数配置化。

2. **适配现有 ONNX**
   - 直接支持 `[1,8,8400]` YOLO11s Detect 输出。
   - 保留现有 NMS 后处理。
   - 只保留 `class_id == 0` 的检测结果用于辅助瞄准。

3. **Aim FOV**
   - YOLO 可以检测整个 640×640 区域。
   - 只有 class 0 进入准星附近指定 `aim_fov` 后才移动鼠标。
   - 避免屏幕边缘检测到敌人就直接大幅拉动。

4. **目标选择**
   - 多个 class 0 同时出现时，优先选择距离准星最近的目标。
   - 增加简单 target lock，避免两个敌人距离接近时频繁左右切换。
   - 当前目标消失或离开 Aim FOV 后再重新选择。

5. **4K 适配**
   - DXGI 自动获取实际屏幕尺寸。
   - 4K `3840×2160` 下默认截取屏幕中心 `640×640`。
   - 不写死 2560×1440。
   - 根据屏幕分辨率、截图区域和 Apex FOV 正确计算鼠标移动量。
   - 避免出现检测位置正确但鼠标位移错误的问题。

6. **模型和截图尺寸**
   当前模型为：

```text
Capture: 640×640
ONNX:    640×640
```

第一版直接保持一致。

但 ONNX 输入尺寸仍从模型自动读取，不要在推理代码中硬编码 640，方便以后更换模型。

7. **鼠标输入**
   用户不是罗技鼠标。

   保留现有 `IbInputSimulator` 自动选择输入后端：

```text
Logitech
Razer
AnyDriver
SendInput
DD
MouClassInputInjection
```

   默认 Auto，启动时打印实际使用的输入后端。

   Logitech 不可用时继续尝试其他后端，不要直接报错退出。

8. **CUDA 推理**
   - 使用 ONNX Runtime CUDA。
   - RTX 3060 Ti GPU 推理。
   - batch=1。
   - 保持 FP32，第一版不做 FP16/TensorRT。
   - OpenVINO 如果不是 ONNX CUDA 必需依赖，改为可选。

### 最终效果

```text
启动 AImer.exe
→ 选择 Apex
→ 加载 apex_v11s_640.onnx
→ DXGI 截取 4K 屏幕中心 640×640
→ CUDA 执行 YOLO11s
→ 检测 class 0/1/2/3
→ 只保留 class 0
→ class 1/2/3 全部忽略
→ Aim FOV 内选择最近敌人
→ 简单锁定当前目标
→ 计算正确鼠标偏移
→ 使用可用鼠标输入后端
```

优先小范围修改现有项目，不要重构。

重点修改：

```text
games.yaml
Games.cpp / Games.hpp
autoAim.cpp
MouseController.cpp
Launcher.cpp
```

`Detector.cpp / OnnxDetector.cpp` 当前已经基本兼容此模型，除非实际测试发现问题，否则不要修改 YOLO 输出结构。

先分析源码，再逐模块修改，并保证每一步都能正常编译。