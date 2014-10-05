# cf. http://umezy12-techmemo.blogspot.jp/2014/05/powershelloutlook.html
if($Args[0] -eq $null){#�R�}���h���C���������w�肪������Ζ{���̗\����o��
  $date = Get-Date
  $enableAlarm = $TRUE
}else{
  $date = $Args[0] -as [DateTime]
  $enableAlarm = $FALSE
}

$Start = $date.AddDays(-1).ToShortDateString()#������10�F00�ȑO�̂��̂͑O�����Ƃ���Restrict���\�b�h�ŏ�������Ă���H�悤�Ȃ̂őO�����J�n�����ɐݒ�
$Start2 = $date.AddDays(0).ToShortDateString()#���Ƃ�if���œ����������������ʂ��邽�߂Ɏg�p
$End = $date.AddDays(1).ToShortDateString()
$Filter = "[Start] >= '$Start' AND [End] < '$End'"

$olFolderCalendar = 9
$outlook = new-object -com outlook.application;
$ns = $outlook.GetNameSpace("MAPI");
$folder = $ns.GetDefaultFolder($olFolderCalendar).Items.Restrict($Filter) 
$output = ""

$staticAlarmHms = @("12:05", "17:25") # �Œ�̋x�ݎ���
$alarmhms = $staticAlarmHms
$folder | Sort-Object Start | foreach{
  $teiki_flg = 0 #����I�ȃA�C�e�����o�͌��ʂɊ܂߂邩�ǂ����̔���t���O
  if( ( $_.RecurrenceState -eq 1) -and ($_.Start.DayOfWeek -eq $date.DayOfWeek) ){#����I�ȃA�C�e���ł���΁A�j������v���Ă���Ώo�͌��ʂɊ܂�
      $teiki_flg = 1 
  }
  if( ($teiki_flg -eq 1) -or ( ($_.Start -gt "$Start2") -and ($_.End -lt "$End") )  ){ #����I�ȃA�C�e���łȂ��ꍇ�́A�J�n�����ƏI���������Ĕ���
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
      $alarm_h = $start_h
      $alarm_m = $start_m - 15
      if ($alarm_m -lt 0) {
          $alarm_m = 60 + $alarm_m
          $alarm_h = $start_h - 1
      }
      $alarmhm = [string] $alarm_h + ":" + $alarm_m
      $alarmhms += $alarmhm
      $output += $_.Location,"`r`n"
    }
} 
$OutputEncoding = [console]::OutputEncoding;
$output += "--------------------"
$output
#$output | clip #�o�͌��ʂ��N���b�v�{�[�h�ɃR�s�[

# �A���[���o�^
if ($enableAlarm) {
  #Start-Process -Verb runas at -ArgumentList $alarmhm,"powershell c:\tmp\Led.ps1"
  $arg = $PSScriptRoot + "\regschjob.ps1 $alarmhms"
  Start-Process PowerShell -Argument $arg -Verb runas
# XXX: UnauthorizedAccessToRegisterScheduledJobDefinition
  #Register-ScheduledJob -Name LEDAlarm -FilePath "c:\tmp\Led.ps1" -ArgumentList "e" -Trigger $trigger -ScheduledJobOption $joboption
}
