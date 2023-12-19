// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#include "geoclipmap.h"
#include "terrain_3d.h"

namespace MoYu
{

	#define LOG(str) \
    MoYu::LogSystem::Instance()->log(MoYu::LogSystem::LogLevel::Info, str);

///////////////////////////
// Private Functions
///////////////////////////

// Initialize static member variable

void Terrain3D::_initialize() {
	// Initialize the system
	if (!_initialized) {
		_build(_mesh_lods, _mesh_size);
		_initialized = true;
	}
}

void Terrain3D::__ready() {
	_initialize();
}

/**
 * This is a proxy for _process(delta) called by _notification() due to
 * https://github.com/godotengine/godot-cpp/issues/1022
 */
void Terrain3D::__process(double delta, glm::float3 cam_pos)
{
	if (!_initialized)
		return;

	// If camera has moved enough, re-center the terrain on it.
    glm::float2 cam_pos_2d = glm::float2(cam_pos.x, cam_pos.z);
    if (glm::length(_camera_last_position - cam_pos_2d) > 0.2f)
    {
        snap(cam_pos);
        _camera_last_position = cam_pos_2d;
    }
}

void Terrain3D::_clear(bool p_clear_meshes) {
    _meshes.clear();
    _data.tiles.clear();
    _data.fillers.clear();
    _data.trims.clear();
    _data.seams.clear();
    _initialized = false;
}

void Terrain3D::_build(int p_mesh_lods, int p_mesh_size) {
    LOG(fmt::format("Building the terrain meshes"));

	// Generate terrain meshes, lods, seams
	_meshes = GeoClipMap::generate(p_mesh_size, p_mesh_lods);
	
	// Get current visual scenario so the instances appear in the scene
	_data.cross = Transform();
	
	for (int l = 0; l < p_mesh_lods; l++) {
		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				if (l != 0 && (x == 1 || x == 2) && (y == 1 || y == 2)) {
					continue;
				}
				// GeoClipMap::TILE
				_data.tiles.push_back(Transform());
			}
		}

		// GeoClipMap::FILLER
        _data.fillers.push_back(Transform());

		if (l != p_mesh_lods - 1) {
			// GeoClipMap::TRIM
            _data.trims.push_back(Transform());

			// GeoClipMap::SEAM
            _data.seams.push_back(Transform());
		}
	}

	//update_aabbs();
	// Force a snap update
	_camera_last_position = glm::float2(__FLT_MAX__, __FLT_MAX__);
}

///////////////////////////
// Public Functions
///////////////////////////

Terrain3D::Terrain3D() { __ready(); }

Terrain3D::~Terrain3D() {}

void Terrain3D::set_mesh_size_lods(int p_size, int p_count)
{
    if (_mesh_lods != p_count || _mesh_size != p_size)
    {
        LOG(fmt::format("Setting mesh levels: {}, mesh size: {}", p_count, p_size));
		_mesh_lods = p_count;
        _mesh_size = p_size;
		_clear();
		_initialize();
	}
}

TransformInstance Terrain3D::get_transform_instance() const
{
	return _data;
}

std::vector<GeoClipPatch> Terrain3D::get_clip_patch() const
{
	return _meshes;
}

/**
 * Centers the terrain and LODs on a provided position. Y height is ignored.
 */
void Terrain3D::snap(glm::float2 p_cam_xz)
{
    LOG(fmt::format("Snapping terrain to: ({},{})", p_cam_xz.x, p_cam_xz.y));

	glm::float3 p_cam_pos = glm::float3(p_cam_xz.x, 0, p_cam_xz.y);

	// Position cross
	{
        Transform t  = Transform();
        t.m_position = glm::floor(p_cam_pos);
		_data.cross = t;
    }

	int edge = 0;
	int tile = 0;

	for (int l = 0; l < _mesh_lods; l++) {
		float scale = float(1 << l);
		glm::float3 snapped_pos = glm::floor(p_cam_pos / scale) * scale;
		glm::float3 tile_size = glm::float3((_mesh_size << l), 0, (_mesh_size << l));
		glm::float3 base = snapped_pos - glm::float3((_mesh_size << (l + 1)), 0, (_mesh_size << (l + 1)));

		// Position tiles
		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				if (l != 0 && (x == 1 || x == 2) && (y == 1 || y == 2)) {
					continue;
				}

				glm::float3 fill = glm::float3(x >= 2 ? 1 : 0, 0, y >= 2 ? 1 : 0) * scale;
				glm::float3 tile_tl = base + glm::float3(x, 0, y) * tile_size + fill;
				//glm::float3 tile_br = tile_tl + tile_size;

				Transform t  = Transform();
                t.m_scale    = glm::float3(scale, 1, scale);
                t.m_position = tile_tl;

				_data.tiles[tile] = t;

				tile++;
			}
		}
		{
			Transform t = Transform();
            t.m_position = snapped_pos;
            t.m_scale    = glm::float3(scale, 1, scale);

			_data.fillers[l] = t;
		}

		if (l != _mesh_lods - 1) {
			float next_scale = scale * 2.0f;
			glm::float3 next_snapped_pos = glm::floor(p_cam_pos / next_scale) * next_scale;

			// Position trims
			{
				glm::float3 tile_center = snapped_pos + (glm::float3(scale, 0, scale) * 0.5f);
				glm::float3 d = p_cam_pos - next_snapped_pos;

				int r = 0;
				r |= d.x >= scale ? 0 : 2;
				r |= d.z >= scale ? 0 : 1;

				float rotations[4] = { 0.0, 270.0, 90, 180.0 };

				float angle = MoYu::f::DEG_TO_RAD * (rotations[r]);

				Transform t = Transform();
                t.m_rotation = glm::toQuat(glm::rotate(-angle, glm::float3(0, 1, 0)));
                t.m_scale = glm::float3(scale, 1, scale);
                t.m_position = tile_center;

				_data.trims[edge] = t;
			}

			// Position seams
			{
				glm::float3 next_base = next_snapped_pos - glm::float3((_mesh_size << (l + 1)), 0, (_mesh_size << (l + 1)));

				Transform t = Transform();
                t.m_scale = glm::float3(scale, 1, scale);
				t.m_position = next_base;

				_data.seams[edge] = t;
			}
			edge++;
		}
	}
}
}