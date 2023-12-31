// Slider.cpp - Sliders, copyright (c) Jules Bloomenthal, 2019, all rights reserved

#include <glad.h>
#include <stdio.h>
#include "Draw.h"
#include "Misc.h"
#include "Slider.h"
#include "Text.h"

void SRect(float x, float y, float w, float h, vec3 col) {
	Quad(vec3(x, y, 0), vec3(x+w, y, 0), vec3(x+w, y+h, 0), vec3(x, y+h, 0), true, col);
}

int SRound(float f)  { return (int)(f < 0.? ceil(f-.5f) : floor(f+.5f)); }

// Sliders

Slider::Slider() { }
//	x = y = size = 0;
//	winW = -1;
//	Init(0, 0, 80, 0, 1, .5f, true, size, NULL, NULL);
//}

Slider::Slider(int x, int y, int size, float min, float max, float init, bool vertical, float textSize, const char *nameA, vec3 *col)
	: x((float)x), y((float)y), size((float)size), min(min), max(max), vertical(vertical), textSize(textSize)
{
	color = col? *col : vec3(0, 0, 0);
	int off = vertical? y : x;
	loc = off+size*(init-min)/(max-min);
	if (nameA)
		name = string(nameA);
}

float Slider::Value() {
	float ref = vertical? y : x;
	return min+((loc-ref)/size)*(max-min);
}

void Slider::SetValue(float val) {
	float ref = vertical? y : x;
	loc = ref+size*(val-min)/(max-min);
}

void Slider::Draw(const char *nameOverride, vec3 *sCol) {
	vec3 blk(0, 0, 0), wht(1);
	vec3 offWht(240.f/255.f, 240.f/255.f, 240.f/255.f);
	vec3 ltGry(227/255.f), mdGry(160/255.f), dkGry(105/255.f);
	float grays[] = {160, 105, 227, 255};
	vec3 knobCol = blk;
	if (sCol != NULL) knobCol = *sCol;
	vec3 slideCol = knobCol;
	if (vertical) {
		for (int i = 0; i < 4; i++)
			SRect(x-1+i, y, 1, size, vec3(grays[i] / 255.f));
		SRect(x-1, y, 4, 1, wht);
		SRect(x, y+1, 1, 1, ltGry);
		SRect(x-1, y+size-1, 3, 1, mdGry);
		// slider
		SRect(x-10, loc-3, 20, 7, offWht); // whole knob
		SRect(x-10, loc-3, 20, 1, dkGry);  // bottom
		SRect(x+10, loc-3, 1, 7, dkGry);   // right
		SRect(x-10, loc-2, 1, 6, wht);     // left
		SRect(x-10, loc+3, 20, 1, wht);    // top
		SRect(x-9, loc-1, 1, 4, ltGry);    // 1 pixel right of left
		SRect(x-9, loc+2, 18, 1, ltGry);   // 1 pixel below top
		SRect(x-9, loc-2, 18, 1, mdGry);   // 1 pixel above bottom
		SRect(x+9, loc-2, 1, 5, mdGry);    // 1 pixel left of right
	}
	else {
		Line(vec2(x, y-2), vec2(x+size, y-2), 2, .6f*color);
	//	if (sCol)
			Line(vec2(loc, y-11), vec2(loc, y+7), 4, sCol? *sCol : color); // knobCol);
	//	else
	//		Line(vec2(loc, y-11), vec2(loc, y+7), 4, vec3(0,0,0));
/*      for (int i = 0; i < 4; i++)
			Rect(x, y-i+1, size, 1, vec3(grays[i]/255.f));
		Rect(x+size-1, y-1, 1, 3, wht);
		Rect(x+size-2, y, 1, 1, ltGry);
		Rect(x, y-1, 1, 3, mdGry);
		// slider
		Rect(iloc-3, y-10, 7, 20, offWht);
		Rect(iloc+3, y-9, 1, 20, dkGry);
		Rect(iloc-3, y-9, 1, 19, wht);
		Rect(iloc-3, y+10, 6, 1, wht);
		Rect(iloc-3, y-10, 7, 1, dkGry);
		Rect(iloc-2, y-9, 1, 18, ltGry);
		Rect(iloc+2, y-9, 1, 19, mdGry);
		Rect(iloc-2, y+9, 4, 1, ltGry);
		Rect(iloc-2, y-9, 5, 1, mdGry); */
	}
	if (nameOverride || !name.empty()) {
		const char *s = nameOverride? nameOverride : name.c_str();
		if (winW == -1)
			ViewportSize(winW, winH);
		char buf[100];
		float val = Value();
		vec3 col(color[0], color[1], color[2]);
		char *start = val > 0 && val < 1? buf+1 : buf; // skip leading zero
		if (vertical) {
			sprintf(buf, "%s", Nice(val));
		//	sprintf(buf, "%g", val);
		//	sprintf(buf, val >= 1? "%3.2f" : "%3.3f", val);
			Text((int)x, (int)(y+size)+6, col, textSize, s);
			Text((int)x, (int)y-17, col, textSize, start);
		}
		else {
			sprintf(buf, "%s: %s", s, Nice(val));
		//  sprintf(num, val >= 1 ? "%3.2f" : val < .001 ? "%4.4f" : "%3.3f", val);
		//  sprintf(buf, "%s: %s", s, val >= 0 && val < 1 ? num+1 : num); // +1: skip zero
			Text((int)(x+size)+8, (int)y-8, blk, textSize, nameOverride? nameOverride : buf);
		//	Text((int)(x+size)+8, (int)y-8, sCol ? .6f * (*sCol) : blk, textSize, nameOverride? nameOverride : buf);
		}
	}
}

bool Slider::Hit(int xMouse, int yMouse) {
	return vertical?
		xMouse >= x-16 && xMouse <= x+16 && yMouse >= y-32 && yMouse <= y+size+27 :
		xMouse >= x && xMouse <= x+size && yMouse >= y-10 && yMouse <= y+10;
}

bool Slider::Mouse(float xMouse, float yMouse) {
	// snap to mouse location
	float old = loc, mouse = vertical? yMouse : xMouse, lim = vertical? y : x;
	loc = mouse < lim? lim : mouse > lim+size? lim+size : mouse;
	return old != loc;
}

bool Slider::Mouse(int xMouse, int yMouse) {
	return Mouse((float) xMouse, (float) yMouse);
}

float Slider::YFromVal(float val) {
	return y+size*((val-min)/(max-min));
}

void Slider::ShiftPosition(vec2 delta) {
	x += delta.x;
	y += delta.y;
	loc += vertical? delta.y : delta.x;
}
