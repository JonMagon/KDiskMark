param(
    [string]$MsysRoot = 'C:\msys64',
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$FioPath
)
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $FioPath -PathType Leaf)) { throw "fio.exe not found: $FioPath" }
$FioPath = (Resolve-Path -LiteralPath $FioPath).Path
$toolBin = Join-Path $MsysRoot 'ucrt64\bin'
$env:PATH = "$toolBin;$env:PATH"
function Run([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE)" }
}
$build = Join-Path $PSScriptRoot 'build-windows'
$dist = Join-Path $PSScriptRoot 'dist'
$distPrefix = [IO.Path]::GetFullPath($dist).TrimEnd('\') + '\'
function RemoveGeneratedPath([IO.FileSystemInfo]$Item) {
    $path = [IO.Path]::GetFullPath($Item.FullName)
    if (!$path.StartsWith($distPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a file outside the deployment directory: $path"
    }
    Remove-Item -LiteralPath $path
}
Run 'cmake' @('-S', $PSScriptRoot, '-B', $build, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release')
Run 'cmake' @('--build', $build, '--parallel', '2')
Run 'cmake' @('--install', $build, '--prefix', $dist)
$exe = Join-Path $dist 'bin\kdiskmark.exe'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'data\kdiskmark.ico') -Destination (Split-Path $exe)
Copy-Item -LiteralPath $FioPath -Destination (Split-Path $exe)
Copy-Item -LiteralPath (Join-Path (Split-Path $FioPath) 'COPYING.txt') -Destination (Join-Path $dist 'FIO-COPYING.txt')
$bin = Split-Path $exe
Run 'windeployqt6' @('--release', '--no-system-d3d-compiler', '--no-opengl-sw', '--no-translations',
    '--skip-plugin-types', 'generic,networkinformation,tls', '--exclude-plugins', 'qgif,qjpeg', $exe)
# Keep standard Qt dialogs translated for every language offered by KDiskMark.
# qt_*.qm metacatalogs duplicate qtbase here and refer to unused Qt modules.
$translationDir = Join-Path $bin 'translations'
New-Item -ItemType Directory -Path $translationDir -Force | Out-Null
$languages = @{}
Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'data\translations') -Filter 'kdiskmark_*.ts' | ForEach-Object {
    if ($_.BaseName -match '^kdiskmark_([a-z]+)(_|$)') { $languages[$Matches[1]] = $true }
}
$qtCatalogs = @{}
Get-ChildItem -LiteralPath (Join-Path $MsysRoot 'ucrt64\share\qt6\translations') -Filter 'qtbase_*.qm' | ForEach-Object {
    if ($_.BaseName -match '^qtbase_([a-z]+)(_|$)' -and $languages.ContainsKey($Matches[1])) {
        Copy-Item -LiteralPath $_.FullName -Destination $translationDir -Force
        $qtCatalogs[$_.Name] = $true
    }
}
Get-ChildItem -LiteralPath $translationDir -File | Where-Object { !$qtCatalogs.ContainsKey($_.Name) } |
    ForEach-Object { RemoveGeneratedPath $_ }
Set-Content -LiteralPath (Join-Path (Split-Path $exe) 'qt.conf') -Encoding ascii -Value "[Paths]`nPrefix=.`nPlugins=.`nTranslations=translations"
# Only these plugins are used: native Windows UI/style and embedded ICO icons.
# PNG support is built into QtGui; the application has no network or JPEG/GIF use.
$pluginPaths = @('platforms\qwindows.dll', 'styles\qmodernwindowsstyle.dll', 'imageformats\qico.dll')
$queue = New-Object 'System.Collections.Generic.Queue[string]'
$queue.Enqueue($exe)
$queue.Enqueue((Join-Path $bin 'kdiskmark.com'))
$queue.Enqueue((Join-Path $bin 'fio.exe'))
foreach ($plugin in $pluginPaths) {
    $pluginPath = Join-Path $bin $plugin
    if (!(Test-Path -LiteralPath $pluginPath)) { throw "Required plugin missing: $pluginPath" }
    $queue.Enqueue($pluginPath)
}
$seen = @{}
while ($queue.Count) {
    $binary = $queue.Dequeue()
    if ($seen.ContainsKey($binary)) { continue }
    $seen[$binary] = $true
    $imports = & objdump -p $binary | Select-String 'DLL Name:\s*(\S+)'
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect dependencies: $binary" }
    foreach ($import in $imports) {
        $name = $import.Matches[0].Groups[1].Value
        $source = Join-Path $toolBin $name
        $target = Join-Path (Split-Path $exe) $name
        if (Test-Path -LiteralPath $source) {
            if (!$seen.ContainsKey($target)) { Copy-Item -LiteralPath $source -Destination $target -Force }
            $queue.Enqueue($target)
        } elseif (Test-Path -LiteralPath $target) {
            $queue.Enqueue($target)
        } elseif ($name -notmatch '^(api-ms-|ext-ms-)' -and !(Test-Path -LiteralPath (Join-Path "$env:SystemRoot\System32" $name))) {
            throw "Unresolved dependency of ${binary}: $name"
        }
    }
}
# Remove stale optional plugins and DLLs from previous, broader deployments.
# Delete individual generated files only; live required DLLs remain untouched.
Get-ChildItem -LiteralPath $bin -Recurse -File -Filter '*.dll' | ForEach-Object {
    if (!$seen.ContainsKey($_.FullName)) { RemoveGeneratedPath $_ }
}
foreach ($unusedType in @('generic', 'networkinformation', 'tls')) {
    $unusedDirectory = Join-Path $bin $unusedType
    if ((Test-Path -LiteralPath $unusedDirectory) -and !(Get-ChildItem -LiteralPath $unusedDirectory -Force | Select-Object -First 1)) {
        RemoveGeneratedPath (Get-Item -LiteralPath $unusedDirectory)
    }
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'LICENSE') -Destination $dist
$licenses = Join-Path $MsysRoot 'ucrt64\share\licenses'
if (Test-Path -LiteralPath $licenses) {
    $licenseTarget = Join-Path $dist 'third-party-licenses'
    New-Item -ItemType Directory -Path $licenseTarget -Force | Out-Null
    Get-ChildItem -LiteralPath $licenses | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $licenseTarget -Recurse -Force
    }
    # Earlier deployments nested a second copy of the licenses directory.
    # Remove only byte-identical duplicates; preserve any unmatched notices.
    $duplicateLicenses = Join-Path $licenseTarget 'licenses'
    if (Test-Path -LiteralPath $duplicateLicenses) {
        Get-ChildItem -LiteralPath $duplicateLicenses -Recurse -File | ForEach-Object {
            $relative = $_.FullName.Substring($duplicateLicenses.Length + 1)
            $canonical = Join-Path $licenseTarget $relative
            if ((Test-Path -LiteralPath $canonical) -and (Get-FileHash -LiteralPath $canonical).Hash -eq (Get-FileHash -LiteralPath $_.FullName).Hash) {
                RemoveGeneratedPath $_
            }
        }
        $licenseDirs = @(Get-ChildItem -LiteralPath $duplicateLicenses -Recurse -Directory | Sort-Object { $_.FullName.Length } -Descending)
        $licenseDirs += Get-Item -LiteralPath $duplicateLicenses
        foreach ($directory in $licenseDirs) {
            if (!(Get-ChildItem -LiteralPath $directory.FullName -Force | Select-Object -First 1)) {
                RemoveGeneratedPath $directory
            }
        }
    }
}
Write-Host "Portable application: $exe"

$project = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'CMakeLists.txt') -Raw
if ($project -notmatch 'project\(kdiskmark\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)') {
    throw 'Cannot determine the KDiskMark version from CMakeLists.txt'
}
$archive = Join-Path $PSScriptRoot "KDiskMark-$($Matches[1])-Windows-x64.zip"
Compress-Archive -Path (Join-Path $dist '*') -DestinationPath $archive -Force
Write-Host "Portable archive: $archive"
