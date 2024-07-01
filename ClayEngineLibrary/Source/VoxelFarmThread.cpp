#include "pch.h"
#include "VoxelFarmThread.h"
#include "VoxelFarmCellRender.h"

using namespace ClayEngine;

#include "VoxelFarm.h"
#include "Generator.h"
#include "perlin.h"
#include "contour.h"

#include "ClipmapView.h"
#include "InstanceMesh.h"

class SimplePerlinVoxelLayer : public VoxelFarm::IVoxelLayer
{
	using Index = VoxelFarm::ContourVoxelData::Index;

public:
	void getContourData(VoxelFarm::CellId cell, VoxelFarm::ContourVoxelData* data, bool& empty, void* threadContext)
	{
		empty = true;

		int level, cx, cy, cz;
		VoxelFarm::unpackCellId(cell, level, cx, cy, cz);
		auto scale = VoxelFarm::CELL_SIZE * (1ll << level);

		for (int z = 0; z < VoxelFarm::BLOCK_SIZE; ++z)
		for (int x = 0; x < VoxelFarm::BLOCK_SIZE; ++x)
		for (int y = 0; y < VoxelFarm::BLOCK_SIZE; ++y)
		{
			Index index(x, y, z);

			auto dx = static_cast<double>(x - VoxelFarm::BLOCK_MARGIN) / static_cast<double>(VoxelFarm::BLOCK_DIMENSION);
			auto dy = static_cast<double>(y - VoxelFarm::BLOCK_MARGIN) / static_cast<double>(VoxelFarm::BLOCK_DIMENSION);
			auto dz = static_cast<double>(z - VoxelFarm::BLOCK_MARGIN) / static_cast<double>(VoxelFarm::BLOCK_DIMENSION);

			auto wx = 0.00001 * (cx + dx) * scale;
			auto wy = 0.00001 * (cy + dy) * scale;
			auto wz = 0.00001 * (cz + dz) * scale;

			double field = VoxelFarm::PerlinNoise3D(wx, wy, wz, 0.5, 2.0, 2);

			// If field is positive, the voxel is solid
			if (field > 0.0)
			{
				// Set material 1 to indicate solid
				data->setMaterial(index, 1);
				empty = false;
			}
		}
	}
};
using SimplePerlinVoxelLayerPtr = std::unique_ptr<SimplePerlinVoxelLayer>;

using MaterialLibrary = VoxelFarm::CMaterialLibrary;
using Material = VoxelFarm::CMaterial;
using Generator = VoxelFarm::CGenerator;
using CellData = VoxelFarm::CCellData;
using ThreadContext = VoxelFarm::CCellData::ThreadContext;

void VoxelFarmThreadFunctor::operator()(Future future)
{
	using namespace VoxelFarm;

	initPerlin();
	LODStats stats;

	MaterialLibrary* materials = VF_NEW MaterialLibrary(); // Instance specific contains Materials
	materials->materialCount = 2;
	materials->materialIndex = VF_ALLOC(Material, materials->materialCount);
	memset(materials->materialIndex, 0, materials->materialCount * sizeof(Material));
	memset(&materials->billboardPack, 0, sizeof(materials->billboardPack));
	materials->materialIndex[0].billboard = -1;
	materials->materialIndex[1].billboard = 1;
	materials->materialIndex[1].billboardType = 1;

	SimplePerlinVoxelLayer* layer = VF_NEW SimplePerlinVoxelLayer(); // Simple perlin noise layer implementation

	Generator* generator = VF_NEW Generator(); // Voxel generator, composes voxels from layers
	generator->addVoxelLayer(layer);

	ContourThreadContext* contour_context = VF_NEW ContourThreadContext(); // A thread for generating voxel contour data
	ThreadContext* context_cell_data = VF_NEW ThreadContext(); // A thread for generating contour mesh data

	while (future.wait_for(std::chrono::nanoseconds(1)) == std::future_status::timeout)
	{
		contour_context->data->clear();

		// To generate cell mesh data you need a CellId, here we're just generating a random CellId
		int level = rand() % VoxelFarm::LEVELS;
		int xc = rand();
		int yc = rand();
		int zc = rand();
		CellId* cell = VF_NEW CellId(packCellId(level, xc, yc, zc));
		
		// First we generate a cell's voxel data and store it in the thread contour_context->data
		bool empty;
		generator->generate(*cell, contour_context->data, empty, stats);
		if (empty) continue;

		// Create a CellData object to hold the cell mesh data
		CellData* cell_data = VF_NEW CellData(*cell);
		if (!contourCellDataMCA(contour_context, NULL, materials, cell_data, NULL, context_cell_data, true, stats))
			continue;

		//Services::GetService<ClayEngine::Game::VoxelFarmCellRender>()->SetCellData(cell_data);

		// Print details of the cell data that gets generated
		int faces = cell_data->faceCount[CellData::MEDIUM_SOLID];
		std::wstringstream wss;
		wss << "LOD_" << level << " [" << xc << "," << yc << "," << zc << "] " << faces << " faces";
		WriteLine(wss.str());


		VF_DELETE cell_data;
		VF_DELETE cell;
	}

	VF_DELETE context_cell_data;
	VF_DELETE contour_context;

	VF_DELETE generator;
	VF_DELETE layer;
	VF_DELETE materials;
}

VoxelFarmThread::VoxelFarmThread()
{
	m_thread = std::thread{ VoxelFarmThreadFunctor(), std::move(m_promise.get_future()) };
}

VoxelFarmThread::~VoxelFarmThread()
{
	m_promise.set_value();
	if (m_thread.joinable()) m_thread.join();
}


////////////////////////////////////////////////////////////////////////////////


//extern "C" __declspec(dllexport) int __cdecl v1_getVoxels(
//	char* object,
//	VoxelFarm::API::CellId cell,
//	double cellOrigin[3],
//	double cellSize,
//	double voxelSize,
//	int blockSize,
//	VoxelFarm::API::VoxelType * changeFlags,
//	VoxelFarm::API::MaterialId * materials,
//	VoxelFarm::API::Vector * vectors,
//	bool& empty,
//	void* threadSafeData)
//{
//	double MinHeight = 100000;
//	double minimumGround = 120000;
//	double MaxHeight = 123000;
//	//I don't care about data above/below are min/max heights.
//	if (cellOrigin[1] + cellSize < MinHeight ||
//		cellOrigin[1] > MaxHeight)
//	{
//		return VoxelFarm::API::RESULT_OK;
//	}
//
//	OreWave& wave = objects[object];
//
//	auto oreVein = ((OreVeinStruct*)threadSafeData);
//	// just contains a double[][]
//
//	int level, xx, yy, zz;
//	unpackCellId(cell, level, xx, yy, zz); // can be found in mapindex.h
//
//	int delta = 1;
//	// LOD0 == 2, LOD1 == 3, LOD2+ is 4
//	if (level == 2) { delta = 4; }
//	else if (level == 3) { delta = 2; }
//	else { delta = 1;  }
//
//	// pre generate the 'height map' for the terrain
//	for (int z = 0; z < blockSize; z += delta)
//	{
//		for (int x = 0; x < blockSize; x += delta)
//		{
//			double worldX = cellOrigin[0] + voxelSize * x;
//			double worldZ = cellOrigin[2] + voxelSize * z;
//
//			double noise = PerlinNoise.octaveNoise0_1(worldX * 0.0005, worldZ * 0.0005, 4); // (x, y, octaves)
//
//			double height = minimumGround + (MaxHeight - minimumGround) * noise;
//			// forcing to up to 4x4 sections for LOD0, 2x2 for LOD1, and 1x1 for above
//			for (int dx = 0; dx < delta; dx++)
//			{
//				for (int dz = 0; dz < delta; dz++)
//				{
//					oreVein->heightmap[x + dx][z + dz] = height;
//				}
//			}
//		}
//	}
//	// using just generated data, lets modify the voxel data
//	int dz = 0, dx = 0, dy = 0;
//	int voxelIndex;
//	for (int z = 0; z < blockSize; z += 1)
//		for (int x = 0; x < blockSize; x += 1)
//		{
//			for (int y = 0; y < blockSize; y += delta)
//			{
//				double worldY = cellOrigin[1] + voxelSize * y;
//
//				if (worldY <= oreVein->heightmap[x][z])
//				{
//					//if we are here, we are not above ground air. 
//					// default material is dirt (underground)
//					int material = 1;
//					// if we are within some number of blocks of surface, lets go for some grass for now. 
//					if (worldY >= oreVein->heightmap[x][z] - voxelSize * 16)
//						material = 2;
//					empty = false;
//
//					for (int dy = 0; dy < delta; dy++)
//					{
//						voxelIndex = (z + 0) * blockSize * blockSize + (x + 0) * blockSize + (y + dy);
//						materials[voxelIndex] = material;
//						changeFlags[voxelIndex] |= VoxelFarm::API::VOXEL_HAS_MATERIAL;
//					}
//				}
//			}
//		}
//
//	//populate some ore clusters
//
//	// NOTE: This isn't a great way to do this, this is just a very basic and bad example that is barely functional.
//	int num_ore_clusters = 5;
//	int ore_size = 12;
//	int material = 3; // put gold/iron material in material id slot = 3.
//
//	// only bother showing to the user if its nearby (LOD0 == 2)
//	if (level == 2)
//	{
//		for (int i = 0; i < num_ore_clusters; i++)
//		{
//			// use values for 'seeds' into perlin noise
//			double worldZ = cellOrigin[2] + voxelSize * ((i * num_ore_clusters) / blockSize);
//			double worldX = cellOrigin[0] + voxelSize * ((i * num_ore_clusters) / blockSize);
//			double worldY = cellOrigin[1] + voxelSize * ((i * num_ore_clusters) / blockSize);
//
//			int initial_z = PerlinNoise.noise0_1(worldZ) * blockSize;
//			int initial_x = PerlinNoise.noise0_1(worldX) * blockSize;
//			int initial_y = PerlinNoise.noise0_1(worldY) * blockSize;
//
//			// are we underground at the core here?
//			if (cellOrigin[1] + voxelSize * initial_y < oreVein->heightmap[initial_x][initial_z])
//			{
//				// we allow ore to 'pop' out of the ground.
//				// just plopping down a cube shaped ore vein!
//				for (int dz = -(ore_size / 2); dz < ore_size / 2; dz++)
//					for (int dx = -(ore_size / 2); dx < ore_size / 2; dx++)
//						for (int dy = -(ore_size / 2); dy < ore_size / 2; dy++)
//						{
//							// don't go outside our particular chunk (0-40)
//							int place_z = clamp(initial_z + dz, 0, 40);
//							int place_x = clamp(initial_x + dx, 0, 40);
//							int place_y = clamp(initial_y + dy, 0, 40);
//							voxelIndex = (place_z)*blockSize * blockSize + (place_x)*blockSize + (place_y);
//							materials[voxelIndex] = material;
//							changeFlags[voxelIndex] |= VoxelFarm::API::VOXEL_HAS_MATERIAL;
//						}
//
//			}
//		}
//	}
//	return VoxelFarm::API::RESULT_OK;
//}
