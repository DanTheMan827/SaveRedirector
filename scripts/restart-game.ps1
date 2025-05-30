# This script restarts the game on the connected device using adb.

Param(
    [Parameter(Mandatory = $false)]
    [String] $packageId = "com.harmonixmusic.kata"
)

# Check if package_id.txt exists and use that as the package name
if (Test-Path "$PSScriptRoot/../package_id.txt") {
    $packageId = Get-Content "$PSScriptRoot/../package_id.txt"
    Write-Output "Using package ID from package_id.txt: $packageId"
}

# Force stop the Audica game
adb shell am force-stop "$packageId"

# Start the Audica game
adb shell am start "$packageId/com.unity3d.player.UnityPlayerActivity"
