param([switch]$Package,[switch]$BackendTests,[string]$SdkRoot='',[string]$JavaBin='')
$ErrorActionPreference='Stop'
# Resolve Python 3 before prepending the SDK's Python 2 directory.
$hostPython=(Get-Command python -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
& $hostPython -c 'import sys; sys.exit(0 if sys.version_info[0]>=3 else 1)'
if ($LASTEXITCODE -ne 0) {throw 'Host Python 3 is required'}
. (Join-Path $PSScriptRoot 'tools/Sdk-Environment.ps1')
$saved=@{}
foreach ($name in @('QNX_HOST','QNX_TARGET','PATH','TEMP','TMP','TMPDIR','JAVA_HOME')) {$saved[$name]=[Environment]::GetEnvironmentVariable($name,'Process')}
$suffix=if ($BackendTests) {'-tests'} else {''}
$outputRoot=Join-Path $PSScriptRoot ('build/native'+$suffix)
New-Item -ItemType Directory -Force -Path $outputRoot,(Join-Path $PSScriptRoot 'build/logs') | Out-Null
$transcript=Join-Path $PSScriptRoot ('build/logs/build'+$suffix+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.log')
Push-Location $PSScriptRoot
Start-Transcript -Path $transcript | Out-Null
try {
    Write-Output '@@Q10_STAGE:sdkPreflight'
    $SdkRoot=Get-BRCodeSdkRoot $SdkRoot
    if ($Package) {$JavaBin=Get-BRCodeJavaBin $SdkRoot $JavaBin}
    $env:QNX_HOST=Join-Path $SdkRoot 'host_10_3_1_12/win32/x86'
    $env:QNX_TARGET=Join-Path $SdkRoot 'target_10_3_1_995/qnx6'
    $env:PATH=(Join-Path $env:QNX_HOST 'usr/bin')+';'+$env:PATH
    if ($Package) {$env:PATH=$JavaBin+';'+$env:PATH;$env:JAVA_HOME=Split-Path $JavaBin -Parent}
    $temporary=Join-Path $outputRoot 'tmp'
    New-Item -ItemType Directory -Force -Path $temporary | Out-Null
    $env:TEMP=$temporary;$env:TMP=$temporary;$env:TMPDIR=$temporary
    $qcc=Join-Path $env:QNX_HOST 'usr/bin/qcc.exe'
    $moc=Join-Path $env:QNX_HOST 'usr/bin/moc.exe'
    $qt=Join-Path $env:QNX_TARGET 'usr/include/qt4'
    $libs=Join-Path $env:QNX_TARGET 'armle-v7/usr/lib/qt4/lib'
    $version=(Select-String -LiteralPath (Join-Path $qt 'QtCore/qglobal.h') -Pattern '#define QT_VERSION_STR').Line
    if ($version -notmatch '"4\.8\.6"') {throw "Unexpected Qt SDK version: $version"}
    & $hostPython -B tools/verify_sources.py
    if ($LASTEXITCODE -ne 0) {throw 'Source/asset verification failed'}
    $flags=@('-V4.6.3,gcc_ntoarmv7le_cpp','-Wc,-std=c++98','-D_QNX_SOURCE','-O2','-g','-Wall','-Wextra','-Wno-psabi',
        "-I$qt","-I$qt/QtCore","-I$qt/QtGui","-I$qt/QtDeclarative",'-Isrc',
        "-L$libs","-Wl,-rpath-link,$libs",'-Wl,-rpath,/usr/lib/qt4/lib')
    Write-Output '@@Q10_STAGE:compile'
    & $qcc @flags -c tests/sdk_spike.cpp -o (Join-Path $outputRoot 'sdk_spike.o')
    if ($LASTEXITCODE -ne 0) {throw 'Native QR API spike compile failed'}
    $sources=@('src/main.cpp','src/App.cpp','src/HistoryStore.cpp','src/QrService.cpp','src/ScanController.cpp')
    foreach ($header in @('src/App.hpp','src/ScanController.hpp')) {
        $generated=Join-Path $outputRoot ('moc_'+[IO.Path]::GetFileNameWithoutExtension($header)+'.cpp')
        & $moc $header -o $generated
        if ($LASTEXITCODE -ne 0) {throw "moc failed: $header"}
        $sources+=$generated
    }
    if ($BackendTests) {$flags+='-DBRCODE_BACKEND_TESTS';$sources+='tests/NativeTests.cpp'}
    $binary=Join-Path $PSScriptRoot ('build/brcode'+$suffix)
    & $qcc @flags -o $binary @sources -lbbcascades -lbbcascadesmultimedia -lbbdata -lbbsystem -lbb -lbps -lQtCore -lQtGui -lQtDeclarative -lsqlite3 -lm
    if ($LASTEXITCODE -ne 0) {Write-Output '@@Q10_ERROR:QNX_COMPILE_FAILED';throw "ARM compilation failed (exit $LASTEXITCODE)"}
    Write-Output '@@Q10_STAGE:elfValidation'
    $readelf=Join-Path $env:QNX_HOST 'usr/bin/ntoarm-readelf.exe'
    $header=(& $readelf -h $binary) -join "`n"
    if ($LASTEXITCODE -ne 0) {throw 'ELF header inspection failed'}
    $dynamic=(& $readelf -d $binary) -join "`n"
    if ($LASTEXITCODE -ne 0) {throw 'ELF dynamic inspection failed'}
    $header | Out-File -LiteralPath (Join-Path $outputRoot 'elf-header.txt') -Encoding utf8
    $dynamic | Out-File -LiteralPath (Join-Path $outputRoot 'elf-dynamic.txt') -Encoding utf8
    if ($header -notmatch 'Class:\s+ELF32' -or $header -notmatch 'Data:.*little endian' -or $header -notmatch 'Machine:\s+ARM' -or $header -notmatch 'Type:\s+(EXEC|DYN)') {throw 'ELF architecture/type invalid'}
    if ($dynamic -match 'libstdc\+\+' -or $dynamic -notmatch 'libcpp\.so\.4') {throw 'Require QNX libcpp.so.4; reject GNU libstdc++'}
    if ($Package) {
        Write-Output '@@Q10_STAGE:package'
        $descriptor=if ($BackendTests) {'bar-descriptor-tests.xml'} else {'bar-descriptor.xml'}
        $bar='build/BRCode'+$suffix+'.bar'
        & (Join-Path $env:QNX_HOST 'usr/bin/blackberry-nativepackager.bat') -package -devMode $bar $descriptor
        if ($LASTEXITCODE -ne 0) {Write-Output '@@Q10_ERROR:BAR_PACKAGING_FAILED';throw 'Native BAR packaging failed'}
        Write-Output '@@Q10_STAGE:barValidation'
        & $hostPython -B tools/audit_bar.py $bar --output ('build/bar-audit'+$suffix+'.json')
        if ($LASTEXITCODE -ne 0) {throw 'BAR audit failed'}
        Write-Output ('Validated BAR: '+(Join-Path $PSScriptRoot $bar))
    }
} finally {
    foreach ($name in $saved.Keys) {[Environment]::SetEnvironmentVariable($name,$saved[$name],'Process')}
    Stop-Transcript | Out-Null
    Pop-Location
}
