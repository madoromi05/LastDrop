#ifndef INCLUDED_WEAPON_RECOIL_H
#define INCLUDED_WEAPON_RECOIL_H

// 銃の反動の量（0〜1）を決める。撃ったティックに一気に 1 へ跳ね、その後は時間で 0 へ戻る。
// 決めるのは量だけ。銃をどう動かすか（どれだけ引く・跳ね上げる）は見た目の係（FirstPersonBody）が決める。
// 視点（照準）と弾の飛ぶ向きには関わらない（こちらは銃の見た目だけ。視点の跳ね上がりは ViewRecoil）。
// 戻る速さを変えるときは WeaponRecoil.cpp の定数だけを直す。
class WeaponRecoil{
public:
	WeaponRecoil();

	// 1ティック進める。fired: このティックに1発撃ったか。
	void update( bool fired );
	// 反動の量（0 = 反動なし、1 = 撃った瞬間）。
	float kick() const;

private:
	float mKick;
};

#endif
