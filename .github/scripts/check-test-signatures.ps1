param(
    [Parameter(Mandatory = $true)][string]$Directory,
    [Parameter(Mandatory = $true)][string]$Verifier
)
$ErrorActionPreference = 'Stop'

# Pin the public SignPath test certificate. It is not a production trust anchor.
$thumbprint = '28899A8F6F2493BC6E6A0349EDCAC4004104F2D4'
$certificate = [Security.Cryptography.X509Certificates.X509Certificate2]::new(
    [Security.Cryptography.X509Certificates.X509Certificate]::CreateFromSignedFile(
        (Join-Path $Directory 'kdiskmark.exe')))
if ($certificate.Thumbprint -ne $thumbprint) {
    throw "Unexpected SignPath test certificate: $($certificate.Thumbprint)"
}
$certificatePath = Join-Path $env:RUNNER_TEMP 'signpath-test-signer.pem'
Set-Content -LiteralPath $certificatePath -Encoding ascii -Value $certificate.ExportCertificatePem()
Write-Host "Test signer: $($certificate.Subject), thumbprint: $thumbprint"

foreach ($name in @('kdiskmark.exe', 'kdiskmark.com')) {
    Write-Host "Verifying ${name} with the pinned test certificate..."
    # Check the Authenticode signature, executable digest and signer certificate.
    # Test-only: skip timestamp/CRL validation and all CRL network retrieval.
    & $Verifier verify -CAfile $certificatePath -require-leaf-hash "sha1:$thumbprint" `
        -ignore-timestamp -ignore-crl -in (Join-Path $Directory $name)
    if ($LASTEXITCODE -ne 0) { throw "Invalid test signature on ${name}" }
}
