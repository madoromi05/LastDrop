#ifndef INCLUDED_GAMELIB_GRAPHICS_RENDERTARGET_H
#define INCLUDED_GAMELIB_GRAPHICS_RENDERTARGET_H

#include "GameLib/Graphics/Enum.h"

namespace GameLib{
namespace Graphics{

///色の描画先。Manager::setRenderTargets で描き込み、Manager::setShaderResource でシェーダから読む。
///同じ RenderTarget を、描き込み先と読み込み元に同時に使うことはできない（GPU が読み込み側を外してしまう）。
class RenderTarget{
public:
	///大きさを画素で決めて作る（影のマップなど、画面と関係ない大きさの物）
	static RenderTarget create( int width, int height, RenderTargetFormat );
	///画面（バックバッファ）の scale 倍の大きさで作る。ウィンドウの大きさが変わると、
	///次に Manager で使われたとき（setRenderTargets / clear / setShaderResource）に作り直す（中身は消える）
	static RenderTarget createScreenSized( float scale, RenderTargetFormat );
	///今の大きさ（画素）。画面倍率で作った物は、まだ作り直されていなければ前の大きさを返す
	int width() const;
	int height() const;
	RenderTargetFormat format() const;
	///今の中身を GPU から読み戻して TGA（24 ビット・アルファなし）に書く。RGBA8 / RGBA8_SRGB の物だけ（数は描画先の 8 ビットのまま）。
	///GPU が描き終わるのを待つので重い。ボタンを押したときなど、1 回きりの書き出しに使う（毎フレーム呼ばない）
	void writeTga( const char* filename ) const;

	//以下ユーザは意識しない関数群
	RenderTarget();
	RenderTarget( const RenderTarget& );
	~RenderTarget();
	operator void*() const;
	RenderTarget& operator=( const RenderTarget& );
	void release();
	bool operator==( const RenderTarget& ) const;
	bool operator!=( const RenderTarget& ) const;

	class Impl;
	RenderTarget( const Impl* );
private:
	friend class Manager;
	Impl* mImpl;
};

} //namespace Graphics
} //namespace GameLib

#endif
