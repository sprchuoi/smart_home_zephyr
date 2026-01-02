# PowerShell script to attach ESP32 to WSL
# Run this in PowerShell as Administrator
#
# Usage: .\attach-esp32.ps1

$BUSID = "2-1"
$DeviceName = "CP210x"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "ESP32 USB Device Attachment for WSL" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "✗ This script must be run as Administrator" -ForegroundColor Red
    Write-Host ""
    Write-Host "Right-click PowerShell and select 'Run as Administrator'" -ForegroundColor Yellow
    Write-Host "Then run this script again."
    exit 1
}

# Check if usbipd is installed
try {
    $null = Get-Command usbipd -ErrorAction Stop
} catch {
    Write-Host "✗ usbipd-win is not installed" -ForegroundColor Red
    Write-Host ""
    Write-Host "Install it by running:" -ForegroundColor Yellow
    Write-Host "  winget install --interactive --exact dorssel.usbipd-win" -ForegroundColor White
    Write-Host ""
    Write-Host "Or download from:" -ForegroundColor Yellow
    Write-Host "  https://github.com/dorssel/usbipd-win/releases" -ForegroundColor White
    exit 1
}

Write-Host "ℹ Current USB devices:" -ForegroundColor Cyan
Write-Host ""
usbipd list
Write-Host ""

# Check if device is already attached
$attached = usbipd list | Select-String -Pattern "$BUSID.*Attached"
if ($attached) {
    Write-Host "✓ Device $BUSID is already attached to WSL" -ForegroundColor Green
    exit 0
}

# Check if device needs binding
$needsBind = usbipd list | Select-String -Pattern "$BUSID.*Not shared"
if ($needsBind) {
    Write-Host "ℹ Device needs to be bound first..." -ForegroundColor Yellow
    try {
        usbipd bind --busid $BUSID
        Write-Host "✓ Device bound successfully" -ForegroundColor Green
    } catch {
        Write-Host "✗ Failed to bind device: $_" -ForegroundColor Red
        exit 1
    }
}

# Attach device to WSL
Write-Host "ℹ Attaching $DeviceName (BUSID: $BUSID) to WSL..." -ForegroundColor Cyan
try {
    usbipd attach --wsl --busid $BUSID
    Write-Host "✓ Device attached successfully" -ForegroundColor Green
} catch {
    Write-Host "✗ Failed to attach device: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Make sure WSL is running and try again." -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "✓ ESP32 attached to WSL" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Ask if user wants to flash from WSL
$flash = Read-Host "Flash firmware from WSL? (y/n)"
if ($flash -eq "y" -or $flash -eq "Y") {
    Write-Host ""
    Write-Host "Starting flash process in WSL..." -ForegroundColor Cyan
    Write-Host ""
    
    # Run the WSL script
    wsl bash -c "cd ~/sandboxes/example-application/software && ./scripts/attach-esp32.sh"
} else {
    Write-Host ""
    Write-Host "In WSL, the device should appear as:" -ForegroundColor White
    Write-Host "  /dev/ttyUSB0" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "You can flash firmware from WSL:" -ForegroundColor White
    Write-Host "  cd ~/sandboxes/example-application/software" -ForegroundColor Yellow
    Write-Host "  ./scripts/attach-esp32.sh" -ForegroundColor Yellow
    Write-Host "  # Or manually:" -ForegroundColor Gray
    Write-Host "  ./make.sh flash --port /dev/ttyUSB0" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To detach when done:" -ForegroundColor White
    Write-Host "  usbipd detach --busid $BUSID" -ForegroundColor Yellow
    Write-Host ""
}
