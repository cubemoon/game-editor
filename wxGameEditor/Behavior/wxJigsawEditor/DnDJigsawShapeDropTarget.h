#ifndef _DND_JIGSAW_SHAPE_DROP_TARGET_H
#define _DND_JIGSAW_SHAPE_DROP_TARGET_H

#include <wx/wx.h>
#include <wx/dnd.h>

struct DND_JigsawShapeInfo;
class wxJigsawEditorCanvas;
class wxJigsawShapeListBox;

class DnDJigsawShapeDropTarget : public wxDropTarget
{
	void OnDropShape(const wxPoint & pos, DND_JigsawShapeInfo * shapeinfo);
public:
    DnDJigsawShapeDropTarget(wxJigsawEditorCanvas * pOwner);

    virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);

	virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);

private:
    wxJigsawEditorCanvas * m_pOwner;
};

#endif