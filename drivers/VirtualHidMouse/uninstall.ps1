#Requires -RunAsAdministrator
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$devices = @(Get-PnpDevice | Where-Object {
    if ($_.InstanceId -notlike 'ROOT\*') { return $false }
    $property = Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName 'DEVPKEY_Device_HardwareIds' -ErrorAction SilentlyContinue
    $property -and @($property.Data) -contains 'Root\AImerVirtualMouse'
})
$packages = @($devices | ForEach-Object {
    (Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName 'DEVPKEY_Device_DriverInfPath').Data
} | Where-Object { $_ -match '^oem[0-9]+\.inf$' } | Sort-Object -Unique)
foreach ($device in $devices) {
    & pnputil.exe /remove-device $device.InstanceId
    if ($LASTEXITCODE -eq 3010) { Write-Warning 'Restart Windows to complete device removal.' }
    elseif ($LASTEXITCODE -ne 0) { throw "Failed to remove $($device.InstanceId)" }
}
foreach ($package in $packages) {
    # No /force: a package still in use is left in place.
    & pnputil.exe /delete-driver $package
    if ($LASTEXITCODE -eq 3010) { Write-Warning 'Restart Windows to complete package removal.' }
    elseif ($LASTEXITCODE -ne 0) { throw "Could not remove $package. Restart Windows and inspect pnputil /enum-drivers." }
}
if ($devices.Count -eq 0) { Write-Host 'No AImer Virtual HID root device found; nothing removed.' }
else { Write-Host 'AImer Virtual HID mouse removed. Change mouse.yaml to send_input before restarting AImer.' }
