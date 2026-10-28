# =====================================================================
# texture_to_dds.ps1
#   Resource/ の TGA を、GPU がそのまま読める圧縮形式（BC7 / BC5）の DDS にする。ミップも前もって全段入れる。
#   仕様は SummaryPlan/個別仕様/LinearToneMap_Plan.txt の 8 章（グラフィック段階 1 の 1-b）。
#     色の画像   … BC7_UNORM_SRGB（-srgbi：TGA の数を画像の色として読み、ミップはリニアで平均してもらう）
#     法線マップ … BC5_UNORM（x と y だけ持つ。z はシェーダが作り直す。GameLib の PixelLightingNormalMapPsHlsl.h・Lighting.hlsli）
#   -dx10 で DDS の追加の頭（DX10）を必ず付ける（GameLib の読み込みは DX10 の頭の形式だけを見る）。
#   元の TGA は消さない・縮めない（後から大きさを戻せるように）。DDS は TGA の横に同じ名前で書く。
#
# いつ走らせるか：画像を足した・描き直した（Blender の書き出し、tools/*_gen.py）とき。起動時には走らない。
#   BC7 は重いので、GPU があれば texconv が GPU で圧縮する（無ければ CPU で、4096² 1 枚に数十秒〜）。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   pwsh -File tools/texture_to_dds.ps1            … 下の一覧を全部
#   pwsh -File tools/texture_to_dds.ps1 zombie     … 名前にこの文字を含む物だけ
#
# texconv（Microsoft の DirectXTex に付いているコマンド）はリポジトリに入れない。下の場所に置くか、
# 環境変数 TEXCONV で場所を渡す。https://github.com/microsoft/DirectXTex/releases から texconv.exe を取る。
# =====================================================================
param( [string]$Filter = "" )

$ErrorActionPreference = "Stop"

$TexconvPath = if ( $env:TEXCONV ) { $env:TEXCONV } else { Join-Path $env:USERPROFILE "Tools\DirectXTex\texconv.exe" }
$ResourceDir = "Resource"

# 画像の一覧。Kind は Color（色）か Normal（法線）。MaxSize を書いた物はその大きさまで縮める（0 は元の大きさのまま）。
# ゾンビ（3,610 三角形）の 4096² は大きすぎるので 2048²（Optimization_Plan.txt 7-3 (b)）。
# ほかの 4096²（建物・道路）は画面で見比べてから決める（LinearToneMap_Plan.txt 8-3）。
# HUD・文字の画像（font.tga など）は入れない（画面へそのまま出すので TGA のまま）。
$Textures = @(
	@{ Name = "building_albedo";      Kind = "Color";  MaxSize = 0 }
	@{ Name = "building_normal";      Kind = "Normal"; MaxSize = 0 }
	@{ Name = "bullet_albedo";        Kind = "Color";  MaxSize = 0 }
	@{ Name = "bullet_normal";        Kind = "Normal"; MaxSize = 0 }
	@{ Name = "fuelcan_albedo";       Kind = "Color";  MaxSize = 0 }
	@{ Name = "fuelcan_normal";       Kind = "Normal"; MaxSize = 0 }
	@{ Name = "player_body_albedo";   Kind = "Color";  MaxSize = 0 }
	@{ Name = "player_body_normal";   Kind = "Normal"; MaxSize = 0 }
	@{ Name = "player_body1_albedo";  Kind = "Color";  MaxSize = 0 }
	@{ Name = "player_body1_normal";  Kind = "Normal"; MaxSize = 0 }
	@{ Name = "rifle_albedo";         Kind = "Color";  MaxSize = 0 }
	@{ Name = "rifle_normal";         Kind = "Normal"; MaxSize = 0 }
	@{ Name = "road_albedo";          Kind = "Color";  MaxSize = 0 }
	@{ Name = "uaz_albedo";           Kind = "Color";  MaxSize = 0 }
	@{ Name = "uaz_normal";           Kind = "Normal"; MaxSize = 0 }
	@{ Name = "zombie_albedo";        Kind = "Color";  MaxSize = 2048 }
	@{ Name = "zombie_normal";        Kind = "Normal"; MaxSize = 2048 }
	# エフェクト（tools/*_gen.py で作り直したら、ここも走らせ直す）
	@{ Name = "blood_drop";           Kind = "Color";  MaxSize = 0 }
	@{ Name = "bullet_hole";          Kind = "Color";  MaxSize = 0 }
	@{ Name = "impact_smoke_sheet";   Kind = "Color";  MaxSize = 0 }
	@{ Name = "muzzle_flash_sheet";   Kind = "Color";  MaxSize = 0 }
	@{ Name = "muzzle_glow";          Kind = "Color";  MaxSize = 0 }
)

if ( -not ( Test-Path $TexconvPath ) ) {
	Write-Error "texconv.exe が見つかりません: $TexconvPath （環境変数 TEXCONV で場所を渡せます）"
}

foreach ( $t in $Textures ) {
	if ( $Filter -and -not $t.Name.Contains( $Filter ) ) { continue }
	$source = Join-Path $ResourceDir ( $t.Name + ".tga" )
	if ( -not ( Test-Path $source ) ) {
		Write-Error "元の画像がありません: $source"
	}
	$arguments = @( "-nologo", "-y", "-dx10", "-m", "0", "-o", $ResourceDir )
	if ( $t.Kind -eq "Color" ) {
		$arguments += @( "-f", "BC7_UNORM_SRGB", "-srgbi" )
	} else {
		$arguments += @( "-f", "BC5_UNORM" )
	}
	if ( $t.MaxSize -gt 0 ) {
		$arguments += @( "-w", $t.MaxSize, "-h", $t.MaxSize )
	}
	Write-Host "== $source ($($t.Kind)$( if ( $t.MaxSize -gt 0 ) { ", $($t.MaxSize)" } ))"
	& $TexconvPath @arguments $source
	if ( $LASTEXITCODE -ne 0 ) {
		Write-Error "texconv が失敗しました: $source"
	}
}
