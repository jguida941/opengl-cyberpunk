$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Write-Host "Setting up CS330Content on Windows"
Write-Host "Repo: $root"

if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    Write-Warning "winget was not found. Install App Installer from Microsoft Store, then re-run this script."
    exit 1
}

Write-Host ""
Write-Host "Installing development tools (if missing)..."
winget install --id Kitware.CMake --silent --accept-package-agreements --accept-source-agreements
winget install --id Git.Git --silent --accept-package-agreements --accept-source-agreements

Write-Host ""
Write-Host "Optional but recommended: Visual Studio 2022 with Desktop development with C++ workload."
Write-Host "If VS is not installed, run the Visual Studio installer and add the C++ workload."

New-Item -ItemType Directory -Force -Path (Join-Path $root "build") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $root "out") | Out-Null

$assignmentSln = Join-Path $root "Projects\6-2_Assignment\6-2_Assignment.sln"
if (Test-Path $assignmentSln) {
    Write-Host ""
    Write-Host "Setup complete."
    Write-Host "Opening 6-2 assignment solution..."
    Start-Process $assignmentSln
} else {
    Write-Host ""
    Write-Host "Setup complete, but 6-2 assignment solution was not found at:"
    Write-Host "  $assignmentSln"
}
