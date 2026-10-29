/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// Port of the Doom 3 BFG Edition framework debug graph (DebugGraph.h/.cpp)
// to the Quake 2 client. BFG draws through the idRenderSystem with float
// coordinates and idVec4 colors; this port draws through re.DrawFill and
// re.DrawChar with palette colors and integer pixel positions, and the BFG
// dynamic idLists became fixed arrays.

#ifndef DNDEBUGGRAPH_H
#define DNDEBUGGRAPH_H

#define DN_MAX_GRAPH_BARS 256
#define DN_MAX_GRID_LINES 16
#define DN_GRAPH_LABEL_CHARS 32

class DN_DebugGraph
{
public:
    DN_DebugGraph(int numItems = 0);

    void Enable(qboolean b) { enable = b; }

    // create a graph with the specified number of bars
    void Init(int numBars);

    void AddGridLine(float value, int color);

    // sets a bar value, pass -1 to append an element
    void SetValue(int b, float value, int color);
    float GetValue(int b) { return bars[b].value; }

    // sets a bar label
    void SetLabel(int b, const char *text);

    enum fillMode_t
    {
        GRAPH_LINE,         // only draw a single top line for each bar
        GRAPH_FILL,         // fill the entire bar from the bottom (or left)
        GRAPH_FILL_REVERSE, // fill the entire bar from the top (or right)
    };
    void SetFillMode(fillMode_t m) { mode = m; }

    // render the graph sideways?
    void SetSideways(qboolean s) { sideways = s; }

    // the background color is what's drawn between bars and in the empty
    // space; colors are palette indexes passed to re.DrawFill
    void SetBackgroundColor(int color) { bgColor = color; }

    // the BFG RGBA label color became the Q2 console ormask bit: 0 draws
    // white characters, 128 the alternate conchars row
    void SetLabelColor(int color) { fontColor = color; }

    // the border specifies the amount of space between bars as well as
    // the amount of space around the entire graph
    void SetBorder(float b) { border = b; }

    // set the screen position for the graph
    void SetPosition(int x, int y, int w, int h) { posX = x; posY = y; width = w; height = h; }

    void Render(void);

private:
    // one 8x8 console character per label character, like the console
    enum { CHAR_WIDTH = 8, CHAR_HEIGHT = 8 };

    struct graphPlot_t
    {
        float value;
        int color;
    };

    graphPlot_t bars[DN_MAX_GRAPH_BARS];
    graphPlot_t grid[DN_MAX_GRID_LINES];
    char labels[DN_MAX_GRAPH_BARS][DN_GRAPH_LABEL_CHARS];

    int numBars;
    int numGridLines;
    qboolean hasLabels;

    int bgColor;
    int fontColor;
    fillMode_t mode;
    qboolean sideways;
    float border;
    int posX, posY, width, height;
    qboolean enable;
};

#endif
