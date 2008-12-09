#include "wxJigsawShape.h"
#include "wxJigsawShapePropertyIO.h"
#include "wxJigsawInputParameterPropertyIO.h"
#include <wx/listimpl.cpp>

wxJigsawShapeStyle IntToJigsawShapeStyle(int value)
{
	switch(value)
	{
	case wxJigsawShapeStyle::wxJS_TYPE_NONE:
		return wxJigsawShapeStyle::wxJS_TYPE_NONE;
	case wxJigsawShapeStyle::wxJS_TYPE_NUMERIC:
		return wxJigsawShapeStyle::wxJS_TYPE_NUMERIC;
	case wxJigsawShapeStyle::wxJS_TYPE_BOOLEAN:
		return wxJigsawShapeStyle::wxJS_TYPE_BOOLEAN;
	default:
		break;
	}
	return wxJigsawShapeStyle::wxJS_TYPE_DEFAULT;
}

wxString JigsawShapeStyleToStr(wxJigsawShapeStyle value)
{
	switch(value)
	{	
	case wxJigsawShapeStyle::wxJS_TYPE_NUMERIC:
		return _("Numeric");
	case wxJigsawShapeStyle::wxJS_TYPE_BOOLEAN:
		return _("Boolean");
	case wxJigsawShapeStyle::wxJS_TYPE_NONE:
	default:
		break;
	}
	return _("None");
}


int wxJigsawShape::CShapeThickness = 10;
wxSize wxJigsawShape::ConnectorSize = wxSize(10, 2);
wxSize wxJigsawShape::ShapeLabelOffset = wxSize(10, 3);
int wxJigsawShape::HeaderElementSpacing = 3;
int wxJigsawShape::ChildrenSpacing = 10;
int wxJigsawShape::HotSpotHeight = 10;

WX_DEFINE_LIST(wxJigsawShapeList);

IMPLEMENT_DYNAMIC_CLASS(wxJigsawShape, xsSerializable);

wxJigsawShape::wxJigsawShape(const wxString & name, const wxString & bitmapFileName, 
	const wxColour & colour)
: m_Parent(NULL), m_Name(name), m_BitmapFileName(bitmapFileName), m_Colour(colour), 
m_Style(wxJigsawShapeStyle::wxJS_TYPE_DEFAULT), m_HasNotch(false), m_HasBump(false),
m_HasCShape(false), m_MinSize(80,17), m_Position(wxDefaultPosition), 
m_NeedCalcLabelSize(true), m_LabelSize(0,0)
{
	ReLoadBitmap();
	m_InputParameters.DeleteContents(true);
	m_Children.DeleteContents(true);

	InitSerialization();
}

wxJigsawShape::wxJigsawShape(const wxJigsawShape & shape)
: xsSerializable(shape), 
m_Parent(shape.m_Parent), m_Name(shape.m_Name), m_Bitmap(shape.m_Bitmap), 
m_Colour(shape.m_Colour), m_Style(shape.m_Style), m_HasNotch(shape.m_HasNotch), 
m_HasBump(shape.m_HasBump), m_HasCShape(shape.m_HasCShape), m_MinSize(shape.m_MinSize), 
m_Position(shape.m_Position), m_NeedCalcLabelSize(shape.m_NeedCalcLabelSize),
m_LabelSize(shape.m_LabelSize)
{
	m_InputParameters.DeleteContents(true);
	CopyInputParameters(*this, shape);

	m_Children.DeleteContents(true);
	CopyChildren(*this, shape);

	InitSerialization();
}

wxJigsawShape::~wxJigsawShape()
{
}

void wxJigsawShape::InitSerialization()
{
	XS_SERIALIZE(m_Name, wxT("name"));
	XS_SERIALIZE(m_BitmapFileName, wxT("bitmap"));
	XS_SERIALIZE(m_Colour, wxT("colour"));
	XS_SERIALIZE(m_Style, wxT("style"));
	XS_SERIALIZE(m_HasNotch, wxT("has_notch"));
	XS_SERIALIZE(m_HasBump, wxT("has_bump"));
	XS_SERIALIZE(m_HasCShape, wxT("has_c_shape"));
	XS_SERIALIZE(m_MinSize, wxT("min_size"));
	XS_SERIALIZE(m_Position, wxT("position"));
	XS_SERIALIZE_LISTJIGSAWINPUTPARAMETER(m_InputParameters, wxT("inputparameters"));
	XS_SERIALIZE_LISTJIGSAWSHAPE(m_Children, wxT("children"));
}

void wxJigsawShape::Draw(wxDC & dc, const wxSize & offset, double scale)
{
	// Size of shape header
	wxSize headerSize = GetHeaderSize(dc, scale);
	// Size of whole shape
	wxSize size = GetSize(dc, scale);
	// Real position on DC, including offset
	wxPoint realPosition(m_Position.x+offset.x, m_Position.y+offset.y);
	// Start position of input parameters (top left corner of the rectangle which contains parameters)
	wxPoint parametersPos = GetinputParametersPosition(dc, scale);
	// Increment by offset
	// Size of input parameter (needed for drawing of each parameter)
	wxSize paramSize(0,0);
	// Draw background of a shape with connectors and C-Shape
	DrawBackground(dc, realPosition, headerSize, size, scale);

	// Rectangle wich contains a label
	wxRect labelRect(realPosition.x + wxJigsawShape::ShapeLabelOffset.GetWidth()*scale, 
		realPosition.y, 
		headerSize.GetWidth()-2*wxJigsawShape::ShapeLabelOffset.GetWidth()*scale, 
		headerSize.GetHeight());
	// Draw a label (on the left side of a shape and centerred vertically)
	dc.SetTextForeground(*wxWHITE);
	if(m_Bitmap.IsOk())
	{
		dc.DrawLabel(m_Name, m_Bitmap, labelRect, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
	}
	else
	{
		dc.DrawLabel(m_Name, labelRect, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
	}
	
	// Draw parameters
	dc.SetTextForeground(*wxWHITE);
	for(wxJigsawInputParameters::Node * node = m_InputParameters.GetFirst();
		node; node = node->GetNext())
	{
		// Obtain a pointer to input parameter
		wxJigsawInputParameter * param = node->GetData();
		// Obtain parameter's size
		paramSize = param->GetSize(dc, scale);
		// Draw the parameter
		parametersPos.y = realPosition.y + (headerSize.GetHeight()-paramSize.GetHeight())/2;
		param->Draw(dc, parametersPos, offset, scale);

		// Move to next parameter
		parametersPos.x += paramSize.GetWidth() + wxJigsawInputParameter::ParameterSpacing*scale;
	}
	wxPoint pos(0, m_Position.y + headerSize.GetHeight());
	for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); node; node = node->GetNext())
	{
		wxJigsawShape * child = node->GetData();
		if(!child) continue;
		child->SetPosition(wxPoint(child->GetPosition().x, pos.y));
		child->Draw(dc, offset, scale);
		pos.y += child->GetSize().GetHeight();
	}

	/*dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(*wxBLACK_PEN);
	dc.DrawRectangle(realPosition, GetHeaderSize());
	dc.DrawRectangle(realPosition, GetSize());*/
}

const wxString & wxJigsawShape::GetName() const
{
	return m_Name;
}

void wxJigsawShape::SetName(const wxString & value)
{
	m_Name = value;
	m_NeedCalcLabelSize = true;
}

const wxBitmap & wxJigsawShape::GetBitmap() const
{
	return m_Bitmap;
}

const wxString & wxJigsawShape::GetBitmapFileName() const
{
	return m_BitmapFileName;
}

void wxJigsawShape::ReLoadBitmap()
{
	do
	{
		if(!wxFileExists(m_BitmapFileName)) break;
		if(!m_Bitmap.LoadFile(m_BitmapFileName, wxBITMAP_TYPE_ANY)) break;
		return;
	}
	while(false);
	m_Bitmap = wxNullBitmap;
}

void wxJigsawShape::SetBitmapFileName(const wxString & value)
{
	m_BitmapFileName = value;
	ReLoadBitmap();
}

bool wxJigsawShape::GetHasNotch() const
{
	return m_HasNotch;
}

void wxJigsawShape::SetHasNotch(bool value)
{
	m_HasNotch = value;
}

bool wxJigsawShape::GetHasBump() const
{
	return m_HasBump;
}

void wxJigsawShape::SetHasBump(bool value)
{
	m_HasBump = value;
}

bool wxJigsawShape::GetHasCShape() const
{
	return m_HasCShape;
}

void wxJigsawShape::SetHasCShape(bool value)
{
	m_HasCShape = value;
}

wxJigsawShapeStyle wxJigsawShape::GetStyle() const
{
	return IntToJigsawShapeStyle(m_Style);
}

void wxJigsawShape::SetStyle(wxJigsawShapeStyle value)
{
	m_Style = value;
	// If a shape has output then it should not have bump, notch and c-shape
	if(value != wxJigsawShapeStyle::wxJS_TYPE_DEFAULT)
	{
		m_HasCShape = false;
		m_HasBump = false;
		m_HasNotch = false;
	}
}

const wxSize & wxJigsawShape::GetMinSize()
{
	return m_MinSize;
}

void wxJigsawShape::GetMinSize(wxCoord * w, wxCoord * h)
{
	if(w)
	{
		*w = m_MinSize.GetWidth();
	}
	if(h)
	{
		*h = m_MinSize.GetHeight();
	}
}

void wxJigsawShape::SetMinSize(wxCoord w, wxCoord h)
{
	m_MinSize = wxSize(w, h);
}

void wxJigsawShape::SetMinSize(const wxSize & value)
{
	m_MinSize = value;
}

wxSize wxJigsawShape::GetSize()
{
	//wxSize res(0, 0);
	//wxSize headerSize = GetHeaderSize();
	//wxSize childrenSize = GetChildrenSize();
	//if(m_HasCShape) 
	//{
	//	childrenSize.SetHeight(wxMax(childrenSize.GetHeight(), 10));
	//}
	//res.x = headerSize.GetWidth();
	//// If a shape has children then we also have to add horizontal spacing
	//int spacing = childrenSize.GetWidth() != 0 ? wxJigsawShape::ChildrenSpacing : 0;
	//res.x = wxMax(res.x, spacing + childrenSize.GetWidth());
	//// If the width is lower than minimal width, then we have to set it to its minimal value

	//res.y = headerSize.GetHeight();
	//res.y = wxMax(res.y, GetMinSize().GetHeight());
	//if(m_HasCShape)
	//{
	//	res.y += childrenSize.GetHeight();
	//	res.y += wxJigsawShape::CShapeThickness;
	//}
	//return res;
	return m_Size;
}

wxSize wxJigsawShape::GetSize(wxDC & dc, double scale)
{
	wxSize res;
	GetSize(dc, &res.x, &res.y, scale);
	return res;
}

void wxJigsawShape::GetSize(wxDC & dc, wxCoord * w, wxCoord * h, double scale)
{
	wxSize headerSize = GetHeaderSize(dc, scale);
	wxSize childrenSize = GetChildrenSize(dc, scale);
	m_Size.Set(0,0);
	if(m_HasCShape) 
	{
		childrenSize.SetHeight(wxMax(childrenSize.GetHeight(), 10*scale));
	}

		int & width = m_Size.x;
		width = 0;
		width = headerSize.GetWidth();
		// If a shape has children then we also have to add horizontal spacing
		int spacing = childrenSize.GetWidth() != 0 ? wxJigsawShape::ChildrenSpacing*scale : 0;
		width = wxMax(width, spacing + childrenSize.GetWidth());
		// If the width is lower than minimal width, then we have to set it to its minimal value
		width = wxMax(width, GetMinSize().GetWidth()*scale);

		int & height = m_Size.y;
		height = 0;
		height = headerSize.GetHeight();
		height = wxMax(height, GetMinSize().GetHeight()*scale);
		if(m_HasCShape)
		{
			height += childrenSize.GetHeight();
			height += wxJigsawShape::CShapeThickness*scale;
		}
	
		if(w)
		{
			*w = width;
		}
		if(h)
		{
			*h = height;
		}
}

const wxPoint & wxJigsawShape::GetPosition()
{
	return m_Position;
}

void wxJigsawShape::GetPosition(int * x, int * y)
{
	if(x)
	{
		*x = m_Position.x;
	}
	if(y)
	{
		*y = m_Position.y;
	}
}

void wxJigsawShape::SetPosition(const wxPoint & value)
{
	m_Position = value;
}

void wxJigsawShape::SetPosition(int x, int y)
{
	m_Position = wxPoint(x, y);
}

wxSize wxJigsawShape::GetHeaderSize(wxDC & dc, double scale)
{
	wxSize res;
	GetHeaderSize(dc, &res.x, &res.y, scale);
	return res;
}

wxSize wxJigsawShape::GetInputParametersSize()
{
	wxSize res(0, 0);
	wxCoord w(0), h(0);
	for(wxJigsawInputParameters::Node * node = m_InputParameters.GetFirst(); 
		node; node = node->GetNext())
	{
		wxJigsawInputParameter * param = node->GetData();
		if(param)
		{
			param->GetSize(&w, &h);
			res.x += w + wxJigsawInputParameter::ParameterSpacing;
			res.y = wxMax(res.y, h);
		}
	}
	return res;
}

wxSize wxJigsawShape::GetInputParametersSize(wxDC & dc, double scale)
{
	wxSize res(0, 0);
	wxCoord w(0), h(0);
	for(wxJigsawInputParameters::Node * node = m_InputParameters.GetFirst(); 
		node; node = node->GetNext())
	{
		wxJigsawInputParameter * param = node->GetData();
		if(param)
		{
			param->GetSize(dc, &w, &h, scale);
			res.x += w + wxJigsawInputParameter::ParameterSpacing*scale;
			res.y = wxMax(res.y, h);
		}
	}
	return res;
}

wxPoint wxJigsawShape::GetinputParametersPosition()
{
	wxPoint result = GetPosition();
	result.x += wxJigsawShape::ShapeLabelOffset.GetWidth();
	// If shape has an icon then increment by width of icon + spacing
	if(m_Bitmap.IsOk())
	{
		result.x += m_Bitmap.GetWidth();
		result.x += wxJigsawShape::HeaderElementSpacing;
	}
	// Add the width of string label + spacing
	result.x += m_LabelSize.GetWidth();
	result.x += wxJigsawShape::HeaderElementSpacing;
	wxSize paramSize = GetInputParametersSize();
	wxSize headerSize = GetHeaderSize();
	result.y += (headerSize.GetHeight()-paramSize.GetHeight())/2;
	return result;
}

wxPoint wxJigsawShape::GetinputParametersPosition(wxDC & dc, double scale)
{
	wxPoint result = GetPosition();
	result.x += wxJigsawShape::ShapeLabelOffset.GetWidth()*scale;
	// If shape has an icon then increment by width of icon + spacing
	if(m_Bitmap.IsOk())
	{
		result.x += m_Bitmap.GetWidth();
		result.x += wxJigsawShape::HeaderElementSpacing;
	}
	// Add the width of string label + spacing
	result.x += m_LabelSize.GetWidth();
	result.x += wxJigsawShape::HeaderElementSpacing*scale;
	wxSize paramSize = GetInputParametersSize(dc, scale);
	wxSize headerSize = GetHeaderSize(dc, scale);
	result.y += (headerSize.GetHeight()-paramSize.GetHeight())/2;
	result.y *= scale;
	return result;
}

wxSize wxJigsawShape::GetHeaderSize()
{
	//wxSize res(0, 0);
	//wxSize paramSize(0, 0);
	//
	//if(m_InputParameters.GetCount() && (m_LabelSize.GetWidth() || m_LabelSize.GetHeight()))
	//{
	//	paramSize = GetInputParametersSize();
	//}

	//	// Add an offset
	//res.x += wxJigsawShape::ShapeLabelOffset.GetWidth();
	//// If shape has an icon then increment by width of icon + spacing
	//if(m_Bitmap.IsOk())
	//{
	//	res.x += m_Bitmap.GetWidth();
	//	res.x += wxJigsawShape::HeaderElementSpacing;
	//}
	//// Add the width of string label + spacing
	//res.x += m_LabelSize.GetWidth();
	//res.x += wxJigsawShape::HeaderElementSpacing;
	//// Add the width of input parameters
	//res.x += paramSize.GetWidth();
	//// Add an offset on the right side
	//res.x += wxJigsawShape::ShapeLabelOffset.GetWidth();
	//res.x = wxMax(res.x, m_MinSize.GetWidth());

	//// Height should have the maximum value of:
	//// Height of icon
	//if(m_Bitmap.IsOk())
	//{
	//	res.y = wxMax(res.y, m_Bitmap.GetHeight());
	//}
	//// height of string label
	//res.y = wxMax(res.y, m_LabelSize.GetHeight());
	//// height of imput parameters
	//res.y = wxMax(res.y, paramSize.GetHeight());
	//// Add an spacing on top and on bottom
	//res.y += 2 * wxJigsawShape::ShapeLabelOffset.GetHeight();
	//res.y = wxMax(res.y, m_MinSize.GetHeight());

	//return res;
	return m_HeaderSize;
}

void wxJigsawShape::RequestSizeRecalculation()
{
	m_NeedCalcLabelSize = true;
	for(wxJigsawInputParameters::Node * node = m_InputParameters.GetFirst(); 
		node; node = node->GetNext())
	{
		wxJigsawInputParameter * param = node->GetData();
		if(!param) continue;
		param->RequestSizeRecalculation();
	}
	for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); node; node = node->GetNext())
	{
		wxJigsawShape * child = node->GetData();
		if(!child) continue;
		child->RequestSizeRecalculation();
	}
}

void wxJigsawShape::GetHeaderSize(wxDC & dc, wxCoord * w, wxCoord * h, double scale)
{
	//wxFAIL_MSG(_("wxJigsawShape::GetHeaderSize not implemented"));
	wxSize paramSize(0, 0);
	m_HeaderSize.Set(0,0);
	if(m_NeedCalcLabelSize)
	{
		dc.GetTextExtent(m_Name, &m_LabelSize.x, &m_LabelSize.y);
		m_NeedCalcLabelSize = false;
	}
	//dc.GetTextExtent(m_Name, &labelWidth, &labelHeight);
	if(m_InputParameters.GetCount() && (w || h))
	{
		paramSize = GetInputParametersSize(dc, scale);
	}
	
		int & width = m_HeaderSize.x;
		// Reset the width
		width = 0;
		// Add an offset
		width += wxJigsawShape::ShapeLabelOffset.GetWidth()*scale;
		// If shape has an icon then increment by width of icon + spacing
		if(m_Bitmap.IsOk())
		{
			width += m_Bitmap.GetWidth();
			width += wxJigsawShape::HeaderElementSpacing*scale;
		}
		// Add the width of string label + spacing
		width += m_LabelSize.GetWidth();
		width += wxJigsawShape::HeaderElementSpacing*scale;
		// Add the width of input parameters
		width += paramSize.GetWidth();
		// Add an offset on the right side
		width += wxJigsawShape::ShapeLabelOffset.GetWidth()*scale;
		width = wxMax(width, m_MinSize.GetWidth()*scale);
	
		int & height = m_HeaderSize.y;
		// Height should have the maximum value of:
		// Height of icon
		if(m_Bitmap.IsOk())
		{
			height = wxMax(height, m_Bitmap.GetHeight());
		}
		// height of string label
		height = wxMax(height, m_LabelSize.GetHeight());
		// height of imput parameters
		height = wxMax(height, paramSize.GetHeight());
		// Add an spacing on top and on bottom
		height += 2 * wxJigsawShape::ShapeLabelOffset.GetHeight()*scale;
		height = wxMax(height, m_MinSize.GetHeight()*scale);
	if(w)
	{
		*w = width;
	}
	if(h)
	{
		*h = height;
	}
}

wxSize wxJigsawShape::GetChildrenSize()
{
	wxSize res(0,0);
	// Width and height of child shape
	wxSize shapeSize(0,0);
	for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); node; node = node->GetNext())
	{
		wxJigsawShape * shape = node->GetData();
		shapeSize = shape->GetSize();
		res.x = wxMax(res.x, shapeSize.GetWidth());
		res.y += shapeSize.GetHeight();
	}
	return res;
}

wxSize wxJigsawShape::GetChildrenSize(wxDC & dc, double scale)
{
	wxSize res(0,0);
	GetChildrenSize(dc, &res.x, &res.y, scale);
	return res;
}

void wxJigsawShape::GetChildrenSize(wxDC & dc, int * w, int * h, double scale)
{
	if(w) *w = 0;
	if(h) *h = 0;
	// Width and height of child shape
	int ww(0), hh(0);
	for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); node; node = node->GetNext())
	{
		wxJigsawShape * shape = node->GetData();
		shape->GetSize(dc, &ww, &hh, scale);
		if(w)
		{
			*w = wxMax(*w, ww);
		}
		if(h)
		{
			*h += hh;
		}
	}
}

wxJigsawInputParameters & wxJigsawShape::GetInputParameters()
{
	return m_InputParameters;
}

wxJigsawShapeList & wxJigsawShape::GetChildren()
{
	return m_Children;
}

xsSerializable* wxJigsawShape::Clone()
{
	return new wxJigsawShape(*this);
}

void wxJigsawShape::DrawShapeHeader(wxDC & dc, const wxPoint & pos, 
		const wxSize & size, const wxJigsawShapeStyle style)
{
	wxPoint * points(NULL);
	switch(style)
	{
	case wxJigsawShapeStyle::wxJS_TYPE_NUMERIC:
		dc.DrawRoundedRectangle(pos, size, size.GetHeight()/2);
		break;
	case wxJigsawShapeStyle::wxJS_TYPE_BOOLEAN:
		// If it is possible to draw a shape then we will draw it
		if(size.GetWidth() >= size.GetHeight())
		{
			points = new wxPoint[7];
			points[0] = wxPoint(0, size.GetHeight()/2);
			points[1] = wxPoint(size.GetHeight()/2, 0);
			points[2] = wxPoint(size.GetWidth()-size.GetHeight()/2, 0);
			points[3] = wxPoint(size.GetWidth(), size.GetHeight()/2);
			points[4] = wxPoint(size.GetWidth()-size.GetHeight()/2, size.GetHeight());
			points[5] = wxPoint(size.GetHeight()/2, size.GetHeight());
			points[6] = wxPoint(0, size.GetHeight()/2);
			dc.DrawPolygon(7, points, pos.x, pos.y);
			wxDELETEA(points);
		}
		else // If it is impossible to draw a shape then we will draw a rectangle
		{
			dc.DrawRectangle(pos, size);
		}
		break;
	case wxJigsawShapeStyle::wxJS_TYPE_NONE:
	default:
		dc.DrawRoundedRectangle(pos, size, 4);
		break;
	}
}

void wxJigsawShape::CopyChildren(wxJigsawShape & dest, const wxJigsawShape & src)
{
	dest.m_Children.Clear();
	for(wxJigsawShapeList::Node * node = src.m_Children.GetFirst(); node; node = node->GetNext())
	{
		wxJigsawShape * child = node->GetData();
		if(child)
		{
			dest.m_Children.Append(new wxJigsawShape(*child));
		}
	}
}

void wxJigsawShape::CopyInputParameters(wxJigsawShape & dest, const wxJigsawShape & src)
{
	dest.m_InputParameters.Clear();
	for(wxJigsawInputParameters::Node * node = src.m_InputParameters.GetFirst(); 
		node; node = node->GetNext())
	{
		wxJigsawInputParameter * param = node->GetData();
		if(param)
		{
			dest.m_InputParameters.Append(new wxJigsawInputParameter(*param));
		}
	}
}

int wxJigsawShape::GetParametersOffset()
{
	return m_ParametersOffset;
}

int wxJigsawShape::GetParametersOffset(double scale)
{
	m_ParametersOffset = 0;
	// Add an offset
	m_ParametersOffset += wxJigsawShape::ShapeLabelOffset.GetWidth()*scale;
	// If shape has an icon then increment by width of icon + spacing
	if(m_Bitmap.IsOk())
	{
		m_ParametersOffset += m_Bitmap.GetWidth();
		m_ParametersOffset += wxJigsawShape::HeaderElementSpacing * scale;
	}
	// Add the width of string label + spacing
	m_ParametersOffset += m_LabelSize.GetWidth();
	m_ParametersOffset += wxJigsawShape::HeaderElementSpacing * scale;
	return m_ParametersOffset;
}

wxJigsawShape::wxJigsawShapeHitTest wxJigsawShape::HitTest(wxPoint pos, 
	wxJigsawShapeHitTestInfo & info, bool bDebug)
{
	wxJigsawShapeHitTest res = wxJS_HITTEST_NONE;
	wxSize headerSize = GetHeaderSize();
	wxRect headerRect(GetPosition(), headerSize);
	wxSize size = GetSize();
	wxRect dockingRectTop = wxRect(m_Position.x, m_Position.y-wxJigsawShape::HotSpotHeight/2, 
		headerSize.GetWidth(), wxJigsawShape::HotSpotHeight);
	wxRect dockingRectBottom = wxRect(m_Position.x, 
		m_Position.y-wxJigsawShape::HotSpotHeight/2+size.GetHeight(), 
		headerSize.GetWidth(), 
		wxJigsawShape::HotSpotHeight);
	if(dockingRectTop.Contains(pos) && m_HasNotch && !GetParent())
	{
		res = wxJS_HITTEST_NOTCH_DOCKING_AREA;
	}
	else if(dockingRectBottom.Contains(pos) && m_HasBump && !GetParent())
	{
		res = wxJS_HITTEST_BUMP_DOCKING_AREA;
	}
	else if(headerRect.Contains(pos))
	{
		do
		{
			int paramRectOffset = GetParametersOffset();
			wxRect labelRect(GetPosition().x, GetPosition().y, 
				paramRectOffset, headerSize.GetHeight());
			if(labelRect.Contains(pos) || m_InputParameters.GetCount() == 0)
			{
				res = wxJS_HITTEST_MOVINGAREA;
				break;
			}
			wxRect paramRect(GetPosition(), headerSize);
			paramRect.Offset(paramRectOffset, 0);
			bool bFound = false;
			int paramIndex(0);
			for(wxJigsawInputParameters::Node * node = m_InputParameters.GetFirst();
				node; node = node->GetNext(), paramIndex++)
			{
				wxJigsawInputParameter * param = node->GetData();
				paramRect.SetSize(param->GetSize());
				switch(param->HitTest(pos, paramRect, bDebug))
				{
				case wxJigsawInputParameter::wxJSP_HITETST_LABEL:
					res = wxJS_HITTEST_MOVINGAREA;
					bFound = true;
					break;
				case wxJigsawInputParameter::wxJSP_HITTEST_SLOT:
					info.SetInputParameterIndex(paramIndex);
					res = wxJS_HITTEST_SLOT;
					bFound = true;
					break;
				case wxJigsawInputParameter::wxJSP_HITTEST_SHAPE:
					res = param->GetShape()->HitTest(pos, info, bDebug);
					bFound = true;
					break;
				default:
					break;
				}
				if(bFound) break;
				paramRect.Offset(paramRect.GetWidth() + wxJigsawInputParameter::ParameterSpacing, 0);
			}
			if(!bFound)
			{
				res = wxJS_HITTEST_MOVINGAREA;
			}
		}
		while(false);
	}
	else if(m_HasCShape)
	{
		do
		{
			wxRect verticalCShapeRect(
				GetPosition().x, 
				GetPosition().y + headerSize.GetHeight()/2,
				wxJigsawShape::CShapeThickness,
				size.GetHeight()-headerSize.GetHeight()/2);

			wxRect horizontalCShapeRect(
				GetPosition().x, 
				GetPosition().y + size.GetHeight() - wxJigsawShape::CShapeThickness,
				GetHeaderSize().GetWidth(),
				wxJigsawShape::CShapeThickness);

			if(m_Children.IsEmpty())
			{
				wxRect cShapeRect(verticalCShapeRect.GetRight(), 
					headerRect.GetBottom(), 
					headerRect.GetWidth()-verticalCShapeRect.GetWidth(),
					horizontalCShapeRect.GetTop()-headerRect.GetBottom());
				if(cShapeRect.Contains(pos))
				{
					res = wxJS_HITTEST_C_SHAPE_BUMP;
				}
			}
			else
			{
				wxRect cShapeRect(
					verticalCShapeRect.GetRight(), 
					headerRect.GetBottom()-wxJigsawShape::ConnectorSize.GetHeight(), 
					wxJigsawShape::ConnectorSize.GetWidth(),
					10/*2 * wxJigsawShape::ConnectorSize.GetHeight()*/);
				int oldWidth(cShapeRect.GetWidth());
				int childIndex = 0;
				for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); 
					node; node = node->GetNext(), childIndex++)
				{
					wxJigsawShape * child = node->GetData();
					if(!child) continue;
					wxSize childSize = child->GetSize();
					cShapeRect.SetWidth(wxMax(childSize.GetWidth(), oldWidth));
					if(cShapeRect.Contains(pos))
					{
						res = wxJS_HITTEST_CHILD_INSERTION_AREA;
						info.SetChildIndex(childIndex);
						break;
					}
					oldWidth = cShapeRect.GetWidth();
					cShapeRect.Offset(0, childSize.GetHeight());
				}
				if(!res == wxJS_HITTEST_NONE) break;
				if(cShapeRect.Contains(pos))
				{
					res = wxJS_HITTEST_C_SHAPE_NOTCH;
					info.SetChildIndex(childIndex);
					break;
				}
				if(res != wxJS_HITTEST_NONE) break;
				for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); node; node = node->GetNext())
				{
					wxJigsawShape * child = node->GetData();
					if(!child) continue;
					res = child->HitTest(pos, info);
					if(res != wxJS_HITTEST_NONE) break;
				}
				if(res != wxJS_HITTEST_NONE) break;
			}
			// C-Shape background
			if(verticalCShapeRect.Contains(pos))
			{
				res = wxJS_HITTEST_MOVINGAREA;
				break;
			}
			if(horizontalCShapeRect.Contains(pos))
			{
				res = wxJS_HITTEST_MOVINGAREA;
				break;
			}
		}
		while(false);
	}
	info.SetResult(res);
	// If point belongs to some meaningful area of a shape (and does not belong to 
	// shape's child or to input parameter)...
	if((res != wxJS_HITTEST_NONE) && !info.GetShape())
	{
		info.SetShape(this);
	}
	return res;
}

bool wxJigsawShape::PerformDragging(wxJigsawShapeHitTest dragAction, const wxPoint & currentPos,
		const wxSize & offset, int & clue)
{
	switch(dragAction)
	{
	case wxJS_HITTEST_MOVINGAREA:
		m_Position.x += offset.GetWidth();
		m_Position.y += offset.GetHeight();
		return true;
	case wxJS_HITTEST_NONE:
	default:
		break;
	};
	return false;
}

const wxColour & wxJigsawShape::GetColour()
{
	return m_Colour;
}

void wxJigsawShape::SetColour(const wxColour & value)
{
	m_Colour = value;
}

void wxJigsawShape::DrawBackground(wxDC & dc, 
		const wxPoint & pos, const wxSize & headerSize, 
		const wxSize & size, double scale)
{
	dc.SetBrush(wxBrush(m_Colour));
	dc.SetPen(*wxTRANSPARENT_PEN);
	
	wxPoint connector[5] = 
	{
		wxPoint(0,0), 
		wxPoint(wxJigsawShape::ConnectorSize.GetHeight(),
			wxJigsawShape::ConnectorSize.GetHeight()), 
		wxPoint(wxJigsawShape::ConnectorSize.GetWidth()-wxJigsawShape::ConnectorSize.GetHeight(),
			wxJigsawShape::ConnectorSize.GetHeight()), 
		wxPoint(wxJigsawShape::ConnectorSize.GetWidth(),0), 
		wxPoint(0,0)
	};
	for(size_t i = 0; i < 5; i++)
	{
		connector[i].x *= scale;
		connector[i].y *= scale;
	}
	wxSize childrenSize = GetChildrenSize(dc, scale);
	int cShapeHeight = wxMax(childrenSize.GetHeight(), 10*scale);
	wxRegion clippingRegion(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight());
	if(m_HasBump)
	{
		wxRegion connectorRegion(5, connector);
		connectorRegion.Offset(pos.x + wxJigsawShape::ConnectorSize.GetWidth()*scale, pos.y);
		clippingRegion.Xor(connectorRegion);
	}
	if(m_HasCShape)
	{
		wxRegion connectorRegion(5, connector);
		connectorRegion.Offset(
			pos.x + (wxJigsawShape::CShapeThickness+wxJigsawShape::ConnectorSize.GetWidth())*scale, 
			pos.y + headerSize.GetHeight()+cShapeHeight);
		clippingRegion.Xor(connectorRegion);
	}

	dc.SetClippingRegion(clippingRegion);
	//dc.DrawRoundedRectangle(m_Position.x, m_Position.y, headerSize.GetWidth(), headerSize.GetHeight(), 4);
	DrawShapeHeader(dc, pos, headerSize, IntToJigsawShapeStyle(m_Style));
	
	if(m_HasCShape)
	{
		dc.DrawRectangle(pos.x, 
			pos.y + headerSize.GetHeight() / 2, 
			(wxJigsawShape::CShapeThickness+4)*scale, 
			headerSize.GetHeight() / 2 + cShapeHeight + (wxJigsawShape::CShapeThickness*scale)/2);
		dc.DrawRoundedRectangle(pos.x, 
			pos.y + headerSize.GetHeight()+cShapeHeight, 
			headerSize.GetWidth(), 
			wxJigsawShape::CShapeThickness*scale, 4);
		// Upper inner notch
		dc.DrawPolygon(5, connector, 
			pos.x + (wxJigsawShape::CShapeThickness+wxJigsawShape::ConnectorSize.GetWidth())*scale, 
			pos.y + headerSize.GetHeight());
	}
	if(m_HasNotch)
	{
		dc.DrawPolygon(5, connector, 
			pos.x + wxJigsawShape::ConnectorSize.GetWidth()*scale, 
			pos.y + size.GetHeight());
	}
	dc.DestroyClippingRegion();
}

const wxSize & wxJigsawShape::GetConnectorSize()
{
	return wxJigsawShape::ConnectorSize;
}

void wxJigsawShape::SetConnectorSize(const wxSize & value)
{
	wxJigsawShape::ConnectorSize = value;
	// Fix the height of connector. It should be lower or equal to half of its width
	ConnectorSize.SetHeight(
		wxMin(wxJigsawShape::ConnectorSize.GetWidth()/2, wxJigsawShape::ConnectorSize.GetHeight()));
}

wxJigsawShape * wxJigsawShape::GetParent()
{
	return m_Parent;
}

void wxJigsawShape::SetParent(wxJigsawShape * value)
{
	m_Parent = value;
}

wxJigsawShape * wxJigsawShape::GetTopParent()
{
	do
	{
		if(!m_Parent) break;
		wxJigsawShape * parent = m_Parent;
		while(parent->GetParent())
		{
			parent = parent->GetParent();
		}
		return parent;
	}
	while(false);
	return NULL;
}

wxJigsawHotSpotArray & wxJigsawShape::GetHotSpots()
{
	return m_HotSpots;
}

void wxJigsawShape::ReCreateHotSpots(wxJigsawHotSpotArray & hotSpots, double scale)
{
	wxSize size = GetSize();
	wxSize headerSize = GetHeaderSize();
	wxPoint parameterPos = GetinputParametersPosition();
	if(m_Style == wxJigsawShapeStyle::wxJS_TYPE_NONE)
	{
		if(m_HasBump)
		{
			hotSpots.Add(wxJigsawHotSpot(this,
				wxRect(m_Position.x, 
					m_Position.y-(wxJigsawShape::HotSpotHeight/2)*scale,
					headerSize.GetWidth(), 
					wxJigsawShape::HotSpotHeight*scale),
				wxJigsawHotSpotType::wxJS_HOTSPOT_BUMP));
		}
		if(m_HasNotch)
		{
			int a = (m_Position.x);
			int b = m_Position.y+size.GetHeight()-wxJigsawShape::HotSpotHeight/2* scale;
			wxLogTrace(wxTraceMask(), _("Notch Hotspot: Position=(%i,%i); Size=(%i,%i)"),
				a, b,
				headerSize.GetWidth(), 
				(int)(wxJigsawShape::HotSpotHeight * scale));
			hotSpots.Add(wxJigsawHotSpot(this,
				wxRect(m_Position.x, 
					m_Position.y+size.GetHeight()-wxJigsawShape::HotSpotHeight/2* scale,
					headerSize.GetWidth(), 
					wxJigsawShape::HotSpotHeight * scale),
				wxJigsawHotSpotType::wxJS_HOTSPOT_NOTCH));
		}
		if(m_HasCShape)
		{
			if(!m_Children.GetCount())
			{
				hotSpots.Add(wxJigsawHotSpot(this,
					wxRect(m_Position.x+wxJigsawShape::CShapeThickness*scale, 
						m_Position.y+headerSize.GetHeight()+wxJigsawShape::ConnectorSize.GetHeight()*scale-1,
						headerSize.GetWidth()-wxJigsawShape::CShapeThickness*scale, 
						wxJigsawShape::HotSpotHeight*scale),
					wxJigsawHotSpotType::wxJS_HOTSPOT_NOTCH));				
			}
			else
			{
				for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); node; node = node->GetNext())
				{
					wxJigsawShape * child = node->GetData();
					if(!child) continue;
					child->ReCreateHotSpots(hotSpots, scale);
				}
			}
		}
	}

	if(m_InputParameters.GetCount())
	{
		int paramRectOffset = GetParametersOffset(scale);
		wxRect paramRect(GetPosition(), headerSize);
		paramRect.Offset(paramRectOffset, 0);
		int index(0);
		int slotOffset(0);
		for(wxJigsawInputParameters::Node * node = m_InputParameters.GetFirst();
			node; node = node->GetNext(), index++)
		{
			wxJigsawInputParameter * param = node->GetData();
			paramRect.SetWidth(param->GetSize().GetWidth());
			slotOffset = param->GetSlotOffset(scale);
			hotSpots.Add(wxJigsawHotSpot(this, 
				wxRect(paramRect.GetLeft()+slotOffset, 
					paramRect.GetTop(),
					paramRect.GetWidth()-slotOffset,
					paramRect.GetHeight()),					
				wxJigsawHotSpotType::wxJS_HOTSPOT_INPUT_PARAMETER, index));
			param->ReCreateHotSpots(hotSpots, scale);
			paramRect.Offset(paramRect.GetWidth() + wxJigsawInputParameter::ParameterSpacing, 0);
		}
	}
}

void wxJigsawShape::Layout(double scale, bool reCreateHotSpots)
{
	do
	{
		if(m_Children.IsEmpty()) break;
		wxSize headerSize = GetHeaderSize();
		wxPoint childPos = wxPoint(
			m_Position.x + wxJigsawShape::CShapeThickness*scale,
			m_Position.y + headerSize.GetHeight());
		for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); node; node = node->GetNext())
		{
			wxJigsawShape * child = node->GetData();
			if(!child) break;
			child->SetParent(this);
			child->SetPosition(childPos);
			child->Layout(scale, reCreateHotSpots);
			wxSize childSize = child->GetSize();
			childPos.y += childSize.GetHeight();
		}
	}
	while(false);
}

void wxJigsawShape::RecalculateLabelSizes(wxDC & dc)
{
	dc.GetTextExtent(m_Name, &m_LabelSize.x, &m_LabelSize.y);
	for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); node; node = node->GetNext())
	{
		wxJigsawShape * child = node->GetData();
		child->RecalculateLabelSizes(dc);
	}
	for(wxJigsawInputParameters::Node * node = m_InputParameters.GetFirst(); 
		node; node = node->GetNext())
	{
		wxJigsawInputParameter * param = node->GetData();
		param->RecalculateLabelSizes(dc);
	}
}

void wxJigsawShape::UpdateParents(wxJigsawShape * thisShapeParent)
{
	m_Parent = thisShapeParent;
	for(wxJigsawShapeList::Node * node = m_Children.GetFirst(); node; node = node->GetNext())
	{
		wxJigsawShape * child = node->GetData();
		if(!child) continue;
		child->UpdateParents(this);
	}
	for(wxJigsawInputParameters::Node * node = m_InputParameters.GetFirst(); 
		node; node = node->GetNext())
	{
		wxJigsawInputParameter * param = node->GetData();
		if(!param) continue;
		wxJigsawShape * paramShape = param->GetShape();
		if(!paramShape) continue;
		paramShape->UpdateParents(this);
	}
}