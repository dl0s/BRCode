# 已核实环境

2026-10-05，Asia/Shanghai。

| 项目 | 结果 |
|---|---|
| 工程 | C:/Users/dove/Documents/BRCode |
| SDK | C:/bbdevtools；用户/进程 INTROOP_SDK_ROOT 起始均指向该目录 |
| QNX_HOST | host_10_3_1_12/win32/x86 |
| QNX_TARGET | target_10_3_1_995/qnx6 |
| 编译器 | qcc -V4.6.3,gcc_ntoarmv7le_cpp；C++98 |
| Qt | 本机 qglobal.h：4.8.6 |
| SQLite | 本机 sqlite3.h：3.7.14.1 |
| ABI | 实际 readelf 检查 ELF32 little-endian ARM / libcpp.so.4，无 libstdc++ |
| Java 候选 | C:/bbndk/features/com.qnx.tools.jre.win32.x86_64_1.7.0.51/jre/bin；构建前实际 -version 核验 Java 7/8 |
| 原生二维码 | 本机 Camera/BarcodeDetector/BarcodeGenerator 头文件；ARM libbbcascadesmultimedia.so |
| 输入法 | 1.0.0.2 使用 Cascades 系统 TextArea/TextField；无 BBIME 依赖或 ARM 字典 |
| 管理器 | 开始时 1.1.6.1；开发期间有其他会话更新，最新指令后重新核实发布 CLI 1.1.6.5 |
| 开发机 | 用户指定 192.168.1.61；实际信息任务返回完整 PIN 2C3353AC |

历史资料冲突已按当前实现处理：AgentIntegration、MultiDeviceManagement 和 schema 描述中的旧版本段落不是当前发布版本；现在仅以固定《标准操作文档》及实际发布 CLI 为准。当前管理器不自动导入 Q10Deploy 配置，本工程也停止旧配置回退。
SDK 最小接口已真实 ARM 编译；主机 SQLite 回归暴露 instr() 在 3.7.14.1 不存在，已改用带转义的字面 LIKE 搜索。原生 Camera 的错误枚举只给通用/API/占用类别，UI 不把一般打开错误武断归为授权拒绝。
生成纠错仅使用本机说明的 M 级；补光及未经核验的 L/Q/H 设置未加入。
系统输入控件不使用 Custom 模式、VirtualKeyboardOff 或自定义按键拦截。自动改字、大写、句点及词语替换关闭；中文语言与候选由系统配置决定。生成/保存读取失焦后的原生字段正文；输入中的草稿不以 QML 反向绑定反复覆盖，历史再生成使用明确的编辑副本信号。
