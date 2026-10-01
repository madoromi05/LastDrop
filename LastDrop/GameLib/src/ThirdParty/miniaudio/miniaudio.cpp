// miniaudio（https://github.com/mackron/miniaudio）の実装を、プロジェクトの中でこの1か所だけでコンパイルする。
// 他のファイルは miniaudio.h を宣言として読むだけ（MINIAUDIO_IMPLEMENTATION を付けて2か所で読むと多重定義になる）。
// miniaudio.h はバージョンを固定して同梱している（v0.11.25。更新するときはヘッダごと差し替える）。
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
