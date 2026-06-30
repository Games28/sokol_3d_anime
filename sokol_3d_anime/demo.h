#define SOKOL_IMPL
#ifdef __EMSCRIPTEN__
#define SOKOL_GLES3
#else
#define SOKOL_GLCORE
#endif
#include "sokol/include/sokol_app.h"
#include "sokol/include/sokol_gfx.h"
#include "sokol/include/sokol_glue.h"
#include "sokol/include/sokol_gl.h"
#include "cmn/math/v3d.h"
#include "cmn/math/mat4.h"

#include "sokol/sokol_engine.h"

#include "cmn/utils.h"
#include "animated_mesh.h"
#include "slider.h"
#include "pose.h"
#include "armature.h"


using cmn::vf3d;
using cmn::mat4;


static cmn::vf3d polarToCartesian(float yaw, float pitch) {
	return {
		std::sin(yaw) * std::cos(pitch),
		std::sin(pitch),
		std::cos(yaw) * std::cos(pitch)
	};
}


static void cartesianToPolar(const cmn::vf3d& pt, float* yaw, float* pitch) {
	//flatten onto xz
	*yaw = std::atan2(pt.x, pt.z);
	//vertical triangle
	*pitch = std::atan2(pt.y, std::sqrt(pt.x * pt.x + pt.z * pt.z));
}

struct VertexData {
	int indexes[4];
	float weights[4];
};

class Demo : public cmn::SokolEngine {

	std::vector<Mesh> meshes;
	Armature armature;
	std::vector<Pose> poses;

	std::vector<VertexData> vertex_data;

	Slider pose_slider;

	float seg_timer = 0, time_per_seg = 1.3f;

	//graphics
	sgl_pipeline pip{};

	//user input
	struct {
		vf3d pos{ -1, 1, 2.5f };
		float yaw = 0, pitch = 0;
		vf3d dir;

		float fov_deg = 75;

		const float near_plane = .001f, far_plane = 1000;
		mat4 proj;

		mat4 view;

		mat4 view_proj;
	} cam;

#pragma region CREATE HELPER

	bool setupMesh()
	{
		std::string base_dir = "assets/knight/";


		const std::vector<std::string> filenames{
			"assets/knight/model.txt"
		};

		for (const auto& f : filenames) {
			Mesh m;
			if (!Mesh::loadFromOBJ(m, f)) return false;
			meshes.push_back(m);
		}

		//load armature
		std::string arm_filename = base_dir + "bones.arm";
		armature = Armature::loadFromARM(arm_filename);

		//load poses
		for (int i = 1; i <= 20; i++) {
			std::string pose_filename = base_dir + "/frames/frame" + std::to_string(i) + ".pose";
			poses.push_back(Pose::loadFromPOSE(pose_filename));
		}

		//load vertex data
		std::string vertex_data_filename = base_dir + "vertex_weights.txt";
		std::ifstream vertex_data_file(vertex_data_filename);
		if (vertex_data_file.fail()) throw std::runtime_error("invalid filename: " + vertex_data_filename);

		for (std::string line; std::getline(vertex_data_file, line);) {
			std::stringstream line_str(line);
			VertexData vd;
			for (int i = 0; i < 4; i++) line_str >> vd.indexes[i];
			for (int i = 0; i < 4; i++) line_str >> vd.weights[i];
			vertex_data.push_back(vd);
		}

		float margin = 30;
		pose_slider = {
			cmn::vf2d(margin, margin),
			cmn::vf2d(sapp_widthf() - margin, margin),
			10
		};

		return true;
	}

	void setupSGL() {
		sgl_desc_t sgl_desc{};
		sgl_setup(sgl_desc);
	}

	void setupPipeline() {
		sg_pipeline_desc pip_desc{};
		pip_desc.face_winding = SG_FACEWINDING_CCW;
		pip_desc.cull_mode = SG_CULLMODE_BACK;
		pip_desc.depth.write_enabled = true;
		pip_desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
		pip = sgl_make_pipeline(pip_desc);
	}

#pragma endregion

	bool user_create() override {

		app_title = "3d animation";

		std::srand(std::time(0));

		if (!setupMesh()) return false;

	
		cartesianToPolar(-cam.pos, &cam.yaw, &cam.pitch);

		setupSGL();

		setupPipeline();


		return true;
	}

#pragma region UPDATE HELPER
	void handleCameraLooking(float dt) {
		//look up, down
		if (GetKey(SAPP_KEYCODE_UP).held) cam.pitch += dt;
		if (GetKey(SAPP_KEYCODE_DOWN).held) cam.pitch -= dt;
		cam.pitch = cmn::clamp(cam.pitch, .001f - cmn::Pi / 2, cmn::Pi / 2 - .001f);

		//look left, right
		if (GetKey(SAPP_KEYCODE_LEFT).held) cam.yaw += dt;
		if (GetKey(SAPP_KEYCODE_RIGHT).held) cam.yaw -= dt;
	}

	void handleCameraMovement(float dt) {
		//move up, down
		if (GetKey(SAPP_KEYCODE_SPACE).held) cam.pos.y += 4.f * dt;
		if (GetKey(SAPP_KEYCODE_LEFT_SHIFT).held) cam.pos.y -= 4.f * dt;

		//move forward, backward
		vf3d fb_dir(std::sin(cam.yaw), 0, std::cos(cam.yaw));
		if (GetKey(SAPP_KEYCODE_W).held) cam.pos += 5.f * dt * fb_dir;
		if (GetKey(SAPP_KEYCODE_S).held) cam.pos -= 3.f * dt * fb_dir;

		//move left, right
		vf3d lr_dir(fb_dir.z, 0, -fb_dir.x);
		if (GetKey(SAPP_KEYCODE_A).held) cam.pos += 4.f * dt * lr_dir;
		if (GetKey(SAPP_KEYCODE_D).held) cam.pos -= 4.f * dt * lr_dir;
	}

	void updateCameraMatrixes() {
		mat4 look_at = mat4::makeLookAt(cam.pos, cam.pos + cam.dir, { 0, 1, 0 });
		cam.view = mat4::inverse(look_at);

		//cam proj can change with window resize
		float asp = sapp_widthf() / sapp_heightf();
		cam.proj = mat4::makePerspective(cam.fov_deg, asp, cam.near_plane, cam.far_plane);

		cam.view_proj = mat4::mul(cam.proj, cam.view);
	}

	void handleUserInput(float dt) {
		handleCameraLooking(dt);

		//set light pos
		//if (GetKey(SAPP_KEYCODE_L).held) light_pos = cam_pos;

		//pose slider input
		const auto slide_action = GetMouse(SAPP_MOUSEBUTTON_LEFT);
		const auto mouse_pos = cmn::vf2d(GetMouseX(), GetMouseY());
		if (slide_action.pressed) pose_slider.startSlide(mouse_pos);
		if (slide_action.held) pose_slider.updateSlide(mouse_pos);
		if (slide_action.released) pose_slider.endSlide();

		cam.dir = polarToCartesian(cam.yaw, cam.pitch);

		handleCameraMovement(dt);

		updateCameraMatrixes();

		
	}

#pragma endregion

	void UpdateAnimation(Mesh & mesh ,float t) {

		
		std::vector<cmn::mat4> mat_pose;
		{
			//get left and right anim
			float ix1f =  t * (poses.size() - 1);
			int ix1 = ix1f;
			int ix2 = (1 + ix1) % poses.size();

			

			

			//should be same size
			std::vector<cmn::mat4> mat_pose1 = armature.calculateAnimationPose(poses[ix1].mat_pose);
			std::vector<cmn::mat4> mat_pose2 = armature.calculateAnimationPose(poses[ix2].mat_pose);
			int num = std::min(mat_pose1.size(), mat_pose2.size());

			//interpolate
			float t = ix1f - ix1;
			for (int i = 0; i < num; i++) {
				const auto& a = mat_pose1[i];
				const auto& b = mat_pose2[i];


				mat_pose.push_back(a + (b - a) * t);
			}
			
			
			for (int i = 0; i < mesh.bind_to_verts.size(); i++) {
				float w = 1;
				const auto& vd = vertex_data[i];
				cmn::mat4 mat_anim;
				for (int j = 0; j < 4; j++) {
					const auto& m = mat_pose[vd.indexes[j]];
					float w = vd.weights[j];
					mat_anim = mat_anim + m * w;

				}
				cmn::mat4 mat_total = cmn::mat4::mul(mesh.mat_world,mat_anim);
				mesh.bind_to_verts[i] = cmn::matMulVec(mat_total ,mesh.verts[i], w);
			}

			mesh.render_verts.clear();
			for (int i = 0; i < mesh.bind_to_verts.size(); i++)
			{
				mesh.render_verts.push_back( mesh.bind_to_verts[i] );
				
			}
		
		
 		}
	}

	bool user_update(float dt) override {
		handleUserInput(dt);

		seg_timer += dt;

		float t = seg_timer / time_per_seg;

		if (seg_timer > time_per_seg)
		{
			seg_timer = 0;
			for (auto& m : meshes) {
				m.translation = { 0,0,0 };
				m.rotation.z = cmn::Pi / 3;
				m.updateTransforms();

			}
			
		}
		

		for (auto& m : meshes)
		{
			


			
			UpdateAnimation(m, t);

		}
		

		return true;
	}

#pragma region RENDER HELPER

	void renderMesh(const Mesh& m) {
		sgl_begin_triangles();

		for (const auto& t : m.tris)
		{
			const auto& a = m.render_verts[t.ix[0]];
			const auto& b = m.render_verts[t.ix[1]];
			const auto& c = m.render_verts[t.ix[2]];

			vf3d ab = b - a, ac = c - a;
			vf3d norm = normalize(cross(ab, ac));
			vf3d ctr = (a + b + c) / 3;

			// if pointing away form cam, use flipped normal
			if (norm.dot(cam.pos - ctr) < 0) norm *= -1;

			//simple shading
			vf3d light_dir = (cam.pos - ctr).norm(); //light = cam
			float dp = cmn::clamp(norm.dot(light_dir), .5f, 1.f);
			sgl_c3f(dp * t.r, dp * t.g, dp * t.b);

			sgl_v3f(a.x, a.y, a.z);
			sgl_v3f(b.x, b.y, b.z);
			sgl_v3f(c.x, c.y, c.z);

			
		}

		sgl_end();
	}

	void Drawslider()
	{
		sgl_begin_lines();

		sgl_c3f(0, 1, 1);

		sgl_v2f(pose_slider.a.x, pose_slider.a.y);
		sgl_v2f(pose_slider.b.x, pose_slider.b.y);

		sgl_end();
	}


#pragma endregion

	bool user_render() override {
		sg_pass pass{};
		pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
		pass.action.colors[0].clear_value = { 0.5,1,1,1 };
		pass.swapchain = sglue_swapchain();
		sg_begin_pass(pass);

		sgl_defaults();
		sgl_load_pipeline(pip);
		sgl_matrix_mode_projection();
		sgl_load_matrix(cam.proj.m);
		sgl_matrix_mode_modelview();
		sgl_load_matrix(cam.view.m);

		Drawslider();

		for (auto& m : meshes) renderMesh(m);



		sgl_draw();
		sg_end_pass();

		sg_commit();


		return true;
	}

};
