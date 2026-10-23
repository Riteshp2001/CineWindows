param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+$')]
    [string]$Version,
    [Parameter(Mandatory = $true)]
    [string]$Date,
    [switch]$Check
)

$ErrorActionPreference = 'Stop'
[datetime]::ParseExact($Date, 'yyyy-MM-dd', [cultureinfo]::InvariantCulture) | Out-Null
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$notesPath = Join-Path $repoRoot "packaging/releases/$Version.md"
$changelogPath = Join-Path $repoRoot 'CHANGELOG.md'
$notes = [IO.File]::ReadAllText($notesPath).Replace("`r`n", "`n").Trim()
if ([string]::IsNullOrWhiteSpace($notes)) {
    throw "Release notes are empty: $notesPath"
}

$original = [IO.File]::ReadAllText($changelogPath).Replace("`r`n", "`n")
$entry = "## [$Version] - $Date`n`n$notes`n`n"
$escapedVersion = [regex]::Escape($Version)
$sectionPattern = "(?ms)^## \[$escapedVersion\] - [^\n]+\n.*?(?=^## \[|^\[[^\]\r\n]+\]:|\z)"
$section = [regex]::Match($original, $sectionPattern)
if ($section.Success) {
    $updated = $original.Remove($section.Index, $section.Length).Insert($section.Index, $entry)
} else {
    $firstRelease = [regex]::Match($original, '(?m)^## \[')
    if (-not $firstRelease.Success) {
        throw 'The changelog has no release heading to prepend to.'
    }
    $updated = $original.Insert($firstRelease.Index, $entry)
}

$reference = "[$Version]: https://github.com/Riteshp2001/CineWindows/releases/tag/v$Version"
$referenceMatch = [regex]::Match($updated, "(?m)^\[$escapedVersion\]:[^\r\n]*")
if ($referenceMatch.Success) {
    $updated = $updated.Remove($referenceMatch.Index, $referenceMatch.Length).Insert($referenceMatch.Index, $reference)
} else {
    $updated = $updated.TrimEnd() + "`n$reference`n"
}

$releaseReferences = [regex]::Matches($updated, '(?m)^\[([0-9]+\.[0-9]+\.[0-9]+)\]:[^\r\n]*(?:\r?\n|$)')
for ($index = $releaseReferences.Count - 1; $index -ge 0; $index--) {
    $releaseVersion = [regex]::Escape($releaseReferences[$index].Groups[1].Value)
    if (-not [regex]::IsMatch($updated, "(?m)^## \[$releaseVersion\] - ")) {
        $referenceSpan = $releaseReferences[$index]
        $updated = $updated.Remove($referenceSpan.Index, $referenceSpan.Length)
    }
}

if ($Check) {
    if ($updated -ne $original) {
        throw "CHANGELOG.md is not synchronized with packaging/releases/$Version.md."
    }
    Write-Output "Changelog $Version is synchronized."
} else {
    [IO.File]::WriteAllText($changelogPath, $updated, [Text.UTF8Encoding]::new($false))
    Write-Output "Generated changelog entry for $Version."
}