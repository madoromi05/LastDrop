#include "Framework.h"
#include "Target.h"
#include "Library/GraphicsDatabase.h"
#include "Library/Model.h"

const float Target::mRadius = 3.0f;      // 弾・ワイヤーが当たる球の半径。見た目の箱より少し大きめに取る。
const int   Target::mMaxHitPoint = 100;  // 最大体力。

Target::Target( const Vector3& position ) :
	mPosition( position ),
	mHitPoint( mMaxHitPoint ) {
	// ダミーは専用モデルを持たず、既存の robo モデルを流用して見た目を用意する。
	mDatabase = NEW GraphicsDatabase( "Resource/robo.txt" );
	mModel = mDatabase->createModel( "robo" );
	mModel->setScale( Vector3( 3.0f, 3.0f, 3.0f ) ); // ダミーだと分かるよう大きめに表示する。
	mModel->setPosition( mPosition );
}

Target::~Target() {
}

void Target::draw() const {
	mModel->draw();
}

const Vector3* Target::getPosition() const {
	return &mPosition;
}

float Target::getRadius() const {
	return mRadius;
}

void Target::setDamage( int damage ) {
	mHitPoint -= damage;
	if (mHitPoint < 0) {
		mHitPoint = 0;
	}
}

int Target::getHitPoint() const {
	return mHitPoint;
}

bool Target::isDown() const {
	return mHitPoint <= 0;
}
