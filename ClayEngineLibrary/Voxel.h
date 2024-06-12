#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Voxel Class (C) 2022 Epoch Meridian, LLC.                       */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "ClayEngine.h"

#include <tuple>

namespace ClayEngine
{
	namespace SimpleMath
	{
		using VoxelData = std::tuple<float, float, float, unsigned int>;

		/// <summary>
		/// Starting here are the constant expressions that are used to make the calculations to "compress" a
		/// typical 3D float vector into 32 bits as 3 bytes and a spare pair of nibbles used for LOD and flags
		/// </summary>
		constexpr auto c_packed_vector_bits{ 8ul };
		constexpr auto c_packed_lod_bits{ 4ul };
		constexpr auto c_packed_flag_bits{ 4ul };

		constexpr auto c_voxel_vector_pack{ 1 << c_packed_vector_bits };
		constexpr auto c_voxel_vector_max{ 2.f }; // Default .5 + 1.5
		constexpr auto c_voxel_vector_min{ -0.5f }; // Value must be negative
		constexpr auto c_voxel_vector_abs{ c_voxel_vector_max - c_voxel_vector_min }; // Absolute value (hacky)
		constexpr auto c_vector_pack_ratio{ c_voxel_vector_abs / c_voxel_vector_pack };

		/// <summary>
		/// Voxel class compresses a 3D vector of floats (128bits) into 32bits (4:1 lossy compression)
		/// </summary>
		class Voxel
		{
			uint32_t m_z : c_packed_vector_bits;   // 8 :: 0 - 7
			uint32_t m_y : c_packed_vector_bits;   // 8 :: 8 - 15
			uint32_t m_x : c_packed_vector_bits;   // 8 :: 16 - 23
			uint32_t m_lod : c_packed_lod_bits;    // 4 :: 24 - 28
			uint32_t m_flags : c_packed_flag_bits; // 4 :: 29 - 32

		public:
			Voxel();
			Voxel(float z, float y, float x, unsigned int lod);
			//Voxel(const Voxel&) = default;
			//Voxel& operator=(const Voxel&) = default;
			//Voxel(Voxel&&) = default;
			//Voxel& operator=(Voxel&&) = default;
			operator uint32_t();
			~Voxel();

			/// <summary>
			/// This function is effectively a normalization comparable to converting F to C as far as the algorithm is concerned.
			/// I've considered just making this a lookup table as there are only 256 possible conversions from char to float.
			/// We're dealing with fast precision 32 bit floats here, that range in value from 2.0 to -0.5, and then we pack them.
			/// Since EncodeVoxelVector returns a uchar the compiler does the bit shift for us to store in the uint32_t.
			/// The LOD and flags part aren't really implemented.
			/// </summary>
			uint8_t EncodeVoxelVector(float vector);
			/// <summary>
			/// Reverse the above algorithm, there are only 256 possible return values for this float, thus the "lossy" compression.
			/// </summary>
			float DecodeVoxelVector(uint8_t vector);

			void SetVoxel(float& z, float& y, float& x, unsigned int& lod);
			void GetVoxel(float& z, float& y, float& x, unsigned int& lod);

			/// <summary>
			/// Oh, I decided to get "fancy" here... I must have just learned about tuple or something...
			/// </summary>
			/// <returns></returns>
			VoxelData GetVoxelData();
		};
	}

	// Data related to procedural physics
	// All voxels participate in procedural erosion
	// We want to simulate heating patterns for air and solids
	// We want to simulate atmospheric pressure
	// We want to simulate evaporation
	// We want to simulate clouds as voxels
	// We want to simulate condensation as rain
	// We want to simulate water erosion of solids
	// Solids need to have a hardness
	// Solids need to have an erosion progress value (saturation of water)
	// Solids become suspensions after erosion (100% saturated solid moves as if it were water)
	// Suspensions are returned to solid at new location after loss of water to some degree
	// SolidState : Hardness, Progress (erosion factor of hardness)
	// LiquidState : Suspension Container
	// VaporState : Temperature, Pressure, Humidity
	// Temperature, Pressure and Density should determine actual state
}
