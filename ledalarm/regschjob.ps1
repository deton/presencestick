#Write-Host $args
Get-ScheduledJob -Name LEDAlarm | Unregister-ScheduledJob
$triggers = @()
foreach ($alarmhm in $args) {
    $t = New-JobTrigger -Once -At $alarmhm
    $triggers += $t
}
if ($triggers.length -gt 0) {
    $script = $PSScriptRoot + "\led.ps1"
    Register-ScheduledJob -Name LEDAlarm -FilePath "$script" -Trigger $triggers
}
#Write-Host pause
#[Console]::ReadKey() | Out-Null
