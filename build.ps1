#
# WFS Client Library Automatic Build Script
#

# Parameter definitions
param (
    [string]$BuildType = "Release",  # Build type: Debug or Release
    [switch]$Clean = $false,         # Whether to clean the build directory
    [switch]$Install = $false,       # Whether to install
    [string]$InstallPrefix = "C:/dev/local"  # Installation path
)

# Set vcpkg path
$VCPKG_PATH = "C:/dev/vcpkg"

# Set color output
function Write-ColorOutput($ForegroundColor, $Message) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    Write-Output $Message
    $host.UI.RawUI.ForegroundColor = $fc
}

# Display script information
Write-ColorOutput "Green" "===== WFS Client Library Automatic Build Script ====="
Write-Output "Build type: $BuildType"

# Clean build directory (if needed)
if ($Clean -and (Test-Path -Path "build")) {
    Write-ColorOutput "Yellow" "[INFO] Cleaning build directory..."
    Remove-Item -Path "build" -Recurse -Force
}

# Create and enter build directory
if (-not (Test-Path -Path "build")) {
    Write-ColorOutput "Cyan" "[INFO] Creating build directory..."
    New-Item -Path "build" -ItemType Directory | Out-Null
}
Write-ColorOutput "Cyan" "[INFO] Switching to build directory..."
Set-Location -Path "build"

# Configure project
Write-ColorOutput "Yellow" "[INFO] Running CMake configuration..."
$configResult = cmake .. -DCMAKE_TOOLCHAIN_FILE="$VCPKG_PATH/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE="$BuildType"
if ($LASTEXITCODE -ne 0) {
    Write-ColorOutput "Red" "[ERROR] CMake configuration failed"
    Set-Location -Path ".."
    exit $LASTEXITCODE
}

# Build project
Write-ColorOutput "Yellow" "[INFO] Building project..."
$buildResult = cmake --build . --config $BuildType
if ($LASTEXITCODE -ne 0) {
    Write-ColorOutput "Red" "[ERROR] Build failed"
    Set-Location -Path ".."
    exit $LASTEXITCODE
}

# Install (if needed)
if ($Install) {
    Write-ColorOutput "Yellow" "[INFO] Installing to $InstallPrefix..."
    $installResult = cmake --install . --prefix $InstallPrefix
    if ($LASTEXITCODE -ne 0) {
        Write-ColorOutput "Red" "[ERROR] Installation failed"
        Set-Location -Path ".."
        exit $LASTEXITCODE
    }
    Write-ColorOutput "Green" "[DONE] Installation completed"
}

# Display success information
Write-ColorOutput "Green" "===== Build Successful ====="
Write-Output "Library file: .\$BuildType\wfs_client.dll"
Write-Output "Example program: .\$BuildType\wfs_client_example.exe"

# Return to project root directory
Set-Location -Path ".." 