#ifndef INCLUDED_AUDIO_ENGINE_H
#define INCLUDED_AUDIO_ENGINE_H

#include "GameSrc/Audio/SoundCatalog.h"
#include "GameLib/Math/Vector3.h"
#include <memory>

// 音を鳴らす窓口。miniaudio を包み、ゲームコードには SoundId と位置だけを見せる（miniaudio.h はこのクラスの .cpp だけが読む）。
//
// ・作ったときに目録（SoundCatalog）の全ての音を読み込む。読めない音はデバッグ出力で知らせ、鳴らしても無音になるだけ（落ちない）。
// ・音はバス（まとめて音量を変えられる系統）に流す。効果音と環境音を分けておき、後でダッキングや設定画面の音量につなぐ。
// ・位置を渡した音は 3D（リスナーからの向きで左右に振り分け、距離で小さくする）、渡さない音は 2D（頭の中で鳴る）。
// ・揺らぎ（同じ音の別テイク）の選び方と音程の揺らし、鳴らし始める位置（頭の無音を飛ばす）は、目録の値に従ってこのクラスが決める（呼び出し側は気にしない）。
//
// 座標はゲームのワールド座標（m）のまま渡してよい。ゲームも miniaudio も右手系（上 +Y）なので、そのまま向きが合う。
class AudioEngine{
public:
	enum Bus{
		BUS_EFFECT,		// 足音・装備の音など、ゲームの出来事の音
		BUS_AMBIENCE,	// 環境音（マップのイベント。今は脱出のサイレン）
		BUS_NUM,
	};
	// ループの音を指す番号。startLoop が返し、stopLoop などに渡す。
	typedef int LoopHandle;
	static const LoopHandle kInvalidLoop = -1;

	AudioEngine();
	~AudioEngine();
	AudioEngine( const AudioEngine& ) = delete;
	AudioEngine& operator=( const AudioEngine& ) = delete;

	// 音を出せる状態か（出力の装置が無いなどで初期化に失敗したら false。そのときも他の関数は呼んでよく、何もしない）。
	bool isReady() const;

	// 毎フレーム呼ぶ。鳴り終わった1回きりの音の後片付けをする。
	void update();
	// すべての音を止める（true）・続きから鳴らし直す（false）。一時停止の間に使う。止めている間に鳴らした音は、戻したときに鳴り始める。
	void setPaused( bool paused );
	// 全体の音量（出力の直前で掛ける倍率。1 で元の大きさ）。設定画面の Master Volume。
	void setMasterVolume( float gain );
	// バスの音量（そのバスに流す全ての音に掛ける倍率。1 で元の大きさ）。設定画面の Effects Volume など。
	void setBusVolume( Bus bus, float gain );

	// 聞き手（カメラ）の位置と向き（長さ1）。毎フレーム、カメラを置いた後に呼ぶ。
	void setListener( const GameLib::Math::Vector3& position, const GameLib::Math::Vector3& forward );

	// 1回きりの音を position から鳴らす（3D）。volume は 0〜1 目安、pitch は 1 で元の高さ。
	// 同時に鳴らせる数を超えたら鳴らさない（その前の音を止めてまで鳴らすほど大事な音はまだ無いため）。
	void play( SoundId, Bus, const GameLib::Math::Vector3& position, float volume, float pitch = 1.f );
	// 1回きりの音を位置なしで鳴らす（2D）。
	void play2D( SoundId, Bus, float volume, float pitch = 1.f );

	// ループの音を位置なしで鳴らし始める（2D）。鳴らせなければ kInvalidLoop。
	LoopHandle startLoop( SoundId, Bus, float volume );
	// ループの音を position から鳴らし始める（3D）。鳴らせなければ kInvalidLoop。
	LoopHandle startLoopAt( SoundId, Bus, const GameLib::Math::Vector3& position, float volume );
	// 3D のループを動かす。velocity（m/秒）はドップラー効果（近づくと高く、遠ざかると低く聞こえる）に使う。
	void setLoopPosition( LoopHandle, const GameLib::Math::Vector3& position, const GameLib::Math::Vector3& velocity );
	// ループを止めて番号を返す。kInvalidLoop を渡してもよい（何もしない）。
	void stopLoop( LoopHandle );

private:
	class Impl;
	std::unique_ptr< Impl > mImpl;
};

#endif
