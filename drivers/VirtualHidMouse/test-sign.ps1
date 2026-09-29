[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Thumbprint,
    [string]$PackagePath,
    [string]$Destination,
    [string]$KitVersion = '10.0.28000.0'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $PackagePath) { $PackagePath = Join-Path $scriptDirectory 'out\x64\Debug\AImerVirtualMouse' }
if (-not $Destination) { $Destination = Join-Path $scriptDirectory '..\..\bin\Release\VirtualHidDriver.Debug' }
if ($PackagePath -match '(?i)[\\/]Release[\\/]' -or $Destination -match '(?i)VirtualHidDriver\.Release(?:[\\/]|$)') {
    throw 'Test signing is only for Debug packages; Release must use Hardware Dev Center signing.'
}
$cert = Get-Item -LiteralPath "Cert:\CurrentUser\My\$Thumbprint"
if (-not $cert.HasPrivateKey -or $cert.NotAfter -le (Get-Date)) { throw 'A valid certificate with a private key is required.' }
if (@($cert.EnhancedKeyUsageList | ForEach-Object { [string]$_.ObjectId }) -notcontains '1.3.6.1.5.5.7.3.3') {
    throw 'The certificate must support code signing.'
}
$tools = "${env:ProgramFiles(x86)}\Windows Kits\10\bin\$KitVersion"
$signTool = Join-Path $tools 'x64\signtool.exe'
$inf2cat = Join-Path $tools 'x86\Inf2Cat.exe'
$source = (Resolve-Path -LiteralPath $PackagePath).Path
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
$destinationPath = (Resolve-Path -LiteralPath $Destination).Path
if ($source -eq $destinationPath) { throw 'Sign a separate package copy; keep build outputs unsigned.' }
foreach ($name in @('AImerVirtualMouse.inf','AImerVirtualMouse.sys')) {
    Copy-Item -LiteralPath (Join-Path $source $name) -Destination (Join-Path $destinationPath $name) -Force
}
$sys = Join-Path $destinationPath 'AImerVirtualMouse.sys'
$cat = Join-Path $destinationPath 'aimervirtualmouse.cat'
# Sign SYS first, regenerate its catalog hashes, then sign CAT. No online timestamp.
& $signTool sign /v /fd SHA256 /s My /sha1 $Thumbprint $sys
if ($LASTEXITCODE -ne 0) { throw 'SYS signing failed.' }
& $inf2cat "/driver:$destinationPath" /os:10_CO_X64,10_NI_X64,10_GE_X64 /uselocaltime
if ($LASTEXITCODE -ne 0) { throw 'Catalog regeneration failed.' }
& $signTool sign /v /fd SHA256 /s My /sha1 $Thumbprint $cat
if ($LASTEXITCODE -ne 0) { throw 'CAT signing failed.' }
Export-Certificate -Cert $cert -FilePath (Join-Path $destinationPath 'AImerLocalTest.cer') -Force | Out-Null
# Verify the CAT signature cryptographically without modifying trusted-root stores.
Add-Type -AssemblyName System.Security
$cms = New-Object System.Security.Cryptography.Pkcs.SignedCms
$cms.Decode([IO.File]::ReadAllBytes($cat))
$cms.CheckSignature($true)
$files = @{}
foreach ($name in @('AImerVirtualMouse.inf','AImerVirtualMouse.sys','aimervirtualmouse.cat','AImerLocalTest.cer')) {
    $files[$name] = (Get-FileHash -LiteralPath (Join-Path $destinationPath $name) -Algorithm SHA256).Hash
}
@{ Thumbprint=$cert.Thumbprint; Expires=$cert.NotAfter.ToString('o'); Files=$files } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $destinationPath 'signing.json') -Encoding ascii
Write-Host "Test-signed package ready: $destinationPath"
Write-Host 'Private key remains in CurrentUser\My. System trust and boot options were not changed.'
