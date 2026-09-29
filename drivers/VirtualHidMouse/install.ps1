#Requires -RunAsAdministrator
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackagePath,
    [Parameter(Mandatory=$true)][string]$DevConPath
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$package = (Resolve-Path -LiteralPath $PackagePath).Path
$devcon = (Resolve-Path -LiteralPath $DevConPath).Path
foreach ($name in @('AImerVirtualMouse.inf', 'AImerVirtualMouse.sys', 'AImerVirtualMouse.cat')) {
    if (-not (Test-Path -LiteralPath (Join-Path $package $name) -PathType Leaf)) {
        throw "Missing driver package file: $name"
    }
}
$inf = Join-Path $package 'AImerVirtualMouse.inf'
$devices = @(Get-PnpDevice | Where-Object {
    if ($_.InstanceId -notlike 'ROOT\*') { return $false }
    $property = Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName 'DEVPKEY_Device_HardwareIds' -ErrorAction SilentlyContinue
    $property -and @($property.Data) -contains 'Root\AImerVirtualMouse'
})
if ($devices.Count -gt 1) { throw 'Multiple AImer root devices found. Run uninstall.ps1 first.' }
# pnputil /add-driver alone cannot create a root-enumerated device.
if ($devices.Count -eq 0) {
    & $devcon install $inf 'Root\AImerVirtualMouse'
} else {
    & $devcon update $inf 'Root\AImerVirtualMouse'
}
$code = $LASTEXITCODE
if ($code -eq 1) { Write-Warning 'Driver installed/updated; restart Windows to finish.'; exit 0 }
if ($code -ne 0) { throw "DevCon failed ($code). Check driver signatures and %windir%\inf\setupapi.dev.log." }
$device = Get-PnpDevice | Where-Object {
    if ($_.InstanceId -notlike 'ROOT\*') { return $false }
    $property = Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName 'DEVPKEY_Device_HardwareIds' -ErrorAction SilentlyContinue
    $property -and @($property.Data) -contains 'Root\AImerVirtualMouse'
}
$device | Format-Table Status, FriendlyName, InstanceId
if (-not $device -or @($device | Where-Object Status -ne 'OK').Count -ne 0) {
    throw 'Root device is not ready. Inspect Device Manager and setupapi.dev.log.'
}
Write-Host 'Installed. Run AImer elevated with mouse_backend: virtual_hid. Only one client may open the device.'
