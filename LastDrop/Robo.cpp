#include "Framework.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"
using namespace GameLib;

#include "Robo.h"
#include "Bullet.h"
#include "Pad.h"
#include "Library/GraphicsDatabase.h"
#include "Library/Model.h"
#include <sstream>
using namespace std;

//�{���̓t�@�C���ɏo���񂾂�H
const int Robo::mJumpUpTime = 20; //�㏸���Ă�������
const int Robo::mJumpStayTime = 60; //�㏸�㉺�~�܂ł̎���
const int Robo::mJumpFallTime = 40; //���~�ɂ����鎞��
const int Robo::mMoveAccelEndCount = 30; //�����n�߂ĉ������I������܂ł̎���
const float Robo::mMaxMoveSpeed = 0.5; //�ő�ړ����x
const float Robo::mJumpHeight = 20.0; //�ő卂�x
const int Robo::mCameraDelayCount = 10; //�W�����v�J�n�㉽�t���[���œG�̕���������
const float Robo::mCameraDistanceZ = 10.0; //�����[�g����납��ʂ��H
const float Robo::mCameraDistanceY = 4.0; //�����낵�
const float Robo::mCameraTargetDistanceZ = 20.0; //�����_�͉����[�g����H
const float Robo::mTurnSpeed = 1.0; //���񑬓x
const int Robo::mMaxHitPoint = 100; //�ő�q�b�g�b�|�C���g
const int Robo::mMaxEnergy = 100; //�ő啐��|�C���g
const int Robo::mEnergyPerBullet = 27; //����̏���G�l���M�[
const int Robo::mEnergyCharge = 1; //���t���[�����܂�G�l���M�[�̗�
const float Robo::mLockOnAngleIn = 10.0; //���b�N�I������p�x
const float Robo::mLockOnAngleOut = 30.0; //���b�N�I�����O���p�x

Robo::Robo( int id ) : 
mPosition( 0.0, 0.0, 0.0 ),
mAngleY( 0.0 ),
mId( id ),
mBulletNumber( 100 ), //���߂���
mCameraCount( 0 ),
mCount( 0 ),
mVelocity( 0.0, 0.0, 0.0 ),
mMode( MODE_ON_LAND ),
mHitPoint( mMaxHitPoint ),
mEnergy( mMaxEnergy ),
mLockOn( true ){ //���������Ďn�܂�̂ōŏ��̓��b�N�I�����ĂĂ������낤�B
	mDatabase = NEW GraphicsDatabase( "robo.txt" );
	mModel = mDatabase->createModel( "robo" );
	mBullets.setSize( mBulletNumber );
}

Robo::~Robo(){
}

void Robo::setPosition( const Vector3& a ){
	mPosition = a;
}

void Robo::setAngleY( float a ){
	mAngleY = a;
}

const Vector3* Robo::getBulletPosition( int i ) const {
	if ( mBullets[ i ].isEmpty() ){
		return 0;
	}else{
		return mBullets[ i ].getPosition();
	}
}

void Robo::update( Robo* enemy ){
	//����ł�
	if ( mHitPoint <= 0 ){
		return;
	}
	//AI�̎v�l�B�v���C���[�̏ꍇ�͓��͂��擾���ĕԂ�����
	bool iJump = false;
	bool iFire = false;
	bool iTurn = false;
	bool iLeft = false;
	bool iRight = false;
	bool iUp = false;
	bool iDown = false;
	think( &iJump, &iFire, &iTurn, &iLeft, &iRight, &iUp, &iDown );
	//�ȉ�����������͂��g���čs��
	const Vector3& enemyPos = *enemy->getPosition();
	++mCount;
	//�W�����v������Ă�H
	float t;
	//�����͂Ɏ��������������Ă݂悤�B�R�[�h�̏d���������邪�u���b�N�P�ʂŌ���΃V���v���ɂȂ�B
	//���ʂ̏������Ƃǂ��炪�ǂ�����ׂĂ݂悤�B
	switch ( mMode ){
		case MODE_JUMP_UP:
			//�J��������肫���Ă��Ȃ��Ȃ�J������]�p��
			if ( mCameraCount < mCameraDelayCount ){
				mAngleY += mAngleVelocityY;
				++mCameraCount;
			}
			//�㏸
			t = mJumpHeight / static_cast< float >( mJumpUpTime );
			mVelocity.y = t;
			if ( !iJump ){ //�W�����v���͂��Ȃ��̂ŉ��~�ɕω�
				mMode = MODE_JUMP_FALL;
				mCount = 0;
			}else if ( mCount >= mJumpUpTime ){ //�㏸�I��
				mMode = MODE_JUMP_STAY; 
				mCount = 0;
			}
			mVelocity.x = mVelocity.z = 0.0; //X,Z�ړ��𖕎E
			break;
		case MODE_JUMP_STAY:
			//�J��������肫���Ă��Ȃ��Ȃ�J������]�p��
			if ( mCameraCount < mCameraDelayCount ){
				mAngleY += mAngleVelocityY;
				++mCameraCount;
			}
			mVelocity.y = 0.0;
			if ( !iJump ){ //�W�����v���͂��Ȃ��̂ŉ��~�ɕω�
				mMode = MODE_JUMP_FALL;
				mCount = 0;
			}else if ( mCount >= mJumpStayTime ){ //���~��
				mMode = MODE_JUMP_FALL;
				mCount = 0;
			}
			break;
		case MODE_JUMP_FALL:
			//�J��������肫���Ă��Ȃ��Ȃ�J������]�p��
			if ( mCameraCount < mCameraDelayCount ){
				mAngleY += mAngleVelocityY;
				++mCameraCount;
			}
			//���~
			t = mJumpHeight / static_cast< float >( mJumpFallTime );
			mVelocity.y = -t;
			//�ڒn����͍ŏI�I�ɂ͏Փˏ����ł��̂ł����ł͂��Ȃ��B
			break;
		case MODE_ON_LAND:
			if ( iJump ){
				mMode = MODE_JUMP_UP;
				mCount = 0;
				mCameraCount = 0;

				//�G�̕��Ɍ�����B
				Vector3 dir;
				dir.setSub( enemyPos, mPosition ); //��������G��
				//Y���p�x��atan2( x, z )
				t = GameLib::Math::atan2( dir.x, dir.z );
				//180�x�ȏ㍷�������+-360�x���ċt��
				if ( t - mAngleY > 180.f ){
					t -= 360.f;
				}else if ( mAngleY - t > 180.f ){
					t += 360.f;
				}
				mAngleVelocityY = ( t - mAngleY ) / static_cast< float >( mCameraDelayCount );
			}else if ( iTurn ){
				turn( iLeft, iRight ); //�R�[�h�������Ȃ�̂Ŋ֐��ɔ�΂�
			}else{
				move( iLeft, iRight, iUp, iDown ); //�R�[�h�������Ȃ�̂Ŋ֐��ɔ�΂�
			}
			mVelocity.y = 0.f;
			break;
	}
	//�������牺�͏Փˏ���������Ƃ��̌�ɂȂ�B
	mPosition += mVelocity;
	if ( mPosition.y < 0.f ){
		mPosition.y = 0.f;
		mMode = MODE_ON_LAND;
	}
	//���퐶��
	if ( iFire ){
		//�㏸�A���~���͌��ĂȂ�
		if ( ( mMode != MODE_JUMP_FALL ) && ( mMode != MODE_JUMP_UP ) ){
			//�G�l���M�[����Ă�H
			if ( mEnergy >= mEnergyPerBullet ){
				//�󂫕����T��
				for ( int i = 0; i < mBulletNumber; ++i ){
					if ( mBullets[ i ].isEmpty() ){
						const char* name = ( mId == 0 ) ? "bulletB" : "bulletR";
						mBullets[ i ].create(
							mDatabase, 
							name, 
							mPosition,
							15.f,
							mAngleY,
							mLockOn );
						mEnergy -= mEnergyPerBullet;
						break;
					}
				}
			}
		}
	}
	//����X�V
	for ( int i = 0; i < mBulletNumber; ++i ){
		if ( !mBullets[ i ].isEmpty() ){
			mBullets[ i ].update( enemyPos );
			//�Փˏ���
			Vector3 t;
			t.setSub( *mBullets[ i ].getPosition(), enemyPos );
			if ( t.squareLength() < 4.f ){
				enemy->setDamage( 1 ); //1�_���炵�Ă݂��B
				mBullets[ i ].die(); //�e�����܂��B
			}
		}
	}
	//����G�l���M�[�`���[�W
	mEnergy += mEnergyCharge;
	if ( mEnergy > mMaxEnergy ){
		mEnergy = mMaxEnergy;
	}
	//���b�N�I������
	//�܂��͊p�x�𑪂��Ă݂悤�B
	//�p�x�͉��ő��邩�H�����A���ς��B
	Vector3 toEnemy;
	toEnemy.setSub( enemyPos, mPosition );
	Vector3 myDir( 0.f, 0.f, -1.f );
	Matrix34 m;
	m.setRotationY( mAngleY + 180.f );
	m.mul( &myDir, myDir );
	toEnemy *= 1.f / toEnemy.length(); //������1��
	float dotProduct = toEnemy.dot( myDir );
	if ( dotProduct > 1.f ){ //�덷�̂�����1���z���邩������Ȃ��B
		dotProduct = 1.f;
	}else if ( dotProduct < -1.f ){
		dotProduct = -1.f;
	}
	//�p�x�ɒ����ƁA
	float angle = GameLib::Math::acos( dotProduct );
	if ( mLockOn ){
		//���b�N�I�����Ă�Ȃ�O��邩�ǂ������ׂ�
		if ( angle > mLockOnAngleOut ){
			mLockOn = false;
		}
	}else{
		//���邩�ǂ������ׂ�
		if ( angle < mLockOnAngleIn ){
			mLockOn = true;
		}
	}
	//�Ō�̏�Ԃ�Model�ɃZ�b�g
	mModel->setAngle( Vector3( 0.f, mAngleY, 0.f ) );
	mModel->setPosition( mPosition );
}

void Robo::think(
bool* jump,
bool* fire,
bool* turn,
bool* left,
bool* right,
bool* up,
bool* down ) const {
	Pad* pad = Pad::instance();
	if ( mId == 0 ){ //�v���C���[
		*jump = pad->isOn( Pad::JUMP, mId );
		*fire = pad->isTriggered( Pad::FIRE, mId );
		*turn = pad->isOn( Pad::TURN, mId );
		*left = pad->isOn( Pad::LEFT, mId );
		*right = pad->isOn( Pad::RIGHT, mId );
		*up = pad->isOn( Pad::UP, mId );
		*down = pad->isOn( Pad::DOWN, mId );
	}else{ //AI
		//���n��
		//���b�N�I�����ĂȂ���΃W�����v����
		*jump = !mLockOn;
		//�e�͌��Ă邾�����B
		*fire = true;
		//����͂��Ȃ�
		*turn = false;
		//�ړ��͕ϐ������̂��ʓ|�Ȃ̂ł��Ȃ��B
		*left = *right = *up = *down = false;
	}
}

void Robo::turn( bool left, bool right ){
	if ( left ){
		mAngleY += mTurnSpeed;
		if ( mAngleY > 180.f ){ //-PI����PI�ɂ����߂�
			mAngleY -= 360.f;
		}
	}
	if ( right ){
		mAngleY -= mTurnSpeed;
		if ( mAngleY < -180.f ){ //-PI����PI�ɂ����߂�
			mAngleY += 360.0;
		}
	}
}

void Robo::move( bool left, bool right, bool up, bool down ){
	//�ړ������B�܂����_���l�����Ȃ������x���o��
	Vector3 move( 0.f, 0.f, 0.f );
	if ( up ){
		move.z = -1.f;
	}
	if ( down ){
		move.z = 1.f;
	}
	if ( left ){
		move.x = -1.f;
	}
	if ( right ){
		move.x = 1.f;
	}
	//�����������������ĉ�]
	Matrix34 m;
	m.setRotationY( mAngleY + 180.f );
	m.mul( &move, move );

	//���~�܂��Ă���Ȃ�b�͑����B�K���ɉ������Ă�邾����
	if ( mVelocity.x == 0.f && mVelocity.z == 0.f ){
		//�����ɂ����鎞�Ԃōő呬�x�������1�t���[��������̉����x���o��B
		float accel = mMaxMoveSpeed / static_cast< float >( mMoveAccelEndCount );
		mVelocity.setMul( move, accel );
	}else{ //���łɓ����Ă���ꍇ
		if ( move.x == 0.f && move.z == 0.f ){ //�ړ����[��
			mVelocity.set( 0.f, 0.f, 0.f ); //�ړ��͂Ƃ܂�B
		}else{
			//���łɓ����Ă���ꍇ���Ȃ�ʓ|�ł���B
			//45�x���������]���������Ƀ[������������Ȃ����Ƃ����̂̓X�g���X���B
			//������A�u���̑��x�ƕ���������Ȃ������������[�������蒼���v�ɂ���B

			//90�x�ȏ�̃^�[���Ȃ��U���x��0�ɂ���B
			//�������������������Q�[�������邪�A���т��ѓ����������Ȃ犵���͎ז����낤�B
			//90�x�ȏ�̃^�[���Ȃ猻���x�Ɖ����̓��ς��}�C�i�X�̂͂���
			float dp = mVelocity.dot( move );
			if ( dp <= 0.f ){
				mVelocity.set( 0.f, 0.f, 0.f );
			}else{ //90�x����
				//���݂̈ړ����x�Ɛ����Ȑ����̂ݎ��o��
				//���������́A�ړ������P�ʃx�N�^�Ƃ̓��ς��A�ړ������P�ʃx�N�^�ɂ�����΂����B
				//�ړ��P�ʃx�N�^E�A�����x�x�N�^V�Ƃ��āA�V�������x�x�N�^V'���Ȃ킿���s������
				//V' = dot(V,E) * E
				//���̎��AE�͈ړ��x�N�^M���g����E=M/|M|�Ə����邩��A
				//V' = dot(V,M) * M / ( |M|^2 )
				//�����A�P�ʃx�N�^�����ۂ̕�������������B|M|���|M|^2�̕����v�Z�͑����̂��B
				float moveSquareLength = move.x * move.x + move.z * move.z;
				float dp = mVelocity.dot( move );
				mVelocity.setMul( move, dp / moveSquareLength );
			}
			//������������B
			//�ړ����x�͍ő呬�x/�������Ԃł���B
			float accel = mMaxMoveSpeed / static_cast< float >( mMoveAccelEndCount );
			mVelocity.madd( move, accel );
			//�ő呬�x�ŃX�g�b�v
			float speed = mVelocity.length();
			if ( speed > mMaxMoveSpeed ){
				mVelocity *= mMaxMoveSpeed / speed;
			}
		}
	}
}

void Robo::draw() const {
	//�`��
	mModel->draw();
	//����
	for ( int i = 0; i < mBulletNumber; ++i ){
		if ( !mBullets[ i ].isEmpty() ){
			mBullets[ i ].draw();
		}
	}
}

const Vector3* Robo::getPosition() const {
	return &mPosition;
}

void Robo::setDamage( int d ){
	mHitPoint -= d;
}

int Robo::getHitPoint() const {
	return mHitPoint;
}

int Robo::getEnergy() const {
	return mEnergy;
}

bool Robo::getLockOn() const {
	return mLockOn;
}

void Robo::getView( Matrix34* vm, Vector3* pos ) const {
	//�܂����ʕ����x�N�^���쐬
	Vector3 d( 0.f, 0.f, 1.f );
	Matrix34 m;
	m.setRotationY( mAngleY );
	m.mul( &d, d );
	//������O����mCameraTargetDistanceZ�����L�΂�
	Vector3 t;
	t.setMul( d, mCameraTargetDistanceZ );
	//���{�������Ƃ���ɂ���Ȃ炿����Ɖ������Ă��B����̓p�����[�^�ɂȂ����̏�H�v�B
	t.y -= mPosition.y * 0.12f; //���̂ւ�̒������K��
	//�����������mCameraDistacneZ�����L�΂�
	Vector3 p;
	p.setMul( d, -mCameraDistanceZ );
	//Y��mCameraDistanceY���v���X
	p.y += mCameraDistanceY;
	//���{�������Ƃ���ɂ���Ȃ炿����ƍ��ڂɂ��ĉ������Ă��B����̓p�����[�^�ɂȂ����̏�H�v�B
	p.y += mPosition.y * 0.12f; //���̂ւ�̒������K��
	//���{���݈ʒu���v���X
	t += mPosition;
	p += mPosition;
	//�r���[�s��쐬
	vm->setViewTransform( p, t, Vector3( 0.f, 1.f, 0.f ) );
	*pos = p;
}
