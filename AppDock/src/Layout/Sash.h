#pragma once
#include <wx/wx.h>

class Node;

/// <summary>
/// Stores connectivity data between Nodes in a Layout. This
/// is used to query clicked positions in the UI that can be 
/// dragged to resize Nodes in the layout.
/// </summary>
class Sash
{
public:

	/// <summary>
	/// Directions representing the possible grains of a sash.
	/// </summary>
	enum class Dir
	{
		/// <summary>
		/// The sash can be dragged left and right to modify
		/// horizontal properties of its referenced Nodes.
		/// </summary>
		Horiz,

		/// <summary>
		/// The sash can be dragged up and down to modify
		/// vertical properties of its referenced Nodes.
		/// </summary>
		Vert
	};
public:

	/// <summary>
	/// The origin (top left) of the sash in the Layout.
	/// </summary>
	wxPoint pos;

	/// <summary>
	/// The dimensions of the sash. This is the padding between nodes.
	/// </summary>
	wxSize size;

	/// <summary>
	/// The grain direction. When clicking and dragging a sash in the
	/// UI, having this cached means it doesn't need to be searched
	/// in the hierarchy.
	/// </summary>
	Dir dir;

	/// <summary>
	/// A container of two Nodes, representing the Nodes on each side
	///  of the sash. It's a mix of struct and union types to allow the
	/// Nodes to be referenced in 3 types of ways:
	/// - With names referencing a horizontal grain (left and right)
	/// - With names referencing a vertical grain (top and bottom)
	/// - As a two-component array.
	/// 
	/// While admittedly complex, it allows us to use in-context names
	/// for various peices of code, to make them more descriptive.
	/// </summary>
	union
	{
		struct
		{
			/// <summary>
			/// A Node representing the top or left side
			/// of the sash.
			/// </summary>
			union
			{
				Node * top;
				Node * left;
			};

			/// <summary>
			/// A Node representing the bottom or right
			/// side of the sash.
			/// </summary>
			union
			{
				Node* bot;
				Node* right;
			};
		};

		/// <summary>
		/// The two sashes in array form.
		/// </summary>
		Node* r[2];
	};

public:
	/// <summary>
	/// Query if a point overlaps with the sash.
	/// </summary>
	/// <param name="pt">The point to query.</param>
	/// <returns>
	/// True, if the point overlaps the sash's rectangle.
	/// </returns>
	bool Contains(const wxPoint& pt);

	/// <summary>
	/// Slide the sash horizontally.
	/// </summary>
	/// <param name="amt">
	/// Signed pixel value for how much to move the sash.
	/// </param>
	void SlideHoriz(int amt);

	/// <summary>
	/// Slide the sash vertically.
	/// </summary>
	/// <param name="amt">
	/// Signed pixel value for how much to move the sash.
	/// </param>
	void SlideVert(int amt);
};