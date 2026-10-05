# BRCode 实机交付记录

2026-10-05（Asia/Shanghai），工程 `C:/Users/dove/Documents/BRCode`。
操作依据：`C:/Users/dove/Documents/BBmanager/Documentation/标准操作文档.md`。
开发机为用户指定的 `192.168.1.61`，登记及设备信息核对完整 PIN `2C3353AC`。
本次使用 BBmanager/Core/CLI 1.1.6.5、共享协议 v1；正式安装由管理器验证准确 1.1.6-M 固件和安装链。

## 当前版本

正式身份 `top.blaccat.BRCode`，版本 `1.0.0.3`，二进制 `brcode`。
用户要求取消 BBIME，当前源码、编译及 BAR 均使用 Cascades 系统输入控件。
正文、标题、备注及搜索恢复系统候选、系统菜单和实体键盘输入，不拦截 Alt/Enter，不依赖 BBIME 运行。
正文 Enter 换行，关闭自动改字、首字母大写、自动句点和词语替换；系统语言与个人词库行为由设备配置决定。

后置相机识别、会话去重、离页/后台/锁屏释放、完整文本详情、SQLite 历史、备注、搜索、40 条分页、删除、编辑副本、原生 M 级二维码、全屏、PNG 导出和显式 HTTP/HTTPS 调用已实现。
数据库位置 `data/history.sqlite`，schema v2；v1 升级为事务迁移。
应用身份、作者及数据库路径在本轮更新中保持一致；实际用户历史保留须人工核对。

## 可重复构建与产物

SDK `C:/bbdevtools`，QNX Host `host_10_3_1_12/win32/x86`，Target `target_10_3_1_995/qnx6`。
qcc GCC 4.6.3 / C++98 / ARMv7 little-endian，Qt 4.8.6、Cascades、SQLite 3.7.14.1、QNX libcpp.so.4。
Java 使用已发现且构建验证的 Momentics JRE 7；构建脚本支持指定 SDK 和 Java 路径。

```powershell
cd C:/Users/dove/Documents/BRCode
python tools/q10_queue.py build
python tools/q10_queue.py observe <正式构建ID>
python tools/q10_queue.py build --tests
python tools/q10_queue.py observe <诊断构建ID>
python tools/audit_bar.py build/BRCode.bar --output build/bar-audit.json
python tools/q10_queue.py --host 192.168.1.61 deploy
python tools/q10_queue.py observe <部署ID>
```

脚本接口为 `build.ps1 -Package -SdkRoot C:/bbdevtools`，正式构建通过共享队列执行。
正式 BAR：`C:/Users/dove/Documents/BRCode/build/BRCode.bar`。
诊断 BAR：`C:/Users/dove/Documents/BRCode/build/BRCode-tests.bar`，独立身份 `top.blaccat.BRCodeTests`；仅构建，未安装或执行。
每项 Manifest 资产、SHA-256、版本、ARM ELF 和 C++ ABI 在 `build/bar-audit.json` / `build/bar-audit-tests.json`。
两个包均声明 `use_camera`、`access_shared`；应用仍使用系统沙箱身份运行。

机器记录为 `build/verification.json`，当前正式安装已完成，运行验收单独记录。

| 操作 | 原任务 ID | 终态 |
| --- | --- | --- |
| 正式构建 1.0.0.3 | `da3836332842438f8f1dcb6047f90f9a` | succeeded |
| 诊断构建 1.0.0.3 | `24b215b9919446ccb0e30c1adee87ea4` | succeeded |
| 正式部署到 .61 | `e114966c78e0401a8dcda2c87eab6627` | succeeded / confirmed |
| 设备信息 | `6c0f1310847b4366acecf8f5f6faf991` | succeeded，实际 PIN 2C3353AC |
| 安装器只读诊断 | `a11dd4bd9cdb43e39eaf4e8ecb56477b` | succeeded；实际部署另有完整预检 |

正式 SHA-256：`917dc1c0bde548e7ab7ec86f7e4c9c7e50e04d528e775e0b43343a0910b9d377`。
诊断 SHA-256：`435e5d9e67155402750ad068894592564129ca5d67116175642f6f58b721e0c1`。
管理器授权、上传回读、原 PPS 和登记结果共同核验；15 个安装阶段均 succeeded。
原 PPS `result=success`、`progress=100`、`actual_name=top.blaccat.BRCode`、`actual_version=1.0.0.3`。
实际目录 `/apps/top.blaccat.BRCode.testDev_ccat_BRCode24ede41a` 已由安装器核对存在，未用手工目录复制代替安装。
同 PID 消费者 UID 88、root 监护器、十文件固件/源摘要、构造器及 application_debug_token_policy、PPS listening 均由本次预检验证。
本轮正式构建、诊断构建及部署的原附件、构建进程日志和文件摘要已复制到各任务 `build/q10-queue/<ID>/attachments/`；不包含私钥正文。

可重新核对已保存的完整证据：

```powershell
python tools/verify_deployment.py e114966c78e0401a8dcda2c87eab6627 --build da3836332842438f8f1dcb6047f90f9a --tests-build 24b215b9919446ccb0e30c1adee87ea4 --host 192.168.1.61 --pin 2C3353AC
```

## 已执行验证

| 范围 | 结果 | 证据与边界 |
| --- | --- | --- |
| 最小原生 Camera/QR SDK 调用 | PASS | `build/spike/`，真实 ARM C++98 编译 |
| 实际 HistoryStore / ScanGate 主机测试 | PASS，252 次断言 | `build/host/results.json`；SQLite 3.7.14.1，与 SDK 一致 |
| 文本存储、搜索、事务失败和重试 | PASS（主机） | 中文、emoji、换行、首尾空格、NUL、字面 `%_`、SQL 注入、保存及备注回滚 |
| 重启、迁移、分页和旧帧 | PASS（主机） | 原文保持、schema v1→v2、未知版本拒绝、同时间 85 条分页无重复、停止后迟到帧 |
| 系统输入边界与 BAR 资源 | PASS | `tools/verify_sources.py` 拒绝 Custom、旧 BBIME 资源及残余类型；最新正式包 7 项资产逐字比对通过 |
| 1.0.0.3 前台启动和页面创建 | PASS（用户反馈与设备日志） | 用户确认可打开；日志 `4d1050273e654b8e8082347eba672b4e`：版本 1.0.0.3、input=system、qmlErrors=0、UID 100 |
| 基本系统输入和生成 | PASS（用户反馈） | 原反馈“可以打开，系统输入和生成正常”；未读取或导出用户正文 |
| 原生 QR/PNG 诊断夹具 | NOT TESTED | 诊断包只编译；不能据此报告中文/emoji 回读通过 |
| 实际相机、权限/占用、后台/锁屏 | NOT TESTED | 等待实机人工操作 |
| 详细中文输入、Enter、粘贴、备注 | NOT TESTED | 基本输入已获确认，逐项保真与交互仍待人工验收 |
| 跨设备扫描屏幕及 PNG | NOT TESTED | 同库回读不能代替跨设备检查 |
| 同身份升级后历史数据保留 | NOT TESTED | 本轮未下载用户数据库，须在设备历史页核对 |

主机证据：`build/host/run-0a06e4f78d7442ebb816a111542b90c0/`。
这些测试覆盖实际源码的存储与会话控制，不覆盖 Cascades 页面创建或原生 Camera/QR 运行。
当前正式安装完成时间为 2026-10-05 13:57:09（Asia/Shanghai）。正式包 995,714 字节，诊断包 1,140,173 字节。
1.0.0.3 日志确认场景创建成功；后续固定进程快照未列出日志中的 PID，因此不把此快照当作持续运行或稳定性证明。
用户已确认当前版本可打开、系统输入及生成正常；长时间稳定性、真实相机和跨设备回读仍未验收。

## 早期版本与实际失败

1.0.0.1 正式构建 `ae050bfe24194b8186ebcec84114fe3d`，部署 `388fddb75bf549cea3d87921d6ec12ee` 均成功。
原包 SHA-256 `9c22e14c63138be84b2746eef2fa617935f7dbc2f98b28de10b65b4d1e1a90b3`。
用户曾反馈启动与生成成功，但随后报告 BBIME 接入障碍并要求取消；应用日志任务 `c8f9b19b6ba547dabe50e9f146f9f256` 保存过 QtDeclarative SIGSEGV。
该日志没有足以认定根因的堆栈，不能宣称已证明 BBIME 是该 SIGSEGV 的唯一原因。

1.0.0.2 系统输入包正式构建 `243a2b4281b8415caadb7c073660f08c`、诊断构建 `f80fb935263c4eb49f1c6263ced505c2` 成功。
部署 `c257547fdf9d4acfb755e148b4ad4aaf` 成功，SHA-256 `ca6eb53d2adde4cc75713026062d2708fcf29b9320872f0dab2b33a235e7842c`。
用户反馈无法打开；日志任务 `67f45694da7a47789255d402be26f986` 确认 `NativeSymbolPanel is not a type`。
移除输入模块时漏掉的附加对象导致页面创建失败；1.0.0.3 删除该引用，并在构建预检增加残余类型检查。
构建 `2a69ad51ca904a4786182d7931183579` 因检查误匹配 `QDateTime::` 失败，修正为单词边界匹配后重新构建；失败任务保留。

旧产物和审计保留在 `build/releases/`，旧输入源码、资源与许可在 `build/legacy-bbime/`，不进入当前 BAR。
已失败版本的安装 succeeded 只证明安装，不代表应用可运行。

## 验收与限制

当前通用队列没有适用于 BRCode 的远程启动入口，前台启动由用户点设备图标；仅通过 `applicationLog` 读取精确安装目录的标准日志。
不使用 BBFile 专用生命周期或任意远程命令。
相机补光、从照片识别和 OCR 未提供。支持原生 QString 可完整表示的文本 QR，不保证任意二进制 QR。
本次不修改固件、初始化策略、凭据、PPS 或锁；未确认事务仅按原 ID 续查。
人工步骤见 `docs/Q10_ACCEPTANCE.md`，实际反馈记录在 `build/manual-verification.json`。
