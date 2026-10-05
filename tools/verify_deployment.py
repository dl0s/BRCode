"""Join saved build, BAR and original installation evidence. Never changes the queue."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil

from audit_bar import audit
from q10_queue import RECORDS, ROOT, persist

FILES = ['bar-audit.json', 'build-artifact.json', 'build-context.json',
         'connection-identity.json', 'deployment-progress.json', 'events.jsonl',
         'installation-intent.json', 'installation.json', 'installer-terminal.json',
         'installer-preflight-before-upload.json', 'installer-preflight.json',
         'installer-policy-before-upload.json', 'installer-policy.json',
         'staging-access.json', 'transfer.json', 'state.json', 'result.json']


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def require(condition, description):
    if not condition:
        raise ValueError(description)


def collect(task_id):
    require(re.fullmatch('[0-9a-f]{32}', task_id), 'Invalid task ID')
    store = Path(os.environ['LOCALAPPDATA']) / 'Q10Manager/Operations/v1'
    candidates = [store / kind / task_id for kind in ['jobs', 'archives']]
    source = next((p for p in candidates if p.is_dir()), None)
    require(source is not None, 'Original task evidence unavailable: ' + task_id)
    output = RECORDS / task_id / 'attachments'
    output.mkdir(parents=True, exist_ok=True)
    paths = [source / name for name in FILES if (source / name).is_file()]
    paths += [p for p in (source / 'processes').glob('*/*')
              if p.is_file() and p.name in ['execution.json', 'stdout.log', 'stderr.log']]
    inventory = []
    for path in paths:
        require(path.resolve().is_relative_to(source.resolve()) and not path.is_symlink(),
                'Evidence path outside original task')
        relative = path.relative_to(source)
        destination = output / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
        inventory.append(dict(path=relative.as_posix(), sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    persist(output / 'inventory.json', dict(taskId=task_id, source=str(source), files=inventory))
    return output


def task(task_id, kind):
    status = read(RECORDS / task_id / 'status.json')
    require(status['state']['status'] == 'succeeded', 'Task has no successful terminal: ' + task_id)
    require(status['request']['id'] == task_id and status['request']['kind'] == kind
            and status['request']['client'] == 'BRCode', 'Task identity mismatch')
    return status, read(RECORDS / task_id / 'result.json')


def verify(args):
    report = audit(ROOT / 'build/BRCode.bar')
    diagnostic = audit(ROOT / 'build/BRCode-tests.bar')
    for task_id, package in [(args.build, report), (args.tests_build, diagnostic)]:
        _, built = task(task_id, 'build')
        require(built['sha256'] == package['sha256'] and built['version'] == package['version']
                and built['packageId'] == package['packageName'], 'Build result differs from current BAR')
        collect(task_id)
    status, result = task(args.deployment, 'deploy')
    request = status['request']
    require(request['connection']['deviceHost'] == args.host
            and request['connection']['expectedPin'] == args.pin, 'Wrong target endpoint/PIN')
    evidence = collect(args.deployment)
    transfer = read(evidence / 'transfer.json')
    intent = read(evidence / 'installation-intent.json')
    terminal = read(evidence / 'installer-terminal.json')
    details = read(RECORDS / args.deployment / 'deployment-details.json')
    digest = report['sha256']
    require(all(value == digest for value in [request['packageSha256'], result['sha256'],
            transfer['expectedSha256'], transfer['readbackSha256'], intent['sha256'], terminal['sha256']])
            and transfer['verified'] is True, 'Authorization/upload/readback/result hash mismatch')
    require(terminal['outcome'] == 'confirmed' and terminal['ppsExitCode'] == 0
            and details['outcome'] == 'succeeded', 'Original installation is not confirmed')
    require(intent['operationId'] == args.deployment and details['operationId'] == args.deployment,
            'Evidence belongs to another installation')
    raw = terminal['evidence']
    fields = {}
    for line in raw.splitlines():
        if '::' in line:
            key, value = line.split('::', 1)
            require(key not in fields, 'Duplicate original PPS field')
            fields[key] = value
    expected_directory = report['packageName'] + '.' + report['manifest']['Package-Id']
    for key, expected in dict(result='success', progress='100', actual_name=report['packageName'],
            actual_app_id=report['manifest']['Application-Id'], actual_version=report['version'],
            actual_app_version=report['version'], actual_dname=expected_directory).items():
        require(fields.get(key) == expected, 'Original PPS mismatch: ' + key)
    require(result['directoryName'] == expected_directory and result['packageId'] == report['packageName']
            and result['version'] == report['version'], 'Installation identity/version/directory mismatch')
    require(result['installerEvidence'] == raw == details['rawPps'], 'Original PPS projections differ')
    require(transfer['remote'] == terminal['remote'] == intent['remote'] == fields['package_location'],
            'Remote snapshot path differs')
    pps_name = 'job.' + Path(transfer['remote']).name
    require(raw.splitlines()[0] == '@' + pps_name
            and intent['ppsPath'].endswith('/' + pps_name), 'Original unique PPS path mismatch')
    required_steps = {'packageSnapshot', 'barValidation', 'rootIdentity', 'installerPreflight',
        'installerPolicy', 'stagingDirectory', 'upload', 'stagingAccess', 'readback',
        'installerRecheck', 'policyRecheck', 'installerSubmit', 'installerTerminal',
        'applicationRegistration', 'cleanup'}
    steps = {s['key']: s['status'] for s in details['steps']}
    require(required_steps <= steps.keys() and all(steps[k] == 'succeeded' for k in required_steps),
            'Installation stages or actual directory proof incomplete')
    consumer = intent['consumer']
    require(consumer['version'] == '1.1.6-M' and consumer['ready'] is True
            and consumer['serviceUid'] == 88 and len(consumer['hashes']) == 7
            and len(consumer['firmwareHashes']) == 3
            and consumer['statusRecord']['constructor_verified'] is True
            and consumer['statusRecord']['pps_listening'] is True,
            'Manager installer preflight evidence incomplete')
    output = dict(verifiedAt=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        scope='Current BAR and saved original manager transaction; runtime separately recorded',
        status='PASS', host=args.host, pin=args.pin, inputMethod='system',
        production={k:report[k] for k in ['path','version','packageName','sha256','elf']},
        diagnostic={k:diagnostic[k] for k in ['path','version','packageName','sha256','elf']},
        buildTask=args.build, diagnosticBuildTask=args.tests_build,
        deployment=dict(taskId=args.deployment,status='succeeded',outcome='confirmed',
            completedAt=result['completedAt'],directoryName=expected_directory,
            applicationDirectory='/apps/'+expected_directory,ppsPath=intent['ppsPath'],
            sha256=digest,ppsResult='success',ppsProgress=100,steps=steps,
            evidenceDirectory=str(evidence),firmware='1.1.6-M',
            servicePid=consumer['servicePid'],serviceUid=88,firmwareHashes=consumer['firmwareHashes'],
            managedSourceHashes=consumer['hashes']),
        hostTests=read(ROOT/'build/host/results.json'),
        manualEvidencePath=str(ROOT/'build/manual-verification.json'),
        runtime='NOT TESTED',nativeDiagnosticExecution='NOT TESTED',
        camera='NOT TESTED',crossDevicePng='NOT TESTED',applicationUpgradeData='NOT TESTED')
    manual_path=ROOT/'build/manual-verification.json'
    if manual_path.is_file():
        manual=read(manual_path)['current']
        require(manual['version']==report['version'] and manual['sha256']==digest
                and manual['deploymentTask']==args.deployment, 'Manual record describes another release')
        output['manual']=manual
        output['runtime']=manual['startup']
        output['camera']=manual['camera']
        output['crossDevicePng']=manual['pngCrossDevice']
        output['applicationUpgradeData']=manual['upgradeHistoryRetention']
    persist(ROOT/'build/verification.json', output)
    print(json.dumps(dict(status='PASS',deployment=args.deployment,version=report['version'],
                         sha256=digest,report=str(ROOT/'build/verification.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('deployment')
    parser.add_argument('--build', required=True)
    parser.add_argument('--tests-build', required=True)
    parser.add_argument('--host', required=True)
    parser.add_argument('--pin', required=True)
    verify(parser.parse_args())
