$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
python "$ScriptDir/run_stress.py" @args
exit $LASTEXITCODE