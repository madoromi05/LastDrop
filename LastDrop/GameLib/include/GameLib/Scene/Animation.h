#ifndef INCLUDED_GAMELIB_SCENE_ANIMATION_H
#define INCLUDED_GAMELIB_SCENE_ANIMATION_H

namespace GameLib{
namespace Scene{

///アニメーションデータ
class Animation{
public:
	Animation();
	~Animation();
	Animation( const Animation& );
	operator void*() const;
	Animation& operator=( const Animation& );
	void release();
	bool operator==( const Animation& ) const;
	bool operator!=( const Animation& ) const;
	///1周の長さ（時刻の単位。全カーブの最後のキーの時刻のうち最大）。空なら0。
	///カーブは最後のキーの時刻で折り返してループするので、これがループの周期になる。
	float period() const;

	class Impl;
	explicit Animation( Impl* );
private:
	friend class Tree;
	Impl* mImpl;
};

}
}

#endif
