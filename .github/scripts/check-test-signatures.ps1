param([Parameter(Mandatory = $true)][string]$Directory)
$ErrorActionPreference = 'Stop'

# WinVerifyTrust with cache-only URL retrieval avoids network waits for a test
# certificate. Still verify the embedded signature and the executable digest.
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class OfflineAuthenticode {
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct FileInfo {
        public uint Size;
        [MarshalAs(UnmanagedType.LPWStr)] public string Path;
        public IntPtr Handle;
        public IntPtr Subject;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct TrustData {
        public uint Size;
        public IntPtr PolicyCallback;
        public IntPtr SipClient;
        public uint UiChoice;
        public uint RevocationChecks;
        public uint UnionChoice;
        public IntPtr File;
        public uint StateAction;
        public IntPtr StateData;
        public IntPtr Url;
        public uint ProviderFlags;
        public uint UiContext;
        public IntPtr SignatureSettings;
    }
    [DllImport("wintrust.dll", ExactSpelling = true)]
    private static extern uint WinVerifyTrust(IntPtr window, ref Guid action, ref TrustData data);
    public static uint Verify(string path) {
        var file = new FileInfo { Size = (uint)Marshal.SizeOf(typeof(FileInfo)), Path = path };
        IntPtr pointer = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(FileInfo)));
        Marshal.StructureToPtr(file, pointer, false);
        var action = new Guid("00AAC56B-CD44-11D0-8CC2-00C04FC295EE");
        var data = new TrustData {
            Size = (uint)Marshal.SizeOf(typeof(TrustData)),
            UiChoice = 2, UnionChoice = 1, File = pointer, StateAction = 1,
            ProviderFlags = 0x1000 | 0x10 // CACHE_ONLY_URL_RETRIEVAL | REVOCATION_CHECK_NONE
        };
        try {
            return WinVerifyTrust(new IntPtr(-1), ref action, ref data);
        } finally {
            data.StateAction = 2; // Close the verification state.
            WinVerifyTrust(new IntPtr(-1), ref action, ref data);
            Marshal.DestroyStructure(pointer, typeof(FileInfo));
            Marshal.FreeHGlobal(pointer);
        }
    }
}
'@

function ReadSigner([string]$Path) {
    # Extract the embedded certificate locally, without running a trust check.
    return [Security.Cryptography.X509Certificates.X509Certificate2]::new(
        [Security.Cryptography.X509Certificates.X509Certificate]::CreateFromSignedFile($Path))
}

$certificate = ReadSigner (Join-Path $Directory 'kdiskmark.exe')
if ($certificate.Subject -ne $certificate.Issuer) {
    throw 'Expected a self-signed SignPath test certificate'
}
Write-Host "Test signer: $($certificate.Subject), thumbprint: $($certificate.Thumbprint)"
$store = [Security.Cryptography.X509Certificates.X509Store]::new('Root', 'CurrentUser')
$added = $false
$certificatePath = Join-Path $env:RUNNER_TEMP 'signpath-test-signer.cer'
try {
    $store.Open([Security.Cryptography.X509Certificates.OpenFlags]::ReadOnly)
    $existing = $store.Certificates.Find('FindByThumbprint', $certificate.Thumbprint, $false)
    $store.Close()
    if ($existing.Count -eq 0) {
        [IO.File]::WriteAllBytes($certificatePath, $certificate.RawData)
        Write-Host 'Temporarily trusting the test certificate on the disposable runner.'
        # -f avoids an interactive root-certificate installation prompt.
        & certutil.exe -user -f -addstore Root $certificatePath
        if ($LASTEXITCODE -ne 0) { throw 'Cannot trust the test certificate' }
        $added = $true
    }
    foreach ($name in @('kdiskmark.exe', 'kdiskmark.com')) {
        $path = Join-Path $Directory $name
        Write-Host "Verifying ${name} offline..."
        $signer = ReadSigner $path
        if ($signer.Thumbprint -ne $certificate.Thumbprint) {
            throw 'EXE and COM have different signing certificates'
        }
        $status = [OfflineAuthenticode]::Verify($path)
        if ($status -ne 0) { throw ('Invalid signature on {0}: 0x{1:X8}' -f $name, $status) }
        Write-Host "${name}: valid signature and executable digest"
    }
} finally {
    $store.Close()
    if ($added) {
        & certutil.exe -user -delstore Root $certificate.Thumbprint
        if ($LASTEXITCODE -ne 0) { throw 'Cannot remove temporary certificate trust' }
    }
}
