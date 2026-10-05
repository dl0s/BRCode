# BRCode：BlackBerry Q10 二维码应用构建提示词

请在 `C:/Users/dove/Documents/BRCode` 实际开发一个名为 **BRCode** 的 BlackBerry Q10 原生二维码工具，完成源码、可重复构建的 ARM BAR、部署接入和验收说明。核心功能为：**扫描二维码、保存解码文字与用户备注、将输入文字生成二维码**。

请直接检查工程并推进实现，不要只输出方案。当前目录在 2026-10-05 检查时为空；执行时先重新检查，若已经存在文件，就在现有成果上继续。所有应用源码及项目文件写入本目录；同级工程用于参考，不要把其他应用的业务代码和测试数据整包复制过来。

## 1. 已核实的本机基线

以下记录来自 2026-10-05（Asia/Shanghai）的本机文件、SDK 头文件和发布程序检查。执行时复核实际路径和版本；不要将历史文档中的版本段落当成当前配置。

| 项目 | 基线 |
| --- | --- |
| 工作目录 | `C:/Users/dove/Documents/BRCode` |
| 目标 | BlackBerry Q10，按 720×720 屏幕及实体键盘适配 |
| SDK 根目录 | `C:/bbdevtools`；现有用户和进程环境变量 `INTROOP_SDK_ROOT` 均指向这里 |
| QNX_HOST | `C:/bbdevtools/host_10_3_1_12/win32/x86` |
| QNX_TARGET | `C:/bbdevtools/target_10_3_1_995/qnx6` |
| 编译环境 | `qcc.exe`、GCC 4.6.3、ARMv7 little-endian、C++98 |
| 界面环境 | Qt **4.8.6**、Cascades、QML；本机已核实 Qt 版本头文件 |
| C++ ABI | QNX `libcpp.so.4`，沿用现有应用的 ABI 检查 |
| 打包 | `blackberry-nativepackager.bat`，原生 `.bar` |
| 可参考的 JRE | `C:/bbndk/features/com.qnx.tools.jre.win32.x86_64_1.7.0.51/jre/bin`；执行时检查 Java 版本和可用性 |
| 电脑端管理器 | 已核实发布 CLI 的 FileVersion 为 **1.1.6.1** |
| 设备部署目标 | **1.1.6-M** 受管固件；需在真实部署预检中核验当前设备身份和固件 |
| 任务协议 | `protocolVersion: 1`，共享目录 `%LOCALAPPDATA%/Q10Manager/Operations/v1/` |

**1.1.6-M 是设备固件版本，1.1.6.1 是电脑端 BBmanager/Core/CLI 版本，两者不可混用。** 当前实机部署基线以 BBmanager 的 `Documentation/Live116M.md` 和实际发布程序为准；较早文档保留的“仅支持 1.1.5-M”“尚未完成安装验收”等段落属于历史阶段。

本提示词的编写没有连接设备，也没有构建或安装 BRCode。其他应用在该固件上的安装成功，只能证明既有部署机制已有记录，不能代替 BRCode 的验收。

## 2. 开始前读取的工程资料

按用途读取以下文件，检查其引用的当前实现；若文本与实现冲突，说明冲突并核查，不照抄过期步骤。

**构建与原生应用参考：**

- `C:/Users/dove/Documents/ChatBerry/build.ps1`
- `C:/Users/dove/Documents/ChatBerry/BerryAI.pro`
- `C:/Users/dove/Documents/ChatBerry/bar-descriptor.xml`
- `C:/Users/dove/Documents/ChatBerry/tools/audit-bar.py`
- `C:/Users/dove/Documents/BBnote/tools/Sdk-Environment.ps1`
- `C:/Users/dove/Documents/BBnote/build.ps1`

**1.1.6-M 部署与队列协议：**

- `C:/Users/dove/Documents/BBmanager/AGENTS.md`
- `C:/Users/dove/Documents/BBmanager/Documentation/Live116M.md`
- `C:/Users/dove/Documents/BBmanager/Documentation/Stability116.md`
- `C:/Users/dove/Documents/BBmanager/Documentation/AgentIntegration.md`
- `C:/Users/dove/Documents/BBmanager/Documentation/OperationProtocol.md`
- `C:/Users/dove/Documents/BBmanager/Documentation/operation-request.schema.json`
- `C:/Users/dove/Documents/BBmanager/Documentation/BarBuildAndInstallerRecovery.md`
- `C:/Users/dove/Documents/BBmanager/Documentation/MultiDeviceManagement.md`
- `C:/Users/dove/Documents/BBmanager/windows/Q10Manager.Cli/Program.cs`
- `C:/Users/dove/Documents/BBFile/tools/q10_queue.py`

ChatBerry 用作构建、原生界面和数据保存参考；BRCode 的部署接入采用当前 BBmanager 协议。不要继承其他工程中仅适用于该工程、该轮任务的直接 SSH 部署例外。

## 3. 产品行为

### 3.1 扫码

实现原生相机预览，默认使用后置摄像头识别 QR Code。进入页面后按相机异步状态启动扫描；识别成功后暂停当前扫描会话，展示结果，并自动保存一条历史记录。

一次识别只入库一次，不能因连续相机帧重复触发而刷出多条记录。用户点击“继续扫描”后开启新会话；下一次主动扫描相同内容允许形成新记录。相机拒绝授权、被占用、打开失败、预览启动失败时，显示可理解的原因和重试入口。

相机离页、应用后台、锁屏和退出时正确停止并释放。回到前台时依据页面及生命周期状态恢复，不在历史页或生成页继续占用相机。补光功能在设备确实支持并核实 API 后提供。

识别出的 URL、Wi-Fi 文本、联系人等首先按完整文本展示。用户可复制；对于经过格式检查的 HTTP/HTTPS 链接，提供显式“打开链接”操作。扫码本身不自动访问链接或执行内容。

### 3.2 保留文字与历史

“相关文字”指二维码解码正文及用户填写的标题/备注。本版本不扩展为照片文字 OCR。

保存每条记录的唯一 ID、来源（扫描/生成）、原始正文、标题、备注、创建时间和更新时间。扫描正文作为原始记录保留；修改内容后生成新二维码时使用单独的编辑副本，不覆盖原始扫描记录。

使用应用私有目录中的 SQLite 数据库，参考已存在工程选择 QtSql 或 sqlite3，避免同时维护两套存储实现。实现 schema 版本及事务迁移；正常升级和重启不得清空历史。正文使用参数绑定写入，保留中文、英文、换行、空格和特殊字符，不自动 trim、改换行或 Unicode 归一化。

历史页支持时间倒序、全文查看、正文/标题/备注搜索、复制、编辑备注、再次生成二维码和删除单条记录。列表摘要可以省略显示，但数据库、详情、复制和再生成必须使用完整内容。较多记录采用分页或增量加载，避免一次构建全部列表项。

数据库写入失败时保留当前扫描结果并显示“保存失败”，支持重试；不能在提交失败时显示“已保存”。关闭并重新打开应用后，已保存的文字和备注仍应存在。

### 3.3 文字生成二维码

提供多行纯文本输入，支持实体键盘、粘贴、中英文、URL、空格及换行。提供“生成”“全屏显示”“保存 PNG”“复制原文”等操作，并允许从历史记录进入生成页。

每次用户明确生成成功后保存一条生成记录；重新布局、刷新预览和重复绘制不能重复入库。输入未提交的草稿可在切换页面后恢复。

二维码必须黑码白底，保留足够静区，保持正方形和清晰模块边缘。导出采用 PNG；放大优先整数倍及无平滑插值，不裁剪静区，不叠加 Logo。预览与导出应对应同一正文。

空字符串给出提示；不能为了判空修改实际输入。超出编码容量时清楚提示，不截断内容，不改编码来假装成功。容量判断考虑实际 UTF-8 字节及编码器结果，不把一个固定“字符数”当成对所有中文、英文都正确的二维码容量。

生成失败或输入已变化时，不应把旧图展示为本次生成成功。导出失败要保留可重试状态，并报告实际保存位置或错误。二维码生成与扫描的核心流程离线完成，不依赖二维码网站或云端接口。

### 3.4 Q10 界面和中文输入

提供“扫描 / 生成 / 历史”三个清晰入口及结果详情页，使用中文文案、Cascades 原生控件和适合方形屏幕的紧凑布局。长文本可滚动，按钮不被候选栏、操作栏或系统区域遮挡。二维码区域始终保持白底，即使整体采用深色主题。

保留多行输入的 Enter 换行行为。可添加页面级快捷键，但文本字段获得焦点及输入法处理按键时，不得触发全局页面切换、删除或生成动作。

先核实现有宿主的中文输入方案。若需要嵌入 BBIME，参考 `C:/Users/dove/Documents/BBIME/module/README.md`、`C:/Users/dove/Documents/BBIME/module/INTEGRATION_WORKFLOW.md` 及 ChatBerry/BBnote 已集成代码，导入固定版本及其许可、字典和所需资源；不要依赖其他应用正在运行。遵守单进程输入服务、字段会话、离页暂停及宿主按键路由规则。二维码正文可能包含凭据，因此相关字段不启用个人词库学习。已有系统输入足够时，不为首版引入不必要的输入法重构。

“从图片识别二维码”可在上述核心闭环完成后使用现有 API 添加；不得以图片识别代替实际相机扫码验收。

## 4. 优先使用本机已存在的原生二维码 API

本机以下 SDK 头文件和 ARM 库已经确认存在：

```text
C:/bbdevtools/target_10_3_1_995/qnx6/usr/include/bb/cascades/multimedia/Camera.hpp
C:/bbdevtools/target_10_3_1_995/qnx6/usr/include/bb/cascades/multimedia/BarcodeDetector.hpp
C:/bbdevtools/target_10_3_1_995/qnx6/usr/include/bb/cascades/multimedia/BarcodeGenerator.hpp
C:/bbdevtools/target_10_3_1_995/qnx6/usr/include/bb/cascades/multimedia/BarcodeGeneratorFormat.hpp
C:/bbdevtools/target_10_3_1_995/qnx6/usr/include/bb/cascades/multimedia/BarcodeView.hpp
C:/bbdevtools/target_10_3_1_995/qnx6/armle-v7/usr/lib/libbbcascadesmultimedia.so
```

实现时先验证这些接口的最小可编译调用，再连接 UI：

- 相机识别：`bb::cascades::multimedia::Camera` + `BarcodeDetector`，QR 格式枚举为 `BarcodeFormat::QrCode`，识别信号为 `detected`。
- 检测依赖相机有效预览帧；同一 Camera 只能挂一个 BarcodeDetector，不能再并接其他预览帧消费者。
- 图片识别：`BarcodeDetector::scanImage(const bb::ImageData &, ...)`，如实现该功能，先完成正确的图像解码和 ImageData 转换。
- 生成和导出：`BarcodeGenerator::generate(...)`；可以获得 ImageData 或写入图片文件。必须检查返回值及 `error()`。
- 可视预览：可评估 `BarcodeView`，同时处理 `finished` 和错误；不要只设置 data 就宣称生成成功。
- 生成格式为 `BarcodeGeneratorFormat::QrCode`；当前头文件将其说明为 **M 级纠错**。没有验证其他实现前，不添加假设存在的 L/Q/H 设置。
- 链接 `-lbbcascadesmultimedia`，其他库按实际使用和现有工程模式配置。QML import 版本与类型注册方式以本机 SDK 和最小编译结果为准，不凭记忆编造。

原生接口返回 QString，不应声称可以无损保存任意二进制 QR 原始字节。首版明确支持文本 QR；对无法正确表示的编码给出限制说明。若中英文、emoji 或多行文本的回读验证揭示原生编码器/解码器限制，再评估本地兼容依赖并记录原因、版本和许可，不默认移植要求现代 C++ 的最新库。

相机权限名称已由 SDK 确认为 `use_camera`；共享目录图片导出按功能声明 `access_shared`。核心功能不需要录音或后台常驻权限。应用通过系统正常沙箱启动运行，部署通道使用 root 并不意味着应用应以 root 运行。

## 5. 工程和构建要求

建议采用如下职责划分，可按现有代码风格简化：

```text
BRCode/
  AGENTS.md
  README.md
  BRCode.pro
  build.ps1
  bar-descriptor.xml
  src/                  # 应用入口、扫描状态、生成服务、历史存储
  assets/               # QML、图标及必要资源
  tools/                # SDK 检查、BAR 审计、队列提交/观察工具
  tests/                # 编解码和持久化等关键行为验证
  docs/                 # 环境说明、部署说明、Q10 验收及交付记录
  build/                # 可清理的编译产物、日志、任务回执
```

应用名称使用 BRCode，二进制建议 `brcode`，正式包为 `build/BRCode.bar`。应用 ID 初始建议 `top.blaccat.BRCode`；首次确定身份后在升级中保持稳定，并明确维护 versionNumber/buildId。所有入口、图标、QML、输入模块和许可资源必须实际加入 BAR。

从现有构建脚本提取必要结构，采用 `-V4.6.3,gcc_ntoarmv7le_cpp` 和 C++98；不要使用 Qt5/Qt6、Qt Quick Controls、Android SDK 或现代 C++ 专属接口。不要复制 ChatBerry 的网络、模型或凭据模块。

`build.ps1` 至少接受 `-Package` 和 `-SdkRoot`。SDK 解析可复用 BBnote 的参数/环境变量策略，新增环境变量时用 BRCode 自己的名称。Java 通过参数、已有配置或安装目录发现，并核实符合旧打包器要求。先解析主机 Python 3，再修改 SDK PATH，防止误用 NDK 自带的旧 Python。

构建需保存/恢复进程环境，检查每个工具退出码，产生可定位的日志，并输出管理器已有阶段标记：sdkPreflight、compile、elfValidation、package、barValidation。运行 `ntoarm-readelf` 核验实际 ELF32、little-endian、ARM 及依赖 ABI；拒绝错误架构和意外 `libstdc++` 依赖。

打包后审计 Manifest、入口、资产列表、资源摘要及图标，计算 BAR SHA-256。可按现有路径使用 `-package -devMode` 产包，但 devMode 参数不是固件已启用 unsigned 策略的证据，安装仍由管理器完整预检决定。

正式包和诊断包分别记录用途及哈希，避免将测试包当作最终交付。适配测试开关时注意队列目前仅允许 `Package`、`BackendTests`、`ImeDiagnostics`，不要照搬 BBnote 的 `Diagnostics` 参数到请求里。

## 6. 按 1.1.6-M 当前机制部署

将 BBmanager 的适用接入规则写入本项目 `AGENTS.md`，并实现工具接入；只有文档引用而没有真正通过共享队列提交，不算完成部署接入。

固定 CLI 路径为：

```text
C:/Users/dove/Documents/BBmanager/dist/Q10Manager/cli/Q10Manager.Cli.exe
```

BRCode 使用 `client: "BRCode"`。BAR 正式构建、设备连接/诊断、传输和部署通过共享协议提交，由 BBmanager 执行；本项目工具只负责准备请求、提交和读取原任务记录。主机侧源码检查、纯算法测试和本地包审计可以直接运行。

可参考 BBFile 的 `tools/q10_queue.py` 编写本项目包装器，但仅保留 BRCode 需要的通用任务，去掉 BBFile 专用操作。请求提交前落盘任务 ID 和回执；提交返回丢失时先查原 ID，不换 ID 重发。

正式构建使用自定义路径，不采用管理器默认的 BBIntroOP 布局。例如：

```json
{
  "protocolVersion": 1,
  "client": "BRCode",
  "kind": "build",
  "projectDirectory": "C:/Users/dove/Documents/BRCode",
  "sdkDirectory": "C:/bbdevtools",
  "buildScriptPath": "C:/Users/dove/Documents/BRCode/build.ps1",
  "buildOutputPath": "C:/Users/dove/Documents/BRCode/build/BRCode.bar",
  "buildSwitches": ["Package"]
}
```

将请求写为 UTF-8 JSON 文件，调用 CLI `submit`，保留返回的任务 ID。通过 `status`、`events`、`result` 读取结果；失败时用 `failure` 和 `build-logs`。入队、脚本退出 0 或旧 BAR 文件存在，都不能单独证明本次构建成功。

设备类请求需要显式填入 `connection`：优先读取 `%LOCALAPPDATA%/Q10Manager/connection.json`，必要时按现有包装器兼容 `%LOCALAPPDATA%/Q10Deploy/config.json`，正确处理字段大小写并验证必填项。**当前 CLI 的 `submit` 只读取请求文件，不会自动合并当前连接配置；也不要认为额外传入 `--connection` 就会补全 submit 的正文。**

目标按当前选择的设备及已登记 PIN/可信主机公钥确定，不硬编码历史 IP 或 PIN。私钥和 known_hosts 只引用已有本机文件路径，不复制其正文。连接失败保留错误，不能删除 known_hosts 或自动放宽身份检查。

构建和包审计通过后，按执行本任务时已有授权提交 `deploy` 请求，包含 `client`、`connection`、`packagePath`、实际 `packageSha256` 等协议字段。若当前会话只授权本地构建，则先完成 BAR、审计结果及部署请求草稿，再在最终设备安装步骤取得授权；已有明确授权不重复询问。

1.1.6-M 的实际安装由当前管理器预检受管固件摘要、安装消费者及监护器身份、PPS 就绪和既有事务，再执行传输、回读和一次安装。不要把管理器内部的临时脚本、PathTrust 或安装 helper 流程重新实现到 BRCode。

不使用其他应用自己的 SSH/SCP 执行器，不运行 CLI `worker`/`run-once`，不以新私有队列绕过协调。管理器未运行时如实记录 queued，由管理端恢复执行；不回退到直接安装。不套用冻结 1.0.9 的 root SUD 恢复工具，不自动更改 1.1.6-M 的 unsigned/root 初始化或重启服务。

安装通过至少需要管理器成功终态及相互一致的上传回读哈希、原 PPS success/100、应用版本/身份和实际安装目录。通过 `deployment-details <id>` 保留阶段和错误证据。helper 退出 0、进度 100、设备在线或 unsigned 配置字样均不单独等于安装成功。

超时、断连或取消后的 `unconfirmed` 保留原任务、PPS 和队列屏障，使用 `continue-deployment <原部署ID>` 或协议的 `deploymentObservation` 观察原事务。续查命令本身也只是提交观察任务，需要读取返回 ID 的结果；它不会自动把原部署状态改为成功。不得换新 ID 重装、清理 PPS、删除锁或自动 acknowledge/abandon/delete。

安装与前台启动、业务验收分别记录。先确认管理器当前协议是否提供适用于 BRCode 的启动入口；没有通用启动能力时，提供由用户点击设备图标的验证步骤，不借用 BBFile 专用任务或编造任意远程命令。无法连接设备或进行物理操作时，完成其余可验证工作并将相应项目标为待验收。

## 7. 实现顺序与验收

先完成环境核对及最小原生二维码接口编译，再完成“扫描 → 查看并保存 → 从历史生成 → 导出 PNG”的闭环。随后补齐持久化、异常状态、实体键盘和生命周期，完成正式 BAR 与部署接入。根据证据修复实际问题，不为低风险文件布局变化编写形式化测试。

关键行为验证至少覆盖：

1. **文本保真**：ASCII、中文、多行、首尾空格、特殊符号、URL 和可支持的 emoji；生成图片后解码并与输入逐项比较。原生 QString 的编码局限如实记录。
2. **编码边界**：空串、仅空白、接近容量、超容量、非法或不支持数据；失败不截断、不保存假成功记录、不显示过期二维码。
3. **识别会话**：同一码连续多帧只保存一次，用户“继续扫描”后可以再次记录；相机事件晚到时不向已关闭页面写入结果。
4. **历史存储**：保存、搜索、完整复制、备注编辑、删除、重启持久化、事务失败及 schema 升级保持。不得依靠删库重建通过迁移测试。
5. **图片导出**：文件确实存在且可重新解码，中文和换行回读一致；路径不可写或共享权限拒绝时有正确错误状态。
6. **Q10 原生交互**：实际相机识别、相机权限拒绝/占用、后台与锁屏恢复、重复进入扫码页、物理键盘输入和长文本操作。用另一设备扫描 BRCode 屏幕或导出 PNG，检查跨设备文本一致性。
7. **应用更新**：同一应用身份升级后仍能查看已有正文和备注；以实际数据核验，不能借用其他应用的升级成功记录。

把主机测试、SDK 编译、设备自动证据及用户人工操作分别记录。图像回读自测不能代替相机测试，同一编解码库自测也不能代替跨设备兼容性测试。没有执行的测试标记 NOT TESTED；环境阻塞说明具体原因；不把估计的响应速度或纸面测试项目数写成实测结果。

## 8. 最终交付

交付可维护源码、构建脚本、完整 BAR 资源清单、本地包审计、SHA-256、队列提交/观察工具、README、部署说明和简短 Q10 人工验收卡。

交付报告写清：实现的功能、确定的依赖及版本、构建命令、BAR 绝对路径及哈希、已执行的测试及结果、部署原任务 ID/终态、实际设备验证范围、剩余限制。源文件或构建成功不能代替可安装产物；安装成功也不能代替扫码、文字保存和中文二维码回读通过。

现在从检查当前目录和上述本机资料开始，按此要求推进应用实现。
