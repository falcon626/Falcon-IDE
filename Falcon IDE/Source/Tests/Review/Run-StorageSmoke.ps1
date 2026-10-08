$ErrorActionPreference = 'Stop'
$taskSourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskVswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$taskVs = & $taskVswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$taskVs) { throw 'MSVC is required.' }
$taskVcvars = Join-Path $taskVs 'VC/Auxiliary/Build/vcvarsall.bat'
$taskEnvironment = & $env:ComSpec /d /c "call `"$taskVcvars`" x64 >nul && set"
if ($LASTEXITCODE) { throw 'vcvarsall failed.' }
foreach ($taskEntry in $taskEnvironment) {
    if ($taskEntry -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
    }
}
$taskOutput = Join-Path $PSScriptRoot 'Release'
New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
$taskTestData = Join-Path $taskOutput 'TestData'
New-Item -ItemType Directory -Path $taskTestData -Force | Out-Null
$env:TEMP = $taskTestData
$env:TMP = $taskTestData
Push-Location $taskOutput
try {
    $taskCases = @(
        @{ Name = 'FlStorageSmoke'; Files = @((Join-Path $PSScriptRoot 'FlStorageSmoke.cpp'), (Join-Path $taskSourceRoot 'StaticLib/FlCrypter/Src/FlCrypter.cpp')) },
        @{ Name = 'FlCrypterSmoke'; Files = @((Join-Path $taskSourceRoot 'StaticLib/FlCrypter/Tests/FlCrypterSmoke.cpp'), (Join-Path $taskSourceRoot 'StaticLib/FlCrypter/Src/FlCrypter.cpp')) },
        @{ Name = 'FlLogWatcherSmoke'; Files = @((Join-Path $PSScriptRoot 'FlLogWatcherSmoke.cpp')) },
        @{ Name = 'FlModelImportSmoke'; Files = @((Join-Path $PSScriptRoot 'FlModelImportSmoke.cpp')) },
        @{ Name = 'FlGuidSmoke'; Files = @((Join-Path $PSScriptRoot 'FlGuidSmoke.cpp')) }
    )
    foreach ($taskCase in $taskCases) {
        & cl.exe /nologo /std:c++20 /EHsc /W4 /WX "/I$(Join-Path $taskSourceRoot 'StaticLib')" "/I$(Join-Path $taskSourceRoot 'packages/directxtk12_desktop_2019.2025.10.28.1/include')" "/I$(Join-Path $taskSourceRoot '../Library/assimp/include')" "/Fe:$($taskCase.Name).exe" @($taskCase.Files) Rpcrt4.lib
        if ($LASTEXITCODE) { throw "$($taskCase.Name) compile failed." }
        & (Join-Path $taskOutput "$($taskCase.Name).exe")
        if ($LASTEXITCODE) { throw "$($taskCase.Name) failed." }
    }
}
finally { Pop-Location }
