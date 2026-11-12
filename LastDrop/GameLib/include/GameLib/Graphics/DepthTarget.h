#ifndef INCLUDED_GAMELIB_GRAPHICS_DEPTHTARGET_H
#define INCLUDED_GAMELIB_GRAPHICS_DEPTHTARGET_H

namespace GameLib{
namespace Graphics{

///深度の描画先（32bit浮動小数）。Manager::setRenderTargets で深度テストに使い、
///Manager::setShaderResource でシェーダから深度の値（0〜1、float1つ）として読める（影・位置の復元・SSR）
///描き込み先と読み込み元に同時に使うことはできない（読み取り専用で付けたとき＝Manager::setRenderTargets の readOnlyDepth だけは、
///奥行きの判定に使いながら読める）
class DepthTarget{
public:
	///大きさを画素で決めて作る（影のマップなど）
	static DepthTarget create( int width, int height );
	///画面（バックバッファ）の scale 倍の大きさで作る。作り直しの決まりは RenderTarget と同じ
	static DepthTarget createScreenSized( float scale );
	int width() const;
	int height() const;

	//以下ユーザは意識しない関数群
	DepthTarget();
	DepthTarget( const DepthTarget& );
	~DepthTarget();
	operator void*() const;
	DepthTarget& operator=( const DepthTarget& );
	void release();
	bool operator==( const DepthTarget& ) const;
	bool operator!=( const DepthTarget& ) const;

	class Impl;
	DepthTarget( const Impl* );
private:
	friend class Manager;
	Impl* mImpl;
};

} //namespace Graphics
} //namespace GameLib

#endif
