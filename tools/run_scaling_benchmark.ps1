$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
python "$ScriptDir/run_scaling_benchmark.py" @args
exit $LASTEXITCODE
