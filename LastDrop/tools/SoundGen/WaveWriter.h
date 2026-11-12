#ifndef INCLUDED_SOUNDGEN_WAVE_WRITER_H
#define INCLUDED_SOUNDGEN_WAVE_WRITER_H

#include "Synth.h"
#include <string>

// モノラル・16bit・Synth::kSampleRate の WAV ファイルに書き出す。-1〜1 を超えるサンプルは切り詰める
// 書き出せなければ false
bool writeWave( const std::string& path, const Synth::Buffer& buffer );

#endif
