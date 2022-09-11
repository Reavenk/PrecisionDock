#pragma once

/// <summary>
/// Layout properties. 
///
/// Contains parameters on how a layout should be performed.
/// </summary>
class LProps
{
public:
    /// <summary>
    /// The tab radius on the top corners
    /// </summary>
    int bevelRad = 10;
   
    /// <summary>
    /// The total height of the tab (including tab radius
    /// </summary>
    int tabHeight = 20;
    
    /// <summary>
    /// The excess space on the top of the padding
    /// </summary>
    int tabPadTop = 2;
    
    /// <summary>
    /// The excess space below the tab until the content
    /// </summary>
    int tabPadBot = 6;

    /// <summary>
    /// The default with of a notebook tab.
    /// </summary>
    int tabWidth = 150;

    /// <summary>
    /// The minimum width of a docked window region.
    /// </summary>
    int minClientWidth  = 100;

    /// <summary>
    /// The minimum height of a docked window region.
    /// </summary>
    int minClientHeight = 100;

    /// <summary>
    /// The width of a horizontally splitting sash.
    /// </summary>
    int sashWidth	    = 5;

    /// <summary>
    /// The height of a vertically splitting sash.
    /// </summary>
    int sashHeight	    = 5;

    /// <summary>
    /// The left padding of the window, when docked, inside its alloted space.
    /// </summary>
    int paddLeft	    = 0;

    /// <summary>
    /// The right padding of the window, when docked, inside its alloted space.
    /// </summary>
    int paddRight	    = 0;

    /// <summary>
    /// The top padding of the window, when docked, inside its alloted space.
    /// </summary>
    int paddTop		    = 0;

    /// <summary>
    /// The bottom padding of the window, when docked, inside it alloted space.
    /// </summary>
    int paddBottom	    = 0;

    /// <summary>
    /// 
    /// </summary>
    int dropEdgeWidth   = 10;

    /// <summary>
    /// When showing a droped-into preview, which is a square in 
    /// the middle of the window/tabbed-region, what is the radius 
    /// of the square to draw?
    /// </summary>
    int dropIntoRad     = 50;
};