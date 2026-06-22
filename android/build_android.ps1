param(
    [ValidateSet("emulator", "device")]
    [string]$Target = "emulator",

    [ValidateSet("x86_64", "arm64-v8a", "armeabi-v7a")]
    [string]$Abi = ""
)

$ErrorActionPreference = "Stop"

$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$javaHome = if ($env:JAVA_HOME) {
    $env:JAVA_HOME
} else {
    "C:\Program Files\Android\Android Studio\jbr"
}
$androidSdk = if ($env:ANDROID_SDK_ROOT) {
    $env:ANDROID_SDK_ROOT
} elseif ($env:ANDROID_HOME) {
    $env:ANDROID_HOME
} else {
    Join-Path $env:LOCALAPPDATA "Android\Sdk"
}

if (-not (Test-Path (Join-Path $javaHome "bin\java.exe"))) {
    throw "JDK not found at '$javaHome'. Install Android Studio or set JAVA_HOME."
}

if (-not (Test-Path $androidSdk)) {
    throw "Android SDK not found at '$androidSdk'. Set ANDROID_SDK_ROOT."
}

$requiredPaths = @(
    (Join-Path $androidSdk "platforms\android-34"),
    (Join-Path $androidSdk "build-tools\34.0.0"),
    (Join-Path $androidSdk "cmake\3.22.1"),
    (Join-Path $androidSdk "ndk\25.1.8937393")
)

$missing = $requiredPaths | Where-Object { -not (Test-Path $_) }
if ($missing.Count -gt 0) {
    throw "Missing Android SDK packages:`n$($missing -join "`n")"
}

if ([string]::IsNullOrWhiteSpace($Abi)) {
    $Abi = if ($Target -eq "emulator") { "x86_64" } else { "arm64-v8a" }
}

$env:JAVA_HOME = $javaHome
$env:ANDROID_SDK_ROOT = $androidSdk
$env:Path = "$(Join-Path $javaHome 'bin');$(Join-Path $androidSdk 'platform-tools');$env:Path"

$escapedSdk = $androidSdk.Replace("\", "/").Replace(":", "\:")
Set-Content -LiteralPath (Join-Path $projectDir "local.properties") -Value "sdk.dir=$escapedSdk" -Encoding ASCII

$cxxDir = Join-Path $projectDir "app\.cxx"
if (Test-Path $cxxDir) {
    $staleCache = Get-ChildItem -LiteralPath $cxxDir -Recurse -File `
        -Include "CMakeCache.txt", "android_gradle_build.json" |
        Select-String -SimpleMatch -Pattern "D:/takumi/", "D:\takumi\" |
        Select-Object -First 1

    if ($staleCache) {
        $resolvedCxx = (Resolve-Path -LiteralPath $cxxDir).Path
        $expectedCxx = (Resolve-Path -LiteralPath (Join-Path $projectDir "app")).Path + "\.cxx"
        if ($resolvedCxx -ne $expectedCxx) {
            throw "Refusing to remove unexpected CMake cache path: $resolvedCxx"
        }
        Write-Host "Removing stale CMake cache from the original developer machine."
        Remove-Item -LiteralPath $resolvedCxx -Recurse -Force
    }
}

Push-Location $projectDir
try {
    if ($Target -eq "emulator") {
        & .\gradlew.bat :app:assembleEmulatorDebug `
            "-PmuRequiredAbis=$Abi" `
            "-PmuEmulatorAbis=$Abi" `
            -PmuFailOnMissingRequiredAbis=true `
            --console=plain
        $apk = Join-Path $projectDir "app\build\outputs\apk\emulator\debug\app-emulator-debug.apk"
    } else {
        & .\gradlew.bat :app:assembleRealDeviceDebug `
            "-PmuRequiredAbis=$Abi" `
            "-PmuRealDeviceAbis=$Abi" `
            -PmuFailOnMissingRequiredAbis=true `
            --console=plain
        $apk = Join-Path $projectDir "app\build\outputs\apk\realDevice\debug\app-realDevice-debug.apk"
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Gradle build failed with exit code $LASTEXITCODE."
    }

    $hash = Get-FileHash -LiteralPath $apk -Algorithm SHA256
    Write-Host ""
    Write-Host "APK: $apk"
    Write-Host "SHA256: $($hash.Hash)"
} finally {
    Pop-Location
}
