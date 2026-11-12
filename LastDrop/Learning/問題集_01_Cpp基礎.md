# 問題集 01：C++ の基礎

章の教材（01 章）に入る前の、土台の確認。4 択が 35 問、記述が 12 問。
ゲームのコードを読むのに最低限要る所だけを選んである。

## 使い方

1. 何も見ずに解く。4 択は「なぜほかの 3 つが違うか」まで言えたら正解とする（当てずっぽうの正解は不正解に数える）
2. 「答え」を開いて確かめる。間違えた問題・迷った問題に印を付ける
3. 印の付いた問題は、**小さなプログラムを自分で書いて動かして確かめる**（答えを読んで分かった気になるのが一番危ない）
4. 3 日あけて、印の付いた問題だけもう一度解く
5. 記述は、紙かファイルに実際に書く。頭の中で答えない

### 採点の目安

| 4 択の正解 | 次にやること |
|---|---|
| 30 問以上 | 記述へ進む。その後 [01 章](01_Cpp.md) へ |
| 20〜29 問 | 間違えた節を「江添亮の C++ 入門」か learncpp.com で読み直してから、3 日後にもう一度 |
| 19 問以下 | 先に入門書を 1 冊通す（[参考資料](参考資料.md) の C++ の節）。問題集はその後で |

コードの問題は、`#include <iostream>`・`<vector>`・`<memory>`・`<string>` と `using std::cout;` があるものとする。

---

# 4 択

## A. ポインタ・参照・メモリ

**Q1.** 次のコードの後、`a` の値は。

```cpp
int a = 5;
int* p = &a;
*p = 8;
```

- A. 5
- B. 8
- C. `a` のアドレス
- D. コンパイルエラー

<details><summary>答え</summary>

**B**。`p` は `a` の場所を指している。`*p` は「p が指す先」なので、`*p = 8` は `a = 8` と同じ。

</details>

**Q2.** 次のコードの後、`a` の値は。

```cpp
int a = 1;
int& r = a;
r = 3;
int b = 7;
r = b;
b = 9;
```

- A. 1
- B. 3
- C. 7
- D. 9

<details><summary>答え</summary>

**C**。参照は、最初に結び付けた相手（`a`）から付け替えられない。`r = b` は「r を b に付け替える」ではなく「a に b の値（7）を入れる」。その後 `b` を変えても `a` は変わらない。

</details>

**Q3.** `const int* p` と宣言したとき、**できない**のはどれか。

- A. `*p = 1;`（指す先の値を書き換える）
- B. `p = &other;`（別の変数を指させる）
- C. `int x = *p;`（指す先の値を読む）
- D. `p = nullptr;`

<details><summary>答え</summary>

**A**。`const int* p` は「指す先が const」。指す先を書き換えられないが、ポインタ自体は付け替えられる。逆に `int* const p` は「ポインタ自体が const」で、付け替えられないが指す先は書き換えられる。`const` は「すぐ左（左に無ければすぐ右）」に掛かる、と読む。

</details>

**Q4.** 次のコードの後、`v` の値は。

```cpp
void add( int x ){ x += 1; }
void add2( int& x ){ x += 1; }

int v = 0;
add( v );
add2( v );
```

- A. 0
- B. 1
- C. 2
- D. コンパイルエラー

<details><summary>答え</summary>

**B**。`add` は値渡しで、`x` は `v` の写し。写しを変えても `v` は変わらない。`add2` は参照渡しで、`x` は `v` そのもの。

</details>

**Q5.** 次の関数の問題は何か。

```cpp
int* makeValue(){
    int value = 10;
    return &value;
}
```

- A. `int*` は関数の戻り値にできない
- B. `value` を `const` にしていない
- C. メモリリークする
- D. 関数を抜けると `value` は消えるので、返したポインタは消えた場所を指す

<details><summary>答え</summary>

**D**。関数の中の普通の変数（ローカル変数）は、関数を抜けると消える。消えた場所を指すポインタ（ダングリングポインタ）を使うと未定義動作。コンパイルは通ってしまう（警告は出る）。

</details>

**Q6.** `int* p = new int[ 10 ];` で確保したメモリを解放する正しい書き方は。

- A. `delete p;`
- B. `free( p );`
- C. `delete[] p;`
- D. 何もしなくてよい（自動で解放される）

<details><summary>答え</summary>

**C**。`new[]` で確保した物は `delete[]`、`new` で確保した物は `delete`。組が違うと未定義動作。そもそも今の C++ では `new`／`delete` を自分で書かず、`std::vector< int > v( 10 );` や `std::unique_ptr` を使う。

</details>

**Q7.** `Enemy* e = nullptr;` のまま `e->hp = 0;` を実行すると。

- A. 未定義動作（多くの場合、アクセス違反で落ちる）
- B. 何も起きない
- C. 自動で `Enemy` が作られる
- D. コンパイルエラー

<details><summary>答え</summary>

**A**。nullptr は「何も指していない」。その先を読み書きするのは未定義動作で、Windows ではたいてい「0x00000000 への書き込みでアクセス違反」で止まる。コンパイラは止めてくれない。

</details>

**Q8.** 次のコードについて正しいのは。

```cpp
int a[ 3 ] = { 1, 2, 3 };
a[ 3 ] = 10;
```

- A. コンパイルエラーになる
- B. 配列が自動で 4 個に伸びる
- C. 必ず実行時にエラーで止まる
- D. 未定義動作。止まらずに、隣にある別の変数を壊すこともある

<details><summary>答え</summary>

**D**。`a[ 3 ]` は 4 個目で、範囲の外。C++ は範囲を確かめないので、コンパイルも通り、実行しても止まるとは限らない。別の変数が黙って書き換わり、関係の無い所で不具合になる。一番見つけにくい種類のバグ。

</details>

## B. クラス

**Q9.** 次のプログラムの出力は。

```cpp
struct A{ A(){ cout << "A"; } ~A(){ cout << "a"; } };
struct B{ B(){ cout << "B"; } ~B(){ cout << "b"; } };

int main(){
    A a;
    {
        B b;
    }
    cout << "-";
}
```

- A. `AB-ba`
- B. `ABb-a`
- C. `ABab-`
- D. `AB-ab`

<details><summary>答え</summary>

**B**。変数は、宣言した所でコンストラクタが呼ばれ、その変数のある `{ }` を抜ける所でデストラクタが呼ばれる。`b` は内側の `}` で消え、`a` は `main` の終わりで消える。

</details>

**Q10.** 次のクラスで、コンストラクタの後の `x` の値は。

```cpp
struct S{
    int x;
    int y;
    S() : y( 1 ), x( y ) {}
};
```

- A. 1
- B. 0
- C. 決まっていない（不定）
- D. コンパイルエラー

<details><summary>答え</summary>

**C**。メンバーは、初期化リストに書いた順ではなく**クラスの中で宣言した順**（`x` → `y`）に初期化される。`x( y )` の時点で `y` はまだ初期化されていない。コンパイラは警告を出す。初期化リストは宣言と同じ順に書く。

</details>

**Q11.** 次のコードはどうなるか。

```cpp
class Counter{
public:
    void add() const { mCount += 1; }
private:
    int mCount = 0;
};
```

- A. コンパイルエラー
- B. 問題なく動く
- C. 実行時にエラー
- D. 警告は出るが動く

<details><summary>答え</summary>

**A**。関数の後ろの `const` は「この関数はメンバーを書き換えない」という約束。破るとコンパイルエラー。値を返すだけの関数（`int count() const`）に付ける。

</details>

**Q12.** `struct` と `class` の違いは。

- A. `struct` は関数を持てない
- B. `struct` は継承できない
- C. `struct` はスタックに、`class` はヒープに作られる
- D. 何も書かないときのアクセスが、`struct` は public、`class` は private

<details><summary>答え</summary>

**D**。違いはそれだけ。C# の `struct`（値型）と `class`（参照型）の違いとは別物なので、Unity の感覚で考えないこと。C++ では、どちらもスタックにもヒープにも作れる。

</details>

**Q13.** 次のコードで起きることは。

```cpp
struct Buf{
    int* p;
    Buf(){ p = new int[ 4 ]; }
    ~Buf(){ delete[] p; }
};

int main(){
    Buf a;
    Buf b = a;
}
```

- A. `b` 用に新しい配列が確保され、問題ない
- B. `a` と `b` が同じ配列を指し、`main` の終わりで同じメモリを 2 回解放する
- C. コンパイルエラー
- D. メモリリークする

<details><summary>答え</summary>

**B**。コピーを自分で書かないと、コンパイラは「メンバーを 1 つずつ写す」コピーを作る。ポインタの値（アドレス）が写るだけで、配列は増えない。両方のデストラクタが同じ配列を `delete[]` する（二重解放）。直し方は記述の問 6。

</details>

**Q14.** `Enemy( const Enemy& ) = delete;` の意味は。

- A. `Enemy` を delete できないようにする
- B. デストラクタを消す
- C. `Enemy` のコピーを禁止する（コピーしようとするとコンパイルエラー）
- D. `Enemy` のメモリを解放する

<details><summary>答え</summary>

**C**。コピーコンストラクタを「使えない」と宣言している。ポインタや GPU の物を持つクラスは、写されると Q13 のようになるので、こうして禁止する。自分のゲームでは `Component` や `GameObject` がこう書いている。

</details>

**Q15.** クラスの `static` なメンバー変数について正しいのは。

- A. そのクラスのオブジェクト全部で 1 つだけ共有される
- B. オブジェクトごとに 1 つずつ持つ
- C. 書き換えられない
- D. そのファイルの中でしか使えない

<details><summary>答え</summary>

**A**。`static` なメンバー変数はクラスに 1 つ。オブジェクトをいくつ作っても増えない。書き換えられないのは `const`。「そのファイルの中だけ」は、クラスの外の `static`（や無名 namespace）の意味で、同じ単語が別の意味を持っているので注意。

</details>

**Q16.** メンバー関数の中の `this` は何か。

- A. そのクラスの名前
- B. 基底クラスへのポインタ
- C. 最後に作られたオブジェクト
- D. その関数が呼ばれているオブジェクト自身へのポインタ

<details><summary>答え</summary>

**D**。`enemy.update()` と呼べば、`update` の中の `this` は `&enemy`。メンバー `mHp` は `this->mHp` の省略。

</details>

## C. 継承と仮想関数

**Q17.** 次のプログラムの出力は。

```cpp
struct Base{
    void f(){ cout << "B"; }
    virtual void g(){ cout << "B"; }
};
struct Derived : Base{
    void f(){ cout << "D"; }
    void g() override { cout << "D"; }
};

int main(){
    Base* p = new Derived;
    p->f();
    p->g();
}
```

- A. `BB`
- B. `DD`
- C. `BD`
- D. `DB`

<details><summary>答え</summary>

**C**。`virtual` の付いていない `f` は、**ポインタの型**（`Base*`）で決まり、`Base::f`。`virtual` の付いた `g` は、**実際のオブジェクトの型**（`Derived`）で決まり、`Derived::g`。これが仮想関数の全部。

</details>

**Q18.** `virtual void update() = 0;` を持つクラスについて正しいのは。

- A. `update` は何もしない関数になる
- B. そのクラスのオブジェクトは直接作れない（継承して `update` を書いたクラスだけ作れる）
- C. `update` は 0 を返す
- D. 継承できなくなる

<details><summary>答え</summary>

**B**。`= 0` は純粋仮想関数。「中身は派生クラスが必ず書く」という宣言で、こういう関数を持つクラス（抽象クラス）は直接作れない。自分のゲームの `IScene` のような「インターフェース」がこれ。

</details>

**Q19.** 基底クラスのデストラクタに `virtual` が無いまま、次のようにすると。

```cpp
Base* p = new Derived;
delete p;
```

- A. コンパイルエラー
- B. `Derived` のデストラクタ → `Base` のデストラクタの順に呼ばれる
- C. 何も呼ばれない
- D. `Derived` のデストラクタが呼ばれない（未定義動作）

<details><summary>答え</summary>

**D**。Q17 と同じ理屈で、`virtual` でないデストラクタはポインタの型（`Base`）の物しか呼ばれない。`Derived` が持っていた物が片付かない。「基底クラスのポインタで消すことがあるクラスは、デストラクタを virtual にする」。

</details>

**Q20.** 派生クラスの関数に `override` を付ける利点は。

- A. 基底クラスに同じ形の virtual 関数が無ければ、コンパイルエラーで教えてくれる
- B. 関数の呼び出しが速くなる
- C. 基底クラスの関数を呼べなくする
- D. 付けないと上書きできない

<details><summary>答え</summary>

**A**。付けなくても上書きはできる。ただ、引数の型や `const` を 1 つ間違えると「上書きのつもりが、別の新しい関数」になり、呼ばれないまま気付かない。`override` を付けておけば、そのときビルドが通らない。

</details>

## D. スマートポインタと標準ライブラリ

**Q21.** 次のコードはどうなるか。

```cpp
std::unique_ptr< Enemy > a = std::make_unique< Enemy >();
std::unique_ptr< Enemy > b = a;
```

- A. `a` と `b` が同じ `Enemy` を持つ
- B. `Enemy` がもう 1 つ作られる
- C. コンパイルエラー（`unique_ptr` はコピーできない）
- D. 実行時にエラー

<details><summary>答え</summary>

**C**。`unique_ptr` は「持ち主は 1 人だけ」の決まりを型で守る。コピーはできず、`std::move( a )` で持ち主を移すことだけできる。

</details>

**Q22.** `std::unique_ptr< Enemy > b = std::move( a );` の後、`a` はどうなっているか。

- A. 元の `Enemy` を指したまま
- B. 空（nullptr）
- C. 使うとコンパイルエラーになる
- D. `Enemy` の写しを持つ

<details><summary>答え</summary>

**B**。持ち主が `b` に移り、`a` は空になる。`a->hp` と書くとコンパイルは通るが、nullptr の先を触ることになる（Q7）。

</details>

**Q23.** 次のコードの後、`v` の中身は。

```cpp
std::vector< int > v = { 1, 2, 3 };
for ( int x : v ){
    x *= 2;
}
```

- A. 1, 2, 3
- B. 2, 4, 6
- C. 空
- D. コンパイルエラー

<details><summary>答え</summary>

**A**。`int x` は要素の**写し**。写しを 2 倍にしても `v` は変わらない。書き換えたいなら `int& x`、読むだけなら `const int& x`（大きい型では写さないため）。

</details>

**Q24.** 次のコードの問題は。

```cpp
std::vector< int > v;
v.push_back( 1 );
int* p = &v[ 0 ];
for ( int i = 0; i < 1000; ++i ){ v.push_back( i ); }
*p = 99;
```

- A. `&v[ 0 ]` は書けない
- B. `push_back` は 1000 回も呼べない
- C. 問題ない
- D. `push_back` で中身が別の場所へ引っ越すことがあり、`p` は消えた場所を指す

<details><summary>答え</summary>

**D**。`vector` は、足りなくなると大きい場所を確保して全部の要素を移す。前に取ったポインタ・参照・イテレータは無効になる。添字（番号）で覚えるのが安全。

</details>

**Q25.** 要素が 3 個の `std::vector< int > v` に対して正しいのは。

- A. `v[ 10 ]` も `v.at( 10 )` も、必ず例外を投げる
- B. `v[ 10 ]` は 0 を返す
- C. `v[ 10 ]` は未定義動作、`v.at( 10 )` は例外を投げる
- D. どちらもコンパイルエラー

<details><summary>答え</summary>

**C**。`[]` は範囲を確かめない（速い）。`at` は確かめて、外なら `std::out_of_range` を投げる。Visual Studio の Debug ビルドでは `[]` も確かめて止めてくれるが、Release では確かめない。

</details>

**Q26.** 次の `if` は何を比べているか。

```cpp
const char* a = getName();
const char* b = getOtherName();
if ( a == b ){ ... }
```

- A. 2 つの文字列の中身
- B. 2 つのポインタ（アドレス）
- C. 2 つの文字列の長さ
- D. 先頭の 1 文字

<details><summary>答え</summary>

**B**。`const char*` はポインタなので、`==` はアドレスを比べる。中身が同じ文字列でも、別の場所にあれば false。中身を比べるなら `std::string` にするか `strcmp`。`std::string` どうしの `==` は中身を比べる。

</details>

**Q27.** 次のコードで、`f()` が返す値は。

```cpp
int n = 1;
auto f = [ n ](){ return n; };
n = 5;
int result = f();
```

- A. 1
- B. 5
- C. 0
- D. コンパイルエラー

<details><summary>答え</summary>

**A**。`[ n ]` は、ラムダを**作った時点**の `n` の値を写して持つ。後から `n` を変えても変わらない。`[ &n ]` なら参照で持つので 5 になる（その代わり、`n` が消えた後に呼ぶとダングリング）。

</details>

## E. ビルドと型

**Q28.** ヘッダ `util.h` に次のように書き、`a.cpp` と `b.cpp` の両方から include した。どうなるか。

```cpp
// util.h
int twice( int x ){ return x * 2; }
```

- A. 問題なくビルドできる
- B. コンパイルエラー
- C. 実行時にエラー
- D. リンクエラー（同じ関数が 2 つ定義されている）

<details><summary>答え</summary>

**D**。include は「その場所にファイルの中身を貼る」だけ。`a.cpp` と `b.cpp` の両方に `twice` の定義ができ、リンクのときにぶつかる（LNK2005）。ヘッダには宣言（`int twice( int x );`）だけ書いて定義は `.cpp` に書くか、`inline` を付ける。

</details>

**Q29.** 「未解決の外部シンボル（LNK2019）」は、どういうエラーか。

- A. 文法の間違い
- B. 宣言はあるが、定義（中身）がどこにも見つからない
- C. 変数を初期化していない
- D. ヘッダファイルが見つからない

<details><summary>答え</summary>

**B**。コンパイルは「宣言があれば」通る。リンクで初めて中身を探し、無ければこのエラー。原因は、関数の中身を書き忘れた・`.cpp` をプロジェクトに入れていない・ライブラリ（`d3d11.lib` など）をリンクしていない、のどれか。

</details>

**Q30.** `static_cast< int >( 3.9f )` の値は。

- A. 4
- B. 3.9
- C. 3
- D. コンパイルエラー

<details><summary>答え</summary>

**C**。小数から整数へのキャストは、0 の方向へ切り捨てる。四捨五入したければ `static_cast< int >( x + 0.5f )`（正の数のとき）か `std::round`。

</details>

**Q31.** 次の 3 つの値は、順に。

```cpp
int   a = 7 / 2;
float b = 7 / 2;
float c = 7 / 2.f;
```

- A. 3、3.0、3.5
- B. 3、3.5、3.5
- C. 3.5、3.5、3.5
- D. 4、3.5、3.5

<details><summary>答え</summary>

**A**。整数どうしの割り算は、結果も整数（3）。`b` は「3 を float に入れた」だけ。片方が小数（`2.f`）なら小数で計算される。画面の縦横比を `width / height` と整数で計算して 1 になってしまうのは、よくある失敗。

</details>

**Q32.** 次のコードの後、`u` の値は（`unsigned int` は 32 ビットとする）。

```cpp
unsigned int u = 0;
u -= 1;
```

- A. −1
- B. 0
- C. 実行時にエラー
- D. 4294967295

<details><summary>答え</summary>

**D**。符号なしの整数は負の数を持てず、0 から 1 を引くと最大値に回り込む。`for ( unsigned i = n - 1; i >= 0; --i )` は、`i >= 0` がいつも真なので終わらない。`size()` の戻り値も符号なしなので、`v.size() - 1` は空のとき巨大な数になる。

</details>

**Q33.** `float a = 0.1f + 0.2f;` のとき、`a == 0.3f` は。

- A. 必ず true
- B. true になるとは限らない（小数は誤差を持つ）
- C. 必ず false
- D. コンパイルエラー

<details><summary>答え</summary>

**B**。`float` は 2 進数で持つので、0.1 のような数はぴったり表せず、計算のたびに小さな誤差が出る。小数を `==` で比べず、「差が十分小さいか」で比べる。

</details>

**Q34.** 次のコードはどうなるか。

```cpp
template< class T >
T maxOf( T a, T b ){ return ( a > b ) ? a : b; }

auto r = maxOf( 1, 2.5 );
```

- A. `r` は 2.5 になる
- B. `r` は 2 になる
- C. コンパイルエラー（T を int にも double にもできない）
- D. 実行時にエラー

<details><summary>答え</summary>

**C**。`T` は 1 つの型でないといけないが、引数から `int` と `double` の両方が出てきて決められない。`maxOf< double >( 1, 2.5 )` と型を書くか、引数の型を揃える。

</details>

**Q35.** ヘッダの先頭の `#pragma once`（や `#ifndef` 〜 `#define` 〜 `#endif`）の役割は。

- A. 1 つの `.cpp` の中で、同じヘッダが 2 回読まれても中身が 2 回入らないようにする
- B. ヘッダを 1 回しかコンパイルしないので、ビルドが必ず速くなる
- C. ヘッダの中の変数を 1 つだけにする
- D. そのヘッダを 1 つの `.cpp` からしか include できなくする

<details><summary>答え</summary>

**A**。A.h が C.h を読み、B.h も C.h を読み、`.cpp` が A.h と B.h を読むと、C.h は 2 回貼られる。クラスの定義が 2 回あるとエラーなので、2 回目を空にする。Q28（別々の `.cpp` に 1 つずつ入る）は防げないことに注意。

</details>

---

# 記述

答えを開く前に、必ず自分で書くこと。模範の答えと言い回しが違ってよい。**要点が入っているか**で採点する。

**問 1.** ポインタと参照の違いを 3 つ書け。

<details><summary>答えの例</summary>

1. ポインタは nullptr（何も指さない）にできる。参照は必ず何かに結び付いている
2. ポインタは後から別の物へ付け替えられる。参照は最初に結び付けた相手から変えられない
3. ポインタは `*p`・`p->x` と書いて先を使う。参照は相手の名前と同じように `r`・`r.x` と書く

使い分けの目安：「無いことがある・付け替える」ならポインタ、「必ずある・読むだけ」なら const 参照。

</details>

**問 2.** スタックに作られる変数と、ヒープに作られる物の違いを説明せよ。それぞれ、いつ消えるか。

<details><summary>答えの例</summary>

- スタック：関数の中で普通に宣言した変数。その変数のある `{ }` を抜けると自動で消える。確保が速い。大きさに限りがある（大きな配列は置けない）
- ヒープ：`new`（や `make_unique`・`vector` の中身）で確保した物。`delete` されるまで消えない。`{ }` をまたいで生かせる。大きい物・数が実行時に決まる物を置く

ヒープの物は、消し忘れるとリーク、2 回消すと二重解放、消した後に使うとダングリング。だから自分で `delete` を書かず、スタックの上の持ち主（`unique_ptr`・`vector`）に消させる。

</details>

**問 3.** RAII とは何か。例を 1 つ挙げて説明せよ。

<details><summary>答えの例</summary>

「確保はコンストラクタで、解放はデストラクタで」行い、物の寿命を変数の寿命に結び付ける書き方。
変数が `{ }` を抜ければ必ずデストラクタが呼ばれるので、途中で `return` しても例外が飛んでも、解放を忘れない。

例：`std::unique_ptr`（デストラクタで `delete`）、`std::vector`（デストラクタで中身を解放）、ファイルを開いて閉じるクラス、時間を測る `ScopedTimer`、D3D の `ComPtr`（デストラクタで `Release`）。

</details>

**問 4.** 次のプログラムの出力を書け。

```cpp
struct Log{
    const char* n;
    Log( const char* s ) : n( s ) { cout << "+" << n << " "; }
    ~Log(){ cout << "-" << n << " "; }
};

int main(){
    Log a( "a" );
    std::unique_ptr< Log > p = std::make_unique< Log >( "p" );
    {
        Log b( "b" );
        std::unique_ptr< Log > q = std::move( p );
    }
    cout << "| ";
    Log c( "c" );
}
```

<details><summary>答え</summary>

```
+a +p +b -p -b | +c -c -a
```

- `q = std::move( p )` で、"p" の持ち主は `q` になる。`p` は空
- 内側の `}` で、宣言の**逆順**に消える。`q`（"p" を消す）→ `b`
- `main` の終わりで、逆順に `c` → `p`（空なので何も出ない）→ `a`

「同じ `{ }` の中では、後に作った物から先に消える」が要点。

</details>

**問 5.** 次のコードには不具合がある。何が起きるかを説明し、直し方を 2 つ書け。

```cpp
std::vector< Enemy > enemies;
enemies.push_back( Enemy() );
Enemy* boss = &enemies[ 0 ];

for ( int i = 0; i < 100; ++i ){
    enemies.push_back( Enemy() );
}
boss->hp = 0;
```

<details><summary>答えの例</summary>

`push_back` で `vector` の中身が別の場所へ引っ越すと、`boss` は消えた場所を指す。`boss->hp = 0` は消えたメモリへの書き込み（未定義動作）。

直し方（どれか）：

- ポインタではなく番号で覚える（`int bossIndex = 0;` → `enemies[ bossIndex ].hp = 0;`）
- `std::vector< std::unique_ptr< Enemy > >` にする（引っ越すのは `unique_ptr` だけで、`Enemy` 本体は動かない）
- 先に `enemies.reserve( 101 )` して引っ越しを起こさない（数が分かっているときだけ。忘れると元に戻るので弱い）
- 全部足し終えてからポインタを取る

</details>

**問 6.** 4 択の Q13 の `Buf` クラスを、二重解放が起きないように直せ。方法を 2 つ書け。

<details><summary>答えの例</summary>

**方法 1：コピーを禁止する**（一番簡単で、ゲームのコードで一番多い）

```cpp
struct Buf{
    int* p;
    Buf(){ p = new int[ 4 ]; }
    ~Buf(){ delete[] p; }
    Buf( const Buf& ) = delete;
    Buf& operator=( const Buf& ) = delete;
};
```

**方法 2：自分で持たず、持ち主に任せる**（一番良い）

```cpp
struct Buf{
    std::vector< int > p = std::vector< int >( 4 );
};
```

`vector` は正しいコピーとデストラクタを持っているので、`Buf` は何も書かなくてよい。

**方法 3：コピーを正しく書く**（中身ごと写す。コピーコンストラクタと代入演算子の両方が要る）

「デストラクタを自分で書いたクラスは、コピーをどうするかも必ず決める」。

</details>

**問 7.** `std::vector< float >` を受け取り、合計を返す関数 `sum` を書け。引数は写さないこと。

<details><summary>答えの例</summary>

```cpp
float sum( const std::vector< float >& values ){
    float total = 0.f;
    for ( float v : values ){
        total += v;
    }
    return total;
}
```

- 引数は `const std::vector< float >&`。`std::vector< float > values` だと、呼ぶたびに全部の要素を写す
- ループの中の `float v` は、`float` が小さいので写しでよい（大きい型なら `const T&`）

</details>

**問 8.** 次の仕様のプログラムを書け。

- 基底クラス `Shape` は、面積を返す `area()` を持つ
- `Circle`（半径）と `Rect`（幅・高さ）が `Shape` を継承する
- `main` で、いくつかの `Circle` と `Rect` を 1 つの `vector` に入れ、面積の合計を出す

<details><summary>答えの例</summary>

```cpp
class Shape{
public:
    virtual ~Shape() = default;
    virtual float area() const = 0;
};

class Circle : public Shape{
public:
    explicit Circle( float radius ) : mRadius( radius ) {}
    float area() const override { return 3.14159265f * mRadius * mRadius; }
private:
    float mRadius;
};

class Rect : public Shape{
public:
    Rect( float width, float height ) : mWidth( width ), mHeight( height ) {}
    float area() const override { return mWidth * mHeight; }
private:
    float mWidth;
    float mHeight;
};

int main(){
    std::vector< std::unique_ptr< Shape > > shapes;
    shapes.push_back( std::make_unique< Circle >( 1.f ) );
    shapes.push_back( std::make_unique< Rect >( 2.f, 3.f ) );

    float total = 0.f;
    for ( const std::unique_ptr< Shape >& s : shapes ){
        total += s->area();
    }
    cout << total;
}
```

確かめる点：

- `Shape` のデストラクタが `virtual` か（Q19）
- `area` に `const` と `override` が付いているか
- `vector< Shape >` ではなく `vector< unique_ptr< Shape > >` にしているか。`vector< Shape >` には派生クラスを入れられない（入れると `Shape` の部分だけが切り取られる。そもそも抽象クラスなので作れない）
- ループが `const ...&` か（`unique_ptr` は写せない）

これは、自分のゲームの「`GameObject` が `vector< unique_ptr< Component > >` を持つ」「`SceneManager` が `unique_ptr< IScene >` を持つ」と同じ形。

</details>

**問 9.** 宣言と定義の違いを説明せよ。ヘッダ（`.h`）と `.cpp` に分ける理由も書け。

<details><summary>答えの例</summary>

- 宣言：「こういう名前・型の物がある」と知らせるだけ（`int twice( int x );`・`class Enemy;`）。何回書いてもよい
- 定義：中身そのもの（関数の `{ }`・クラスの `{ }`・変数の実体）。プログラム全体で 1 つだけ

`.cpp` は 1 つずつ別々にコンパイルされる。別の `.cpp` にある関数を使うには、宣言が見えていないといけないので、宣言をヘッダに書いて include する。定義を `.cpp` に書くのは、(1) 定義が 2 つにならないように、(2) 中身を直してもその `.cpp` だけ再コンパイルすれば済むように、(3) 使う側に中身を見せないように。

</details>

**問 10.** 次のクラスに、付けるべき `const` と `&` を全部付けよ。

```cpp
class Player{
public:
    Player( std::string name ) : mName( name ), mHp( 100 ) {}
    std::string name() { return mName; }
    int hp() { return mHp; }
    void damage( int amount ) { mHp -= amount; }
    bool isDead() { return mHp <= 0; }
private:
    std::string mName;
    int mHp;
};
```

<details><summary>答えの例</summary>

```cpp
class Player{
public:
    explicit Player( const std::string& name ) : mName( name ), mHp( 100 ) {}
    const std::string& name() const { return mName; }
    int hp() const { return mHp; }
    void damage( int amount ) { mHp -= amount; }
    bool isDead() const { return mHp <= 0; }
private:
    std::string mName;
    int mHp;
};
```

- メンバーを変えない関数（`name`・`hp`・`isDead`）に `const`
- `std::string` の引数と戻り値は `const std::string&`（写さない）
- `int` は小さいので、そのまま値で
- `damage` はメンバーを変えるので `const` を付けない
- 引数 1 つのコンストラクタには `explicit`（文字列から勝手に `Player` が作られるのを防ぐ。発展）

</details>

**問 11.** 「持ち主」と「借りる」を、`std::unique_ptr` と生のポインタを使って説明せよ。生のポインタを `delete` してよいのはどんなときか。

<details><summary>答えの例</summary>

- 持ち主：その物を消す責任を持つ人。`std::unique_ptr< T >` で持つ。持ち主が消えるとき、物も消える。持ち主は 1 人だけ
- 借りる：使うだけで、消さない人。`T*` か `T&` で持つ。持ち主が生きている間だけ使える

この決まりにすると、生のポインタを `delete` してよいときは**無い**。生のポインタを見たら「借りているだけ」と読める。
気を付けるのは、持ち主が先に消えると、借りている側のポインタがダングリングになること（持ち主の方が長生きするように作る）。

</details>

**問 12.** コンパイルエラー・リンクエラー・実行時のエラーの違いを、例を 1 つずつ挙げて説明せよ。

<details><summary>答えの例</summary>

- コンパイルエラー：1 つの `.cpp` を機械語にする段階で見つかる。文法の間違い・型が合わない・宣言が見えない。例：`;` の忘れ、`const` な関数の中でメンバーを書き換えた、include を忘れた（C2065 など）
- リンクエラー：コンパイルした物をつなぐ段階で見つかる。定義が無い・定義が 2 つある。例：関数の中身を書き忘れた、`d3d11.lib` をリンクしていない（LNK2019）、ヘッダに定義を書いた（LNK2005）
- 実行時のエラー：ビルドは通り、動かして初めて起きる。例：nullptr の先を触る、配列の範囲外、ダングリングポインタ、0 で割る

上ほど早く・確実に見つかる。だから「実行時にしか分からない間違いを、コンパイル時に分かるようにする」書き方（`const`・`override`・`= delete`・`unique_ptr`・`static_assert`・`enum class`）に価値がある。

</details>
