# =====================================================================
# texture_to_dds.ps1
#   Resource/Texture/<用途>/ の TGA を、GPU がそのまま読める圧縮形式（BC7 / BC5）の DDS にする。ミップも前もって全段入れる。
#   仕様は SummaryPlan/個別仕様/グラフィック/LinearToneMap_Plan.txt の 8 章（グラフィック段階 1 の 1-b）。
#     色の画像   … BC7_UNORM_SRGB（-srgbi：TGA の数を画像の色として読み、ミップはリニアで平均してもらう）
#     法線マップ … BC5_UNORM（x と y だけ持つ。z はシェーダが作り直す。GameLib の PixelLightingNormalMapPsHlsl.h・Lighting.hlsli）
#     数の画像   … BC7_UNORM（色ではない数をそのまま。材質の画像 ORM・水たまりの模様。SsrDecalMaterial_Plan.txt 5・6 章）
#   -dx10 で DDS の追加の頭（DX10）を必ず付ける（GameLib の読み込みは DX10 の頭の形式だけを見る）。
#   DDS は TGA の横に同じ名前で書く。TGA は作る途中の物なので Resource には残さない（2026-11-08 に全部消した。
#   .gitignore で Resource/Texture の TGA は入らない）。作り直すときは Blender の書き出し・tools/*/*_gen.py で TGA を出し直してから走らせる。
#
# いつ走らせるか：画像を足した・描き直した（Blender の書き出し、tools/*/*_gen.py）とき。起動時には走らない。
#   BC7 は重いので、GPU があれば texconv が GPU で圧縮する（無ければ CPU で、4096² 1 枚に数十秒〜）。
#
# 使い方（作業フォルダはプロジェクトの LastDrop）:
#   pwsh -File tools/convert/texture_to_dds.ps1            … 下の一覧を全部
#   pwsh -File tools/convert/texture_to_dds.ps1 zombie     … 名前にこの文字を含む物だけ
#
# texconv（Microsoft の DirectXTex に付いているコマンド）はリポジトリに入れない。下の場所に置くか、
# 環境変数 TEXCONV で場所を渡す。https://github.com/microsoft/DirectXTex/releases から texconv.exe を取る。
# =====================================================================
param( [string]$Filter = "" )

$ErrorActionPreference = "Stop"

$TexconvPath = if ( $env:TEXCONV ) { $env:TEXCONV } else { Join-Path $env:USERPROFILE "Tools\DirectXTex\texconv.exe" }
$TextureDir = "Resource/Texture" # この下の用途ごとのフォルダ（Character・Weapon・Prop・Map・Effect）から名前で探す

# 画像の一覧。Kind は Color（色）・Normal（法線）・Data（数）。MaxSize を書いた物はその大きさまで縮める（0 は元の大きさのまま）。
# ゾンビ（3,610 三角形）の 4096² は大きすぎるので 2048²（Optimization_Plan.txt 7-3 (b)）。
# ほかの 4096²（建物・道路）は画面で見比べてから決める（LinearToneMap_Plan.txt 8-3）。
# HUD・文字の画像（font.tga など）は入れない（画面へそのまま出すので TGA のまま）。
$Textures = @(
	@{ Name = "building_albedo";      Kind = "Color";  MaxSize = 0 }
	@{ Name = "building_normal";      Kind = "Normal"; MaxSize = 0 }
	@{ Name = "building_orm";         Kind = "Data";   MaxSize = 0 }  # 段階11 の ORM は tools/convert/orm_gen.py で作る（MaterialTexture_Plan.txt）
	@{ Name = "bullet_albedo";        Kind = "Color";  MaxSize = 0 }
	@{ Name = "bullet_normal";        Kind = "Normal"; MaxSize = 0 }
	@{ Name = "bullet_orm";           Kind = "Data";   MaxSize = 0 }
	@{ Name = "bullet9_albedo";       Kind = "Color";  MaxSize = 0 }  # ゾンビが落とす 9mm の弾（Pistol_Plan.txt）
	@{ Name = "bullet9_normal";       Kind = "Normal"; MaxSize = 0 }
	@{ Name = "bullet9_orm";          Kind = "Data";   MaxSize = 0 }
	@{ Name = "fuelcan_albedo";       Kind = "Color";  MaxSize = 0 }
	@{ Name = "fuelcan_normal";       Kind = "Normal"; MaxSize = 0 }
	@{ Name = "fuelcan_orm";          Kind = "Data";   MaxSize = 0 }
	@{ Name = "fire_hydrant_albedo";  Kind = "Color";  MaxSize = 0 }
	@{ Name = "fire_hydrant_normal";  Kind = "Normal"; MaxSize = 0 }
	@{ Name = "fire_hydrant_orm";     Kind = "Data";   MaxSize = 0 }
	@{ Name = "loot_props_albedo";    Kind = "Color";  MaxSize = 0 }  # ゴミ箱・木箱・包帯・腕時計・ノートパソコン（tools/modelgen/loot_prop_gen.py。R3_Plan.txt Q2）
	@{ Name = "loot_props_orm";       Kind = "Data";   MaxSize = 0 }
	@{ Name = "player_body_albedo";   Kind = "Color";  MaxSize = 0 }
	@{ Name = "player_body_normal";   Kind = "Normal"; MaxSize = 0 }
	@{ Name = "player_body_orm";      Kind = "Data";   MaxSize = 0 }
	@{ Name = "player_body1_albedo";  Kind = "Color";  MaxSize = 0 }
	@{ Name = "player_body1_normal";  Kind = "Normal"; MaxSize = 0 }
	@{ Name = "player_body1_orm";     Kind = "Data";   MaxSize = 0 }
	@{ Name = "pistol_albedo";        Kind = "Color";  MaxSize = 0 }  # 拳銃（G2-Combat.fbx。Pistol_Plan.txt）
	@{ Name = "pistol_normal";        Kind = "Normal"; MaxSize = 0 }
	@{ Name = "pistol_orm";           Kind = "Data";   MaxSize = 0 }
	@{ Name = "rifle_albedo";         Kind = "Color";  MaxSize = 0 }
	@{ Name = "rifle_normal";         Kind = "Normal"; MaxSize = 0 }
	@{ Name = "rifle_orm";            Kind = "Data";   MaxSize = 0 }
	@{ Name = "road_albedo";          Kind = "Color";  MaxSize = 0 }
	@{ Name = "road_normal";          Kind = "Normal"; MaxSize = 0 }
	@{ Name = "road_orm";             Kind = "Data";   MaxSize = 0 }
	@{ Name = "uaz_albedo";           Kind = "Color";  MaxSize = 0 }
	@{ Name = "uaz_normal";           Kind = "Normal"; MaxSize = 0 }
	@{ Name = "uaz_orm";              Kind = "Data";   MaxSize = 0 }
	@{ Name = "zombie_albedo";        Kind = "Color";  MaxSize = 2048 }
	@{ Name = "zombie_normal";        Kind = "Normal"; MaxSize = 2048 }
	@{ Name = "zombie_orm";           Kind = "Data";   MaxSize = 2048 }
	# エフェクト（tools/*/*_gen.py で作り直したら、ここも走らせ直す）
	@{ Name = "blood_drop";           Kind = "Color";  MaxSize = 0 }
	@{ Name = "blood_stain";          Kind = "Color";  MaxSize = 0 }  # 血の跡のデカール（BloodStain_Plan.txt 5 章）
	@{ Name = "blood_stain_normal";   Kind = "Normal"; MaxSize = 0 }
	@{ Name = "bullet_hole";          Kind = "Color";  MaxSize = 0 }
	@{ Name = "bullet_hole_normal";   Kind = "Normal"; MaxSize = 0 }  # デカールの凹み（SsrDecalMaterial_Plan.txt 8-4）
	@{ Name = "impact_smoke_sheet";   Kind = "Color";  MaxSize = 0 }
	@{ Name = "muzzle_flash_sheet";   Kind = "Color";  MaxSize = 0 }
	@{ Name = "muzzle_glow";          Kind = "Color";  MaxSize = 0 }
	@{ Name = "puddle_mask";          Kind = "Data";   MaxSize = 0 }  # tools/texgen/puddle_gen.py
	@{ Name = "moon";                 Kind = "Color";  MaxSize = 0 }  # tools/texgen/moon_gen.py（夜空の月。MoonStarsLut_Plan.txt 4 章）
)

if ( -not ( Test-Path $TexconvPath ) ) {
	Write-Error "texconv.exe が見つかりません: $TexconvPath （環境変数 TEXCONV で場所を渡せます）"
}

foreach ( $t in $Textures ) {
	if ( $Filter -and -not $t.Name.Contains( $Filter ) ) { continue }
	$found = Get-ChildItem -Path $TextureDir -Recurse -File -Filter ( $t.Name + ".tga" ) | Select-Object -First 1
	if ( -not $found ) {
		Write-Error "元の画像がありません: $TextureDir/*/$( $t.Name ).tga"
	}
	$source = $found.FullName
	$arguments = @( "-nologo", "-y", "-dx10", "-m", "0", "-o", $found.DirectoryName )
	if ( $t.Kind -eq "Color" ) {
		$arguments += @( "-f", "BC7_UNORM_SRGB", "-srgbi" )
	} elseif ( $t.Kind -eq "Data" ) {
		$arguments += @( "-f", "BC7_UNORM" )
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
