#ifndef INCLUDED_WEAPON_POSE_PARAMS_H
#define INCLUDED_WEAPON_POSE_PARAMS_H

#include "GameLib/Math/Vector3.h"

// 一人称の銃の構え（走るときの抱え方・撃った反動の見た目・目の位置）の調整値。使うのは FirstPersonBody。
// 起動中に F6 のエディタで動かせる（SummaryPlan/個別仕様/最適化と計測/DebugTools_Profiling_Plan.txt の 5-5）。毎フレーム読むので、変えればすぐ効く。
// 既定値を直すときはここだけを直す（エディタの Copy as C++ で、変えた項目がこの形で出てくる）。
struct WeaponPoseParams{
	// ── 走るときの抱え方（胸元へ寄せる）──
	// 胸の骨から見た、右手首（銃を付けた骨）を置く点（m）。胸の向き（体の前方をピッチぶん傾けたもの）で測る。
	// 骨格の根の空間では左が +X なので、右へ寄せるのは -X。銃が体から離れて見えるなら z を減らし、腕が詰まって見えるなら増やす。
	GameLib::Math::Vector3 carryWristFromChest = GameLib::Math::Vector3( -0.12f, -0.12f, 0.28f );
	// 胸の向きから見た銃の向き（度）。この順（内側へ振る → 下へ向ける → 銃口まわりに傾ける）に掛ける。
	float carryMuzzleInward = 25.f; // 正で銃口が体の内側（左）を向く
	float carryMuzzleDown = 30.f;   // 正で銃口が下を向く
	float carryCant = 15.f;         // 正で銃の上面が右へ倒れる

	// ── 撃った反動（反動の量が 1 のとき＝撃った瞬間の動き）──
	// 銃を銃口と逆の向きへ引く長さ（m）と、手首を中心に銃口を跳ね上げる角度（度）。
	// 覗いている間は小さくする（肩と頬で銃を押さえているのと、目のすぐ前でサイトが大きく動くと狙いが見えなくなるため）。
	float recoilBackHip = 0.035f;
	float recoilBackAim = 0.015f;
	float recoilMuzzleRiseHip = 2.5f;
	float recoilMuzzleRiseAim = 0.8f;

	// ── 目の位置（m）──
	// 頭の骨の原点は頭蓋の付け根（首の上端）にあるので、そこから目の高さ・顔の前面まで出す。
	// 前へ出すのは、潰した頭の頂点（骨の原点に集まる）や首の内側が視界に入らないようにするため。
	float eyeAboveHead = 0.08f;
	float eyeForward = 0.20f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（エディタの描画・ファイルの読み書き・既定値との比較が、ここ 1 か所から作られる）。
	// 名前はファイルのキーと C++ のコピーにも使うので、メンバの名前と同じにする。
	template< class V > void visit( V& v ) {
		v( "carryWristFromChest", carryWristFromChest, -0.5f, 0.5f, "m" );
		v( "carryMuzzleInward", carryMuzzleInward, -90.f, 90.f, "deg" );
		v( "carryMuzzleDown", carryMuzzleDown, -90.f, 90.f, "deg" );
		v( "carryCant", carryCant, -90.f, 90.f, "deg" );
		v( "recoilBackHip", recoilBackHip, 0.f, 0.2f, "m" );
		v( "recoilBackAim", recoilBackAim, 0.f, 0.2f, "m" );
		v( "recoilMuzzleRiseHip", recoilMuzzleRiseHip, 0.f, 15.f, "deg" );
		v( "recoilMuzzleRiseAim", recoilMuzzleRiseAim, 0.f, 15.f, "deg" );
		v( "eyeAboveHead", eyeAboveHead, -0.2f, 0.3f, "m" );
		v( "eyeForward", eyeForward, -0.2f, 0.3f, "m" );
	}
};

// 起動中ずっと 1 つだけの調整値（FirstPersonBody.cpp が持つ）。シーンを作り直しても（Retry）、調整した値はそのまま残る。
WeaponPoseParams& weaponPoseParams();

#endif
