$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$build = Join-Path $root 'build'
cmake -S $root -B $build -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
cmake --build $build --config Release
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed; package not produced.' }
# Unique staging directory prevents stale files from leaking into later packages.
$stage = Join-Path $root ('dist/stage-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $stage 'plugins'), (Join-Path $stage 'mods'), (Join-Path $stage 'tools') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $build 'Release/Titanfall2VR_PSVR2.dll') -Destination (Join-Path $stage 'plugins')
Copy-Item -LiteralPath (Join-Path $root 'config/Titanfall2VR_PSVR2.ini') -Destination (Join-Path $stage 'plugins')
Copy-Item -LiteralPath (Join-Path $root 'mod/Titanfall2VR_PSVR2') -Destination (Join-Path $stage 'mods') -Recurse
Copy-Item -LiteralPath (Join-Path $build 'Release/psvr2_probe.exe') -Destination (Join-Path $stage 'tools')
foreach ($name in @('README.md', 'GAME_PROFILES.md', 'THIRD_PARTY_NOTICES.md', 'Install.ps1')) {
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination $stage
}
$zip = Join-Path $root 'dist/Titanfall2VR_PSVR2-0.2.2-preview.zip'
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -Force
Get-FileHash -LiteralPath $zip | Format-List
Write-Output "Package folder: $stage"
Write-Output "ZIP: $zip"
