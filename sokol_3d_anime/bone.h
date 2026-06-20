#pragma once
#ifndef BONE_STRUCT_H
#define BONE_STRUCT_H
#include "cmn/math/mat4.h"

struct Bone {
	int parent = -1;
	cmn::mat4 mat_local;
	cmn::mat4 mat_inv_model;
};
#endif