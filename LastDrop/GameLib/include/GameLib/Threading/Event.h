#ifndef INCLUDED_GAMELIB_THREADING_EVENT_H
#define INCLUDED_GAMELIB_THREADING_EVENT_H

namespace GameLib{
namespace Threading{

class Event{
public:
	static Event create( bool initialiState = false );
	void wait() const;	///trueにされるのを待つ
	void set();			///trueにする
	void reset();		///falseにする
	bool isSet() const;	///true?それともfalse?

	Event();
	~Event();
	Event( const Event& );
	operator void*() const;
	Event& operator=( const Event& );
	void release();
	bool operator==( const Event& ) const;
	bool operator!=( const Event& ) const;
private:
	class Impl;
	Impl* mImpl;
};

} //namespace Threading
} //namespace GameLib

#endif
