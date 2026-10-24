#ifndef INCLUDED_GAMELIB_PROFILING_SYSTEM_INFO_H
#define INCLUDED_GAMELIB_PROFILING_SYSTEM_INFO_H

#ifdef ZF_EDITOR

namespace GameLib{
namespace Profiling{

class JsonWriter;

// 計測した PC の情報と、計測の条件（計画書 6-5）。CSV・ロード・スパイクの JSON に一緒に書く。
// 「実際に使っている GPU」を書くので、ノート PC で内蔵 GPU が選ばれていても気づける。
// Graphics::Manager ができた後に呼ぶ（GPU とバックバッファの大きさを読むため。できる前は GPU の項目を書かない）。
namespace SystemInfo{

// key の名前で、情報のオブジェクトを 1 つ書く。
void write( const char* key, JsonWriter* json );

} //namespace SystemInfo

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
