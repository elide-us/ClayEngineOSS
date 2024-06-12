#include "pch.h"
#include "Voxel.h"

using namespace ClayEngine::SimpleMath;

uint8_t Voxel::EncodeVoxelVector(float vector)
{
	// (double + .5) / (2.5 / 256)
	auto r = (vector + std::abs(c_voxel_vector_min)) / c_vector_pack_ratio;
	return static_cast<uint8_t>(std::floor(r));
}

float Voxel::DecodeVoxelVector(uint8_t vector)
{
	// (char * (2.5 / 256)) - .5
	return float((vector * c_vector_pack_ratio) - std::abs(c_voxel_vector_min));
}

void Voxel::SetVoxel(float& z, float& y, float& x, unsigned int& lod)
{
	m_z = EncodeVoxelVector(z);
	m_y = EncodeVoxelVector(y);
	m_x = EncodeVoxelVector(x);
	m_lod = static_cast<uint32_t>(lod);
}

void Voxel::GetVoxel(float& z, float& y, float& x, unsigned int& lod)
{
	z = DecodeVoxelVector(m_z);
	y = DecodeVoxelVector(m_y);
	x = DecodeVoxelVector(m_x);
	lod = static_cast<uint32_t>(m_lod);
}

VoxelData Voxel::GetVoxelData()
{
	return std::tuple{ DecodeVoxelVector(m_z), DecodeVoxelVector(m_y), DecodeVoxelVector(m_x), m_lod };
}

Voxel::Voxel()
{
	m_z = 0;
	m_y = 0;
	m_x = 0;
	m_lod = 0;
	m_flags = 0;
}

Voxel::Voxel(float z, float y, float x, unsigned int lod)
{
	m_z = EncodeVoxelVector(z);
	m_y = EncodeVoxelVector(y);
	m_x = EncodeVoxelVector(x);
	m_lod = static_cast<uint32_t>(lod); //TODO: Research if this static cast actually does anything here...
	m_flags = static_cast<uint32_t>(0x00);
}

Voxel::~Voxel()
{

}

Voxel::operator uint32_t()
{
	return *(reinterpret_cast<uint32_t*>(this)); // pointer to this, reinterpreted as a uint32_t pointer, dereferenced.
}
