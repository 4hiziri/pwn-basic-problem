# pwn バッファオーバフローを使った攻撃
x86を想定する。
他のアーキテクチャだと話が変わってくるかもしれない。
pwnでもっとも基本的な攻撃だと思われるバッファオーバーフローについてのメモ。

## 必要な知識
バッファオーバーフローがどのようにして引き起され、どのようにして攻撃に利用するかを理解するために必要な知識。

### 関数呼び出しによるスタックの変化
関数呼び出しが行なわれると様々なお決まりの処理が行なわれる。
その中でも値がどのようにスタックに積まれるかは、様々なexploitで使える知識である。
スタックは下位の方向に積まれることに注意。
例えば、push命令が実行されるとespの値は減る、スタックの位置を戻すときはespに加算されることになる。

参考までに関数呼び出しされた直後に実行される命令は以下のような感じになる。
この例ではcanaryはなし。
```nasm
 80484c4:	e8 92 ff ff ff       	call   804845b <vuln> ; 1

0804845b <vuln>:
 804845b:	55                   	push   ebp ; 2
 804845c:	89 e5                	mov    ebp,esp ; set current stack top to ebp.
 804845e:	81 ec 18 01 00 00    	sub    esp,0x118 ; 4
 ...
```

以下の順番で値が積まれる。

1. call命令の次の命令があるアドレスをスタックに積む、いわゆるリターンアドレス
2. その時点でのebpをスタックに積む
3. canaryを設定する、これがある場合バッファオーバフローが検出される
4. ローカル変数があるなら、ここにそのための領域を確保する

スタックはこのような状態になる。

|stack|
|:-:|
|4 (local var)|
|3 (canary)|
|2 ebp|
|1 return addr|

()付きのものは場合によっては無いときもあるもの。

ebpを積むのは、関数を抜けるときに値を復元するためである。
どの時点で関数が呼ばれても引数やローカル変数に対して同じ命令でアクセスできるように、ベースポインタを現在のスタックトップに設定する必要があるのでebpは変更される。

canaryを積むのは、バッファオーバーフローによる不正なスタックの書き換えを検出するためである。
大抵の場合乱数が使われ、先頭がNULLバイトになるようになっている。
canaryを書き換えてしまうと、バッファオーバーフローが検出され書き換えたリターンアドレスを実行させることができなくなる。
そのためcanaryがある場合にバッファオーバーフローさせるには、canaryの値を上書きしないように同じ値を書き込むようにしなければならない。
forkしたりしてもcanaryの値は同じままなので、そのような場合には一度canaryをリークさせてから再びその値を用いてオーバーフローさせることもできる。

### 関数からのreturn
関数からリターンするときの処理について。
大抵一番最後に`leave; ret`の命令が出現する。
参考までに実際の命令。
```nasm
 80484b1:	c9                   	leave
 80484b2:	c3                   	ret
```

leave命令はespをebpに設定し、popしてebpに設定する。
つまり`mov esp, ebp; pop ebp`と等価である。
ebpは一番最初に設定したcall時のespになっている。
そしてそれは元のebpが格納されている位置を指している。
前の図でいうと`2 ebp`を指していることになる。
つまり、ebpを復元していることになる。またpopしたことでespはひとつ戻り、リターンアドレスの位置を指すようになる。

ret命令は細かい仕様が色々あるけど、通常は`pop eip`と同じだと思っていい。
つまり、現在のスタックトップに格納されている値の位置に命令カウンタを動かすという理解で十分である。
正しく実行されたならば、この時点でespはリターンアドレスを指しているため、関数呼び出しの次の命令がeipにセットされる。
そしてpopが実行されたことでespも復元される。

ちなみに返り値はたいていの場合eaxにセットされている。
ただし、これはコンパイラの生成するコードに依存するため、場合によっては違うレジスタを介して返り値を返したりしているバイナリもあるかもしれない。

### バッファへの書き込み
C言語などでは、入力を受けとるときにfgetsを使うことができる。
宣言はman見た限りこんな感じ。

`fgets(char* s, int size, FILE* stream);`

動作としてはsにsizeバイトだけstreamから読み込むといった感じになる。
ここで一つ大きな問題がある。fgetsはsの大きさがsizeより小さいかどうかをチェックしないのである。
では、sより大きなsizeを引数に渡すとどうなるのか?
答えは、単純に大きさを無視してメモリ上に書き込んでいくのである。

これで必要な知識は全て揃っているはず。

## バッファオーバーフロー
### 書き換えの範囲
これで、fgetsなどでバッファのサイズをきちんと管理してないとバッファの大きさを無視してメモリに書き込んでしまうということが分かった。
このバッファがローカル変数の固定配列として宣言されていると考えてみよう。
このとき、スタックの4の位置にバッファが存在していることになる。
バッファに対する書き込みは低位から高位に向かって行なわれるため、図でいうところの下の方向に向かって書き込まれていくことになる。
これらのことを統合して考えると、バッファのサイズより書き込めるサイズが十分に大きいとき3、2、1全ての位置に任意の値を書き込めるということが分かる。
ただし、canaryは書き換えて違う値にしてしまうとバッファオーバーフローとして検知される。

また、単にバッファより先に確保された変数についても書き換えが可能である。

### 書き換えの範囲にある値
さらにここで、1が書き換え可能であるということを掘り下げる。
といっても単純なことで、リターンアドレスは関数から返った後に実行される命令の位置なので、リターンアドレスを書き換えられると任意の位置の命令を実行させられるよね?ということである。

これがいわゆる典型的なバッファオーバーフロー攻撃である。

## デモ
簡単なオーバフローによる書き換えのデモ。
ここでは、オーバーフローによって関数内で定義されたローカル変数を変更することができるを試してみる。
bofの脆弱性を含ませたコードは以下。

```c
#include <stdio.h>

void vuln() {
  int flag = 0;
  char buf[256];

  fgets(buf, 300, stdin); // vulnerability here, you can write 300 chars to 256 len buffer.

  if (flag == 0) {
    printf("Failed!\n");
  } else {
    printf("You won FLAG!\n");
  }
}

int main(int argc, char** argv){
  vuln();
  return 0;
}
```

コンパイルするには`gcc -m32 -fno-stack-protector overflow.c -o overflow`

main関数から一度呼び出しを挟んでいるのは、main関数だけ変なバリデーションのようなコードが生成されて上手くいかないときがあるので、それを避けるためで意味はないです。
vuln関数では、最初に`flag = 0`されているので、"Failed!"が表示されるはず。
しかし、バッファオーバーフローさせることでflagの値を書き換えることができるので、if文の分岐を変更させられる。

300文字のAとかを入力すれば成功するはず。
これで、実際にバッファ配列とは無関係な値を変更し、プログラムの挙動を意図していないものに変更させられることが確認できると思う。
