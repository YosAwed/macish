--------------------
MacOS XCode用にソースコードをパッチ（2020/10/29) By Awed

Vectorにあった、UNIX版ishをXCode用に手直ししたものです。そのままコンパイルしてもいいですが、コンパイル済みのバイナリもサブディレクトリの
中に置いてあるので、手っ取り早く使いたい人は、そこからコピーして使ってください。%> ish -s7 -ma xxxx.xxxで、ishファイルが生成されます。

Mac用のishは、すごく古いものしか存在せず、今のMacOSでは動かなかったので、ソースからポーティングしてみました。

--------------- Original readme.txt
*** ＩＳＨマルチボリューム対応版

このＩＳＨは、taka@exsys.prm.prug.or.jp.or.jpが、ish201に
マルチボリューム対応を追加したものです。
ただし、restore（復元）のみしか対応していません。

その後、aka@redcat.niiza.saitama.jpさんがencodeを追加して下さいました。
そのバージョンが、ish201a3となっておりましたので、バグを修正し、
ish201a4として、リリースします。

なお、akaさんのメールアドレスは上記に変更になってます。

その他、詳しくは、他のdocファイルを参照してください。


*** おもな変更点。
１、マルチボリュームの復元が可能。
２、復元時に複数のファイル指定が可能。
	ex.	ish -r file1 file2 file3
３、コンパイル時に -DNAMELOWERを指定すると、出力ファイル名を小文字に変換。
４、復元時にファイル名を指定しない場合、標準入力より読み込む。
	ex.	cat file | ish -r
５、MS-DOSでのdisp#は、dispishです。中間ファイルはMS-DOSと同一です。

*** 動作確認機種
SunOS 4.1.3
NEWS-OS 4.0C
SCO-UNIX386 (Version?)
SCO-XENIX386 (Version?)


by 小路　孝司　( taka@exsys.prm.prug.or.jp)

