#ifndef INCLUDED_CAMERA_SHAKE_H
#define INCLUDED_CAMERA_SHAKE_H

// 見える絵の揺れ（上下・左右・ロールの角度）を決める。仕様は SummaryPlan/個別仕様/射撃と手応え/CameraShake_Recoil_Plan.txt の 3 章。
// 揺れの強さをトラウマ値（0〜1）で持ち、撃つ・攻撃されるなどで足して、時間で減らす。揺れの量はトラウマの 2 乗 × なめらかなノイズ。
// 弾の向き・照準・体の向きには入れない（入れるのは描く行列だけ。FirstPersonCamera::setShake）。
// 弾道のずれは AimRecoil・AimPunch だけで決め、「押さえれば当たる」をはっきりさせるため。
// 何で揺らすか（撃った・攻撃された）は知らない。足す量は呼ぶ側の定数、揺れ方は CameraShake.cpp の定数で直す。
class CameraShake{
public:
	// seed: ノイズの種（遊ぶたびに揺れ方を変える）。
	explicit CameraShake( int seed );

	// トラウマを amount だけ足す。ただし足した結果は limit より上げない（もう limit より上なら下げもしない）。
	// 弱い揺れ（射撃）を続けても強い揺れ（攻撃された）ほどにはならないようにするため。amount・limit は 0〜1。
	void addTrauma( float amount, float limit );
	// 1ティック進める（トラウマを減らし、ノイズを読む位置を進める）。
	void update();

	// 描く行列へ足す角度（度）。揺れていなければ 0。向きの約束は FirstPersonCamera と同じ（ピッチは正で見下ろす・ヨーは正で左）。
	float pitch() const;
	float yaw() const;
	float roll() const; // 正で右へ傾く（画面が時計回りに回る）
	float trauma() const;

private:
	// channel 番目のノイズ（−1〜1）を今の位置で読む。
	float noise( int channel ) const;

	unsigned mSeed;
	float mTrauma;    // 0〜1
	float mPhase;     // ノイズを読む位置（0 以上。1 進むと格子 1 つ）
	int mLatticeBase; // mPhase を小さく保つために折り返した格子の数（精度が落ちないように）
};

#endif
