$ErrorActionPreference = 'Stop'
$sourceRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$smokeDir = Join-Path ([IO.Path]::GetTempPath()) ('falcon-ecs-smoke-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $smokeDir | Out-Null
$encoding = [Text.Encoding]::GetEncoding(932)
$runtimeAPI = [IO.File]::ReadAllText((Join-Path $sourceRoot 'Src/Framework/Module/FlRunTimeAndDLLsCommon.h++'), $encoding)
$runtimeInput = (Join-Path $sourceRoot 'Src/Framework/Module/RuntimeModule/Input.h').Replace('\', '/')
$runtimeAPI = $runtimeAPI.Replace('"RuntimeModule/Input.h"', '"' + $runtimeInput + '"')
[IO.File]::WriteAllText((Join-Path $smokeDir 'FlECSRuntimeAPI.inc'), $runtimeAPI, [Text.UTF8Encoding]::new($false))
$header = [IO.File]::ReadAllText((Join-Path $sourceRoot 'Src/Core/FlEntityComponentSystemKernel.h'), $encoding)
$implementation = [IO.File]::ReadAllText((Join-Path $sourceRoot 'Src/Core/FlEntityComponentSystemKernel.cpp'), $encoding)
$header = $header -replace '(?m)^#(?:pragma once|include[^\r\n]*)\r?\n', ''
$implementation = $implementation -replace '(?m)^#include[^\r\n]*\r?\n', ''
[IO.File]::WriteAllText((Join-Path $smokeDir 'FlECSKernelProduction.inc'), $header + "`r`n" + $implementation, [Text.UTF8Encoding]::new($false))
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Visual C++ build tools not found.' }
$devCmd = Join-Path $installation 'Common7/Tools/VsDevCmd.bat'
$testFile = Join-Path $PSScriptRoot 'FlECSKernelSmoke.cpp'
$exe = Join-Path $smokeDir 'FlECSKernelSmoke.exe'
$compileFile = Join-Path $smokeDir 'compile.cmd'
[IO.File]::WriteAllText($compileFile, "@echo off`r`ncall `"$devCmd`" -no_logo -arch=amd64`r`nif errorlevel 1 exit /b %errorlevel%`r`ncl /nologo /std:c++20 /EHsc /MDd /W4 /WX /utf-8 /I `"$smokeDir`" `"$testFile`" /Fo`"$smokeDir\FlECSKernelSmoke.obj`" /Fe`"$exe`"`r`n", [Text.Encoding]::Default)
& $env:ComSpec /d /c $compileFile
if ($LASTEXITCODE -ne 0) { throw 'ECS smoke compilation failed.' }
$process = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
if (!$process.WaitForExit(15000)) { $process.Kill(); throw 'ECS smoke timed out (possible deadlock).' }
if ($process.ExitCode -ne 0) { throw "ECS smoke failed: $($process.ExitCode)" }
'ECS smoke passed: callback reentry, snapshot removal, missing DLL roundtrip, owner unload.'
