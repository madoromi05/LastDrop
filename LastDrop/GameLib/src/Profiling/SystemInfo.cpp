#ifdef ZF_EDITOR

#include <d3d11.h>
#include <dxgi.h>
#include <intrin.h>
#undef max
#undef min

#include "GameLib/GameLib.h"
#include "Profiling/SystemInfo.h"
#include "Profiling/JsonWriter.h"
#include "Profiling/OutputFiles.h"
#include "Profiling/FrameTimeline.h"
#include "Graphics/ManagerImpl.h"

#include <cstdio>
#include <string>
#include <vector>

namespace GameLib{
namespace Profiling{
namespace SystemInfo{

namespace{
	const int kCpuBrandLeafCount = 3;              // ブランド文字列は 3 つの CPUID に 16 文字ずつ入っている
	const unsigned kCpuBrandFirstLeaf = 0x80000002;
	const unsigned kCpuExtendedLeafQuery = 0x80000000;
	const int kCpuRegisterCount = 4;
	const double kBytesPerMegabyte = 1024.0 * 1024.0;
	const long long kMillisecondsPerSecond = 1000;
	const BYTE kAcLineOnline = 1;
	const BYTE kAcLineOffline = 0;
	const BYTE kBatteryPercentUnknown = 255;
	const int kTextBufferSize = 64;
	const int kDriverVersionPartBits = 16;
	const int kDriverVersionPartMask = 0xffff;

#if defined( _DEBUG )
	const char* const kBuildConfiguration = "Debug";
#else
	const char* const kBuildConfiguration = "Profile";
#endif

	std::string cpuBrand(){
		int registers[ kCpuRegisterCount ] = {};
		__cpuid( registers, kCpuExtendedLeafQuery );
		unsigned maxLeaf = static_cast< unsigned >( registers[ 0 ] );
		if ( maxLeaf < kCpuBrandFirstLeaf + kCpuBrandLeafCount - 1 ){
			return "";
		}
		char brand[ kCpuBrandLeafCount * sizeof( registers ) + 1 ] = {};
		for ( int i = 0; i < kCpuBrandLeafCount; ++i ){
			__cpuid( registers, static_cast< int >( kCpuBrandFirstLeaf + i ) );
			memcpy( brand + i * sizeof( registers ), registers, sizeof( registers ) );
		}
		// 前に空白が詰めてある CPU があるので落とす
		std::string text( brand );
		size_t first = text.find_first_not_of( ' ' );
		return ( first == std::string::npos ) ? "" : text.substr( first );
	}

	int physicalCoreCount(){
		DWORD length = 0;
		GetLogicalProcessorInformationEx( RelationProcessorCore, nullptr, &length );
		if ( length == 0 ){
			return 0;
		}
		std::vector< char > buffer( length );
		auto* info = reinterpret_cast< SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* >( buffer.data() );
		if ( !GetLogicalProcessorInformationEx( RelationProcessorCore, info, &length ) ){
			return 0;
		}
		// 1 コアにつき 1 件。件ごとに長さが違うので、Size ずつ進めて数える
		int cores = 0;
		for ( DWORD offset = 0; offset < length; ){
			auto* entry = reinterpret_cast< SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* >( buffer.data() + offset );
			++cores;
			offset += entry->Size;
		}
		return cores;
	}

	// GetVersionEx は互換のため古い番号を返すことがあるので、ntdll の RtlGetVersion で読む
	std::string osVersion(){
		typedef LONG ( WINAPI *RtlGetVersionFunction )( OSVERSIONINFOW* );
		HMODULE ntdll = GetModuleHandleA( "ntdll.dll" );
		if ( !ntdll ){
			return "";
		}
		auto rtlGetVersion = reinterpret_cast< RtlGetVersionFunction >( GetProcAddress( ntdll, "RtlGetVersion" ) );
		if ( !rtlGetVersion ){
			return "";
		}
		OSVERSIONINFOW info = {};
		info.dwOSVersionInfoSize = sizeof( info );
		if ( rtlGetVersion( &info ) != 0 ){
			return "";
		}
		char buffer[ kTextBufferSize ];
		snprintf( buffer, sizeof( buffer ), "%lu.%lu.%lu", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber );
		return buffer;
	}

	std::string toUtf8( const wchar_t* text ){
		int size = WideCharToMultiByte( CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr );
		if ( size <= 0 ){
			return "";
		}
		std::string result( static_cast< size_t >( size ), '\0' );
		WideCharToMultiByte( CP_UTF8, 0, text, -1, &result[ 0 ], size, nullptr, nullptr );
		result.resize( static_cast< size_t >( size - 1 ) ); // 終わりの '\0' を落とす
		return result;
	}

	// 実際に描画に使っているデバイスの GPU を書く（デバイスを作るときにアダプタを指定していないので、
	// どの GPU が選ばれたかはここで初めて分かる）
	void writeGpu( JsonWriter* json ){
		Graphics::ManagerImpl* graphics = Graphics::gManagerImpl;
		if ( !graphics || !graphics->dxDevice() ){
			return;
		}
		IDXGIDevice* dxgiDevice = nullptr;
		if ( FAILED( graphics->dxDevice()->QueryInterface( __uuidof( IDXGIDevice ), reinterpret_cast< void** >( &dxgiDevice ) ) ) ){
			return;
		}
		IDXGIAdapter* adapter = nullptr;
		if ( SUCCEEDED( dxgiDevice->GetAdapter( &adapter ) ) ){
			DXGI_ADAPTER_DESC desc = {};
			if ( SUCCEEDED( adapter->GetDesc( &desc ) ) ){
				json->string( "gpu", toUtf8( desc.Description ) );
				json->number( "gpuDedicatedMemoryMB", static_cast< double >( desc.DedicatedVideoMemory ) / kBytesPerMegabyte );
				json->integer( "gpuVendorId", static_cast< long long >( desc.VendorId ) );
			}
			LARGE_INTEGER umd = {};
			if ( SUCCEEDED( adapter->CheckInterfaceSupport( __uuidof( IDXGIDevice ), &umd ) ) ){
				// 64 ビットに 16 ビットずつ 4 つの番号が入っている（例：31.0.15.5244）
				long long v = umd.QuadPart;
				char buffer[ kTextBufferSize ];
				snprintf( buffer, sizeof( buffer ), "%lld.%lld.%lld.%lld",
					( v >> ( kDriverVersionPartBits * 3 ) ) & kDriverVersionPartMask,
					( v >> ( kDriverVersionPartBits * 2 ) ) & kDriverVersionPartMask,
					( v >> kDriverVersionPartBits ) & kDriverVersionPartMask,
					v & kDriverVersionPartMask );
				json->string( "gpuDriverVersion", buffer );
			}
			adapter->Release();
		}
		dxgiDevice->Release();
		json->integer( "backBufferWidth", graphics->mBackBufferWidth );
		json->integer( "backBufferHeight", graphics->mBackBufferHeight );
		json->boolean( "vsync", graphics->mVSync );
	}

	void writeDisplay( JsonWriter* json ){
		DEVMODEA mode = {};
		mode.dmSize = sizeof( mode );
		if ( EnumDisplaySettingsA( nullptr, ENUM_CURRENT_SETTINGS, &mode ) ){
			json->integer( "displayWidth", static_cast< long long >( mode.dmPelsWidth ) );
			json->integer( "displayHeight", static_cast< long long >( mode.dmPelsHeight ) );
			json->integer( "displayRefreshHz", static_cast< long long >( mode.dmDisplayFrequency ) );
		}
	}

	void writePower( JsonWriter* json ){
		SYSTEM_POWER_STATUS power = {};
		if ( !GetSystemPowerStatus( &power ) ){
			return;
		}
		const char* source = "unknown";
		if ( power.ACLineStatus == kAcLineOnline ){
			source = "ac";
		}else if ( power.ACLineStatus == kAcLineOffline ){
			source = "battery";
		}
		json->string( "powerSource", source );
		if ( power.BatteryLifePercent != kBatteryPercentUnknown ){
			json->integer( "batteryPercent", power.BatteryLifePercent );
		}
	}
}

void write( const char* key, JsonWriter* json ){
	json->beginObject( key );
	json->string( "measuredAt", OutputFiles::timestamp() );
	json->string( "buildConfiguration", kBuildConfiguration );
	json->number( "requiredFps", kRequiredFps );
	json->number( "goalFps", kGoalFps );
	json->string( "cpu", cpuBrand() );
	json->integer( "cpuCores", physicalCoreCount() );
	json->integer( "cpuThreads", static_cast< long long >( GetActiveProcessorCount( ALL_PROCESSOR_GROUPS ) ) );
	MEMORYSTATUSEX memory = {};
	memory.dwLength = sizeof( memory );
	if ( GlobalMemoryStatusEx( &memory ) ){
		json->number( "memoryMB", static_cast< double >( memory.ullTotalPhys ) / kBytesPerMegabyte );
	}
	writeGpu( json );
	writeDisplay( json );
	json->string( "os", osVersion() );
	writePower( json );
	// 再起動してからの時間。短ければ、ファイルを OS がまだ覚えていない（ロードが遅く出る）状態で測ったと分かる
	json->integer( "systemUptimeSeconds", static_cast< long long >( GetTickCount64() ) / kMillisecondsPerSecond );
	json->endObject();
}

} //namespace SystemInfo
} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
