#ifndef INCLUDED_MOVE_HEIGHT_H
#define INCLUDED_MOVE_HEIGHT_H

// 移動の当たり判定が見る高さの範囲（地面からの m）。
// 移動は真上から見た 2D で解くので、「どの高さにある物が体をふさぐか」をここ1か所で決める。
// この範囲の外にある物（足元の歩道の板、頭より上の庇や看板）は、移動をふさがない。
namespace MoveHeight{
	// 歩いて越えられる段差の高さ。これ以下の物はふさがない。
	const float kStepOver = 0.3f;
	// 体の高さ。これより上の物はふさがない。
	const float kBodyTop = 1.8f;
}

#endif
