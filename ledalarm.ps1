# cf. http://umezy12-techmemo.blogspot.jp/2014/05/powershelloutlook.html
if($Args[0] -eq $null){#コマンドライン引数が指定が無ければ本日の予定を出力
  $date = Get-Date
}else{
  $date = $Args[0] -as [DateTime]
}

$Start = $date.AddDays(-1).ToShortDateString()#当日の10：00以前のものは前日分としてRestrictメソッドで処理されている？ようなので前日を開始時刻に設定
$Start2 = $date.AddDays(0).ToShortDateString()#あとでif文で当日分をきっちり区別するために使用
$End = $date.AddDays(1).ToShortDateString()
$Filter = "[Start] >= '$Start' AND [End] < '$End'"

$olFolderCalendar = 9
$outlook = new-object -com outlook.application;
$ns = $outlook.GetNameSpace("MAPI");
$folder = $ns.GetDefaultFolder($olFolderCalendar).Items.Restrict($Filter) 
$output = ""

$ledsb = {
  $port = new-Object System.IO.Ports.SerialPort COM3,115200,None,8,one
  $port.Open()
  $port.Write("r")
  start-sleep -m 50
  $port.Close()
}

#$credential = new-object -typename System.Management.Automation.PSCredential 

$folder | Sort-Object Start | foreach{
  $teiki_flg = 0 #定期的なアイテムを出力結果に含めるかどうかの判定フラグ
  if( ( $_.RecurrenceState -eq 1) -and ($_.Start.DayOfWeek -eq $date.DayOfWeek) ){#定期的なアイテムであれば、曜日が一致していれば出力結果に含む
      $teiki_flg = 1 
  }
  if( ($teiki_flg -eq 1) -or ( ($_.Start -gt "$Start2") -and ($_.End -lt "$End") )  ){ #定期的なアイテムでない場合は、開始時刻と終了時刻を再判定
      $start_ar = $_.Start -split " "
      $end_ar = $_.End -split " "
      $start_t = $start_ar[1] -split ":"
      $end_t = $end_ar[1] -split ":"
      $start_h = $start_t[0]
      $start_m = $start_t[1]
      $start_s = $start_t[2]
      $end_h = $end_t[0]
      $end_m = $end_t[1]
      $end_s = $end_t[2]
      $output += "--------------------`r`n"
      $output += $_.Subject,"`r`n"
      $output += $start_h+":"+$start_m+"-"+$end_h+":"+$end_m+"`r`n"
      $starthm = $start_h + ":" + $start_m
      Start-Process -Verb runas at -ArgumentList $starthm,"powershell c:\tmp\Led.ps1"
      ##$joboption = New-ScheduledJobOption -HideInTaskScheduler
      # XXX: UnauthorizedAccessToRegisterScheduledJobDefinition
      #Register-ScheduledJob -Name LEDAlarm -FilePath "c:\tmp\Led.ps1" -ArgumentList "r" -Trigger $trigger -ScheduledJobOption $joboption
      #Register-ScheduledJob -Name LEDAlarm -ScriptBlock $ledsb -Trigger $trigger
      #$cmd = {
      #    param ($starthm);
      #    $trigger = New-JobTrigger -Once -At $starthm
      #    Register-ScheduledJob -Name LEDAlarm -FilePath "c:\tmp\Led.ps1" -Trigger $trigger
      #}
      #Start-Process PowerShell -Argument "c:\Users\kihara\src\regschjob.ps1 $starthm" -Verb runas
      #Start-Process PowerShell -ArgumentList @('-noexit', '-command', "$cmd $starthm") -Verb runas
      #$sc = "-command &{Start-Process PowerShell -ArgumentList {-noexit -command "$cmd $starthm"} -Verb runas }"
      #Start-Process PowerShell -ArgumentList $sc
      $output += $_.Location,"`r`n"
    }
} 
$OutputEncoding = [console]::OutputEncoding;
$output += "--------------------"
$output
#$output | clip #出力結果をクリップボードにコピー
