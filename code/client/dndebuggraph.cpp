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

#include "client.h"
#include "dndebuggraph.h"

/*
================
DN_DebugGraph::DN_DebugGraph
================
*/
DN_DebugGraph::DN_DebugGraph(int numItems)
    : bgColor(0), fontColor(0), enable(true), mode(GRAPH_FILL),
      sideways(false), border(0.0f), posX(100), posY(100), width(100), height(100)
{
    Init(numItems);
}

/*
================
DN_DebugGraph::Init
================
*/
void DN_DebugGraph::Init(int numBars)
{
    int i;

    if (numBars < 0)
    {
        numBars = 0;
    }
    if (numBars > DN_MAX_GRAPH_BARS)
    {
        numBars = DN_MAX_GRAPH_BARS;
    }

    this->numBars = numBars;
    numGridLines = 0;
    hasLabels = false;

    for (i = 0; i < numBars; i++)
    {
        bars[i].value = 0.0f;
        bars[i].color = fontColor;
    }
    for (i = 0; i < DN_MAX_GRAPH_BARS; i++)
    {
        labels[i][0] = 0;
    }
}

/*
================
DN_DebugGraph::AddGridLine
================
*/
void DN_DebugGraph::AddGridLine(float value, int color)
{
    if (numGridLines >= DN_MAX_GRID_LINES)
    {
        return;
    }
    grid[numGridLines].value = value;
    grid[numGridLines].color = color;
    numGridLines++;
}

/*
================
DN_DebugGraph::SetValue
================
*/
void DN_DebugGraph::SetValue(int b, float value, int color)
{
    int i;

    if (!enable)
    {
        return;
    }
    if (b < 0)
    {
        // append: drop the oldest bar once the ring is full, like the
        // BFG RemoveIndex(0) + Alloc pair
        if (numBars >= DN_MAX_GRAPH_BARS)
        {
            for (i = 1; i < DN_MAX_GRAPH_BARS; i++)
            {
                bars[i - 1] = bars[i];
            }
            numBars--;
        }
        bars[numBars].value = value;
        bars[numBars].color = color;
        numBars++;
    }
    else if (b < numBars)
    {
        bars[b].value = value;
        bars[b].color = color;
    }
}

/*
================
DN_DebugGraph::SetLabel
================
*/
void DN_DebugGraph::SetLabel(int b, const char *text)
{
    if (b < 0 || b >= numBars || !text)
    {
        return;
    }
    strncpy(labels[b], text, DN_GRAPH_LABEL_CHARS - 1);
    labels[b][DN_GRAPH_LABEL_CHARS - 1] = 0;
    if (labels[b][0])
    {
        hasLabels = true;
    }
}

/*
================
DN_DebugGraph::Render
================
*/
void DN_DebugGraph::Render(void)
{
    int i, j;

    if (!enable)
    {
        return;
    }

    re.DrawFill(posX, posY, width, height, bgColor);

    if (numBars == 0)
    {
        return;
    }

    if (sideways)
    {
        float barWidth = width - border * 2.0f;
        float barHeight = (height - border) / (float)numBars;
        float barLeft = posX + border;
        float barTop = posY + border;

        for (i = 0; i < numBars; i++)
        {
            float rx, ry, rw, rh;

            if (mode == GRAPH_LINE)
            {
                rx = barLeft + barWidth * bars[i].value;
                ry = barTop + i * barHeight;
                rw = 1.0f;
                rh = barHeight - border;
            }
            else if (mode == GRAPH_FILL)
            {
                rx = barLeft;
                ry = barTop + i * barHeight;
                rw = barWidth * bars[i].value;
                rh = barHeight - border;
            }
            else
            {
                rx = barLeft + barWidth;
                ry = barTop + i * barHeight;
                rw = barWidth - barWidth * bars[i].value;
                rh = barHeight - border;
            }

            if (rw < 1.0f)
            {
                rw = 1.0f;
            }
            if (rh < 1.0f)
            {
                rh = 1.0f;
            }
            re.DrawFill((int)rx, (int)ry, (int)rw, (int)rh, bars[i].color);
        }

        if (hasLabels)
        {
            int maxLen = 0;

            for (i = 0; i < numBars; i++)
            {
                int len = (int)strlen(labels[i]);
                if (len > maxLen)
                {
                    maxLen = len;
                }
            }
            re.DrawFill(posX - CHAR_WIDTH * maxLen, posY, CHAR_WIDTH * maxLen, height, bgColor);
            for (i = 0; i < numBars; i++)
            {
                for (j = 0; labels[i][j]; j++)
                {
                    re.DrawChar((int)(barLeft - CHAR_WIDTH * maxLen) + j * CHAR_WIDTH,
                                (int)(barTop + i * barHeight),
                                fontColor | labels[i][j]);
                }
            }
        }
    }
    else
    {
        float barWidth = (width - border) / (float)numBars;
        float barHeight = height - border * 2.0f;
        float barLeft = posX + border;
        float barTop = posY + border;
        float barBottom = barTop + barHeight;

        for (i = 0; i < numGridLines; i++)
        {
            re.DrawFill(posX, (int)(barBottom - barHeight * grid[i].value), width, 1, grid[i].color);
        }

        for (i = 0; i < numBars; i++)
        {
            float rx, ry, rw, rh;

            if (mode == GRAPH_LINE)
            {
                rx = barLeft + i * barWidth;
                ry = barBottom - barHeight * bars[i].value;
                rw = barWidth - border;
                rh = 1.0f;
            }
            else if (mode == GRAPH_FILL)
            {
                rx = barLeft + i * barWidth;
                ry = barBottom - barHeight * bars[i].value;
                rw = barWidth - border;
                rh = barHeight * bars[i].value;
            }
            else
            {
                rx = barLeft + i * barWidth;
                ry = barTop;
                rw = barWidth - border;
                rh = barHeight * bars[i].value;
            }

            if (rw < 1.0f)
            {
                rw = 1.0f;
            }
            if (rh < 1.0f)
            {
                rh = 1.0f;
            }
            re.DrawFill((int)rx, (int)ry, (int)rw, (int)rh, bars[i].color);
        }

        if (hasLabels)
        {
            re.DrawFill(posX, (int)(barTop + barHeight), width, CHAR_HEIGHT, bgColor);
            for (i = 0; i < numBars; i++)
            {
                for (j = 0; labels[i][j]; j++)
                {
                    re.DrawChar((int)(barLeft + i * barWidth) + j * CHAR_WIDTH,
                                (int)(barBottom + border),
                                fontColor | labels[i][j]);
                }
            }
        }
    }
}
