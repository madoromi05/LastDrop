#ifndef INCLUDED_VERTEX_COLOR_H
#define INCLUDED_VERTEX_COLOR_H

// 頂点カラーを GameLib の頂点形式（Graphics::Vertex::mColor）へ詰める。
// このエンジンは R と B が入れ替わる並び（シェーダ側の .zyxw で RGB に戻る）なので、
// 並びの規則はここにだけ書き、SkinnedMesh / StaticMeshData から共通で使う。
namespace VertexColor{
	const unsigned WHITE = 0xffffffffu; // 既定値。テクスチャに乗算しても色を変えない

	// r, g, b は 0..1。範囲外は丸める。アルファは常に不透明。
	inline unsigned pack( float r, float g, float b ) {
		const float kMax = 255.f;
		unsigned rr = static_cast< unsigned >( ( r < 0.f ? 0.f : ( r > 1.f ? 1.f : r ) ) * kMax );
		unsigned gg = static_cast< unsigned >( ( g < 0.f ? 0.f : ( g > 1.f ? 1.f : g ) ) * kMax );
		unsigned bb = static_cast< unsigned >( ( b < 0.f ? 0.f : ( b > 1.f ? 1.f : b ) ) * kMax );
		return 0xff000000u | ( bb << 16 ) | ( gg << 8 ) | rr;
	}
}

#endif
