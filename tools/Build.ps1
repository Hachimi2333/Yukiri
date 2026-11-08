param(
	[ValidateSet('msvc-debug', 'msvc-release', 'mingw-debug', 'mingw-release')]
	[string]$Preset = 'msvc-debug',
	[string]$Target = '',
	[int]$Jobs = 4,
	[switch]$Test,
	[switch]$Install
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location -LiteralPath $projectRoot
try
{
	if ($Preset.StartsWith('msvc'))
	{
		$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
		if (!(Test-Path -LiteralPath $vswherePath)) { throw 'Visual Studio Installer / vswhere was not found.' }
		$installationPath = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
		if (!$installationPath) { throw 'An x64 MSVC toolchain was not found.' }
		$developerCommand = Join-Path $installationPath 'Common7\Tools\VsDevCmd.bat'
		$developerEnvironmentCommand = "call `"$developerCommand`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
		$environmentLines = & $env:ComSpec /d /c $developerEnvironmentCommand
		if ($LASTEXITCODE -ne 0) { throw 'Loading the MSVC developer environment failed.' }
		foreach ($environmentLine in $environmentLines)
		{
			if ($environmentLine -match '^([^=]+)=(.*)$')
			{
				[Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
			}
		}
	}
	& cmake --preset $Preset
	if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
	$buildArguments = @('--build', '--preset', $Preset, '--parallel', $Jobs)
	if ($Target) { $buildArguments += @('--target', $Target) }
	& cmake @buildArguments
	if ($LASTEXITCODE -ne 0) { throw 'The build failed.' }
	if ($Test)
	{
		& ctest --preset $Preset
		if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
	}
	if ($Install)
	{
		& cmake --install "out/build/$Preset"
		if ($LASTEXITCODE -ne 0) { throw 'Installation failed.' }
	}
}
finally
{
	Pop-Location
}
