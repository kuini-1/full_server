# Mirror Windows workspace to WSL .vs folder, configure & build in WSL
param(
  [string]$Src = "E:\SERVER\full_server",
  [string]$Remote = "\\wsl.localhost\Ubuntu\home\mashu\.vs\full_server",
  [string]$Preset = "wsl-debug",
  [int]$Jobs = 6
)

Write-Output "Mirroring $Src -> $Remote (excluding build artifacts)..."
robocopy $Src $Remote /MIR /XD ".git" ".vs" "out" "build" "build_win" /XF "*.obj" "*.pdb" "*.tlog" /R:2 /W:2 | Out-Null

Write-Output "Running CMake configure inside WSL (preset: $Preset)..."
Push-Location $env:TEMP
try {
  wsl -d Ubuntu -e bash -c "cd ~/.vs/full_server && rm -rf out/build/$Preset && cmake --preset $Preset" 2>&1 | Write-Output
} finally {
  Pop-Location
}

Write-Output "`nBuilding with Ninja (jobs: $Jobs)..."
Push-Location $env:TEMP
try {
  wsl -d Ubuntu -e bash -c "cd ~/.vs/full_server && ninja -C out/build/$Preset -j$Jobs" 2>&1 | Write-Output
} finally {
  Pop-Location
}

Write-Output "`nBuild complete!"