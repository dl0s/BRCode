# BRCode 开发规则

目标 BlackBerry Q10 / ARMv7 / Qt 4.8.6 / Cascades / C++98，应用身份 `top.blaccat.BRCode`。
源码、固定输入模块、资源与测试只放本工程；不编入兄弟工程的业务代码或真实数据。

## Q10 构建、连接及部署

统一依据 `C:/Users/dove/Documents/BBmanager/Documentation/标准操作文档.md`。
2026-10-05 当前 CLI 为 1.1.6.5；设备仅支持准确 1.1.6-M，必须由实时预检确认，两者不同。
用户指定测试机 192.168.1.61，使用档案中该准确入口的凭据及完整 PIN，不把另一台设备的连接改写地址。
正式 BAR 构建、连接、SSH/SCP、诊断、传输、安装都通过
`C:/Users/dove/Documents/BBmanager/dist/Q10Manager/cli/Q10Manager.Cli.exe`
提交到 `%LOCALAPPDATA%/Q10Manager/Operations/v1/`，使用 `protocolVersion: 1` 和 `client: BRCode`。
本工程 `tools/q10_queue.py` 只准备、提交和读取任务，不执行队列。禁止 worker/run-once、自建私有队列、直接 SSH/SCP 或其他应用的部署例外。
最小 SDK 接口编译、源码检查、主机测试和本地 BAR 审计可以直接执行。
构建显式指定本工程 `build.ps1` 和 `build/BRCode.bar`；开关限 Package/BackendTests/ImeDiagnostics。
submit 不合并连接配置；设备请求必须读取当前 connection.json 并嵌入 connection 快照，私钥及 known_hosts 仅引用路径。
任务 ID 和回执先落盘。返回丢失先查原 ID，禁止自动换 ID 重发。管理器未运行时保留 queued。
安装授权沿用当前用户任务范围。部署前实际审计 BAR 并计算 SHA-256，包变化后重新核验。
入队、脚本退出 0、helper 0、progress 100 或 unsigned 字样均不代替成功证据。
安装须成功终态、上传回读哈希、原 PPS success/100、包身份/版本与实际目录一致；读取 deployment-details 保留证据。
unconfirmed 保留原任务、PPS、锁与队列屏障；continue-deployment 原 ID 仅提交只读观察，读取新观察 ID 结果。
不自动 acknowledge/abandon/delete，不删 known_hosts、锁、PPS，不执行 1.0.9 恢复或 1.1.6-M 初始化/重启。
当前协议没有 BRCode 通用启动能力时由用户点击设备图标；禁止套用 BBFile 专用任务。

## 产品与验证

文本不得 trim、归一化、改变换行或截断。原扫描正文不可编辑覆盖。
保存成功只在事务提交后成立。QR 成功必须检查原生编码器及回读；页面重绘不保存。
相机离页/后台/锁屏停止；一个 Camera 只挂一个检测器，异步旧会话不得提交。
用户要求取消 BBIME；全部文字字段使用 Cascades 系统输入模式，不接管实体按键，不加入候选栏、字典或自定义输入服务。
禁止重新引入 BBIME 或 Custom/VirtualKeyboardOff 输入配置。关闭系统自动改字、首字母大写、句点和词语替换；语言与候选由系统输入法配置决定。
设备实测、主机测试、SDK 编译与人工验收分别记录，未运行写 NOT TESTED。
