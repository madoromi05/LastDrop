#ifndef INCLUDED_I_INTERACTABLE_H
#define INCLUDED_I_INTERACTABLE_H

class Inventory;

// 見て操作キー（E）で使える物（落ちている物を拾う・車に給油する など）。
// 拾う処理も給油の処理も、Interactor がこの同じ入口から呼ぶ（新しい「使える物」を足しても Interactor は変わらない）。
// 画面には文字の案内を出さず、物の上にマークだけを出す（InteractMarkers。SummaryPlan/個別仕様/InteractMarker_Plan.txt）。
class IInteractable{
public:
	virtual ~IInteractable(){}
	// 今の持ち物で使えるか（使えなければマークを灰色にする。カバンが一杯・ガソリンを持っていない・満タン など）。
	virtual bool canInteract( const Inventory& inventory ) const = 0;
	// 使う。canInteract が true のときだけ呼ばれる。
	virtual void interact( Inventory* inventory ) = 0;
};

#endif
