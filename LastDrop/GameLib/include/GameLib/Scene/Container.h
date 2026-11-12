#ifndef INCLUDED_GAMELIB_SCENE_CONTAINER_H
#define INCLUDED_GAMELIB_SCENE_CONTAINER_H

namespace GameLib{

namespace PseudoXml{
	class ConstElement;
};

namespace Scene{

class Batch;
class Model;
class Tree;
class Animation;

///描画データを入れるコンテナ
/*!
createModel()して、Model::draw()すればいい。2行で絵が出る
*/
class Container{
public:
	static Container create( PseudoXml::ConstElement, const char* extraDataPath = 0 );	///ニセxmlから生成。追加データパスは省略されればexeの場所になる
	static Container create( const char* filename, const char* extraDataPath = 0 );		///ファイルから直接生成。追加データパスがなければファイルと同じ場所を使う
	Model createModel( const char* batchName ) const;									///バッチ名を指定してモデルを作る
	Model createModel( int batchId ) const;												///バッチ番号を指定してモデルを作る
	Tree createTree( const char* treeName ) const;										///ツリー名を指定してツリーを作る
	Tree createTree( int treeId ) const;												///ツリーの番号を指定してツリーを作る

	bool isReady();	///ロード終了検知。必ず呼んでtrueが返ることを確認してから使うこと

	Batch batch( const char* batchName ) const;
	Batch batch( int batchIndex ) const;
	Animation animation( const char* animationName ) const;
	Animation animation( int animationIndex ) const;
	int batchNumber() const;		//Batch数がわかる。入っている全部のものを出したい時にはこれとgetBatch(int)を併用すれば良い
	int treeNumber() const;			//Tree数がわかる。入っている全部のものを出したい時にはこれとcreateTree(int)を併用すれば良い
	int animationNumber() const;	//アニメーション数がわかる

	//以下ユーザは意識しなくていい
	Container();
	~Container();
	Container( const Container& );
	operator void*() const;
	Container& operator=( const Container& );
	void release();
	bool operator==( const Container& ) const;
	bool operator!=( const Container& ) const;

	class Impl;
private:
	explicit Container( Impl* );
	Impl* mImpl;
};

} //namespace Scene
} //namespace GameLib

#endif
