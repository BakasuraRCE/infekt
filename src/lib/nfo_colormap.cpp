﻿/**
 * Copyright (C) 2014 cxxjoe
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 **/

#include "stdafx.h"
#include "nfo_colormap.h"

#define NFORGB(R, G, B) (1 | ((B) << 8) | ((G) << 16) | ((R) << 24))


CNFOColorMap::CNFOColorMap()
{
	// default mapping = VGA colors
	m_rgbMapping[NFOCOLOR_BLACK] = NFORGB(0, 0, 0);
	m_rgbMapping[NFOCOLOR_RED] = NFORGB(170, 0, 0);
	m_rgbMapping[NFOCOLOR_GREEN] = NFORGB(0, 170, 0);
	m_rgbMapping[NFOCOLOR_YELLOW] = NFORGB(170, 85, 0);
	m_rgbMapping[NFOCOLOR_BLUE] = NFORGB(0, 0, 170);
	m_rgbMapping[NFOCOLOR_MAGENTA] = NFORGB(170, 0, 170);
	m_rgbMapping[NFOCOLOR_CYAN] = NFORGB(0, 170, 170);
	m_rgbMapping[NFOCOLOR_GRAY] = NFORGB(170, 170, 170);
	m_rgbMapping[NFOCOLOR_DARK_GRAY] = NFORGB(85, 85, 85);
	m_rgbMapping[NFOCOLOR_BRIGHT_RED] = NFORGB(255, 85, 85);
	m_rgbMapping[NFOCOLOR_BRIGHT_GREEN] = NFORGB(85, 255, 85);
	m_rgbMapping[NFOCOLOR_BRIGHT_YELLOW] = NFORGB(255, 255, 85);
	m_rgbMapping[NFOCOLOR_BRIGHT_BLUE] = NFORGB(85, 85, 255);
	m_rgbMapping[NFOCOLOR_PINK] = NFORGB(288, 85, 255);
	m_rgbMapping[NFOCOLOR_BRIGHT_CYAN] = NFORGB(85, 255, 255);
	m_rgbMapping[NFOCOLOR_WHITE] = NFORGB(255, 255, 255);

	// init m_previous...
	Clear();
}


void CNFOColorMap::Clear()
{
	m_previousBack.bold = false;
	m_previousBack.color = NFOCOLOR_DEFAULT;

	m_previousFore.bold = false;
	m_previousFore.color = NFOCOLOR_DEFAULT;

	m_stopsFore.clear();
	m_stopsBack.clear();
}


void CNFOColorMap::PushGraphicRendition(size_t a_row, size_t a_col, const std::vector<uint8_t>& a_params)
{
	int bold = -1;
	ENFOColor fore_color = _NFOCOLOR_MAX;
	ENFOColor back_color = _NFOCOLOR_MAX;
	uint32_t fore_color_rgba = 0, back_color_rgba = 0;

	for(uint8_t p : a_params)
	{
		// from SGR parameters on http://en.wikipedia.org/wiki/ANSI_escape_code#CSI_codes

		switch(p)
		{
		case 0: // Reset / Normal
			bold = 0;
			fore_color = NFOCOLOR_DEFAULT;
			back_color = NFOCOLOR_DEFAULT;
			break;
		case 1: // Bold or increased intensity
			bold = 1;
			break;
		case 21: // Bold: off or Underline: Double
		case 22: // Normal color or intensity
			bold = 0;
			break;
		case 38: // Set xterm-256 text color (foreground)
			if(!InterpretAdvancedColor(a_params, fore_color, fore_color_rgba))
				return;
			break;
		case 39: // Default text color (foreground)
			fore_color = NFOCOLOR_DEFAULT;
			break;
		case 48: // Set xterm-256 background color
			if(!InterpretAdvancedColor(a_params, back_color, back_color_rgba))
				return;
			break;
		case 49: // Default background color
			back_color = NFOCOLOR_DEFAULT;
			break;
		default:
			if(p >= 30 && p <= 37)
			{
				fore_color = static_cast<ENFOColor>(NFOCOLOR_BLACK + (p - 30));
			}
			else if(p >= 40 && p <= 47)
			{
				back_color = static_cast<ENFOColor>(NFOCOLOR_BLACK + (p - 40));
			}
			else if(p >= 90 && p <= 97)
			{
				fore_color = static_cast<ENFOColor>(NFOCOLOR_DARK_GRAY + (p - 90));
			}
			else if(p >= 100 && p <= 107)
			{
				back_color = static_cast<ENFOColor>(NFOCOLOR_DARK_GRAY + (p - 100));
			}
		} // end of switch
	} // end of for loop

	if(fore_color != _NFOCOLOR_MAX)// || bold != -1)
	{
		SNFOColorStop fore_stop(m_previousFore);

		fore_stop.color = fore_color;
		fore_stop.color_rgba = fore_color_rgba;

		m_stopsFore[a_row][a_col] = fore_stop;

		m_previousFore = fore_stop;
	}

	if(back_color != _NFOCOLOR_MAX)
	{
		SNFOColorStop back_stop(m_previousBack);

		back_stop.color = back_color;
		back_stop.color_rgba = back_color_rgba;

		m_stopsBack[a_row][a_col] = back_stop;

		m_previousBack = back_stop;
	}
}


bool CNFOColorMap::InterpretAdvancedColor(const std::vector<uint8_t>& a_params, ENFOColor& ar_color, uint32_t& ar_rgba) const
{
	ar_color = NFOCOLOR_RGB; // default

	if(a_params.size() == 5 && a_params[1] == 2)
	{
		ar_rgba = NFORGB(a_params[2], a_params[3], a_params[4]);
	}
	else if(a_params.size() == 3 && a_params[1] == 5)
	{
		uint8_t p = a_params[2];

		if(p >= 0 && p <= 7) // standard colors (as in ESC [ 30..37 m)
		{
			ar_color = static_cast<ENFOColor>(NFOCOLOR_BLACK + p);
		}
		else if(p >= 8 && p <= 0x0F) // high intensity colors (as in ESC [ 90..97 m)
		{
			ar_color = static_cast<ENFOColor>(NFOCOLOR_DARK_GRAY + (p - 8));
		}
		else if(p >= 0x10 && p <= 0xE7) // 6*6*6=216 colors: 16 + 36*r + 6*g + b (0≤r,g,b≤5)
		{
			p -= 0x10;

			uint8_t r = p / 36,
				g = (p - r * 36) / 6,
				b = (p - r * 36 - g * 6);

			ar_rgba = NFORGB((255 / 5) * r, (255 / 5) * g, (255 / 5) * b);
		}
		else // 0xe8-0xff: grayscale from black to white in 24 steps
		{
			uint8_t g = (255 / 23) * (p - 0xE8);

			ar_rgba = NFORGB(g, g, g);
		}
	}
	else
	{
		return false;
	}

	return true;
}


bool CNFOColorMap::FindRow(const TColorStopMap& a_stops, size_t a_row, size_t& ar_row) const
{
	if(a_stops.empty())
	{
		return false;
	}

	bool found = false;

	// :TODO: above if does not fully rule out size_t wrap conditions!

	for(size_t row = a_row; row >= 0; --row)
	{
		if(a_stops.find(row) == a_stops.end())
		{
			continue;
		}

		ar_row = row;
		found = true;
		break;
	}

	return found;
}


bool CNFOColorMap::GetForegroundColor(size_t a_row, size_t a_col, uint32_t a_defaultColor, uint32_t& ar_color) const
{
	size_t row;

	if(!FindRow(m_stopsFore, a_row, row))
	{
		ar_color = a_defaultColor;

		return false;
	}

	if(row == a_row)
	{
		const auto& row_data = m_stopsFore.find(row)->second;
		auto walk_it = row_data.begin(), it = walk_it;
		
		while(walk_it != row_data.end() && walk_it->first < a_col)
		{
			it = walk_it++;
		}

		if(it->second.color == NFOCOLOR_DEFAULT)
		{
			ar_color = a_defaultColor;

			return false;
		}
		else
		{
			ar_color = GetRGB(it->second);
		}
	}
	else
	{
		const SNFOColorStop& last_stop = m_stopsFore.find(row)->second.rbegin()->second;

		if(last_stop.color == NFOCOLOR_DEFAULT)
		{
			ar_color = a_defaultColor;

			return false;
		}

		ar_color = GetRGB(last_stop);
	}

	return true;
}


bool CNFOColorMap::GetLineBackgrounds(size_t a_row, uint32_t a_defaultColor, std::vector<size_t>& ar_sections, std::vector<uint32_t>& ar_colors) const
{
	size_t row;

	ar_sections.clear();
	ar_colors.clear();

	if(!FindRow(m_stopsBack, a_row, row))
	{
		ar_colors.push_back(a_defaultColor);

		return false;
	}

	// one (the last) section in ar_sections is always implicit (to EOL).

	if(row == a_row)
	{
		const auto& row_data = m_stopsBack.find(row)->second;

		size_t prev_end_col = 0;

		for(const auto& sub : row_data)
		{
			uint32_t color;

			if(sub.second.color == NFOCOLOR_DEFAULT)
				color = a_defaultColor;
			else
				color = GetRGB(sub.second);

			ar_colors.push_back(color);

			if(ar_sections.size() + 1 < row_data.size())
			{
				ar_sections.push_back(sub.first - prev_end_col);

				prev_end_col = sub.first;
			}
		}
	}
	else
	{
		const SNFOColorStop& last_stop = m_stopsBack.find(row)->second.rbegin()->second;

		if(last_stop.color == NFOCOLOR_DEFAULT)
		{
			ar_colors.push_back(a_defaultColor);

			return false;
		}

		ar_colors.push_back(GetRGB(last_stop));
	}

	return true;
}


uint32_t CNFOColorMap::GetRGB(const SNFOColorStop& a_stop) const
{
	if(a_stop.color == NFOCOLOR_RGB)
	{
		return a_stop.color_rgba;
	}
	else
	{
		auto it = m_rgbMapping.find(a_stop.color);

		if(it != m_rgbMapping.end())
			return it->second;

		_ASSERT(false);

		return 0;
	}
}
