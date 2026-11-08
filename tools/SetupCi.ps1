param([ValidateSet('msvc', 'mingw')][string]$Compiler)
$ErrorActionPreference = 'Stop'
if (!$env:GITHUB_ACTIONS) { throw 'This setup script is intended for disposable GitHub Actions runners.' }
$sdkVersion = '1.4.363.1'
$sdkRoot = Join-Path $env:RUNNER_TEMP "VulkanSDK/$sdkVersion"
$installer = Join-Path $env:RUNNER_TEMP 'VulkanSDK-Installer.exe'
$download = "https://sdk.lunarg.com/sdk/download/$sdkVersion/windows/vulkansdk-windows-X64-$sdkVersion.exe"
Invoke-WebRequest -Uri $download -OutFile $installer
$process = Start-Process -FilePath $installer -ArgumentList @('--root', "`"$sdkRoot`"", '--accept-licenses', '--default-answer', '--confirm-command', 'install', 'copy_only=1') -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode -ne 0) { throw 'Vulkan SDK installation failed.' }
"VULKAN_SDK=$sdkRoot" | Add-Content -LiteralPath $env:GITHUB_ENV -Encoding utf8
(Join-Path $sdkRoot 'Bin') | Add-Content -LiteralPath $env:GITHUB_PATH -Encoding utf8
if ($Compiler -eq 'mingw')
{
	$compilerBin = Join-Path $env:MSYS2_LOCATION 'ucrt64/bin'
	if (!(Test-Path -LiteralPath (Join-Path $compilerBin 'g++.exe'))) { throw 'The UCRT64 GCC toolchain was not found.' }
	$compilerBin | Add-Content -LiteralPath $env:GITHUB_PATH -Encoding utf8
}
