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

$finalSln = Join-Path $root "Utilities\7-1_FinalProject\7-1_FinalProjectMilestones.sln"
if (Test-Path $finalSln) {
    Write-Host ""
    Write-Host "Setup complete."
    Write-Host "Opening final project solution..."
    Start-Process $finalSln
} else {
    Write-Host ""
    Write-Host "Setup complete, but final project solution was not found at:"
    Write-Host "  $finalSln"
}
