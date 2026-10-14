#ifndef INCLUDED_VERTEX_COLOR_H
#define INCLUDED_VERTEX_COLOR_H

// 頂点カラーを GameLib の頂点形式（Graphics::Vertex::mColor）へ詰める。
// このエンジンは R と B が入れ替わる並び（シェーダ側の .zyxw で RGB に戻る）なので、
// 並びの規則はここにだけ書き、SkinnedMesh / StaticMeshData から共通で使う。
namespace VertexColor{
	const unsigned WHITE = 0xffffffffu; // 既定値。テクスチャに乗算しても色を変えない

	// 0..1 の値を 0..255 の1バイトにする。範囲外は丸める。
	inline unsigned toByte( float value ) {
		const float kMax = 255.f;
		return static_cast< unsigned >( ( value < 0.f ? 0.f : ( value > 1.f ? 1.f : value ) ) * kMax );
	}

	// r, g, b, a は 0..1。範囲外は丸める。a は不透明度（1 で不透明。半透明に描くときは描く側で混ぜ方を BLEND_LINEAR にする）。
	inline unsigned pack( float r, float g, float b, float a ) {
		return ( toByte( a ) << 24 ) | ( toByte( b ) << 16 ) | ( toByte( g ) << 8 ) | toByte( r );
	}

	// r, g, b は 0..1。範囲外は丸める。アルファは常に不透明。
	inline unsigned pack( float r, float g, float b ) {
		const float kOpaque = 1.f;
		return pack( r, g, b, kOpaque );
	}
}

#endif
