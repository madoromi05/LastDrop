#ifndef INCLUDED_I_UI_ELEMENT_H
#define INCLUDED_I_UI_ELEMENT_H

class UiRenderer;

// 画面に描ける部品（文字・ボタンなど）。UiCanvas が持って、まとめて描く
// 選んで決定できる部品は、これに加えて IUiSelectable も実装する（描くだけの部品は実装しない）
class IUiElement{
public:
	virtual ~IUiElement(){}
	// 描く内容を renderer へ積む（実際に描くのは UiRenderer::draw）。何回呼んでも同じ絵になること
	virtual void draw( UiRenderer& renderer ) const = 0;
};

#endif
