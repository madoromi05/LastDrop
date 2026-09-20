#ifndef INCLUDED_ISCENE_H
#define INCLUDED_ISCENE_H

// シーン共通インターフェース。1フレームの更新＋描画を行う。
// 注意: GameLib::Scene 名前空間と名前衝突しないよう、あえて IScene とする。
class IScene{
public:
	virtual ~IScene(){}
	virtual void update() = 0;
};

#endif
