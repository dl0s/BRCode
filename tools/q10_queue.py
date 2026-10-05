"""BRCode submits to the shared BBmanager queue; it never executes jobs."""
import argparse
import ctypes
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import uuid

ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT.parent / 'BBmanager/dist/Q10Manager/cli/Q10Manager.Cli.exe'
RECORDS = ROOT / 'build/q10-queue'
ALLOWED = {'information', 'applications', 'installerStatus', 'installerDiagnostics',
           'applicationLog', 'processes', 'build', 'deploy', 'deploymentObservation'}

def invoke(command, *arguments):
    result = subprocess.run([str(CLI), command, *map(str, arguments)], capture_output=True, timeout=30)
    def decode(data):
        try:
            return data.decode('utf-8-sig')
        except UnicodeDecodeError:
            page = ctypes.windll.kernel32.GetOEMCP() if os.name == 'nt' else 65001
            return data.decode('cp' + str(page), errors='replace')
    output = decode(result.stdout).strip()
    if result.returncode:
        raise RuntimeError(decode(result.stderr).strip() or output or 'CLI failed')
    return output

def connection(target_host=''):
    local = Path(os.environ['LOCALAPPDATA'])
    # Manager 1.1.6.2 deliberately removed automatic legacy profile imports.
    profiles = [local / 'Q10Manager/connection.json']
    profile = next((p for p in profiles if p.is_file()), None)
    if profile is None:
        raise FileNotFoundError('BBmanager connection.json is required')
    for attempt in range(3):
        try:
            values = json.loads(profile.read_text(encoding='utf-8-sig'))
            break
        except (json.JSONDecodeError, PermissionError):
            if attempt == 2:
                raise
            time.sleep(0.05)
    fields = ['deviceHost', 'sshPort', 'sshUser', 'sshKeyPath', 'knownHostsPath',
              'connectTimeoutSeconds', 'legacyAlgorithms', 'expectedPin',
              'hostPublicKey', 'hostKeyAlias']
    if target_host and values.get('deviceHost', values.get('DeviceHost')) != target_host:
        matches = [endpoint for device in json.loads(invoke('devices')) for endpoint in device['endpoints']
                   if endpoint['deviceHost'] == target_host]
        if len(matches) != 1:
            raise ValueError('Target must have one verified registry endpoint: ' + target_host)
        values = matches[0]
    folded = {k.lower(): v for k, v in values.items()}
    result = {k: folded[k.lower()] for k in fields if k.lower() in folded}
    for k in ('deviceHost', 'sshKeyPath', 'knownHostsPath'):
        if not result.get(k):
            raise ValueError('Missing connection field: ' + k)
    for k in ('sshKeyPath', 'knownHostsPath'):
        p = Path(result[k])
        if not p.is_absolute() or not p.is_file():
            raise ValueError('Missing absolute credential path: ' + k)
    if result.get('sshUser', 'root') != 'root':
        raise ValueError('Manager requires the configured root transport')
    if result.get('expectedPin') and not result.get('hostPublicKey'):
        raise ValueError('Registered PIN requires its trusted host public key')
    return result

def persist(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_suffix(path.suffix + '.tmp')
    with temp.open('w', encoding='utf-8', newline='\n') as output:
        json.dump(value, output, ensure_ascii=False, indent=2)
        output.flush()
        os.fsync(output.fileno())
    os.replace(temp, path)

def submit(kind, draft=False, target_host='', **fields):
    if kind not in ALLOWED:
        raise ValueError('Unsupported BRCode operation')
    if {'id', 'client', 'protocolVersion', 'kind', 'connection'} & fields.keys():
        raise ValueError('Cannot override request identity or connection')
    task_id = uuid.uuid4().hex
    request = dict(protocolVersion=1, id=task_id, client='BRCode', kind=kind, **fields)
    if kind != 'build':
        request['connection'] = connection(target_host)
    request_path = RECORDS / 'requests' / (task_id + '.json')
    persist(request_path, request)
    receipt = dict(taskId=task_id, client='BRCode', kind=kind,
                   store=str(Path(os.environ['LOCALAPPDATA']) / 'Q10Manager/Operations/v1'),
                   requestPath=str(request_path), status='prepared',
                   requestSha256=hashlib.sha256(request_path.read_bytes()).hexdigest(),
                   preparedAt=datetime.datetime.now(datetime.timezone.utc).isoformat())
    receipt_path = RECORDS / (task_id + '.submission.json')
    persist(receipt_path, receipt)
    if draft:
        return receipt
    try:
        returned = invoke('submit', request_path)
        if returned != task_id:
            raise RuntimeError('Returned ID differs from prepared ID')
        receipt['status'] = 'submitted'
    except Exception as error:
        receipt.update(status='submissionUnconfirmed', error=str(error))
        persist(receipt_path, receipt)
        raise RuntimeError('Query original ID ' + task_id + '; do not resubmit. ' + str(error))
    persist(receipt_path, receipt)
    return receipt

def observe(task_id):
    if not re.fullmatch('[a-fA-F0-9]{32}', task_id):
        raise ValueError('Invalid task ID')
    target = RECORDS / task_id
    target.mkdir(parents=True, exist_ok=True)
    status = json.loads(invoke('status', task_id))
    persist(target / 'status.json', status)
    state = status.get('state', status)
    outcome = state.get('status', status.get('statusText'))
    commands = ['events']
    if outcome == 'succeeded':
        commands += ['result']
    elif outcome not in ('queued', 'running'):
        commands += ['failure']
    request = status.get('request', {})
    if request.get('kind') == 'build':
        commands += ['build-logs']
    if request.get('kind') == 'deploy':
        commands += ['deployment-details']
    for command in commands:
        try:
            output = invoke(command, task_id)
            (target / (command + '.json')).write_text(output, encoding='utf-8')
        except RuntimeError as error:
            (target / (command + '.error.txt')).write_text(str(error), encoding='utf-8')
            if command == 'result' and outcome == 'succeeded':
                raise
    return dict(taskId=task_id, status=outcome, message=state.get('message'),
                stage=state.get('currentStage'), evidenceDirectory=str(target))

def main():
    sys.stdout.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', default='', help='Use the exact verified registry endpoint; never rewrite another device profile')
    commands = parser.add_subparsers(dest='command', required=True)
    build = commands.add_parser('build')
    build.add_argument('--sdk', default=os.environ.get('BRCODE_SDK_ROOT') or os.environ.get('INTROOP_SDK_ROOT') or 'C:/bbdevtools')
    build.add_argument('--tests', action='store_true')
    deployment = commands.add_parser('deploy')
    deployment.add_argument('bar', type=Path, nargs='?', default=ROOT / 'build/BRCode.bar')
    deployment.add_argument('--draft', action='store_true')
    watch = commands.add_parser('observe')
    watch.add_argument('task_id')
    continuation = commands.add_parser('continue')
    continuation.add_argument('task_id')
    generic = commands.add_parser('submit')
    generic.add_argument('kind', choices=sorted(ALLOWED - {'build', 'deploy'}))
    generic.add_argument('--fields', type=Path)
    args = parser.parse_args()
    if args.command == 'build':
        diagnostic = args.tests
        receipt = submit('build', projectDirectory=str(ROOT), sdkDirectory=str(Path(args.sdk).resolve(strict=True)),
                         buildScriptPath=str(ROOT / 'build.ps1'),
                         buildOutputPath=str(ROOT / ('build/BRCode-tests.bar' if diagnostic else 'build/BRCode.bar')),
                         buildSwitches=['Package'] + (['BackendTests'] if diagnostic else []))
    elif args.command == 'deploy':
        from audit_bar import audit
        bar = args.bar.resolve(strict=True)
        report = audit(bar)
        if report['diagnostic']:
            raise ValueError('Diagnostic BAR cannot be deployed as the production app')
        receipt = submit('deploy', draft=args.draft, target_host=args.host, packagePath=str(bar), packageSha256=report['sha256'])
    elif args.command == 'observe':
        receipt = observe(args.task_id)
    elif args.command == 'continue':
        original = json.loads(invoke('status', args.task_id))['request']
        # Bind the original connection snapshot instead of the current selection.
        if original['kind'] != 'deploy' or original['client'] != 'BRCode':
            raise ValueError('Not an original BRCode deployment')
        task = dict(protocolVersion=1, id=uuid.uuid4().hex, client='BRCode',
                    kind='deploymentObservation', originalDeploymentId=args.task_id,
                    packageSha256=original['packageSha256'], connection=original['connection'])
        path = RECORDS / 'requests' / (task['id'] + '.json')
        persist(path, task)
        receipt_path = RECORDS / (task['id'] + '.submission.json')
        receipt = dict(taskId=task['id'], originalDeploymentId=args.task_id, status='prepared', requestPath=str(path))
        persist(receipt_path, receipt)
        try:
            if invoke('submit', path) != task['id']:
                raise RuntimeError('Observation ID mismatch')
            receipt['status'] = 'submitted'
        except Exception as error:
            receipt.update(status='submissionUnconfirmed', error=str(error))
            persist(receipt_path, receipt)
            raise
        persist(receipt_path, receipt)
    else:
        fields = json.loads(args.fields.read_text(encoding='utf-8-sig')) if args.fields else {}
        receipt = submit(args.kind, target_host=args.host, **fields)
    print(json.dumps(receipt, ensure_ascii=False))

if __name__ == '__main__':
    main()
