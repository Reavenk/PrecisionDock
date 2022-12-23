#pragma once

#include <vector>
#include <wx/wx.h>

/// <summary>
/// Generate points, local to where a node region is drawn, for tabs.
/// </summary>
/// <param name="drawBot">If true, generate points for the bottom of a selected tab.</param>
/// <param name="tabPadBot">Padding for the bottom of the tab system.</param>
/// <param name="tabRect">The rectangle for tab.</param>
/// <param name="tabX">The X offset to generate the points at</param>
/// <param name="barRgnWidth"></param>
/// <param name="bevelRad">The bevel radius of a tab</param>
/// <returns></returns>
std::vector<wxPoint> GenerateTabPoints(bool drawBot, int tabPadBot, wxSize tabSz, int tabX, int barRgnWidth, float bevelRad);