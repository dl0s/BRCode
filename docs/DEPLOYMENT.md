# .61 构建和部署

2026-10-05，Asia/Shanghai。统一使用 `C:/Users/dove/Documents/BBmanager/Documentation/标准操作文档.md`，当前 CLI 1.1.6.5 / 固件 1.1.6-M / protocolVersion 1。

BRCode 队列客户端只提交和按原 ID 主动读取，BBmanager 统一执行。开发机为用户指定的 192.168.1.61；登记档案对应完整 PIN 2C3353AC。使用 `--host 192.168.1.61` 会核对当前配置或选择该准确已核验档案入口，不仅替换地址。目标尚未登记时停止；按管理端标准登记流程处理，不绕过信任或未确认屏障。

## 正式请求与证据

构建请求显式指定本工程根目录、SDK、`build.ps1`、`build/BRCode.bar` 与 `["Package"]`。本地请求、原任务 ID、请求 SHA-256 和提交回执先落盘至 `build/q10-queue/`，之后调用固定发布 CLI submit。设备任务把 connection 放在请求正文，私钥和 known_hosts 仅引用已有文件路径。

`observe <id>` 保存 status、events；成功时保存 result；失败或未确认保存 failure，构建另存 build-logs，安装另存 deployment-details。本地摘要输出中的 queued/running 均不是成功，原始记录是证据来源。

设备核验与安装：

```powershell
python tools/q10_queue.py --host 192.168.1.61 submit information
python tools/q10_queue.py --host 192.168.1.61 submit installerDiagnostics
python tools/q10_queue.py build
python tools/q10_queue.py observe <构建ID>
python tools/audit_bar.py build/BRCode.bar --output build/bar-audit.json
python tools/q10_queue.py --host 192.168.1.61 deploy
python tools/q10_queue.py observe <部署ID>
```

安装成功至少联合判断管理器 succeeded 与 result、授权/上传/回读 SHA-256、原 PPS success/100、包身份/版本、actual_dname 和实际安装目录。管理器预检十文件摘要、真实 UID 88 消费者、root 监护器、构造器审计和 PPS 就绪。BRCode 不重复实现这些管理器内部步骤，不执行 unsigned/root 初始化或重启。

## 中断与原事务

部署断线、超时、取消或不一致证据出现 unconfirmed 时保留原任务与屏障：

```powershell
python tools/q10_queue.py observe <原部署ID>
python tools/q10_queue.py continue <原部署ID>
python tools/q10_queue.py observe <返回的观察ID>
```

观察任务 succeeded 只代表只读观察完成，查看业务 outcome；不会把原部署改为成功。禁止换新 ID 重装、清理原 PPS、删锁、改 known_hosts、自动 acknowledge/abandon/delete。旧记录不可读时保存读取错误，核查原任务；不伪造终态。

安装与启动分别验收。当前通用协议没有 BRCode 启动入口；由用户在 .61 设备点 BRCode 图标。如发生启动失败，可用 `applicationLog` 对管理器返回的精确 applicationDirectory 提交只读日志任务；不得套用 BBFile 专用目录或远程命令。

## 本轮结果

1.0.0.3 已通过原任务 `e114966c78e0401a8dcda2c87eab6627` 成功安装，完整证明及哈希见 `docs/DELIVERY.md` 和 `build/verification.json`。
用户确认前台启动、系统输入、生成正常；本版不含 BBIME。设备日志任务 `4d1050273e654b8e8082347eba672b4e` 返回该版本 scene ready、qmlErrors=0、UID 100。
1.0.0.2 曾因残留的 NativeSymbolPanel 无法创建页面，已在 1.0.0.3 删除该引用。实机相机、PNG 跨设备及已有历史保留仍待人工验收。
