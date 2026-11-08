param(
	[ValidateSet('msvc-debug', 'msvc-release', 'mingw-debug', 'mingw-release')]
	[string]$Preset = 'msvc-debug'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
foreach ($program in @('YukiriPlatformSmoke', 'YukiriDemo'))
{
	$executable = Join-Path $projectRoot "out/build/$Preset/$program.exe"
	if (!(Test-Path -LiteralPath $executable)) { throw "Build $program before running GPU smoke." }
	$stdoutPath = Join-Path $projectRoot "out/$Preset-$program.stdout.log"
	$stderrPath = Join-Path $projectRoot "out/$Preset-$program.stderr.log"
	$parameters = @{
		FilePath = $executable
		WorkingDirectory = $projectRoot
		WindowStyle = 'Hidden'
		PassThru = $true
		RedirectStandardOutput = $stdoutPath
		RedirectStandardError = $stderrPath
	}
	if ($program -eq 'YukiriDemo') { $parameters.ArgumentList = '--smoke' }
	$process = Start-Process @parameters
	if (!$process.WaitForExit(60000))
	{
		Stop-Process -Id $process.Id
		throw "$program GPU smoke timed out."
	}
	$process.Refresh()
	Get-Content -LiteralPath $stdoutPath
	if ((Get-Item -LiteralPath $stderrPath).Length) { Get-Content -LiteralPath $stderrPath }
	if ($process.ExitCode -ne 0) { throw "$program GPU smoke exited with $($process.ExitCode)." }
	if (Select-String -LiteralPath $stderrPath -Pattern 'Validation Error|VUID-' -Quiet)
	{
		throw "$program reported a Vulkan validation diagnostic."
	}
}
