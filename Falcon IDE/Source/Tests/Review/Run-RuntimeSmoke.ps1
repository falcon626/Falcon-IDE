param([ValidateSet('Debug','Release')][string]$Configuration = 'Release', [switch]$Launcher)
$ErrorActionPreference = 'Stop'
$taskSource = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskSandbox = Join-Path ([IO.Path]::GetTempPath()) ('FIDE-' + [guid]::NewGuid().ToString('N').Substring(0,8))
$taskRuntimeSource = Join-Path $taskSandbox 'Source'
$taskRuntimeOutput = Join-Path $taskRuntimeSource "x64/$Configuration"
New-Item -ItemType Directory -Path $taskRuntimeOutput -Force | Out-Null
Get-ChildItem -LiteralPath (Join-Path $taskSource "x64/$Configuration") | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $taskRuntimeOutput -Recurse -Force
}
Copy-Item -LiteralPath (Join-Path $taskSource 'CryptedAssets') -Destination $taskRuntimeSource -Recurse -Force
Copy-Item -LiteralPath (Join-Path $taskSource 'FlProject-DX12.slnx') -Destination $taskRuntimeSource
foreach ($taskRelative in @('Src/Framework/Module/ScriptDLLs', 'Src/Framework/System/VisualStudioManager/Sample')) {
    $taskDestination = Join-Path $taskRuntimeSource $taskRelative
    New-Item -ItemType Directory -Path (Split-Path $taskDestination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $taskSource $taskRelative) -Destination $taskDestination -Recurse -Force
}
if (!('FalconSmokeWindow' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class FalconSmokeWindow {
    private delegate bool Callback(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] private static extern bool EnumWindows(Callback callback, IntPtr parameter);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wparam, IntPtr lparam);
    [DllImport("user32.dll")] private static extern IntPtr SendMessageTimeout(IntPtr window, uint message, IntPtr wparam, IntPtr lparam, uint flags, uint timeout, out IntPtr result);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr OpenProcess(uint access, bool inherit, uint process);
    [DllImport("kernel32.dll", SetLastError=true)] private static extern bool GetExitCodeProcess(IntPtr process, out uint code);
    [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
    public static uint ExitCode(IntPtr process) { uint code; if (!GetExitCodeProcess(process, out code)) throw new System.ComponentModel.Win32Exception(); return code; }
    public static IntPtr Find(uint pid) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((window, unused) => { uint owner; GetWindowThreadProcessId(window, out owner);
            if (owner == pid && Title(window).StartsWith("Falcon IDE <Fps = ")) { found = window; return false; }
            return true; }, IntPtr.Zero);
        return found;
    }
    public static string Title(IntPtr window) { var text = new StringBuilder(256); GetWindowText(window, text, text.Capacity); return text.ToString(); }
    public static string Titles(uint pid) { var titles = new StringBuilder(); EnumWindows((window, unused) => { uint owner; GetWindowThreadProcessId(window, out owner); if (owner == pid) titles.Append(Title(window)).Append(" | "); return true; }, IntPtr.Zero); return titles.ToString(); }
    public static bool Responds(IntPtr window) { IntPtr result; return SendMessageTimeout(window, 0, IntPtr.Zero, IntPtr.Zero, 2, 2000, out result) != IntPtr.Zero; }
}
'@
}
$taskExe = Join-Path $taskRuntimeOutput 'FlProject-DX12.exe'
$taskBuiltExe = Join-Path $taskSource "x64/$Configuration/FlProject-DX12.exe"
if ((Get-FileHash $taskExe).Hash -ne (Get-FileHash $taskBuiltExe).Hash) { throw 'Runtime binary does not match the current build.' }
$taskStart = Get-Date
if ($Launcher) {
    $taskExecution = Join-Path $taskSandbox 'Execution'
    New-Item -ItemType Directory -Path $taskExecution -Force | Out-Null
    $taskLauncherRoot = (Resolve-Path (Join-Path $taskSource '../Execution')).Path
    Copy-Item -LiteralPath (Join-Path $taskLauncherRoot "Launcher/x64/$Configuration/Launcher.exe") -Destination $taskExecution
    $taskConfig = [IO.File]::ReadAllText((Join-Path $taskLauncherRoot 'launcher.cfg')).Replace('\Release\', "\$Configuration\")
    [IO.File]::WriteAllText((Join-Path $taskExecution 'launcher.cfg'), $taskConfig)
    $taskLauncher = Start-Process -FilePath (Join-Path $taskExecution 'Launcher.exe') -WorkingDirectory $taskExecution -WindowStyle Hidden -PassThru
    if (!$taskLauncher.WaitForExit(10000) -or $taskLauncher.ExitCode -ne 0) { throw 'Launcher failed.' }
    $taskProcess = $null
    for ($taskTry=0; $taskTry -lt 50 -and !$taskProcess; $taskTry++) {
        $taskProcess = Get-Process -Name 'FlProject-DX12' -ErrorAction SilentlyContinue | Where-Object Path -EQ $taskExe | Select-Object -First 1
        if (!$taskProcess) { Start-Sleep -Milliseconds 100 }
    }
    if (!$taskProcess) { throw 'Launcher did not start the copied build.' }
}
else { $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRuntimeSource -WindowStyle Hidden -PassThru }
$taskHandle = [FalconSmokeWindow]::OpenProcess(0x1000, $false, $taskProcess.Id)
if ($taskHandle -eq [IntPtr]::Zero) { throw 'Could not retain the test process handle.' }
try {
    $taskWindow = [IntPtr]::Zero
    for ($taskTry=0; $taskTry -lt 100 -and $taskWindow -eq [IntPtr]::Zero; $taskTry++) {
        if ($taskProcess.HasExited) { throw "Runtime exited during startup: $($taskProcess.ExitCode)" }
        $taskWindow = [FalconSmokeWindow]::Find($taskProcess.Id)
        if ($taskWindow -eq [IntPtr]::Zero) { Start-Sleep -Milliseconds 100 }
    }
    if ($taskWindow -eq [IntPtr]::Zero) { throw 'No running editor window found.' }
    for ($taskSample=0; $taskSample -lt 5; $taskSample++) {
        Start-Sleep -Seconds 3
        if ($taskProcess.HasExited -or ![FalconSmokeWindow]::Responds($taskWindow)) { throw 'Runtime stopped responding.' }
    }
    $taskTitle = [FalconSmokeWindow]::Title($taskWindow)
    [FalconSmokeWindow]::PostMessage($taskWindow, 0x10, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    $taskExited = $taskProcess.WaitForExit(20000)
    $taskExitCode = [FalconSmokeWindow]::ExitCode($taskHandle)
    if (!$taskExited -or $taskExitCode -ne 0) { throw "Runtime did not exit normally: exited=$taskExited, code=$taskExitCode, windows=$([FalconSmokeWindow]::Titles($taskProcess.Id)), sandbox=$taskSandbox" }
    if (!(Test-Path -LiteralPath (Join-Path $taskRuntimeSource 'Assets/Scene/lastTime.flscene'))) { throw 'Working assets were not retained.' }
    if (!(Test-Path -LiteralPath (Join-Path $taskRuntimeSource 'CryptedAssets'))) { throw 'Archive was not retained.' }
    $taskEventErrors = @()
    $taskCrashes = @(Get-WinEvent -FilterHashtable @{LogName='Application';Id=1000,1001;StartTime=$taskStart} -ErrorAction SilentlyContinue -ErrorVariable taskEventErrors | Where-Object Message -Match 'FlProject-DX12')
    if (@($taskEventErrors | Where-Object FullyQualifiedErrorId -NotMatch 'NoMatchingEventsFound').Count) { throw 'Application event log could not be verified.' }
    if ($taskCrashes.Count) { throw 'Application Error/WER recorded a runtime failure.' }
    [pscustomobject]@{Configuration=$Configuration;Launcher=[bool]$Launcher;Samples=5;ResponsiveSeconds=15;Exit=$taskExitCode;Title=$taskTitle;SHA256=(Get-FileHash $taskExe).Hash;Sandbox=$taskSandbox;Crashes=$taskCrashes.Count} | ConvertTo-Json -Compress
}
finally { if (!$taskProcess.HasExited) { Stop-Process -Id $taskProcess.Id -Force }; [FalconSmokeWindow]::CloseHandle($taskHandle) | Out-Null }
