param(
	[ValidateSet('msvc-debug', 'msvc-release', 'mingw-debug', 'mingw-release')]
	[string]$Preset = 'msvc-debug',
	[switch]$Source,
	[switch]$BuildOnly
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$previousSdk = $env:VULKAN_SDK
Push-Location -LiteralPath $projectRoot
try
{
	if ($Preset.StartsWith('msvc'))
	{
		$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
		$installationPath = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
		$developerCommand = Join-Path $installationPath 'Common7\Tools\VsDevCmd.bat'
		$environmentLines = & $env:ComSpec /d /c "call `"$developerCommand`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
		if ($LASTEXITCODE -ne 0) { throw 'Loading MSVC failed.' }
		foreach ($environmentLine in $environmentLines)
		{
			if ($environmentLine -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
		}
		$env:VSLANG = '1033'
		$compiler = 'cl'
	}
	else
	{
		$compiler = 'g++'
		$compilerDirectory = Split-Path -Parent (Get-Command g++).Source
		$env:PATH = "$compilerDirectory;$env:PATH"
	}
	$configuration = if ($Preset.EndsWith('debug')) { 'Debug' } else { 'Release' }
	$mode = if ($Source) { 'source' } else { 'installed' }
	$consumerBuild = "out/consumer/$Preset-$mode"
	$arguments = @('-S', 'tests/Consumer', '-B', $consumerBuild, '-G', 'Ninja', "-DCMAKE_CXX_COMPILER=$compiler", "-DCMAKE_BUILD_TYPE=$configuration")
	if ($Source)
	{
		$arguments += "-DYUKIRI_SOURCE_DIR=$projectRoot"
		$arguments += if ($Preset.StartsWith('msvc')) { '-DCMAKE_C_COMPILER=cl' } else { '-DCMAKE_C_COMPILER=gcc' }
	}
	else
	{
		$installed = Join-Path $projectRoot "out/install/$Preset"
		if (!(Test-Path -LiteralPath $installed)) { throw 'Install the package before verifying it.' }
		$relocated = Join-Path $projectRoot "out/relocated/$Preset-$([Guid]::NewGuid().ToString('N'))"
		Copy-Item -LiteralPath $installed -Destination $relocated -Recurse
		$arguments += @("-DCMAKE_PREFIX_PATH=$relocated", "-DYukiri_DIR=$relocated/lib/cmake/Yukiri", '-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=TRUE')
		$env:VULKAN_SDK = ''
	}
	& cmake @arguments
	if ($LASTEXITCODE -ne 0) { throw 'Consumer configuration failed.' }
	& cmake --build $consumerBuild --parallel 4
	if ($LASTEXITCODE -ne 0) { throw 'Consumer build failed.' }
	if (!$BuildOnly)
	{
		& (Join-Path $projectRoot "$consumerBuild/YukiriConsumer.exe")
		if ($LASTEXITCODE -ne 0) { throw 'Consumer execution failed.' }
	}
	$checks = if ($BuildOnly) { 'configure/link' } else { 'configure/link/run' }
	Write-Output "$Preset $mode consumption passed ($checks)."
}
finally
{
	$env:VULKAN_SDK = $previousSdk
	Pop-Location
}
