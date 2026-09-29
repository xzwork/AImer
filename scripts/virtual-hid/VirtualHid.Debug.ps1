[CmdletBinding()]
param([ValidateSet('EnableTestMode','Install','Start')][string]$Action = 'Start')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

try {
    $admin = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
    if (-not $admin) {
        $child = Start-Process powershell.exe -Verb RunAs -WindowStyle Normal -PassThru -ArgumentList @(
            '-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$PSCommandPath+'"'),'-Action',$Action)
        $child.WaitForExit()
        exit $child.ExitCode
    }

    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class AImerTestMode {
    [StructLayout(LayoutKind.Sequential)]
    public struct Info { public UInt32 Length; public UInt32 Options; }
    [DllImport("ntdll.dll")]
    private static extern int NtQuerySystemInformation(int kind, ref Info info, int size, IntPtr returned);
    public static bool Active() {
        Info info = new Info { Length = 8 };
        int status = NtQuerySystemInformation(103, ref info, 8, IntPtr.Zero);
        if (status != 0) throw new InvalidOperationException("Cannot read code integrity options: " + status);
        return (info.Options & 2) != 0;
    }
}
'@
    # Keep Start-AImer.cmd as the only required user entry point. First-run setup
    # remains explicit because it changes boot policy and trusts a test certificate.
    if ($Action -eq 'Start' -and -not [AImerTestMode]::Active()) {
        Write-Host 'Virtual HID needs first-time setup before AImer can start.'
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Action EnableTestMode
        exit $LASTEXITCODE
    }
    if ($Action -eq 'EnableTestMode') {
        if ([AImerTestMode]::Active()) { Write-Host 'Test signing is already active.' }
        else {
            Write-Host 'This enables Windows TESTSIGNING for locally test-signed kernel drivers.'
            Write-Host 'It is a persistent boot-policy change for an offline/controlled test machine.'
            Write-Host 'A restart is required. Windows may show a Test Mode watermark.'
            if ((Read-Host 'Type YES to enable test signing, or anything else to cancel') -cne 'YES') { exit 1 }
            & bcdedit.exe /set testsigning on
            if ($LASTEXITCODE -ne 0) { throw 'Windows refused the boot-policy change. No automatic workaround is attempted.' }
            Write-Host 'Restart Windows manually, then open Start-AImer.cmd again to install the driver.'
            Write-Host 'To undo later: bcdedit /set testsigning off, then restart.'
        }
    } else {
        if (-not [AImerTestMode]::Active()) {
            throw 'Test signing is not active. Run Enable-VirtualHid-TestMode.bat, then restart Windows first.'
        }
        Set-Location -LiteralPath $PSScriptRoot
        if ($Action -eq 'Install') {
            $package = Join-Path $PSScriptRoot 'VirtualHidDriver.Debug'
            $manifest = Get-Content -LiteralPath (Join-Path $package 'signing.json') -Raw | ConvertFrom-Json
            foreach ($name in @('AImerVirtualMouse.inf','AImerVirtualMouse.sys','aimervirtualmouse.cat','AImerLocalTest.cer')) {
                if ((Get-FileHash -LiteralPath (Join-Path $package $name) -Algorithm SHA256).Hash -ne $manifest.Files.$name) {
                    throw "Package hash mismatch: $name"
                }
            }
            $certificate = [Security.Cryptography.X509Certificates.X509Certificate2]::new((Join-Path $package 'AImerLocalTest.cer'))
            if ($certificate.Thumbprint -ne $manifest.Thumbprint -or $certificate.NotAfter -le (Get-Date)) {
                throw 'Test certificate does not match the manifest or has expired.'
            }
            foreach ($name in @('AImerVirtualMouse.sys','aimervirtualmouse.cat')) {
                $signature = Get-AuthenticodeSignature -LiteralPath (Join-Path $package $name)
                if (-not $signature.SignerCertificate -or $signature.SignerCertificate.Thumbprint -ne $certificate.Thumbprint -or
                    $signature.Status -eq 'HashMismatch' -or $signature.Status -eq 'NotSigned') {
                    throw "Invalid package signature: $name"
                }
            }
            $devcon = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\Tools" -Directory |
                Where-Object { $_.Name -match '^10\.0\.[0-9]+\.[0-9]+$' } |
                Sort-Object { [version]$_.Name } -Descending |
                ForEach-Object { Join-Path $_.FullName 'x64\devcon.exe' } |
                Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
            if (-not $devcon) { throw 'WDK x64 devcon.exe is missing.' }
            Write-Host "Trusting local test code-signing certificate: $($certificate.Thumbprint)"
            foreach ($store in @('Cert:\LocalMachine\Root','Cert:\LocalMachine\TrustedPublisher')) {
                Import-Certificate -FilePath (Join-Path $package 'AImerLocalTest.cer') -CertStoreLocation $store | Out-Null
            }
            foreach ($name in @('AImerVirtualMouse.sys','aimervirtualmouse.cat')) {
                if ((Get-AuthenticodeSignature -LiteralPath (Join-Path $package $name)).Status -ne 'Valid') {
                    throw "Windows signature verification failed after certificate import: $name"
                }
            }
            & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Install-VirtualHidDriver.ps1') -PackagePath $package -DevConPath $devcon
            if ($LASTEXITCODE -ne 0) { throw 'Driver installation failed. See the output above.' }
            & (Join-Path $PSScriptRoot 'VirtualHidCheck.exe')
            if ($LASTEXITCODE -ne 0) { throw 'Device open check failed; inspect Device Manager or restart if requested.' }
            Write-Host 'Installed and device opened successfully. Use Start-AImer.cmd.'
        } else {
            & (Join-Path $PSScriptRoot 'VirtualHidCheck.exe')
            if ($LASTEXITCODE -ne 0) {
                Write-Host 'Virtual HID is unavailable. First installation trusts the local test certificate and installs the driver.'
                if ((Read-Host 'Type YES to install the test driver, or anything else to cancel') -cne 'YES') { exit 1 }
                & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Action Install
                if ($LASTEXITCODE -ne 0) { throw 'Virtual HID setup did not complete. AImer was not started.' }
            }
            $env:AIMER_INPUT_BACKEND = 'virtual_hid'
            $env:AIMER_INPUT_TRACE = '1'
            $cuda = 'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.8\bin'
            if (Test-Path -LiteralPath $cuda) { $env:PATH = "$cuda;$env:PATH" }
            & (Join-Path $PSScriptRoot 'AImer.exe')
            if ($LASTEXITCODE -ne 0) { throw "AImer exited with code $LASTEXITCODE." }
        }
    }
    [void](Read-Host 'Press Enter to close')
    exit 0
} catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    [void](Read-Host 'Press Enter to close')
    exit 1
}
