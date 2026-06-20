#pragma once
#ifndef ANIMATED_MESH_H
#define ANIMATED_MESH_H

#include <vector>
#include <fstream>
#include <sstream>

#include <exception>


struct IndexTriangle {
	int ix[3];
	int r = 1, g = 1, b = 1;
};

float random() {
	static const float rand_max = RAND_MAX;
	return rand() / rand_max;
}

struct Mesh {
	std::vector<cmn::vf3d> verts;
	std::vector<IndexTriangle> tris;
	cmn::vf3d rotation, scale{ 1,1,1 }, translation;
	cmn::mat4 mat_world;
	
	void updateTransforms() {
		//combine all transforms
		cmn::mat4 mat_rot_x = cmn::mat4::makeRotX(rotation.x);
		cmn::mat4 mat_rot_y = cmn::mat4::makeRotX(rotation.y);
		cmn::mat4 mat_rot_z = cmn::mat4::makeRotX(rotation.z);
		cmn::mat4 mat_rot = cmn::mat4::mul( mat_rot_z,cmn::mat4::mul(mat_rot_x, mat_rot_y));
		cmn::mat4 mat_scale = cmn::mat4::makeScale(scale);
		cmn::mat4 mat_trans = cmn::mat4::makeTranslation(translation);
		mat_world = cmn::mat4::mul(mat_scale, cmn::mat4::mul(mat_trans,mat_rot));
	}

	static bool loadFromOBJ(Mesh& m, const std::string& filename) {
		std::ifstream file(filename);
		if (file.fail()) return false;

		//parse file line by line
		std::string line;
		while (std::getline(file, line)) {
			std::stringstream line_str(line);
			std::string type; line_str >> type;

			if (type == "v") {
				cmn::vf3d v;
				line_str >> v.x >> v.y >> v.z;
				m.verts.push_back(v);

			}
			else if (type == "f") {
				std::vector<int> v_ixs;

				//parsae v/t/n until fail
				int num = 0;
				for (std::string vtn; line_str >> vtn; num++) {
					std::stringstream vtn_str(vtn);
					int v_ix;
					if (vtn_str >> v_ix) v_ixs.push_back(v_ix - 1);
				}

				//trangulate
				for (int i = 2; i < num; i++) {
					m.tris.push_back({
						v_ixs[0],
						v_ixs[i - 1],
						v_ixs[i]
						});

				}
			}
		}

		file.close();

		return true;
	}

};

#endif // !ANIMATED_MESH_H
