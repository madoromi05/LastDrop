#ifndef INCLUDED_GAME_TIME_H
#define INCLUDED_GAME_TIME_H

// ゲームの時間の単位。ゲームは描画の速さ（モニターのリフレッシュレート）に関係なく、1/60 秒ずつ進む。
// この 1/60 秒を「ティック」と呼ぶ。速さ（m/フレーム）・長さ（〜フレーム）など、ゲームの更新側のコメントにある
// 「フレーム」はこの1ティックのこと（描画の回数ではない）。
// 1回の描画で何ティック進めるかは SceneManager が決める（遅いフレームでは複数回、速いフレームでは0回）。
namespace GameTime{
	// 1秒あたりのティック数。秒で決めた値をティックあたりに直すのに使う（例: 速さ 1.4m/秒 → 1.4 / kTicksPerSecond）。
	const float kTicksPerSecond = 60.f;
	// 1ティックの長さ（秒）。
	const float kTickSeconds = 1.f / kTicksPerSecond;
	// 1回の描画で進めてよいティック数の上限。これを超えた遅れは捨てる（ゲームが少し遅く進む）。
	// 捨てないと、重いフレーム → 更新が増える → さらに重くなる、と遅れが膨らみ続けて止まってしまうため。
	const int kMaxTicksPerFrame = 5;
}

#endif
