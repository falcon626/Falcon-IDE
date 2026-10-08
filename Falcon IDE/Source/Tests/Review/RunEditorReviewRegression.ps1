$ErrorActionPreference = 'Stop'
$sourceRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$encoding = [Text.Encoding]::GetEncoding(932)
$template = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'FlEditorReviewRegression.cpp.in'))
$methods = @{
    WriteProjectTextFile = @('Framework/System/VisualStudioManager/FlVisualStudioManager.cpp', 'static bool WriteProjectTextFile')
    LoadTextFile = @('Framework/System/VisualStudioManager/FlVisualStudioManager.cpp', 'static const std::string LoadTextFile')
    IsValidProjectName = @('Framework/System/VisualStudioManager/FlVisualStudioManager.cpp', 'bool FlVisualStudioProjectManager::IsValidProjectName')
    CreateNewProject = @('Framework/System/VisualStudioManager/FlVisualStudioManager.cpp', 'bool FlVisualStudioProjectManager::CreateNewProject')
    CreateDetectedBuildCommand = @('Framework/System/VisualStudioManager/FlVisualStudioManager.cpp', 'std::string FlVisualStudioProjectManager::CreateBuildCommand')
    ProcessExecuteCommand = @('Framework/Unit/FlProcessCreater.ixx', 'export bool ExecuteCommand')
    CreateSourceFiles = @('Framework/System/VisualStudioManager/FlVisualStudioManager.cpp', 'bool FlVisualStudioProjectManager::CreateSourceFiles')
    CreateVcxproj = @('Framework/System/VisualStudioManager/FlVisualStudioManager.cpp', 'bool FlVisualStudioProjectManager::CreateVcxproj')
    CreateFilters = @('Framework/System/VisualStudioManager/FlVisualStudioManager.cpp', 'bool FlVisualStudioProjectManager::CreateFilters')
    AddProjectToSolutionSlnx = @('Framework/System/VisualStudioManager/FlVisualStudioManager.cpp', 'bool FlVisualStudioProjectManager::AddProjectToSolutionSlnx')
    ReadFileBinary = @('../StaticLib/FlCrypter/Src/FlCrypter.cpp', 'bool ReadFileBinary')
    WriteFileBinary = @('../StaticLib/FlCrypter/Src/FlCrypter.cpp', 'bool WriteFileBinary')
    SetParent = @('Framework/ImGui/Editor/FlECSInspectorAndHierarchy.cpp', 'void FlECSInspectorAndHierarchy::SetParent')
    DeleteEntityRecursive = @('Framework/ImGui/Editor/FlECSInspectorAndHierarchy.cpp', 'void FlECSInspectorAndHierarchy::DeleteEntityRecursive')
    RenderEntityNode = @('Framework/ImGui/Editor/FlECSInspectorAndHierarchy.cpp', 'void FlECSInspectorAndHierarchy::RenderEntityNode')
    ExecuteCommand = @('Framework/ImGui/Editor/FlTerminalEditor.cpp', 'std::future<bool> FlTerminalEditor::ExecuteCommand')
    WorkerThread = @('Framework/ImGui/Editor/FlTerminalEditor.cpp', 'void FlTerminalEditor::WorkerThread')
    ChangedFilesRefresh = @('Framework/ImGui/Editor/FlScriptModuleEditor.cpp', 'void FlScriptModuleEditor::ChangedFilesRefresh')
    Update = @('Framework/ImGui/Editor/FlScriptModuleEditor.cpp', 'void FlScriptModuleEditor::Update')
    CanDeleteProject = @('Framework/ImGui/Editor/FlScriptModuleEditor.cpp', 'bool FlScriptModuleEditor::CanDeleteProject')
    RenderPopup = @('Framework/ImGui/Editor/FlScriptModuleEditor.cpp', 'void FlScriptModuleEditor::RenderPopup')
}
foreach ($entry in $methods.GetEnumerator()) {
    $body = [IO.File]::ReadAllText((Join-Path $sourceRoot ('Src/' + $entry.Value[0])), $encoding)
    $pattern = '(?ms)^(?<indent>[ \t]*)' + [regex]::Escape($entry.Value[1]) + '\(.*?^\k<indent>}'
    $match = [regex]::Match($body, $pattern)
    if (!$match.Success) { throw "Method not found: $($entry.Key)" }
    $method = $match.Value
    if ($entry.Key -eq 'ProcessExecuteCommand') { $method = $method.Replace('export bool', 'bool') }
    if ($entry.Key -eq 'CreateDetectedBuildCommand') { $method = $method.Replace('::CreateBuildCommand', '::CreateDetectedBuildCommand') }
    $template = $template.Replace('@' + $entry.Key + '@', $method)
}
$terminalHeader = [IO.File]::ReadAllText((Join-Path $sourceRoot 'Src/Framework/ImGui/Editor/FlTerminalEditor.h'), $encoding).Replace('#pragma once', '')
$template = $template.Replace('@TerminalHeader@', $terminalHeader)
$tinyXmlCpp = (Join-Path $sourceRoot 'Src/Framework/System/XMLParser/tinyxml2.cpp').Replace('\', '/')
$template = $template.Replace('@TinyXmlCpp@', '#include "' + $tinyXmlCpp + '"')
$testDir = Join-Path $env:TEMP ('falcon-editor-regression-' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($testDir) | Out-Null
$fixtureDir = Join-Path $PSScriptRoot ('.editor-fixture-' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($fixtureDir) | Out-Null
$testCpp = Join-Path $testDir 'regression.cpp'
[IO.File]::WriteAllText($testCpp, $template, [Text.UTF8Encoding]::new($false))
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath -utf8
if ($LASTEXITCODE -ne 0 -or !$installation) { throw 'Visual C++ build tools not found.' }
$vcvars = Join-Path $installation 'VC/Auxiliary/Build/vcvarsall.bat'
$includeEditor = Join-Path $sourceRoot 'Src/Framework/ImGui/Editor'
$includeLibrary = [IO.Path]::GetFullPath((Join-Path $sourceRoot '../Library'))
$includeStatic = Join-Path $sourceRoot 'StaticLib'
$exe = Join-Path $testDir 'regression.exe'
$cmdFile = Join-Path $testDir 'compile.cmd'
$compile = 'call "' + $vcvars + '" x64 >nul && cl /nologo /std:c++latest /EHsc /W4 /utf-8 /I"' + $includeEditor + '" /I"' + $includeStatic + '" /I"' + $includeLibrary + '" "' + $testCpp + '" /Fe:"' + $exe + '" /Fo:"' + (Join-Path $testDir 'regression.obj') + '"'
[IO.File]::WriteAllText($cmdFile, "@echo off`r`n" + $compile, [Text.Encoding]::Default)
& $env:ComSpec /d /c $cmdFile
if ($LASTEXITCODE -ne 0) { throw 'Editor regression compilation failed.' }
& $exe $fixtureDir
if ($LASTEXITCODE -ne 0) { throw 'Editor regression check failed.' }
$resolvedFixture = [IO.Path]::GetFullPath($fixtureDir)
$fixturePrefix = [IO.Path]::GetFullPath($PSScriptRoot) + [IO.Path]::DirectorySeparatorChar + '.editor-fixture-'
if (!$resolvedFixture.StartsWith($fixturePrefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected fixture cleanup path.' }
Remove-Item -LiteralPath $resolvedFixture -Recurse -Force
Write-Output "PASS: collision/name, atomic project retry, modal build/deletion guard, hierarchy draw/deletion, toolchain detection, build versions and owned process shutdown. $testDir"
