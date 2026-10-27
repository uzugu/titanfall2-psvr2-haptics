param(
    [string]$GameDirectory = 'C:\Program Files (x86)\Steam\steamapps\common\Titanfall2',
    [switch]$LoggingOnly
)
$ErrorActionPreference = 'Stop'
$game = (Resolve-Path -LiteralPath $GameDirectory).Path
$profile = Join-Path $game 'TF2VR'
if (!(Test-Path -LiteralPath (Join-Path $profile 'Northstar.dll'))) { throw 'Install CircuitLord Titanfall 2 VR first.' }
if (!(Test-Path -LiteralPath (Join-Path $PSScriptRoot 'plugins/Titanfall2VR_PSVR2.dll'))) {
    throw 'Run this script from the extracted release package, not the source folder.'
}
$running = Get-Process Titanfall2,Titanfall2VRLauncher -ErrorAction SilentlyContinue
if ($running) { throw 'Close Titanfall 2 before installing, then run this script again.' }
$plugin = Join-Path $profile 'plugins/Titanfall2VR_PSVR2.dll'
$ini = Join-Path $profile 'plugins/Titanfall2VR_PSVR2.ini'
$mod = Join-Path $profile 'mods/Titanfall2VR_PSVR2'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
if ((Test-Path -LiteralPath $plugin) -or (Test-Path -LiteralPath $mod) -or (Test-Path -LiteralPath $ini)) {
    $backup = Join-Path $profile ('psvr2-backups/' + $stamp)
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    foreach ($item in @($plugin, $ini, $mod)) {
        if (Test-Path -LiteralPath $item) { Copy-Item -LiteralPath $item -Destination $backup -Recurse }
    }
}
New-Item -ItemType Directory -Path (Join-Path $profile 'plugins'), $mod -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'plugins/Titanfall2VR_PSVR2.dll') -Destination $plugin -Force
if (!(Test-Path -LiteralPath $ini)) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'plugins/Titanfall2VR_PSVR2.ini') -Destination $ini
}
$sourceMod = Join-Path $PSScriptRoot 'mods/Titanfall2VR_PSVR2'
Copy-Item -Path (Join-Path $sourceMod '*') -Destination $mod -Recurse -Force
if ($LoggingOnly) {
    $text = Get-Content -LiteralPath $ini -Raw
    $text = $text -replace '(?m)^LoggingOnly\s*=.*$', 'LoggingOnly=1'
    Set-Content -LiteralPath $ini -Value $text -Encoding ascii
}
Write-Output "Installed preview into $profile. Launch through CircuitLord's installer."
Write-Output "Config: $ini"
Write-Output "Log: $(Join-Path $profile 'plugins/Titanfall2VR_PSVR2.log')"
