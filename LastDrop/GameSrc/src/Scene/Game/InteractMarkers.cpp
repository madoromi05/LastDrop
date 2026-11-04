#include "GameSrc/Scene/Game/InteractMarkers.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Player/FirstPersonCamera.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/ScreenProjection.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector2.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// マークを出す距離（m）。手が届く 2.5m（GameScene の kInteractReach）より少し遠くから、近づくと物があると気づける長さ（Q1）。
	const float kMarkDistance = 4.f;

	// 絵（tools/interact_mark_gen.py で作る白いひし形。UI の絵なので LINEAR で読む）。
	const char* kMarkTexture = "Resource/ui_interact_mark.tga";

	// 画面の上の大きさ（画素）。遠くても見えるよう、距離では縮めない（ゾンビの頭上の体力のバーと同じ）。
	const float kSmallSize = 14.f;  // 近くの物
	const float kTargetSize = 34.f; // 見ている物（中に「E」が入る大きさ）
	const float kHalf = 0.5f;

	const unsigned kUsableColor = VertexColor::WHITE;
	const unsigned kUnusableColor = VertexColor::pack( 0.45f, 0.45f, 0.45f ); // 使えない物（Q3）
	const unsigned kKeyColor = VertexColor::pack( 0.05f, 0.05f, 0.05f );     // 白いひし形の上の「E」
	const char* kKeyText = "E";

	// center を中心にした size 四方。
	UiRect squareAround( const Vector2& center, float size ) {
		UiRect rect;
		rect.left = center.x - size * kHalf;
		rect.top = center.y - size * kHalf;
		rect.width = size;
		rect.height = size;
		return rect;
	}
}

InteractMarkers::InteractMarkers() :
	mTexture( Graphics::Texture::create( kMarkTexture, Graphics::TEXTURE_COLOR_SPACE_LINEAR ) ),
	mHasTexture( false ) {
}

void InteractMarkers::update( const InteractionSystem& interaction, const Vector3& eye, const IInteractable* target, const Inventory& inventory ) {
	// 読み込みは非同期なので、読めたかを毎回確かめる（isReady は読み込みを進めるので const の add では呼べない）。
	mHasTexture = mTexture.isReady() && !mTexture.isError();
	mNearby.clear();
	mMarks.clear();
	interaction.collectNearby( eye, kMarkDistance, &mNearby );
	for ( const InteractionSystem::NearbyInteractable& nearby : mNearby ) {
		Mark mark;
		mark.position = nearby.markPosition;
		mark.usable = nearby.interactable->canInteract( inventory );
		mark.targeted = ( nearby.interactable == target );
		mMarks.push_back( mark );
	}
	mNearby.clear(); // 物を指す物は残さない（拾われた物は次の世界の update で消える）
}

void InteractMarkers::clear() {
	mMarks.clear();
}

void InteractMarkers::add( const FirstPersonCamera& camera, UiRenderer* renderer ) const {
	ASSERT( renderer && "InteractMarkers::add : renderer is null." );
	Matrix44 projectionView = camera.projectionViewMatrix();
	// 絵が変わるたびに描く回数が増えるので、ひし形を全部積んでから文字を積む（UiRenderer は文字を後でまとめて描く）。
	for ( const Mark& mark : mMarks ) {
		Vector2 screen;
		if ( !ScreenProjection::worldToScreen( projectionView, mark.position, &screen ) ) {
			continue; // カメラの後ろ
		}
		UiRect rect = squareAround( screen, mark.targeted ? kTargetSize : kSmallSize );
		unsigned color = mark.usable ? kUsableColor : kUnusableColor;
		if ( mHasTexture ) {
			renderer->addImage( rect, mTexture, color );
		} else {
			renderer->addRect( rect, color ); // 絵が無いときは四角で（場所だけは分かるように）
		}
		if ( mark.targeted ) {
			renderer->addText( kKeyText, screen, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kKeyColor );
		}
	}
}
