#define TINYOBJLOADER_IMPLEMENTATION

#include "model.h"

#include "utils/error_handler.h"

#include <linalg.h>


using namespace linalg::aliases;
using namespace cg::world;

#define vx(index) (3 * index)
#define vy(index) (3 * index + 1)
#define vz(index) (3 * index + 2)

#define fill_color(vname, mname) \
	vname##_r = mname[0];        \
	vname##_g = mname[1];        \
	vname##_b = mname[2];
#define make_index_tuple(idx) std::make_tuple(idx.vertex_index, idx.normal_index, idx.texcoord_index);

cg::world::model::model() {}

cg::world::model::~model() {}

void cg::world::model::load_obj(const std::filesystem::path& model_path)
{
	auto model_folder = model_path.parent_path();

	tinyobj::ObjReaderConfig config;
	config.mtl_search_path = model_folder.string();
	config.triangulate = true;

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(model_path.string(), config) && !reader.Error().empty()) {
		THROW_ERROR(reader.Error());
	}

	auto& shapes = reader.GetShapes();
	auto& attrib = reader.GetAttrib();
	auto& materials = reader.GetMaterials();

	allocate_buffers(shapes);
	fill_buffers(shapes, attrib, materials, model_folder);
}

void model::allocate_buffers(const std::vector<tinyobj::shape_t>& shapes)
{
	for (const auto& shape: shapes) {
		size_t vertex_offset = 0;
		unsigned int vertex_buffer_size = 0;
		unsigned int index_buffer_size = 0;

		std::map<std::tuple<int, int, int>, unsigned int> index_map;
		const auto& mesh = shape.mesh;

		for (const auto& fv: shape.mesh.num_face_vertices) {
			for (size_t v = 0; v < fv; v++) {
				auto idx = mesh.indices[vertex_offset + v];
				auto idx_tuple = make_index_tuple(idx);

				if (index_map.count(idx_tuple) == 0) {
					index_map[idx_tuple] = vertex_buffer_size;
					++vertex_buffer_size;
				}

				index_buffer_size++;
			}
			vertex_offset += fv;
		}

		vertex_buffers.push_back(std::make_shared<cg::resource<cg::vertex>>(vertex_buffer_size));
		index_buffers.push_back(std::make_shared<cg::resource<unsigned int>>(index_buffer_size));
	}

	textures.resize(shapes.size());
}

float3 cg::world::model::compute_normal(const tinyobj::attrib_t& attrib, const tinyobj::mesh_t& mesh, size_t index_offset)
{
	auto a_id = mesh.indices[index_offset];
	auto b_id = mesh.indices[index_offset + 1];
	auto c_id = mesh.indices[index_offset + 2];

	float3 a{
			attrib.vertices[vx(a_id.vertex_index)],
			attrib.vertices[vy(a_id.vertex_index)],
			attrib.vertices[vz(a_id.vertex_index)],
	};

	float3 b{
			attrib.vertices[vx(b_id.vertex_index)],
			attrib.vertices[vy(b_id.vertex_index)],
			attrib.vertices[vz(b_id.vertex_index)],
	};

	float3 c{
			attrib.vertices[vx(c_id.vertex_index)],
			attrib.vertices[vy(c_id.vertex_index)],
			attrib.vertices[vz(c_id.vertex_index)],
	};

	return normalize(cross(b - a, c - a));
}

void model::fill_vertex_data(cg::vertex& vertex, const tinyobj::attrib_t& attrib, const tinyobj::index_t idx, const float3 computed_normal, const tinyobj::material_t material)
{
	vertex.x = attrib.vertices[vx(idx.vertex_index)];
	vertex.y = attrib.vertices[vy(idx.vertex_index)];
	vertex.z = attrib.vertices[vz(idx.vertex_index)];

	if (idx.normal_index < 0) {
		vertex.nx = computed_normal.x;
		vertex.ny = computed_normal.y;
		vertex.nz = computed_normal.z;
	}
	else {
		vertex.nx = attrib.normals[vx(idx.normal_index)];
		vertex.ny = attrib.normals[vy(idx.normal_index)];
		vertex.nz = attrib.normals[vz(idx.normal_index)];
	}

	if (idx.texcoord_index < 0) {
		vertex.tx = 0.f;
		vertex.ty = 0.f;
	}
	else {
		vertex.tx = attrib.texcoords[2 * idx.texcoord_index];
		vertex.ty = attrib.texcoords[2 * idx.texcoord_index + 1];
	}

	fill_color(vertex.ambient, material.ambient);
	fill_color(vertex.diffuse, material.diffuse);
	fill_color(vertex.emissive, material.emission);
}

void model::fill_buffers(const std::vector<tinyobj::shape_t>& shapes, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::material_t>& materials, const std::filesystem::path& base_folder)
{
	for (size_t s = 0; s < shapes.size(); s++) {
		size_t index_offset = 0;

		unsigned int vertex_buffer_id = 0;
		unsigned int index_buffer_id = 0;

		auto vertex_buffer = vertex_buffers[s];
		auto index_buffer = index_buffers[s];
		std::map<std::tuple<int, int, int>, unsigned int> index_map;
		const auto& mesh = shapes[s].mesh;

		for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
			int fv = mesh.num_face_vertices[f];
			float3 normal;

			if (mesh.indices[index_offset].normal_index < 0) {
				normal = compute_normal(attrib, mesh, index_offset);
			}

			for (size_t v = 0; v < fv; v++) {
				auto idx = mesh.indices[index_offset + v];
				auto idx_tuple = make_index_tuple(idx);

				if (index_map.count(idx_tuple) == 0) {
					cg::vertex& vertex = vertex_buffer->item(vertex_buffer_id);
					const auto& material = materials[mesh.material_ids[f]];
					fill_vertex_data(vertex, attrib, idx, normal, material);
					index_map[idx_tuple] = vertex_buffer_id;
					vertex_buffer_id++;
				}

				index_buffer->item(index_buffer_id) = index_map[idx_tuple];
				index_buffer_id++;
			}

			index_offset += fv;
		}

		if (!materials[mesh.material_ids[0]].diffuse_texname.empty()) {
			textures[s] = base_folder / materials[mesh.material_ids[0]].diffuse_texname;
		}
	}
}


const std::vector<std::shared_ptr<cg::resource<cg::vertex>>>&
cg::world::model::get_vertex_buffers() const
{
	return vertex_buffers;
}

const std::vector<std::shared_ptr<cg::resource<unsigned int>>>&
cg::world::model::get_index_buffers() const
{
	return index_buffers;
}

const std::vector<std::filesystem::path>& cg::world::model::get_per_shape_texture_files() const
{
	return textures;
}


const float4x4 cg::world::model::get_world_matrix() const
{
	return float4x4{
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}};
}
