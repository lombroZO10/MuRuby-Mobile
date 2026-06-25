param(
    [string]$Source = "C:\Users\tita_\Downloads\assets\interface",
    [string]$Destination = "C:\Users\tita_\Downloads\MuMobile+Takumi\ClientBuild_192.168.99.200\Data\Interface"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $Source)) {
    throw "Source path not found: $Source"
}

if (-not (Test-Path -LiteralPath $Destination)) {
    throw "Destination path not found: $Destination"
}

function Get-RelativePath {
    param(
        [string]$BasePath,
        [string]$FullPath
    )

    return $FullPath.Substring($BasePath.Length).TrimStart('\')
}

function Get-NormalizedKey {
    param([string]$RelativePath)

    return ($RelativePath -replace '\\', '/').ToLowerInvariant()
}

function Get-CanonicalRelativePath {
    param([string]$RelativePath)

    $explicitMap = @{
        "ingameshop/ingame_bt02.ozt" = "InGameShop\Ingame_Bt02.OZT"
        "ingameshop/ingame_shopbackct.ozj" = "InGameShop\Ingame_ShopbackCT.OZJ"
        "ingameshop/ingame_shopbackct1.ozj" = "InGameShop\Ingame_ShopbackCT1.OZJ"
        "macroui/macrobar.ozj" = "MacroUI\MacroBar.OZJ"
        "macroui/macromain.ozt" = "MacroUI\MacroMain.OZT"
        "macroui/macrosetup.ozt" = "MacroUI\MacroSetup.OZT"
        "macroui/macrostart.ozt" = "MacroUI\MacroStart.OZT"
        "macroui/macrostop.ozt" = "MacroUI\MacroStop.OZT"
        "macroui/macroui_camera.ozt" = "MacroUI\MacroUI_Camera.OZT"
        "macroui/macroui_clearpk.ozt" = "MacroUI\MacroUI_ClearPK.OZT"
        "macroui/macroui_menunew.ozt" = "MacroUI\MacroUI_MenuNew.OZT"
        "macroui/macroui_ranking.ozt" = "MacroUI\MacroUI_Ranking.OZT"
        "macroui/offatk.ozt" = "MacroUI\OffAtk.OZT"
    }

    $normalizedRelativePath = Get-NormalizedKey -RelativePath $RelativePath
    if ($explicitMap.ContainsKey($normalizedRelativePath)) {
        return $explicitMap[$normalizedRelativePath]
    }

    $segments = $RelativePath -split '[\\/]'
    if ($segments.Length -gt 0) {
        switch ($segments[0].ToLowerInvariant()) {
            "ingameshop" { $segments[0] = "InGameShop" }
            "macroui" { $segments[0] = "MacroUI" }
            "partcharge1" { $segments[0] = "partCharge1" }
        }
    }

    return ($segments -join '\')
}

$destinationIndex = @{}
Get-ChildItem -LiteralPath $Destination -Recurse -File | ForEach-Object {
    $relativePath = Get-RelativePath -BasePath $Destination -FullPath $_.FullName
    $destinationIndex[(Get-NormalizedKey -RelativePath $relativePath)] = $relativePath
}

$copied = 0
$replaced = 0
$added = 0

Get-ChildItem -LiteralPath $Source -Recurse -File | ForEach-Object {
    $sourceFile = $_.FullName
    $sourceRelativePath = Get-RelativePath -BasePath $Source -FullPath $sourceFile
    $normalizedKey = Get-NormalizedKey -RelativePath $sourceRelativePath

    if ($destinationIndex.ContainsKey($normalizedKey)) {
        $targetRelativePath = $destinationIndex[$normalizedKey]
        $replaced++
    }
    else {
        $targetRelativePath = Get-CanonicalRelativePath -RelativePath $sourceRelativePath
        $destinationIndex[$normalizedKey] = $targetRelativePath
        $added++
    }

    $targetPath = Join-Path $Destination $targetRelativePath
    $targetDir = Split-Path -Parent $targetPath
    if (-not (Test-Path -LiteralPath $targetDir)) {
        New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    }

    Copy-Item -LiteralPath $sourceFile -Destination $targetPath -Force
    $copied++
}

Write-Host "Interface sync complete."
Write-Host "  Source      : $Source"
Write-Host "  Destination : $Destination"
Write-Host "  Copied      : $copied"
Write-Host "  Replaced    : $replaced"
Write-Host "  Added       : $added"
