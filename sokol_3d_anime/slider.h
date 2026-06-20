#pragma once
#ifndef SLIDER_CLASS_H
#define SLIDER_CLASS_H

#include "cmn/math/v2d.h"

struct Slider {
	cmn::vf2d a, b;
	float rad = 1;

	float t = 0;
	bool holding = false;

	void startSlide(const cmn::vf2d& p) {
		//get close pt
		cmn::vf2d ba = b - a, pa = p - a;
		float new_t = ba.dot(pa) / ba.dot(ba);
		new_t = std::max(0.f, std::min(1.f, new_t));
		cmn::vf2d close_pt = a + new_t * ba;
		if ((close_pt - p).mag() < rad) {
			t = new_t;
			holding = true;
		}
	}

	void updateSlide(const cmn::vf2d& p) {
		//get close pt
		cmn::vf2d ba = b - a, pa = p - a;
		float new_t = ba.dot(pa) / ba.dot(ba);
		new_t = std::max(0.f, std::min(1.f, new_t));
		if (holding) {
			t = new_t;
		}
	}

	void endSlide() {
		holding = false;
	}

	
};
#endif
