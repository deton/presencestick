# PresenceStick

自分のプレゼンス情報を周囲に伝えるためのUSB接続LEDです。

会議/授業/オフィスにおいて、
自分の状態/気分を雰囲気として周りの人に伝えるために使うことを想定
(Augmented Body Language)。

例えば、次の予定があるので、
会議に参加できる残り時間が限られていることをそれとなく伝えたい場合向け。
(話に割り込むほどでないが、それとなく雰囲気として伝えたい場合)

![PresenceStick写真](https://github.com/deton/presencestick/raw/master/PresenceStick.jpg)

## 用途
+ 会議に参加できる残り時間が限られていることを、それとなく周りに伝える。
  (複雑な話を始める時間があるか把握できるように。)
+ 説明がわからなかった/行き詰まり気味なので相談したい気分/困っている等を周りに伝える。
+ 発言に対する拍手やうなずきによる同意。へぇーボタン。いいねボタン。

もちろん、メールが届いた等、PCの状態を自分が知る用途にも使用可能です
(blink(1)同様)。

## 機能
* カラータイマー。15分間のカラータイマー。
  Outlookに入っている次の予定開始前の通知を行う目的。
 * 最初の10分間は黄色点滅。点滅間隔が2秒間隔から短くなっていく。
 * 最後の5分間は赤色点滅(黄色と赤色を交互に点滅)。
   点滅間隔が2秒間隔から短くなっていく。
 * 15分経過後は、赤点灯。リマインドのため、4秒おきに赤点滅。
* 困ってる表示。虹色に色をなめらかに変化させて表示。
* 拍手。青色点滅を3秒間。
* 指定した色や点滅間隔での表示。

いずれの機能も、COMポートへの文字列書き込みで指定。

## 入力: タクトスイッチ2つ
* 困ってる表示開始。(元は拍手用スイッチにしていたが、拍手はあまり使わないので。)
* LED表示オフ。困ってる表示終了、カラータイマー終了。

## Outlook連携
Outlookの今日の予定表を取得して、
予定開始時刻の15分前にカラータイマーを開始するよう、
ScheduledJobを登録するPowerShellスクリプトです。

* ledalarm.ps1: 今日の予定表を取得し、各予定に対してregschjobを呼び出す。
  (一般ユーザで実行する必要あり)
* regschjob.ps1: ScheduledJobを登録する。
  (ScheduledJob登録には管理者権限が必要なのでledalarm.ps1とは別スクリプト。
  確認ダイアログが出る)
* led.ps1: ScheduledJobにより指定時刻に実行される。
  USB接続したArduino Pro Microに対しCOMポート経由でカラータイマー開始を指示。

ledalarm.ps1を毎朝1回実行して使います。

ScheduledJobを使っているので、PowerShell 3.0以降が必要です。
(Windows 7だと標準で入っているのはPowerShell 2.0なので、別途インストール要)

ScheduledJob登録時に、
DirectoryNotFoundWhenRegisteringScheduledJobDefinitionエラーが出た時は、
Windowsのタスクスケジューラに、
\Microsoft\Windows\PowerShell\ScheduledJobs
を追加してください
([参考](http://technet.microsoft.com/en-us/library/hh849755.aspx))。
また、何らかの形でおかしくなった場合は、PowerShellで
Get-ScheduledJobを実行すると修復してくれることがあるようです。

## その他機能: 画面ロック回避
LED表示とは関係ないですが、9分おきにマウス操作をPCに送り付ける機能も入れています。
(Pro Microはキーボード/マウスデバイスとしても見えます。)

ポリシー上10分でロックをかけることになっているが、
別PCを使っている間にロックがかかると、
毎日何回もロック解除のためのパスワード入力をしていて無駄なので、
自席にいる間はロックしないようにするための機能です。

人感センサ等は付けていないので、席を離れる時はUSBから抜く運用で使っています。
(カラータイマーの開始時刻に抜かれていると、カラータイマーが開始されない問題あり)

## 部品
+ [Arduino Pro Micro 5V](http://www.switch-science.com/catalog/1623/)
+ [NeoPixelフルカラーLED](http://www.switch-science.com/catalog/1398/)、
  [Arduino用ライブラリ](https://github.com/adafruit/Adafruit_NeoPixel)
+ [プラケース [F52X22X13B]](http://www.aitendo.com/product/5186)
+ タクトスイッチ(2本足) 2個
+ (USB microケーブル。あきばお～で購入)

プラケースはPro Microにちょうどいいサイズなのですが、
ピンヘッダを付けると蓋が閉まらなくなるので、
タクトスイッチ等は直接はんだ付けしてます。

LEDがまぶしかったので、紙をかぶせました。
いい感じに拡散されて見やすくなりました(あんどん風)。

![内部写真](https://github.com/deton/presencestick/raw/master/PresenceStickInside.jpg)
(単3電池はサイズ比較用)

## 拡張案
* 今の気分を示す。Don't disturb等
 * 自席で作業中に邪魔されたくない時。
   集中作業中。集中力が切れかけなので割り込み歓迎、等
* 離席中、机上PCのLEDに状態表示。
  すぐに戻るのか、近くの机での会議か、離れた部屋のでの会議か、
  どの程度で戻るかを表示。
* 全体的な雰囲気表示。場の空気の見える化
 * 「お静かに」等をWeb経由で集計して結果LED表示。
   個人PC/スマホでLED表示すると、誰がそう感じてるか周りに知られるので、
   全体的な雰囲気だけを伝えたい場合。
   (角が立たないように。気分を表明しやすいように)
* 集計して、よくわからなかった人が多かったら赤。
* 雷情報、強風情報、安全衛生見回り中、高温(エアコンの温度設定)、騒音、
  電話が小さい音で鳴り続けてて遠くの人が気付いてない時

## 関連
* [自動車用しっぽ: サンクステイル](http://www.itmedia.co.jp/lifestyle/articles/0412/15/news033.html)、Drivemocion、LEDメッセンジャー
* [necomimi](http://neurowear.com/projects_detail/necomimi.html)
* PCにUSB接続するLED: [blink(1)](http://gigazine.net/news/20140804-blink1-mk2/)、[BlinkStick](http://www.blinkstick.com/)
* iPhoneイヤホンジャックに刺すLED: [AYATORI](http://ayatori-dev.vinclu.me/)、[myLED](http://gigazine.net/news/20121225-myled/)
* スマホからWiFi-ZigBeeで制御できる照明: [Philips hue](http://trendy.nikkeibp.co.jp/article/column/20140211/1055136/)

最初は、blink(1)が欲しかったのですが、その時は在庫無しで買えませんでした。
回路図はgithubにあるので自作可能ですが面倒になったのでArduino Pro Microで作成。

ちなみに、BlinkStickやblink(1)(mk2でない方)の回路図を見ると、
[Gainer互換pepper](http://morecatlab.akiba.coocan.jp/morecat_lab/Pepper.html)
の回路図とよく似ているので、
pepperの作成例が参考になりそうです。
(もう1サイズ小さいプラケース[E44.5X18.5X9.6B](http://www.aitendo.com/product/5185)に収められるかも。)

## 蛇足
USBケーブルとして、硬くて好きに曲げて形を付けて使うケーブルも使ってみましたが、
接続したまま曲げようとした際に力のかけ方がまずくて、
Pro Microのmicro USBコネクタをはがして1台駄目にしました。
硬いケーブルを使うのはやめました。
