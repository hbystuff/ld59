param([switch]$ForWeb)

$extensions = @(
    ".png"
    ".json"
    ".wav"
    ".ogg"
    ".glsl"
    ".txt"
    ".ttf"
)

$outName = "housesitting"
$outZip  = "$PSScriptRoot\$outName.zip"

if ($ForWeb -eq $true) {
  $outZip  = "$PSScriptRoot\$outName.web.zip"
  Write-Host "$outZip"
}

if (Test-Path $outZip) { Remove-Item $outZip }

$staging = "$env:TEMP\${outName}_staging"
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item $staging -ItemType Directory | Out-Null

$inner = "$staging\"
if ($ForWeb -eq $false) {
  $inner = "$staging\$outName"
  New-Item $inner -ItemType Directory | Out-Null
}

if ($ForWeb -eq $true) {
  Copy-Item "$PSScriptRoot\program.wasm" "$inner\program.wasm"
  Copy-Item "$PSScriptRoot\program.js" "$inner\program.js"
  Copy-Item "$PSScriptRoot\index.html" "$inner\index.html"
}
else {
  Copy-Item "$PSScriptRoot\program.exe" "$inner\$outName.exe"
}

$dataDest = "$inner\data"
$dataSrc  = "$PSScriptRoot\data"

Get-ChildItem $dataSrc -Recurse -File |
    Where-Object { $extensions -contains $_.Extension.ToLower() -and $_.Name -notlike '_*' -and ($_.FullName.Substring($dataSrc.Length) -notmatch '\\_[^\\]*\\') } |
    ForEach-Object {
        $rel  = $_.FullName.Substring($dataSrc.Length)
        $dest = Join-Path $dataDest $rel
        $dir  = Split-Path $dest -Parent
        if (!(Test-Path $dir)) { New-Item $dir -ItemType Directory -Force | Out-Null }
        Copy-Item $_.FullName $dest
    }

Compress-Archive -Path "$staging\*" -DestinationPath $outZip
Remove-Item $staging -Recurse -Force

Write-Host "Packaged to $outZip"
