// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#include "geoclipmap.h"

namespace MoYu
{
///////////////////////////
// Public Functions
///////////////////////////

/* Generate clipmap meshes originally by Mike J Savage
 * Article https://mikejsavage.co.uk/blog/geometry-clipmaps.html
 * Code http://git.mikejsavage.co.uk/medfall/file/clipmap.cc.html#l197
 * In email communication with Cory, Mike clarified that the code in his
 * repo can be considered either MIT or public domain.
 */
std::vector<GeoClipPatch> GeoClipMap::generate(int p_size, int p_levels)
{
    MoYu::LogSystem::Instance()->log(MoYu::LogSystem::LogLevel::Info,
                                     fmt::format("Generating meshes of size: {} levels: {}", p_size, p_levels));

	std::vector<GeoClipPatch> geoClipPatch_mesh = {};
    geoClipPatch_mesh.resize(5);

	int TILE_RESOLUTION = p_size;
	int PATCH_VERT_RESOLUTION = TILE_RESOLUTION + 1;
	int CLIPMAP_RESOLUTION = TILE_RESOLUTION * 4 + 1;
	int CLIPMAP_VERT_RESOLUTION = CLIPMAP_RESOLUTION + 1;
	int NUM_CLIPMAP_LEVELS = p_levels;

    AxisAlignedBox aabb;

	int n = 0;

	// Create a tile mesh
	// A tile is the main component of terrain panels
	// LOD0: 4 tiles are placed as a square in each center quadrant, for a total of 16 tiles
	// LOD1..N 3 tiles make up a corner, 4 corners uses 12 tiles
	{
        std::vector<glm::float3> vertices;
		vertices.resize(PATCH_VERT_RESOLUTION * PATCH_VERT_RESOLUTION);
        std::vector<uint32_t> indices;
		indices.resize(TILE_RESOLUTION * TILE_RESOLUTION * 6);

		n = 0;

		for (int y = 0; y < PATCH_VERT_RESOLUTION; y++) {
			for (int x = 0; x < PATCH_VERT_RESOLUTION; x++) {
				vertices[n++] = glm::float3(x, 0, y);
			}
		}

		n = 0;

		for (int y = 0; y < TILE_RESOLUTION; y++) {
			for (int x = 0; x < TILE_RESOLUTION; x++) {
				indices[n++] = _patch_2d(x, y, PATCH_VERT_RESOLUTION);
				indices[n++] = _patch_2d(x + 1, y + 1, PATCH_VERT_RESOLUTION);
				indices[n++] = _patch_2d(x, y + 1, PATCH_VERT_RESOLUTION);

				indices[n++] = _patch_2d(x, y, PATCH_VERT_RESOLUTION);
				indices[n++] = _patch_2d(x + 1, y, PATCH_VERT_RESOLUTION);
				indices[n++] = _patch_2d(x + 1, y + 1, PATCH_VERT_RESOLUTION);
			}
		}

		std::reverse(indices.begin(), indices.end());

		aabb = AxisAlignedBox(glm::float3(0, 0, 0), glm::float3(PATCH_VERT_RESOLUTION, 0.1, PATCH_VERT_RESOLUTION));
        geoClipPatch_mesh[MeshType::TILE] = (GeoClipPatch {aabb, vertices, indices});
	}

	// Create a filler mesh
	// These meshes are small strips that fill in the gaps between LOD1+,
	// but only on the camera X and Z axes, and not on LOD0.
	{
        std::vector<glm::float3> vertices;
		vertices.resize(PATCH_VERT_RESOLUTION * 8);
        std::vector<uint32_t> indices;
		indices.resize(TILE_RESOLUTION * 24);

		n = 0;
		int offset = TILE_RESOLUTION;

		for (int i = 0; i < PATCH_VERT_RESOLUTION; i++) {
			vertices[n] = glm::float3(offset + i + 1, 0, 0);
			aabb.merge(vertices[n]);
			n++;

			vertices[n] = glm::float3(offset + i + 1, 0, 1);
            aabb.merge(vertices[n]);
			n++;
		}

		for (int i = 0; i < PATCH_VERT_RESOLUTION; i++) {
			vertices[n] = glm::float3(1, 0, offset + i + 1);
            aabb.merge(vertices[n]);
			n++;

			vertices[n] = glm::float3(0, 0, offset + i + 1);
            aabb.merge(vertices[n]);
			n++;
		}

		for (int i = 0; i < PATCH_VERT_RESOLUTION; i++) {
			vertices[n] = glm::float3(-(offset + i), 0, 1);
            aabb.merge(vertices[n]);
			n++;

			vertices[n] = glm::float3(-(offset + i), 0, 0);
            aabb.merge(vertices[n]);
			n++;
		}

		for (int i = 0; i < PATCH_VERT_RESOLUTION; i++) {
			vertices[n] = glm::float3(0, 0, -(offset + i));
            aabb.merge(vertices[n]);
			n++;

			vertices[n] = glm::float3(1, 0, -(offset + i));
            aabb.merge(vertices[n]);
			n++;
		}

		n = 0;
		for (int i = 0; i < TILE_RESOLUTION * 4; i++) {
			int arm = i / TILE_RESOLUTION;

			int bl = (arm + i) * 2 + 0;
			int br = (arm + i) * 2 + 1;
			int tl = (arm + i) * 2 + 2;
			int tr = (arm + i) * 2 + 3;

			if (arm % 2 == 0) {
				indices[n++] = br;
				indices[n++] = bl;
				indices[n++] = tr;
				indices[n++] = bl;
				indices[n++] = tl;
				indices[n++] = tr;
			} else {
				indices[n++] = br;
				indices[n++] = bl;
				indices[n++] = tl;
				indices[n++] = br;
				indices[n++] = tl;
				indices[n++] = tr;
			}
		}

		std::reverse(indices.begin(), indices.end());

		geoClipPatch_mesh[MeshType::FILLER] = (GeoClipPatch {aabb, vertices, indices});
	}

	// Create trim mesh
	// This mesh is a skinny L shape that fills in the gaps between
	// LOD meshes when they are moving at different speeds and have gaps
	{
        std::vector<glm::float3> vertices;
		vertices.resize((CLIPMAP_VERT_RESOLUTION * 2 + 1) * 2);
        std::vector<uint32_t> indices;
		indices.resize((CLIPMAP_VERT_RESOLUTION * 2 - 1) * 6);

		n = 0;
		glm::float3 offset = glm::float3(0.5f * (CLIPMAP_VERT_RESOLUTION + 1), 0, 0.5f * (CLIPMAP_VERT_RESOLUTION + 1));

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION + 1; i++) {
			vertices[n] = glm::float3(0, 0, CLIPMAP_VERT_RESOLUTION - i) - offset;
			aabb.merge(vertices[n]);
			n++;

			vertices[n] = glm::float3(1, 0, CLIPMAP_VERT_RESOLUTION - i) - offset;
			aabb.merge(vertices[n]);
			n++;
		}

		int start_of_horizontal = n;

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION; i++) {
			vertices[n] = glm::float3(i + 1, 0, 0) - offset;
			aabb.merge(vertices[n]);
			n++;

			vertices[n] = glm::float3(i + 1, 0, 1) - offset;
			aabb.merge(vertices[n]);
			n++;
		}

		n = 0;

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION; i++) {
			indices[n++] = (i + 0) * 2 + 1;
			indices[n++] = (i + 0) * 2 + 0;
			indices[n++] = (i + 1) * 2 + 0;

			indices[n++] = (i + 1) * 2 + 1;
			indices[n++] = (i + 0) * 2 + 1;
			indices[n++] = (i + 1) * 2 + 0;
		}

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION - 1; i++) {
			indices[n++] = start_of_horizontal + (i + 0) * 2 + 1;
			indices[n++] = start_of_horizontal + (i + 0) * 2 + 0;
			indices[n++] = start_of_horizontal + (i + 1) * 2 + 0;

			indices[n++] = start_of_horizontal + (i + 1) * 2 + 1;
			indices[n++] = start_of_horizontal + (i + 0) * 2 + 1;
			indices[n++] = start_of_horizontal + (i + 1) * 2 + 0;
		}

		std::reverse(indices.begin(), indices.end());

		geoClipPatch_mesh[MeshType::TRIM] = (GeoClipPatch {aabb, vertices, indices});
	}

	// Create center cross mesh
	// This mesh is the small cross shape that fills in the gaps along the
	// X and Z axes between the center quadrants on LOD0.
	{
		std::vector<glm::float3> vertices;
		vertices.resize(PATCH_VERT_RESOLUTION * 8);
        std::vector<uint32_t> indices;
		indices.resize(TILE_RESOLUTION * 24 + 6);

		n = 0;

		for (int i = 0; i < PATCH_VERT_RESOLUTION * 2; i++) {
			vertices[n] = glm::float3(i - (TILE_RESOLUTION), 0, 0);
			aabb.merge(vertices[n]);
			n++;

			vertices[n] = glm::float3(i - (TILE_RESOLUTION), 0, 1);
			aabb.merge(vertices[n]);
			n++;
		}

		int start_of_vertical = n;

		for (int i = 0; i < PATCH_VERT_RESOLUTION * 2; i++) {
			vertices[n] = glm::float3(0, 0, i - (TILE_RESOLUTION));
			aabb.merge(vertices[n]);
			n++;

			vertices[n] = glm::float3(1, 0, i - (TILE_RESOLUTION));
			aabb.merge(vertices[n]);
			n++;
		}

		n = 0;

		for (int i = 0; i < TILE_RESOLUTION * 2 + 1; i++) {
			int bl = i * 2 + 0;
			int br = i * 2 + 1;
			int tl = i * 2 + 2;
			int tr = i * 2 + 3;

			indices[n++] = br;
			indices[n++] = bl;
			indices[n++] = tr;
			indices[n++] = bl;
			indices[n++] = tl;
			indices[n++] = tr;
		}

		for (int i = 0; i < TILE_RESOLUTION * 2 + 1; i++) {
			if (i == TILE_RESOLUTION) {
				continue;
			}

			int bl = i * 2 + 0;
			int br = i * 2 + 1;
			int tl = i * 2 + 2;
			int tr = i * 2 + 3;

			indices[n++] = start_of_vertical + br;
			indices[n++] = start_of_vertical + tr;
			indices[n++] = start_of_vertical + bl;
			indices[n++] = start_of_vertical + bl;
			indices[n++] = start_of_vertical + tr;
			indices[n++] = start_of_vertical + tl;
		}

		std::reverse(indices.begin(), indices.end());

		geoClipPatch_mesh[MeshType::CROSS] = (GeoClipPatch {aabb, vertices, indices});
	}

	// Create seam mesh
	// This is a very thin mesh that is supposed to cover tiny gaps
	// between tiles and fillers when the vertices do not line up
	{
		std::vector<glm::float3> vertices;
		vertices.resize(CLIPMAP_VERT_RESOLUTION * 4);
        std::vector<uint32_t> indices;
		indices.resize(CLIPMAP_VERT_RESOLUTION * 6);

		n = 0;

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION; i++) {
			n = CLIPMAP_VERT_RESOLUTION * 0 + i;
			vertices[n] = glm::float3(i, 0, 0);
			aabb.merge(vertices[n]);

			n = CLIPMAP_VERT_RESOLUTION * 1 + i;
			vertices[n] = glm::float3(CLIPMAP_VERT_RESOLUTION, 0, i);
			aabb.merge(vertices[n]);

			n = CLIPMAP_VERT_RESOLUTION * 2 + i;
			vertices[n] = glm::float3(CLIPMAP_VERT_RESOLUTION - i, 0, CLIPMAP_VERT_RESOLUTION);
			aabb.merge(vertices[n]);

			n = CLIPMAP_VERT_RESOLUTION * 3 + i;
			vertices[n] = glm::float3(0, 0, CLIPMAP_VERT_RESOLUTION - i);
			aabb.merge(vertices[n]);
		}

		n = 0;

		for (int i = 0; i < CLIPMAP_VERT_RESOLUTION * 4; i += 2) {
			indices[n++] = i + 1;
			indices[n++] = i;
			indices[n++] = i + 2;
		}

		indices[indices.size() - 1] = 0;

		std::reverse(indices.begin(), indices.end());

		geoClipPatch_mesh[MeshType::SEAM] = (GeoClipPatch {aabb, vertices, indices});
	}

	// skirt mesh
	/*{
		real_t scale = real_t(1 << (NUM_CLIPMAP_LEVELS - 1));
		real_t fbase = real_t(TILE_RESOLUTION << NUM_CLIPMAP_LEVELS);
		Vector2 base = -Vector2(fbase, fbase);

		Vector2 clipmap_tl = base;
		Vector2 clipmap_br = clipmap_tl + (Vector2(CLIPMAP_RESOLUTION, CLIPMAP_RESOLUTION) * scale);

		real_t big = 10000000.0;
		Array vertices = Array::make(
			Vector3(-1, 0, -1) * big,
			Vector3(+1, 0, -1) * big,
			Vector3(-1, 0, +1) * big,
			Vector3(+1, 0, +1) * big,
			Vector3(clipmap_tl.x, 0, clipmap_tl.y),
			Vector3(clipmap_br.x, 0, clipmap_tl.y),
			Vector3(clipmap_tl.x, 0, clipmap_br.y),
			Vector3(clipmap_br.x, 0, clipmap_br.y)
		);

		Array indices = Array::make(
			0, 1, 4, 4, 1, 5,
			1, 3, 5, 5, 3, 7,
			3, 2, 7, 7, 2, 6,
			4, 6, 0, 0, 6, 2
		);

		skirt_mesh = _create_mesh(PackedVector3Array(vertices), PackedInt32Array(indices));

	}*/

	return geoClipPatch_mesh;
}

} // namespace MoYu