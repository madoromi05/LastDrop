#ifndef INCLUDED_WEAPON_CATALOG_H
#define INCLUDED_WEAPON_CATALOG_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include "GameSrc/Audio/SoundCatalog.h"

// 銃の種類。増やすときはここに1つ足し、WeaponCatalog.cpp の表にも1行足す（ここを見れば全種類が分かる）
// 銃ごとの決まり（弾倉・連射・威力・反動・持ち方…）は表にデータで持ち、銃の処理（GunComponent など）には書かない
// 種類を足しても、処理は変えずに表へ 1 行足すだけで済むようにするため。SummaryPlan/個別仕様/射撃と手応え/Pistol_Plan.txt
enum WeaponId{
	WEAPON_PISTOL, // 拳銃（9mm。出撃のたびにタダでもらえる）
	WEAPON_RIFLE,  // ライフル（AKM。町で拾う）

	WEAPON_NUMBER,
	WEAPON_NONE = -1, // 銃ではない物（weaponForItem の答え）
};

// 銃を右手にどう持つか・どこを覗くか・弾がどこから出るか（銃のモデル座標。m。Blender で頂点から求めた値）
struct WeaponGrip{
	GameLib::Math::Vector3 gripOnWeapon; // 銃のモデル上の握りの中心
	GameLib::Math::Vector3 gripInHand;   // 握りの中心を置く、手の骨から見た位置（cm。骨格の単位）
	// 手の骨から見た銃の向き（度）。骨格と同じく Y→X→Z の順に掛ける
	float rotationY;
	float rotationX;
	float rotationZ;
	GameLib::Math::Vector3 sightEye;     // 覗いたときに目を置く点
	GameLib::Math::Vector3 muzzlePoint;  // 銃口の先端（銃口の向きはどの銃も +Z）
};

// リロード 1 種類の時間割（腕を差し替えるアニメと、音の合図の時刻）。合図の時刻はアニメの手の動きに合わせた値なので、
// アニメを作り直したら一緒に直す（拳銃は tools/configs/pistol_reload.json の点の時刻と同じ）
struct WeaponReloadSpec{
	const char* animation;  // 腕を差し替えるアニメ（tools/configs/player.json の名前）
	float animationSeconds; // アニメの使う長さ（頭から。秒）。この後は見せない
	float durationSeconds;  // 込め終えるまでの秒。アニメはこの長さに収まるよう速めて（遅めて）流す
	float magOutSeconds;    // 弾倉を抜く合図（アニメの時刻。秒）
	float magInSeconds;     // 新しい弾倉を挿す合図
	float boltSeconds;      // 槓桿・スライドを引く合図。引かないリロードは kNoBoltCue
};
// WeaponReloadSpec::boltSeconds の「引かない」
const float kNoBoltCue = -1.f;

// 銃 1 種類の決まり
struct WeaponSpec{
	const char* name;         // 物の名前（F6 の物の一覧・デバッグ出力。ゲームの判断には使わない）
	ItemId item;              // カバンに入る銃の物
	ItemId ammo;              // 使う弾（リロードでカバンから込める）
	const char* modelFile;    // 銃の形（弾倉を除いた形。手に持って描く）
	const char* magazineFile; // 弾倉の形（銃と同じ座標。リロードで左手に持つ）
	const char* groundFile;   // 地面に落ちた銃の形（横に寝かせ、原点を一番下の真ん中にした形。拾える物）
	int magazineCapacity;     // 弾倉に入る弾の数
	float roundsPerMinute;    // 連射の速さ（発/分）。セミオートでは、次に撃てるまでの最短の間隔になる
	bool fullAuto;            // true：引いている間撃ち続ける。false：1 回引くと 1 発（離すまで次を撃たない）
	float damage;             // 弾 1 発の、部位の倍率を掛ける前のダメージ
	float recoilScale;        // 視点の跳ね上がり・弾道のずれ・画面の揺れの大きさ（ライフルが 1）
	// 腕のポーズ（tools/configs/player.json のアニメの名前。その 1 フレーム目）。持っている間はいつも腕だけこのポーズに差し替える
	// （脚と体は今のアニメのまま。拳銃は Shooting.fbx の構え）。nullptr なら体のアニメの腕のまま（ライフル）
	const char* holdAnimation;
	// 覗いていない間の腕のポーズ（その 1 フレーム目）。覗くと holdAnimation へ上がる。拳銃は、ライフルの構えの体の上で、ライフルの握りと同じ所・
	// 同じ向きに拳銃を持たせた形（pistol_ready）
	// nullptr なら覗いていない間も holdAnimation のまま。holdAnimation が nullptr の銃では使わない
	const char* readyAnimation;
	// readyAnimation を作ったときの体のアニメ（その 1 フレーム目。拳銃は "rifle_idle"）。今の体のアニメ（歩き・走り）の右手が、
	// これの右手からどれだけ動いたかのとおりに、腕をまとめて動かす（FirstPersonBody::setArmOverride の followReference）
	// → 歩き・走りでも拳銃はライフルと同じ所・同じ向きで持つ。nullptr なら動かさない（胸に付けて写すだけ）
	const char* readyBodyAnimation;
	// 支える手（左手）が銃のどこに来るかを求めるアニメ（その 1 フレーム目で、右手に持った銃から見た左手の位置）
	const char* gripCalibrationAnimation;
	WeaponReloadSpec reload;      // 弾倉に弾が残っているときのリロード
	WeaponReloadSpec emptyReload; // 弾倉が空のときのリロード（拳銃はスライドを引く。タルコフ・実銃と同じく空になるとスライドが後ろで止まるため）
	SoundId shotSound;        // 銃声
	float shotVolume;         // 銃声の大きさ（素材の音量の違いもここで揃える）
	WeaponGrip grip;
};

// 種類 id の決まり。id は WEAPON_NUMBER 未満であること
const WeaponSpec& weaponSpec( WeaponId id );
// 物の種類 item がどの銃か。銃でなければ WEAPON_NONE
WeaponId weaponForItem( ItemId item );

#endif
