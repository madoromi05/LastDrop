#ifndef INCLUDED_GAMELIB_GRAPHICS_CONSTANTBUFFER_H
#define INCLUDED_GAMELIB_GRAPHICS_CONSTANTBUFFER_H

namespace GameLib{
namespace Graphics{

///シェーダへ渡す定数の入れ物。Manager::setConstantBuffer で b1 以降に付ける（頂点・ピクセルの両方から読める）
///中身の並びは HLSL の cbuffer と揃えること（float4 単位＝16バイト境界に詰められる規則に注意）
class ConstantBuffer{
public:
	///byteSize: 中身の大きさ。16の倍数に切り上げて作る
	static ConstantBuffer create( int byteSize );
	///中身を書き換える。byteSize は create で渡した大きさ以下。次の描画から使われる
	void set( const void* data, int byteSize );
	int size() const;

	//以下ユーザは意識しない関数群
	ConstantBuffer();
	ConstantBuffer( const ConstantBuffer& );
	~ConstantBuffer();
	operator void*() const;
	ConstantBuffer& operator=( const ConstantBuffer& );
	void release();
	bool operator==( const ConstantBuffer& ) const;
	bool operator!=( const ConstantBuffer& ) const;

	class Impl;
	ConstantBuffer( const Impl* );
private:
	friend class Manager;
	Impl* mImpl;
};

} //namespace Graphics
} //namespace GameLib

#endif
