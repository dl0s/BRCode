function Get-BRCodeSdkRoot {
    param([string]$SdkRoot='')
    if (-not $SdkRoot) {
        foreach ($name in @('BRCODE_SDK_ROOT','INTROOP_SDK_ROOT')) {
            foreach ($scope in @('User','Process')) {
                $candidate=[Environment]::GetEnvironmentVariable($name,$scope)
                if ($candidate) { $SdkRoot=$candidate; break }
            }
            if ($SdkRoot) { break }
        }
    }
    if (-not $SdkRoot) { throw 'Pass -SdkRoot or configure BRCODE_SDK_ROOT / INTROOP_SDK_ROOT' }
    $root=(Resolve-Path -LiteralPath ([Environment]::ExpandEnvironmentVariables($SdkRoot))).Path
    foreach ($relative in @('host_10_3_1_12/win32/x86/usr/bin/qcc.exe',
        'host_10_3_1_12/win32/x86/usr/bin/moc.exe','host_10_3_1_12/win32/x86/usr/bin/ntoarm-readelf.exe',
        'target_10_3_1_995/qnx6/usr/include/qt4/QtCore/qglobal.h',
        'target_10_3_1_995/qnx6/armle-v7/usr/lib/libbbcascadesmultimedia.so')) {
        if (-not (Test-Path -LiteralPath (Join-Path $root $relative) -PathType Leaf)) { throw "Missing SDK dependency: $relative" }
    }
    return $root
}
function Get-BRCodeJavaBin {
    param([string]$SdkRoot,[string]$JavaBin='')
    if (-not $JavaBin) {
        foreach ($name in @('BRCODE_JAVA_BIN','INTROOP_JAVA_BIN')) {
            foreach ($scope in @('User','Process')) {
                $candidate=[Environment]::GetEnvironmentVariable($name,$scope)
                if ($candidate) {$JavaBin=$candidate;break}
            }
            if ($JavaBin) {break}
        }
    }
    $candidates=@()
    if ($JavaBin) {$candidates=@([Environment]::ExpandEnvironmentVariables($JavaBin))}
    else {
        foreach ($directory in @((Join-Path $SdkRoot 'features'),(Join-Path (Split-Path $SdkRoot -Parent) 'bbndk/features'))) {
            if (Test-Path -LiteralPath $directory) {
                $candidates+=@(Get-ChildItem -LiteralPath $directory -Directory -Filter 'com.qnx.tools.jre.*' |
                    Sort-Object Name -Descending | ForEach-Object {Join-Path $_.FullName 'jre/bin'})
            }
        }
        if ($env:JAVA_HOME) {$candidates+=Join-Path $env:JAVA_HOME 'bin'}
        $onPath=Get-Command java.exe -ErrorAction SilentlyContinue
        if ($onPath) {$candidates+=Split-Path $onPath.Source -Parent}
    }
    foreach ($directory in $candidates) {
        $java=Join-Path $directory 'java.exe'
        if (-not (Test-Path -LiteralPath $java -PathType Leaf)) {continue}
        $previousError=$ErrorActionPreference
        try {
            $ErrorActionPreference='Continue'
            $version=(& $java -version 2>&1 | ForEach-Object {$_.ToString()}) -join "`n"
            $code=$LASTEXITCODE
        } finally {$ErrorActionPreference=$previousError}
        if ($code -eq 0 -and $version -match 'version "1\.[78]\.') {return (Get-Item -LiteralPath $directory).FullName}
    }
    throw 'Native BAR packager requires Java 7 or 8. Use -JavaBin / BRCODE_JAVA_BIN.'
}
