#ifndef __UIBUTTON_H
#define __UIBUTTON_H

#include "UIControl.h"
#include "IUIButton.h"

class CUIButton : public CUIControl<IUIButton>
{
public:
	CUIButton(ULONG uID);

	void createNode(gui::dom::IDOMdocument *pDomDocument, gui::dom::IDOMnode *pParentNode) override;
};

#endif
