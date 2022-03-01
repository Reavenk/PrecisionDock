#pragma once

class LProps
{
public:
    // The tab radius on the top corners
    int bevelRad = 10;
    // The total height of the tab (including tab radius
    int tabHeight = 20;
    // The excess space on the top of the padding
    int tabPadTop = 2;
    // The excess space below the tab until the content
    int tabPadBot = 6;

    int tabWidth = 150;

    int minClientWidth  = 100;
    int minClientHeight = 100;

    int sashWidth	    = 5;
    int sashHeight	    = 5;

    int paddLeft	    = 0;
    int paddRight	    = 0;
    int paddTop		    = 0;
    int paddBottom	    = 0;

    int dropEdgeWidth   = 10;
    int dropIntoRad     = 50;
};