# AImer Virtual HID Mouse（本地 / 离线测试）

这是仓库自行实现的 KMDF HID source driver，使用 Windows 内置 VHF 枚举鼠标；没有第三方鼠标驱动依赖，没有反作弊绕过、隐藏或检测规避功能。仅提供相对 X/Y 位移，不支持按键、滚轮、绝对移动。

```text
AImer → MouseController → VirtualHidBackend
      → CreateFile / DeviceIoControl → KMDF / VHF → Windows 鼠标输入
```

## 配置与行为

### 使用现有 Start-AImer.cmd

构建 AImer 会将驱动启动入口部署为 `bin\Release\Start-AImer.cmd`。这个入口固定设置 `AIMER_INPUT_BACKEND=virtual_hid`，先以管理员权限检查设备，再启动 AImer；不会回退到其他鼠标后端。原有 `Start-AImer-Win32.cmd` / `Start-AImer-SendInput.cmd` 可继续独立使用。

首次双击 `Start-AImer.cmd`，如测试签名模式未开启，会解释影响并要求输入 `YES` 才启用；随后需要手动重启 Windows。重启后再次打开同一个文件，若设备尚未安装，会在确认后导入本地测试证书并安装驱动。安装和设备打开检查成功后启动 AImer；以后直接使用这个入口即可。拒绝设置、安装失败或设备检查失败都不会启动 AImer。脚本不会自动重启，也不会自动关闭 Secure Boot。

脚本依赖同目录的 `VirtualHid.ps1`、`VirtualHidCheck.exe`、`Install-VirtualHidDriver.ps1` 和 `VirtualHidDriver` 签名包，请保留在一起。`Enable-VirtualHid-TestMode.bat` / `Install-VirtualHid.bat` 仅是可选的独立设置入口。

当前生成的是**本地测试签名**，并非微软生产签名。证书有效期至 2027-09-28，私钥仅保存在本机当前用户的 `My` 证书库，未导出。签名后的包位于 `bin\Release\VirtualHidDriver`，不会被普通用户态构建覆盖。修改驱动并重新构建后，必须重新签名：

```powershell
.\drivers\VirtualHidMouse\test-sign.ps1 -Thumbprint E76C20D0337B05B8A1A5B51CEA3BBBA58A205AA0
```

脚本先签署 SYS，再重建 CAT 并签署 CAT，导出公钥证书和 SHA-256 文件清单。安装脚本会核对文件清单与签名证书，再向 LocalMachine Root / TrustedPublisher 导入该代码签名证书。签名本身不会开启测试模式或导入系统信任；安装前 Windows 可能将自签名证书显示为不受信任。

卸载后如不再需要测试环境，可在管理员终端运行 `bcdedit /set testsigning off` 并重启。确认没有其他使用此测试证书的驱动后，按上述精确指纹移除 LocalMachine Root / TrustedPublisher 中的测试证书；不要删除其他证书。

修改 **可执行文件旁** 的 `mouse.yaml`，或修改仓库根目录文件后重新编译：

```yaml
mouse_backend: virtual_hid
```

| 配置值 | 后端 |
| --- | --- |
| `virtual_hid` | 本项目的 KMDF/VHF 驱动 |
| `send_input` / `Win32` | 原生 Windows SendInput（默认） |
| `ib_input` / `SendInput` | IbInputSimulator 的 SendInput 模式 |
| `Auto` | 保留原有 IbInputSimulator 驱动选择流程 |

已有的 `AIMER_INPUT_BACKEND` 环境变量优先于 YAML，接受表中的值。移除环境变量才能使用文件配置。未找到 `mouse.yaml` 时保持原有默认后端；配置无效时记录错误并禁用输入。配置在首次初始化时读取，修改后重启应用。构建时会将仓库 `mouse.yaml` 复制到输出目录。

两个 `autoAim.cpp` 无需修改：现有 `MoveRelative(dx, dy)` 调用直接进入所选后端。Virtual HID 的 `fire()` 为无操作，并仅提示一次“不支持点击”；不会借用其他后端点击。

设备限定一个客户端，ACL 仅允许 SYSTEM / 管理员。测试程序与 AImer 需要以管理员身份运行。未安装、设备未启动、权限不足、设备占用时，`CreateFile` 错误会写入 stderr，禁用该后端，不抛出初始化异常、不调用空函数指针、不自动回退。安装或恢复设备后重启应用。IOCTL 失败记录 Win32 错误码，鼠标诊断日志按秒合并；`AIMER_INPUT_TRACE=1` 可额外写入当前工作目录的 `mouse-input.log`。

协议定义在 `shared/VirtualHidProtocol.h`：一个 `METHOD_BUFFERED`、需要写权限的 IOCTL，以及固定 8 字节的两个 `LONG`。驱动严格校验 IOCTL、输入/输出长度和范围。HID 报告为 ID 1 + 两个有符号 16 位相对坐标，无按键字段。较大的 `int` 位移由用户态拆分，保持总位移；任何分段失败即停止，不重放已经成功的片段。因此失败可能已经产生部分移动。API 成功表示报告已提交，不保证光标像素距离或任意应用的接收结果。

## Release 编译

驱动和 AImer 分开构建。目标为 Windows 10 2004+ / Windows 11 x64；安装脚本使用 Windows PowerShell 5.1 的 PnpDevice 模块。准备与所选 WDK 版本兼容的 Visual Studio C++ 工具、Windows SDK、WDK 及其 Visual Studio 驱动集成。**只有 SDK 中的 `vhf.h` 不够**，必须同时存在 KMDF 的 `wdf.h`、`VhfKm.lib` 和 `WindowsKernelModeDriver10.0` 工具集。

`WinSDKSetup.exe` 和 `WindowsSDK.iso` 只安装 SDK，不能替代 WDK。VS 2026 使用匹配的 28000 系列 SDK/WDK，并在 Visual Studio Installer 的“单个组件”中安装 Windows Driver Kit 和 x64/x86 Spectre 构建库；参见 [微软 WDK 安装说明](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)。

在对应 Visual Studio 的 Developer PowerShell 中，从仓库根目录运行：

```powershell
& "$env:VSINSTALLDIR\MSBuild\Current\Bin\amd64\MSBuild.exe" .\drivers\VirtualHidMouse\AImerVirtualMouse.vcxproj /m /p:Configuration=Release /p:Platform=x64 /nr:false
```

必须使用 **x64 MSBuild**；本次安装的 WDK 在 x86 MSBuild 下出现 InfVerif DLL 加载错误和 API 校验工具退出码 193。上面的命令在 Developer PowerShell 中通过 `VSINSTALLDIR` 定位 x64 工具。工程默认 SDK/WDK 版本为 `10.0.28000.0`，如需其他匹配版本，附加 `/p:WindowsTargetPlatformVersion=<已安装的匹配版本>`。

项目会生成 SYS、处理 INF 并创建驱动包/目录文件。已验证的包目录为 `drivers\VirtualHidMouse\out\x64\Release\AImerVirtualMouse`，内含 **AImerVirtualMouse.inf、AImerVirtualMouse.sys、aimervirtualmouse.cat**。将这个目录作为后续 `$package`。项目关闭自动签名，**Release 不等于已签名**。需要静态分析时，在构建命令后追加 `/t:Rebuild /p:RunCodeAnalysis=true`。

AImer 使用原有 OpenCV / ONNX Runtime 等依赖。示例（VS 2026；其他版本使用对应生成器）：

```powershell
cmake -S . -B out\release-vhid -G "Visual Studio 18 2026" -A x64 -DOpenCV_DIR="C:/path/to/opencv/build/x64/vc16/lib" -DAIMER_WITH_OPENVINO=OFF
cmake --build out\release-vhid --config Release --target AImer AImer-aimlab --parallel
```

无需给 AImer 配置 WDK include/lib。保持项目原有 DLL 部署要求；编译输出会带上 `mouse.yaml`。默认配置保留 `send_input`，按上文显式切换。

## 测试签名与安装

在隔离测试机或可恢复 VM 中使用正常的 Windows 测试签名流程。准备受测试机信任的代码签名证书，分别签名 SYS 和最终 CAT；不要修改签名后的 INF/SYS。需要修改 SYS 时，应先签名 SYS，再用 WDK `Inf2Cat` 重建 CAT，最后签名 CAT。例如，`$package` 指向前述驱动包，`$thumbprint` 为已有测试证书的指纹：

```powershell
signtool sign /v /fd SHA256 /s My /sha1 $thumbprint "$package\AImerVirtualMouse.sys"
Inf2Cat /driver:$package /os:10_X64 /uselocaltime
signtool sign /v /fd SHA256 /s My /sha1 $thumbprint "$package\AImerVirtualMouse.cat"
```

目标系统和 WDK 使用不同的 Inf2Cat OS 标识时，按 `Inf2Cat /?` 选取匹配标识。测试证书的公钥证书需由测试机管理员导入 LocalMachine 的 Root / TrustedPublisher 存储。按 Windows 官方流程配置测试签名并重启；Secure Boot/组织策略可能阻止开启测试模式，按测试环境策略处理。脚本不会创建证书、改变启动策略或关闭系统安全功能。生产签名不在本项目范围内。

在管理员 **Windows PowerShell** 中安装，`DevConPath` 指向 WDK 的 x64 `devcon.exe`：

```powershell
.\drivers\VirtualHidMouse\install.ps1 -PackagePath $package -DevConPath "C:\path\to\WDK\x64\devcon.exe"
```

首次安装用 DevCon 创建 `Root\AImerVirtualMouse` 根设备；已有单个设备时更新，避免重复创建。`pnputil /add-driver /install` 本身不能创建这个 root devnode。DevCon 仅用于安装本项目驱动，不参与运行时输入。检查设备管理器里的 **AImer Virtual HID Mouse (Local Test)** 与其 VHF 子设备；安装失败查看 `%windir%\inf\setupapi.dev.log`，运行失败查看程序的 Win32 错误码，VHF 创建/启动失败可用内核调试器查看 `AImerVirtualMouse` 日志。

退出所有客户端后卸载：

```powershell
.\drivers\VirtualHidMouse\uninstall.ps1
```

脚本仅匹配 `Root\AImerVirtualMouse` 的精确硬件 ID，移除该设备及其绑定的 `oem*.inf` 包，不删除系统 VHF，不使用强制包删除。若包仍被占用，重启后检查 `pnputil /enum-drivers`；对于已无设备引用的遗留 AImer 包，核对 Original Name 为 `AImerVirtualMouse.inf`、Provider 为 `AImer Local Test` 后，通过 `pnputil /delete-driver oemNN.inf` 删除该特定包。脚本不会自动清理无关联的包或测试证书。需要恢复启动策略时由测试机管理员执行原流程的逆操作。

## 不依赖模型/GPU的验证

独立工程只需用户态 C++ 工具和 Windows SDK，无需 WDK 或 OpenCV：

```powershell
cmake -S tests\virtual_hid -B out\virtual-hid-tests -G "Visual Studio 18 2026" -A x64
cmake --build out\virtual-hid-tests --config Release --parallel
ctest --test-dir out\virtual-hid-tests -C Release --output-on-failure
```

自动测试替换用户态 OS 调用，覆盖设备缺失/拒绝访问/占用、句柄生命周期、零位移、正负位移、16 位边界、INT_MIN/MAX 的完整拆分和中途 IOCTL 失败；另一个用例检查无效后端不会崩溃。测试不会创建真实输入，**不能代替内核加载验证**。

真实设备检查（默认只打开设备，不移动）：

```powershell
.\out\virtual-hid-tests\Release\VirtualHidCheck.exe
# 仅在受控桌面上主动移动：
.\out\virtual-hid-tests\Release\VirtualHidCheck.exe --move 80 -40
# 检查实际配置解析与缺失驱动保护，不移动或点击：
$env:AIMER_INPUT_BACKEND = 'virtual_hid'
.\out\virtual-hid-tests\Release\VirtualHidCheck.exe --controller-noop
Remove-Item Env:AIMER_INPUT_BACKEND
```

直接检查程序的退出码：0 成功，1 参数/其他异常，2 设备打开失败，3 IOCTL 失败。`--controller-noop` 用于验证“不崩溃”，后端不可用时仍返回 0，需检查日志。也可在检查程序旁放 `mouse.yaml` 验证文件配置。

交付前在测试 VM 上补做：加载/卸载与重装、正负/零/大位移、未知 IOCTL/错误长度/越界输入的拒绝、客户端存活时设备移除、休眠唤醒、Driver Verifier。确认设备移除后的调用失败且应用继续运行，恢复后重启客户端。

实现依据：[Microsoft VHF/KMDF 指南](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/virtual-hid-framework--vhf-)、[VhfReadReportSubmit 的默认缓冲语义](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/vhf/nf-vhf-vhfreadreportsubmit)。驱动采用 VHF 默认缓冲和 KMDF 顺序队列，在设备清理时同步删除 VHF 对象。

## 本次验证记录

2026-09-28：使用 MSVC 19.51 / Windows SDK 10.0.26100.0 完成 AImer、AImer-aimlab 和独立检查程序的 x64 Release 编译；CTest 2/2 通过。实际运行确认 YAML 选择、环境变量优先级、损坏 YAML 的容错，以及缺失设备时 `CreateFile` 返回错误 2 且控制器继续运行。安装/卸载脚本通过 PowerShell 语法解析，项目 XML 解析通过。

同日补齐 Visual Studio WDK 集成、Spectre 构建库及 WDK 10.0.28000.2526 后，使用 x64 MSBuild 完成驱动 x64 Release 完整重编译及 `/p:RunCodeAnalysis=true` 静态分析，结果通过。修正了 SDK/WDK 版本定位和 INF 的最低 Windows build 19041 声明；INF 校验、Universal API 校验、Inf2Cat 可签名性检查及 CAT 生成均通过，未报告错误或警告。

随后完成 `bin\Release\VirtualHidDriver` 中 SYS/CAT 的 SHA-256 本地测试签名、CAT 密码学签名校验和文件清单生成。尚未安装或加载 AImer 驱动，实际 VHF 鼠标移动和上述 VM 验证仍待首次设置完成。没有更改测试签名设置或启动安全策略；签名成功不代表已验证内核运行。
