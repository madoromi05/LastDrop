#ifndef INCLUDED_COMPONENT_TYPE_ID_H
#define INCLUDED_COMPONENT_TYPE_ID_H

// 部品の種類の番号。GameObject と GameObjectWorld が「この種類の部品」を探すのに使う（RTTI・dynamic_cast を使わない）。
// 番号は、その種類を初めて使ったときに 0, 1, 2 … と振る。並びに意味は無く、起動ごとに変わることがあるので保存しないこと。
// 関数の中の static は、ヘッダの inline 関数でもプログラム全体で 1 つ（どの .cpp から呼んでも同じ番号になる）。
inline int nextComponentTypeId(){
	static int counter = 0;
	return counter++;
}

// T の番号。T は Component を継承した部品の型（const を付けずに渡す）。
template< class T >
int componentTypeId(){
	static const int id = nextComponentTypeId();
	return id;
}

#endif
