$ErrorActionPreference = 'Stop'
$taskSourceRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskVswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$taskVs = & $taskVswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
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
$taskToolkit = Join-Path $taskSourceRoot 'packages/directxtk12_desktop_2019.2025.10.28.1'
Push-Location $taskOutput
try {
    & cl /nologo /std:c++20 /EHsc /MD /W4 /WX "/I$taskToolkit/include" /Fe:FlInputSmoke.exe (Join-Path $taskSourceRoot 'Tests/Input/FlInputSmoke.cpp') (Join-Path $taskSourceRoot 'Src/Framework/System/Input/FlInput.cpp') (Join-Path $taskToolkit 'native/lib/x64/Release/DirectXTK12.lib') user32.lib
    if ($LASTEXITCODE) { throw 'Input smoke compile failed.' }
    & './FlInputSmoke.exe'
    if ($LASTEXITCODE) { throw 'Input smoke failed.' }
    & cl /nologo /std:c++20 /EHsc /MD /W4 /WX /LD /Fe:FlInputScriptModuleSmoke.dll (Join-Path $taskSourceRoot 'Tests/Input/FlInputScriptModuleSmoke.cpp')
    if ($LASTEXITCODE) { throw 'Input module compile failed.' }
    & cl /nologo /std:c++20 /EHsc /MD /W4 /WX /Fe:FlInputScriptModuleHostSmoke.exe (Join-Path $taskSourceRoot 'Tests/Input/FlInputScriptModuleHostSmoke.cpp')
    if ($LASTEXITCODE) { throw 'Input host compile failed.' }
    & './FlInputScriptModuleHostSmoke.exe' (Join-Path $taskOutput 'FlInputScriptModuleSmoke.dll')
    if ($LASTEXITCODE) { throw 'Input module smoke failed.' }
}
finally { Pop-Location }
