#ifndef INCLUDED_I_INTERACTABLE_H
#define INCLUDED_I_INTERACTABLE_H

#include <string>

class Inventory;

// 見て操作キー（E）で使える物（落ちている物を拾う・車に給油する など）。
// 拾う処理も給油の処理も、Interactor がこの同じ入口から呼ぶ（新しい「使える物」を足しても Interactor は変わらない）。
class IInteractable{
public:
	virtual ~IInteractable(){}
	// 今の持ち物で使えるか。
	virtual bool canInteract( const Inventory& inventory ) const = 0;
	// 見ているときに画面に出す案内（英語）。使えないときは、なぜ使えないか（何が要るか）を書く。
	virtual std::string prompt( const Inventory& inventory ) const = 0;
	// 使う。canInteract が true のときだけ呼ばれる。
	virtual void interact( Inventory* inventory ) = 0;
};

#endif
