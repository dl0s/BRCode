# BRCode

BlackBerry Q10 的离线文本二维码工具，原生 Cascades 中文界面，适配 720×720 与实体键盘。
本工程真实构建、包审计和设备队列证据见 [交付记录](docs/DELIVERY.md)。扫码、安装、前台启动和业务验收分别记录，不互相代替。
当前提交基线为 1.0.0.3（系统输入法），发布摘要见 [版本记录](docs/releases/1.0.0.3.json)。Git 保留源码/资源原字节；构建产物、队列请求及本机数据不纳入版本控制。

## 功能

- 后置相机扫码：异步打开/预览/关闭；每个会话只接收一次，点击继续扫描可再次记录同一码。离页、后台、锁屏停止，旧事件不入库。
- 原文与历史：SQLite 私有目录保存扫描/生成来源、UUID、全文、标题、备注及 UTC 创建/更新时间。40 条增量分页；正文/标题/备注字面搜索、完整复制、备注编辑、删除和编辑副本。
- 文字生成：多行输入、粘贴、中文与英文。原生 M 级 QR 编码，校验错误码，再对实际预览图回读；不一致则拒绝成功。黑码白底、四模块静区、整数倍像素绘制、全屏和 PNG 导出。
- 文字输入：正文、标题、备注、搜索使用 Cascades 系统输入法和实体键盘。恢复系统候选及复制/粘贴菜单，正文 Enter 换行；关闭自动改字、首字母大写、自动句点及词语替换。语言与中文输入能力由设备系统设置决定。
- 扫描首先展示完整文本；只有有效 HTTP/HTTPS 链接显示打开操作，用户点击后交给系统浏览器。

二维码正文支持可由原生 QString 完整表示的文本，不保证任意二进制 QR。生成失败、输入变化时移除旧图；保存失败保留当前结果并提供重试，不报告已保存。导出写入并回读临时 PNG 后发布，失败保留当前二维码。

## 构建与设备

唯一操作依据：`C:/Users/dove/Documents/BBmanager/Documentation/标准操作文档.md`。
当前执行基线为 BBmanager/Core/CLI 1.1.6.5、设备 1.1.6-M、共享协议 v1。
用户指定开发机 `192.168.1.61`，使用登记入口的实际 PIN 和凭据，不改写另一设备配置。

```powershell
cd C:/Users/dove/Documents/BRCode
python tools/q10_queue.py build
python tools/q10_queue.py observe <构建任务ID>
# succeeded 且 result / BAR 审计一致后：
python tools/audit_bar.py build/BRCode.bar --output build/bar-audit.json
python tools/q10_queue.py --host 192.168.1.61 deploy
python tools/q10_queue.py observe <部署任务ID>
```

固定 CLI 为 `C:/Users/dove/Documents/BBmanager/dist/Q10Manager/cli/Q10Manager.Cli.exe`；共享队列为 `%LOCALAPPDATA%/Q10Manager/Operations/v1/`。
所有正式 BAR 构建、连接、诊断和安装由 BBmanager 桌面程序执行。本工具只准备/提交/读取，不启动 worker，不直连 SSH/SCP。无执行器时任务 queued；提交丢失先查原 ID，不换 ID 重发。可以使用 `deploy --draft` 准备请求而不安装。

构建脚本执行接口为 `build.ps1 -Package -SdkRoot C:/bbdevtools`，由上述队列调用；可另传 `-JavaBin`。
SDK 根解析顺序：显式参数 → 用户/进程 `BRCODE_SDK_ROOT` → 用户/进程 `INTROOP_SDK_ROOT`。
Java 解析为参数、BRCODE/INTROOP_JAVA_BIN、Momentics JRE、JAVA_HOME/PATH，要求 Java 7/8。先确定主机 Python 3 再改 SDK PATH；全部进程环境在 finally 恢复。
编译使用 `-V4.6.3,gcc_ntoarmv7le_cpp -std=c++98`，Qt 4.8.6。脚本输出五个阶段标记、保存日志，严格检查退出码、ARM ELF 与 libcpp.so.4。

正式产物：`C:/Users/dove/Documents/BRCode/build/BRCode.bar`，身份 `top.blaccat.BRCode`，版本 `1.0.0.3`，二进制 `brcode`。
`python tools/q10_queue.py build --tests` 产生独立 `BRCode-tests.bar` / `top.blaccat.BRCodeTests`，不覆盖正式身份。诊断包启动才执行原生 QR 夹具，编译成功不表示夹具已运行。
普通构建只读取本工程源码；不会读取兄弟工程源码。BAR 的 ZIP 时间戳可能不同，不承诺两次包字节完全相同。

## 检查与资源

```powershell
python tools/verify_sources.py
python tools/test_host.py
```

主机测试编译实际 `HistoryStore.cpp` 和 `ScanGate.hpp`，使用固定 SHA-1 的官方 SQLite 3.7.14.1 amalgamation（缓存 `build/host/sqlite`，首次运行需要获取该历史源码）。需要本机 MinGW C++98 工具；测试夹具独立，不使用用户数据。
包审计核对 Manifest/ZIP 资产集合、每项 SHA-512、入口 ARM ELF、ABI、114×114 PNG 图标和当前资源逐字节一致性，并拒绝旧输入模块资源。详细资产清单和 SHA-256 在 `build/bar-audit.json`。

生产库：QtCore、QtGui、QtDeclarative、bbcascades、bbcascadesmultimedia、bbdata、bbsystem、bb、bps、sqlite3、libcpp.so.4；无网络二维码依赖，无 QtSql 第二实现。
申请权限仅 `use_camera`、`access_shared`。数据库位于应用沙箱 `data/history.sqlite`，schema v2；v1 → v2 在事务内增加更新时间并保留所有正文/备注。未来未知 schema 明确拒绝，不删库。
导出位置为共享照片目录 `shared/photos/BRCode`；UI 成功时显示解析后的实际绝对路径。

按用户要求，1.0.0.2 已移除全部 BBIME 源码、服务、按键路由、候选栏及字典，不编译或打包这些资源。原输入模块及其许可保留在 `build/legacy-bbime/`，原 1.0.0.1 BAR 与审计在 `build/releases/1.0.0.1-bbime/`，仅用于追溯，不是当前交付。
本应用不维护个人词库；系统输入法的学习行为由系统控制，本工程不声称通过自动改字开关禁用系统词库学习。

## 后续升级

保留应用 ID、作者及 devMode 身份生成条件；更新 descriptor 的 versionNumber/buildId、runtime 版本及审计预期版本，再通过队列构建。用实际已保存数据验证同身份升级，不能借用其他应用的结果。
相机补光、从照片识别和 OCR 未提供。系统无适用 BRCode 通用启动接口，安装后由用户点击设备图标；不套用 BBFile 生命周期任务。
人工操作步骤见 [Q10 验收卡](docs/Q10_ACCEPTANCE.md)。
