#pragma once
#ifndef POSE_STRUCT_H
#define POSE_STRUCT_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "cmn/math/mat4.h"

struct Pose {
	std::vector<cmn::mat4> mat_pose;

	static Pose loadFromPOSE(const std::string& filename) {

		std::ifstream file(filename);
		if (file.fail()) throw std::runtime_error("invalid filename: " + filename);

		Pose pose;

		//load bone list
		for (std::string line; std::getline(file, line);) {
			std::stringstream line_str(line);

			//parse bone matrix
			cmn::mat4 m;
			for (int i = 0; i < 16; i++) line_str >> m(i % 4,i / 4);

			pose.mat_pose.push_back(m);
		}

		return pose;
	}
};
#endif
