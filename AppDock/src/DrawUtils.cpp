#include "DrawUtils.h"


std::vector<wxPoint> GenerateTabPoints(
	bool drawBot, 
	int tabPadBot,
	wxSize tabSz, 
	int tabX, 
	int barRgnWidth, 
	float bevelRad)
{
	const float myPI = 3.14159f;

	int tabWidth = tabSz.x;
	int tabHeight = tabSz.y;

	const int tabBaseline = tabHeight;
	const int tabPadShank = tabHeight - bevelRad;

	std::vector<wxPoint> tabPts = 
	{
		wxPoint(tabX, tabBaseline - 0),
		wxPoint(tabX, tabBaseline - tabPadShank),
		wxPoint(tabX + bevelRad + (int)(cos(myPI * 7.0f / 8.0f) * bevelRad),   tabBaseline - (tabPadShank + (int)(sin(myPI * 7.0f / 8.0f) * bevelRad))),
		wxPoint(tabX + bevelRad + (int)(cos(myPI * 6.0f / 8.0f) * bevelRad),   tabBaseline - (tabPadShank + (int)(sin(myPI * 6.0f / 8.0f) * bevelRad))),
		wxPoint(tabX + bevelRad + (int)(cos(myPI * 5.0f / 8.0f) * bevelRad),   tabBaseline - (tabPadShank + (int)(sin(myPI * 5.0f / 8.0f) * bevelRad))),
		wxPoint(tabX + bevelRad + (int)(cos(myPI * 4.0f / 8.0f) * bevelRad),   tabBaseline - (tabPadShank + (int)(sin(myPI * 4.0f / 8.0f) * bevelRad))),
		//
		wxPoint(tabX + tabWidth - bevelRad + (int)(cos(myPI * 4.0f / 8.0f) * bevelRad),  tabBaseline - (tabPadShank + (int)(sin(myPI * 4.0f / 8.0f) * bevelRad))),
		wxPoint(tabX + tabWidth - bevelRad + (int)(cos(myPI * 3.0f / 8.0f) * bevelRad),  tabBaseline - (tabPadShank + (int)(sin(myPI * 3.0f / 8.0f) * bevelRad))),
		wxPoint(tabX + tabWidth - bevelRad + (int)(cos(myPI * 2.0f / 8.0f) * bevelRad),  tabBaseline - (tabPadShank + (int)(sin(myPI * 2.0f / 8.0f) * bevelRad))),
		wxPoint(tabX + tabWidth - bevelRad + (int)(cos(myPI * 1.0f / 8.0f) * bevelRad),  tabBaseline - (tabPadShank + (int)(sin(myPI * 1.0f / 8.0f) * bevelRad))),
		wxPoint(tabX + tabWidth, tabBaseline - tabPadShank),
		wxPoint(tabX + tabWidth, tabBaseline - 0),
	};

	if(drawBot)
	{
		tabPts.push_back(wxPoint(barRgnWidth,  tabBaseline));
		tabPts.push_back(wxPoint(barRgnWidth,  tabBaseline + tabPadBot));
		tabPts.push_back(wxPoint(0,            tabBaseline + tabPadBot));
		tabPts.push_back(wxPoint(0,            tabBaseline));
	}

	return tabPts;
}