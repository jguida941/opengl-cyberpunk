[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("Win32")]
    [string]$Platform = "Win32"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = (Resolve-Path (Join-Path $scriptDir "..")).Path
$solutionPath = Join-Path $projectDir "6-2_Assignment.sln"

if (-not (Test-Path $solutionPath)) {
    throw "Solution not found at '$solutionPath'."
}

function Get-MSBuildPath {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        throw "vswhere.exe was not found. Install Visual Studio 2022 or Build Tools."
    }

    $msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
    if (-not $msbuild) {
        throw "MSBuild.exe was not found. Verify Visual Studio 2022 installation."
    }

    return $msbuild
}

$msbuildPath = Get-MSBuildPath
Write-Host "Building $solutionPath ($Configuration|$Platform)..."
& $msbuildPath $solutionPath /m /p:Configuration=$Configuration /p:Platform=$Platform

if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

$candidateExePaths = @(
    (Join-Path $projectDir "$Configuration\6-2_Assignment.exe"),
    (Join-Path $projectDir "$Platform\$Configuration\6-2_Assignment.exe"),
    (Join-Path $projectDir "6-2_Assignment\$Configuration\6-2_Assignment.exe"),
    (Join-Path $projectDir "6-2_Assignment\$Platform\$Configuration\6-2_Assignment.exe")
)

$exePath = $candidateExePaths | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $exePath) {
    $exePath = Get-ChildItem -Path $projectDir -Filter "6-2_Assignment.exe" -Recurse -File |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}

if (-not $exePath) {
    throw "Build succeeded, but 6-2_Assignment.exe was not found."
}

$exeDir = Split-Path -Parent $exePath
$glewSourcePath = Join-Path $projectDir "glew32.dll"
$glewTargetPath = Join-Path $exeDir "glew32.dll"

if (Test-Path $glewSourcePath) {
    Copy-Item $glewSourcePath $glewTargetPath -Force
}

Write-Host "Launching $exePath"
Start-Process -FilePath $exePath -WorkingDirectory $exeDir
