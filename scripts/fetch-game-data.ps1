$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$catalog = Get-Content -LiteralPath (Join-Path $root 'data/weapon-facts.json') -Raw | ConvertFrom-Json
$dest = Join-Path $root 'research/game'
New-Item -ItemType Directory -Path $dest -Force | Out-Null
foreach ($weapon in $catalog.weapons) {
    $url = 'https://raw.githubusercontent.com/Syampuuh/Titanfall2/' + $catalog.sourceCommit + '/scripts/weapons/' + $weapon.id + '.txt'
    $target = Join-Path $dest ($weapon.id + '.txt')
    Invoke-WebRequest -Uri $url -OutFile $target
    if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant() -ne $weapon.sha256) {
        throw "Source hash mismatch for $($weapon.id)"
    }
}
Invoke-WebRequest -Uri ('https://raw.githubusercontent.com/Syampuuh/Titanfall2/' + $catalog.sourceCommit + '/resource/r1_english.txt') -OutFile (Join-Path $dest 'r1_english.txt')
Write-Output 'Pinned weapon definitions downloaded and hashes verified.'
