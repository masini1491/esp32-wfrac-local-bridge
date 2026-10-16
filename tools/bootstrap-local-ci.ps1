[CmdletBinding()]
param(
    [string]$Distribution = 'Ubuntu-22.04',
    [string]$LinuxRepoPath,
    [string]$ExpectedBranch = 'main',
    [string]$ExpectedSha,
    [string]$ImageDigest,
    [switch]$SudoDocker
)
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Core' -or $PSVersionTable.PSVersion.Major -lt 7) {
    throw 'TOOLCHAIN: PowerShell 7/Core (pwsh) required; no PS5.1 fallback'
}
if (-not $IsWindows) { throw 'Use bash tools/bootstrap-local-ci.sh on native Linux' }
$wsl = (Get-Command wsl.exe -ErrorAction Stop).Source
$distros = (& $wsl --list --quiet) -replace "`0", ''
if ($LASTEXITCODE -ne 0 -or $Distribution -notin @($distros | ForEach-Object { $_.Trim() })) {
    throw 'WSL distribution unavailable; request explicit Windows/WSL setup, never auto-install'
}
$running = (& $wsl --list --running --quiet) -replace "`0", ''
if ($LASTEXITCODE -ne 0 -or $Distribution -notin @($running | ForEach-Object { $_.Trim() })) {
    throw 'WSL distribution is not running; start it in your own terminal, then rerun preflight'
}
$versions = (& $wsl --list --verbose) -replace "`0", ''
$pattern = '^\s*\*?\s*' + [regex]::Escape($Distribution) + '\s+.+\s+2\s*$'
if ($LASTEXITCODE -ne 0 -or -not @($versions | Where-Object { $_ -match $pattern }).Count) {
    throw 'WSL2 version not established; no automatic conversion or installation'
}
if (-not $LinuxRepoPath) {
    $repo = Split-Path -Parent $PSScriptRoot
    $LinuxRepoPath = (& $wsl -d $Distribution -- wslpath -a $repo).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'WSL path translation failed' }
}
# Pass an argument vector, never interpolate paths into bash -c or use Invoke-Expression.
$arguments = @('-d', $Distribution, '--', 'bash', "$LinuxRepoPath/tools/bootstrap-local-ci.sh",
               '--repo', $LinuxRepoPath, '--expected-branch', $ExpectedBranch)
if ($ExpectedSha) { $arguments += @('--expected-sha', $ExpectedSha) }
if ($ImageDigest) { $arguments += @('--image-digest', $ImageDigest) }
if ($SudoDocker) { $arguments += '--sudo-docker' } # Separate approval required; never prompts/captures passwords.
& $wsl @arguments
exit $LASTEXITCODE
