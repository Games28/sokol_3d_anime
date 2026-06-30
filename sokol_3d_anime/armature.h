#pragma once
#ifndef ARMATURE_STRUCT_H
#define ARMATURE_STRUCT_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <exception>

#include "bone.h"

struct Armature {
	std::vector<Bone> bones;

	std::vector<cmn::mat4> calculateAnimationPose(std::vector<cmn::mat4> mat_in) const {
		std::vector<cmn::mat4> mat_local(bones.size());
		for (int i = 0; i < bones.size(); i++) {
			mat_local[i] = cmn::mat4::anim_mul(mat_in[i], bones[i].mat_local);
		}

		std::vector<cmn::mat4> mat_model(bones.size());
		mat_model[0] = mat_local[0];
		for (int i = 1; i < bones.size(); i++) {
			int p = bones[i].parent;
			mat_model[i] = cmn::mat4::anim_mul( mat_local[i], mat_model[p]);
		}

		std::vector<cmn::mat4> mat_out(bones.size());
		for (int i = 0; i < bones.size(); i++) {
			mat_out[i] = cmn::mat4::anim_mul( bones[i].mat_inv_model, mat_model[i]);
		}
		int i = 0;
		return mat_out;
	}

	static Armature loadFromARM(const std::string& filename) {
		std::ifstream file(filename);
		if (file.fail()) throw std::runtime_error("invalid filename: " + filename);

		Armature arm;

		for (std::string line; std::getline(file, line);) {
			std::stringstream line_str(line);

			Bone b;
			line_str >> b.parent;

			for (int i = 0; i < 16; i++) {
				int x = i % 4;
				int y = i / 4;
				line_str >> b.mat_local(y,x);
			}
			for (int i = 0; i < 16; i++) {
				int x = i % 4;
				int y = i / 4;
				line_str >> b.mat_inv_model(y, x);
			}
			arm.bones.push_back(b);
		}

		file.close();

		return arm;
	}
};

#endif // !ARMATURE_STRUCT_H

