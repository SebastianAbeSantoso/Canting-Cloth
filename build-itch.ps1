<#
.SYNOPSIS
    Builds, cooks, packages, zips, and optionally pushes CantingCloth to itch.io via butler.

.DESCRIPTION
    Uses Unreal Automation Tool (RunUAT.bat BuildCookRun) to produce a standalone Windows build
    (CantingCloth.exe + Engine/Content pak files) under Releases\Win64\ and compresses it into
    a .zip archive under Releases\. With -Push, uploads the packaged folder directly to itch.io
    using butler (enabling delta patches).

.PARAMETER Configuration
    Build configuration: Shipping (default, smallest & fastest for itch.io) or Development (includes console/debug).

.PARAMETER EngineDir
    Optional explicit path to your Unreal Engine root (e.g. "D:\UE_5.9" or "C:\Program Files\Epic Games\UE_5.8").
    If omitted, the script auto-detects from the Windows registry using the .uproject's EngineAssociation,
    falling back to any installed UE 5.x version.

.PARAMETER Version
    Version tag for the output zip and itch.io --userversion (e.g. "0.1.0"). Defaults to the current date (yyyyMMdd).

.PARAMETER Clean
    Performs a clean rebuild (-clean) and wipes previous staged/archived output first.

.PARAMETER SkipZip
    Skips creating the .zip archive under Releases\ (useful when pushing directly with butler).

.PARAMETER Push
    After building, pushes the packaged Windows folder to itch.io via butler.

.PARAMETER PushOnly
    Skips building and pushes the existing Releases\Win64\Windows folder to itch.io via butler.

.PARAMETER ItchTarget
    itch.io project slug in user/game format. Defaults to "okene/canting-cloth".

.PARAMETER Channel
    itch.io butler channel name. Defaults to "windows".

.PARAMETER ButlerPath
    Optional explicit path to butler.exe. If omitted, searches PATH and common install locations.

.EXAMPLE
    .\build-itch.ps1
    .\build-itch.ps1 -Push -Version "0.1.0"
    .\build-itch.ps1 -PushOnly -Version "0.1.0"
    .\build-itch.ps1 -Push -Configuration Development -Channel "windows-dev"
#>

[CmdletBinding()]
param(
    [ValidateSet("Shipping", "Development")]
    [string]$Configuration = "Shipping",

    [string]$EngineDir = "",

    [string]$Version = (Get-Date -Format "yyyyMMdd"),

    [switch]$Clean,

    [switch]$SkipZip,

    [switch]$Push,

    [switch]$PushOnly,

    [string]$ItchTarget = "okene/canting-cloth",

    [string]$Channel = "windows",

    [string]$ButlerPath = ""
)

$ErrorActionPreference = "Stop"

if ($PushOnly) {
    $Push = $true
}

$RepoRoot    = $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot "CantingCloth\CantingCloth.uproject"
$ReleaseRoot = Join-Path $RepoRoot "Releases"
$ArchiveDir  = Join-Path $ReleaseRoot "Win64"
$ZipName     = "CantingCloth-Win64-$Configuration-$Version.zip"
$ZipPath     = Join-Path $ReleaseRoot $ZipName
$ButlerSpec  = "${ItchTarget}:${Channel}"

if (-not (Test-Path $ProjectFile)) {
    throw "Could not find .uproject at: $ProjectFile"
}

# --- Helper: Locate butler.exe ---
function Resolve-ButlerExe {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        if (-not (Test-Path $ExplicitPath)) {
            throw "butler.exe not found at specified -ButlerPath: $ExplicitPath"
        }
        return (Resolve-Path $ExplicitPath).Path
    }

    $inPath = Get-Command "butler" -ErrorAction SilentlyContinue
    if ($inPath) {
        return $inPath.Source
    }

    $candidates = @(
        (Join-Path $env:LOCALAPPDATA "butler\butler.exe"),
        (Join-Path $env:LOCALAPPDATA "itch\butler\butler.exe"),
        "C:\butler\butler.exe",
        "D:\butler\butler.exe",
        (Join-Path $env:USERPROFILE "butler\butler.exe"),
        (Join-Path $env:USERPROFILE "scoop\shims\butler.exe"),
        (Join-Path $env:ProgramData "chocolatey\bin\butler.exe")
    )

    foreach ($c in $candidates) {
        if ($c -and (Test-Path $c)) {
            return $c
        }
    }

    $brothDir = Join-Path $env:APPDATA "itch\broth\butler"
    if (Test-Path $brothDir) {
        $found = Get-ChildItem -Path $brothDir -Recurse -Filter "butler.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }

    throw @"
Could not find butler.exe in PATH or common folders.
Either:
  1. Pass -ButlerPath "C:\path\to\butler.exe", OR
  2. Download butler for Windows from https://itch.io/docs/butler/installing.html, add it to PATH, and run: butler login
"@
}

# --- Helper: Locate Unreal Engine ---
function Resolve-EngineDir {
    param([string]$ExplicitDir, [string]$UProjectPath)

    if ($ExplicitDir) {
        if (-not (Test-Path (Join-Path $ExplicitDir "Engine\Build\BatchFiles\RunUAT.bat"))) {
            throw "RunUAT.bat not found under specified -EngineDir: $ExplicitDir"
        }
        return (Resolve-Path $ExplicitDir).Path
    }

    $uprojectJson = Get-Content $UProjectPath -Raw | ConvertFrom-Json
    $assoc = $uprojectJson.EngineAssociation

    # Try exact match in HKLM (Epic Games Launcher installs)
    if ($assoc) {
        $regPath = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$assoc"
        if (Test-Path $regPath) {
            $dir = (Get-ItemProperty $regPath).InstalledDirectory
            if ($dir -and (Test-Path (Join-Path $dir "Engine\Build\BatchFiles\RunUAT.bat"))) {
                return $dir
            }
        }

        # Try source builds in HKCU
        $cuPath = "HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds"
        if (Test-Path $cuPath) {
            $prop = (Get-ItemProperty $cuPath).PSObject.Properties | Where-Object { $_.Name -eq $assoc }
            if ($prop -and (Test-Path (Join-Path $prop.Value "Engine\Build\BatchFiles\RunUAT.bat"))) {
                return $prop.Value
            }
        }

        # Scan local drives for UE_<assoc> (e.g. F:\UE_5.8, C:\Program Files\Epic Games\UE_5.8)
        $drives = Get-PSDrive -PSProvider FileSystem | Select-Object -ExpandProperty Root
        foreach ($root in $drives) {
            $probeDirs = @(
                (Join-Path $root "UE_$assoc"),
                (Join-Path $root "Epic Games\UE_$assoc"),
                (Join-Path $root "Program Files\Epic Games\UE_$assoc")
            )
            foreach ($dir in $probeDirs) {
                if (Test-Path (Join-Path $dir "Engine\Build\BatchFiles\RunUAT.bat")) {
                    return $dir
                }
            }
        }
    }

    # Fallback: pick newest installed UE 5.x in HKLM
    $installed = Get-ItemProperty -Path "HKLM:\SOFTWARE\EpicGames\Unreal Engine\*" -ErrorAction SilentlyContinue |
        Where-Object { $_.InstalledDirectory -and (Test-Path (Join-Path $_.InstalledDirectory "Engine\Build\BatchFiles\RunUAT.bat")) } |
        Sort-Object PSChildName -Descending

    if ($installed) {
        $chosen = $installed | Select-Object -First 1
        Write-Warning "Project EngineAssociation is '$assoc', which was not found locally. Using installed UE $($chosen.PSChildName) at '$($chosen.InstalledDirectory)'."
        Write-Warning "If compilation fails due to engine version differences, pass -EngineDir or install UE $assoc."
        return $chosen.InstalledDirectory
    }

    throw "Could not auto-detect an Unreal Engine installation. Pass -EngineDir `"C:\Path\To\UE_5.x`"."
}

# Fail fast if -Push was requested but butler is missing
$ResolvedButler = $null
if ($Push) {
    $ResolvedButler = Resolve-ButlerExe -ExplicitPath $ButlerPath
}

if (-not $PushOnly) {
    $ResolvedEngineDir = Resolve-EngineDir -ExplicitDir $EngineDir -UProjectPath $ProjectFile
    $RunUAT = Join-Path $ResolvedEngineDir "Engine\Build\BatchFiles\RunUAT.bat"
}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host " CantingCloth -> itch.io Windows Build$(if ($Push) { ' & Push' })" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host " Project       : $ProjectFile"
if (-not $PushOnly) {
    Write-Host " Engine        : $ResolvedEngineDir"
    Write-Host " Configuration : $Configuration"
}
Write-Host " Version       : $Version"
Write-Host " Output Folder : $ArchiveDir"
if (-not $SkipZip -and -not $PushOnly) {
    Write-Host " Output Zip    : $ZipPath"
}
if ($Push) {
    Write-Host " Butler        : $ResolvedButler"
    Write-Host " itch.io Target: $ButlerSpec"
}
Write-Host "============================================================" -ForegroundColor Cyan

# --- 1. Build & Package (unless -PushOnly) ---
if (-not $PushOnly) {
    if ($Clean -and (Test-Path $ArchiveDir)) {
        Write-Host "[1/4] Cleaning previous archive directory..." -ForegroundColor Yellow
        Remove-Item -Path $ArchiveDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $ArchiveDir -Force | Out-Null

    $UatArgs = @(
        "BuildCookRun",
        "-project=`"$ProjectFile`"",
        "-noP4",
        "-platform=Win64",
        "-clientconfig=$Configuration",
        "-serverconfig=$Configuration",
        "-cook",
        "-map=/Game/Maps/L_Workshop",
        "-build",
        "-stage",
        "-pak",
        "-package",
        "-archive",
        "-archivedirectory=`"$ArchiveDir`"",
        "-prereqs",
        "-unattended",
        "-utf8output",
        "-ubtargs=`"-NoUBA -MaxParallelActions=2`""
    )

    if ($Configuration -eq "Shipping") {
        $UatArgs += "-nodebuginfo"
    }

    if ($Clean) {
        $UatArgs += "-clean"
    }

    Write-Host "[2/4] Running BuildCookRun (this may take several minutes on the first run)..." -ForegroundColor Yellow
    & $RunUAT @UatArgs
    if ($LASTEXITCODE -ne 0) {
        throw "BuildCookRun failed with exit code $LASTEXITCODE."
    }
}

# Find the packaged Windows folder (UE 5.9 archives directly into $ArchiveDir; UE 5.0-5.8 uses \Windows)
if (Test-Path (Join-Path $ArchiveDir "CantingCloth.exe")) {
    $PackagedDir = $ArchiveDir
}
elseif (Test-Path (Join-Path $ArchiveDir "Windows")) {
    $PackagedDir = Join-Path $ArchiveDir "Windows"
}
elseif (Test-Path (Join-Path $ArchiveDir "WindowsNoEditor")) {
    $PackagedDir = Join-Path $ArchiveDir "WindowsNoEditor"
}
else {
    throw "Expected packaged build (CantingCloth.exe or Windows\ folder) not found in $ArchiveDir. Run without -PushOnly first."
}

$ExePath = Join-Path $PackagedDir "CantingCloth.exe"
if (-not (Test-Path $ExePath)) {
    Write-Warning "CantingCloth.exe was not found directly at $ExePath. Check contents of $PackagedDir."
}

# --- 2. Zip for manual upload / backup ---
if (-not $SkipZip -and -not $PushOnly) {
    Write-Host "[3/4] Creating zip archive..." -ForegroundColor Yellow
    if (Test-Path $ZipPath) {
        Remove-Item -Path $ZipPath -Force
    }

    # Prefer tar.exe (built into Windows 10/11) because Compress-Archive has a 2 GB limit
    $TarExe = Get-Command "tar.exe" -ErrorAction SilentlyContinue
    if ($TarExe) {
        Push-Location $PackagedDir
        try {
            & tar.exe -a -cf $ZipPath *
            if ($LASTEXITCODE -ne 0) {
                throw "tar.exe failed to create zip (exit code $LASTEXITCODE)."
            }
        }
        finally {
            Pop-Location
        }
    }
    else {
        Compress-Archive -Path (Join-Path $PackagedDir "*") -DestinationPath $ZipPath -Force
    }

    $ZipSizeMB = [math]::Round((Get-Item $ZipPath).Length / 1MB, 1)
    Write-Host "  Zip created: $ZipPath ($ZipSizeMB MB)" -ForegroundColor Gray
}

# --- 3. Push to itch.io via butler ---
if ($Push) {
    Write-Host "[4/4] Pushing $PackagedDir to $ButlerSpec (version $Version)..." -ForegroundColor Yellow
    # Push the directory directly (not the zip) so butler can generate fast delta patches
    & $ResolvedButler push $PackagedDir $ButlerSpec --userversion $Version
    if ($LASTEXITCODE -ne 0) {
        throw "butler push failed with exit code $LASTEXITCODE. If you haven't logged in yet, run: `"$ResolvedButler`" login"
    }
}

Write-Host ""
Write-Host "ALL DONE!" -ForegroundColor Green
Write-Host "  Executable : $ExePath" -ForegroundColor Green
if (-not $SkipZip -and -not $PushOnly -and (Test-Path $ZipPath)) {
    Write-Host "  Zip Archive: $ZipPath" -ForegroundColor Green
}
if ($Push) {
    Write-Host "  itch.io    : https://okene.itch.io/canting-cloth ($ButlerSpec, v$Version)" -ForegroundColor Green
}
