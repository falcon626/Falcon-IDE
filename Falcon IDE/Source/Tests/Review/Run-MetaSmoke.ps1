param([string] $OutputRoot = [IO.Path]::GetTempPath())
$ErrorActionPreference = 'Stop'
$metaSourceRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$metaOutput = Join-Path ([IO.Path]::GetFullPath($OutputRoot)) ('falcon-meta-smoke-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $metaOutput | Out-Null
function Read-MetaSource([string] $path) {
    $bytes = [IO.File]::ReadAllBytes($path)
    try { return [Text.UTF8Encoding]::new($false, $true).GetString($bytes) }
    catch { return [Text.Encoding]::GetEncoding(932).GetString($bytes) }
}
function Write-MetaCombined([string] $destination, [string[]] $paths) {
    $combined = ($paths | ForEach-Object {
        (Read-MetaSource (Join-Path $metaSourceRoot $_)) -replace '(?m)^#(?:pragma once|include[^\r\n]*)\r?\n', ''
    }) -join "`r`n"
    [IO.File]::WriteAllText((Join-Path $metaOutput $destination), $combined, [Text.UTF8Encoding]::new($false))
}
Write-MetaCombined 'FlMetaWatcher.inc' @('Src/Framework/System/Watcher/FlFileWatcher.h', 'Src/Framework/System/Watcher/FlFileWatcher.cpp')
Write-MetaCombined 'FlMetaProduction.inc' @('Src/Framework/Resource/Meta/FlMetaFileManager.h', 'Src/Framework/Resource/Meta/FlMetaFileManager.cpp')
$metaJson = Read-MetaSource (Join-Path $metaSourceRoot 'Src/Framework/Utility/FlUtilityJson.hxx')
[IO.File]::WriteAllText((Join-Path $metaOutput 'FlMetaJson.inc'), $metaJson, [Text.UTF8Encoding]::new($false))
$metaVswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$metaVs = & $metaVswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$metaVs) { throw 'Visual C++ build tools not found.' }
$metaDevCmd = Join-Path $metaVs 'Common7/Tools/VsDevCmd.bat'
$metaTestFile = Join-Path $PSScriptRoot 'FlMetaSmoke.cpp'
$metaCrypterFile = Join-Path $metaSourceRoot 'StaticLib/FlCrypter/Src/FlCrypter.cpp'
$metaIncludes = Join-Path $metaSourceRoot 'StaticLib'
$metaExe = Join-Path $metaOutput 'FlMetaSmoke.exe'
$metaCompileFile = Join-Path $metaOutput 'compile.cmd'
[IO.File]::WriteAllText($metaCompileFile, "@echo off`r`ncall `"$metaDevCmd`" -no_logo -arch=amd64`r`nif errorlevel 1 exit /b %errorlevel%`r`ncd /d `"$metaOutput`"`r`ncl /nologo /std:c++20 /EHsc /MDd /W4 /WX /utf-8 /I `"$metaOutput`" /I `"$metaIncludes`" `"$metaTestFile`" `"$metaCrypterFile`" /Fe`"$metaExe`"`r`n", [Text.Encoding]::Default)
& $env:ComSpec /d /c $metaCompileFile
if ($LASTEXITCODE -ne 0) { throw 'Meta smoke compilation failed.' }
$metaLog = Join-Path $metaOutput 'run.log'
$metaErrorLog = Join-Path $metaOutput 'error.log'
$metaProcess = Start-Process -FilePath $metaExe -ArgumentList ('"' + $metaOutput + '"') -WindowStyle Hidden -PassThru -RedirectStandardOutput $metaLog -RedirectStandardError $metaErrorLog
if (!$metaProcess.WaitForExit(20000)) { $metaProcess.Kill(); throw 'Meta smoke timed out (possible watcher deadlock).' }
Get-Content -LiteralPath $metaLog
Get-Content -LiteralPath $metaErrorLog
if ($metaProcess.ExitCode -ne 0) { throw "Meta smoke failed: $($metaProcess.ExitCode); logs: $metaOutput" }
