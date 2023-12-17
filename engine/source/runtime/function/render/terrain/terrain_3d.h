// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef TERRAIN3D_CLASS_H
#define TERRAIN3D_CLASS_H

#ifndef __FLT_MAX__
#define __FLT_MAX__ FLT_MAX
#endif

#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/terrain/geoclipmap.h"

namespace MoYu
{

	struct TransformInstance
	{
        Transform cross;
        std::vector<Transform> tiles;
        std::vector<Transform> fillers;
        std::vector<Transform> trims;
        std::vector<Transform> seams;
	};

	class Terrain3D {
	private:
		// Terrain state
		bool _initialized = false;

		// Terrain settings
		int _mesh_size = 48;
		int _mesh_lods = 7;

		// X,Z Position of the camera during the previous snapping. Set to max float value to force a snap update.
		glm::float2 _camera_last_position = glm::float2(__FLT_MAX__, __FLT_MAX__);

		// Meshes and Mesh instances
		std::vector<GeoClipPatch> _meshes;
        TransformInstance _data;

		// Renderer settings
		void _initialize();
		void __ready();
		void __process(double delta, glm::float3 cam_pos);

		void _clear(bool p_clear_meshes = true);
		void _build(int p_mesh_lods, int p_mesh_size);

	public:
		Terrain3D();
		~Terrain3D();

		// Terrain settings
		void set_mesh_size_lods(int p_size, int p_count);
		int get_mesh_lods() const { return _mesh_lods; }
		int get_mesh_size() const { return _mesh_size; }

		TransformInstance get_transform_instance() const;
        std::vector<GeoClipPatch> get_clip_patch() const;

		// Terrain methods
		void snap(glm::float3 p_cam_pos);
	};

} // namespace MoYu

#endif // TERRAIN3D_CLASS_H
