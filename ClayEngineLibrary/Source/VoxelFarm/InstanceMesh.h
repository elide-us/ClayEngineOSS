#pragma once

#include "VoxelFarm.h"
#include "fileutils.h"
#include "glrender.h"
#include "shaders.h"
#include "tga.h"
#include "CellData.h"
#include "ClipmapView.h"
#include "gl\glew.h"

#include <string>
#include <sstream>

namespace VoxelFarm
{
	using namespace Algebra;
	using namespace GL;

	class InstanceMeshManager
	{
		class BakedInstanceMesh
		{
		public:
			GLuint vao; //  vertex array object
			GLuint vboVertices; //  vertex buffer object

			GLuint instanceFaceCount;
			GLuint vboInstanceFaces;
		};

		struct Shader
		{
			GLuint glInstanceProgram;
			// Shader parameter to control fading effect for cell. This is used to smooth LOD transitions
			GLuint uniform_instanceProgram_fade;
			// Shader parameter for the current world time. This is used to animate wind.
			GLuint uniform_instanceProgram_time;
			// Shader parameter for the instance texture
			GLuint uniform_instanceProgram_texture;
			// Shader parameter for a pink noise texture
			GLuint uniform_instanceProgram_noise;
			// Shader parameter for camera up direction vector
			GLuint uniform_instanceProgram_camUp;
			// Shader parameter for camera right direction vector
			GLuint uniform_instanceProgram_camRight;
			// Shader parameter for the cell's absolute offset
			GLuint uniform_instanceProgram_offset;
			// Shader parameter for the cell dimension
			GLuint uniform_instanceProgram_cellSize;
			// Shader parameter for the cell's local offset
			GLuint uniform_instanceProgram_localOffset;
			// Shader parameter for the instance normal
			GLuint uniform_instanceProgram_instanceNormal;
			// Shader parameter for the local cell dimension
			GLuint uniform_instanceProgram_localCellSize;
			// Shader parameter for local instance rotation
			GLuint uniform_instanceProgram_localRotMatrix;
			// Shader parameter for current viewer position
			GLuint uniform_instanceProgram_viewerPos;
			// Shader parameter for the Sunlight shadow map
			GLuint uniform_instanceProgram_shadowMap;
			// Shader parameter for the Skylight shadow map
			GLuint uniform_instanceProgram_skylightMap;
			// Shader parameter for an array of colors used to colorize instances
			GLuint uniform_instanceProgram_applyColor;
			GLuint uniform_instanceProgram_sunDir;
			GLuint uniform_instanceProgram_sunColor;
			GLuint uniform_instanceProgram_skyColor;
			GLuint uniform_instanceProgram_ambColor1;
			GLuint uniform_instanceProgram_ambColor2;
			GLuint uniform_instanceProgram_ambBase;
		};

		using BakedCellInstances = std::map<CellId, BakedInstanceMesh*>;
		BakedCellInstances m_bakedcellinstances = {};

		Shader m_shader = {};

		CClipmapView* m_clipmapview = nullptr;
		
	public:
		InstanceMeshManager(CClipmapView* clipmapView)
		{
			m_clipmapview = clipmapView;
		}
		~InstanceMeshManager()
		{
			//std::for_each(m_bakedcellinstances.begin(), m_bakedcellinstances.end(), [](auto& element) { VF_DELETE element.second; });
			BakedCellInstances::iterator iter;
			for (iter = m_bakedcellinstances.begin(); iter != m_bakedcellinstances.end(); ++iter)
				VF_DELETE iter->second;
			m_bakedcellinstances.clear();
		};

		void init()
		{
			loadMeshesAndTextures();
			loadShader();
		}

		void loadMeshesAndTextures()
		{
			const int mapSize = 512;
			const float maxAnisotropy = 1;

			int lastInstanceSlice = 0;

			for (TMap<std::string, int>::const_iterator iter = m_clipmapview->instanceNameIdMap.begin(); iter != m_clipmapview->instanceNameIdMap.end(); ++iter)
			{
				if (m_clipmapview->instanceMeshes.find(iter->second) != m_clipmapview->instanceMeshes.end()) continue;

				char instanceFile[MAX_PATH];
				sprintf_s(instanceFile, MAX_PATH, "instances\\%s.minst", iter->first.c_str());

				std::wstringstream wss;
				wss << instanceFile;

				if (!fileExists(instanceFile))
				{
					MessageBox(0, wss.str().c_str(), L"Material Instance File Not Found!", MB_ICONERROR | MB_OK);
					return;
				}

				CCellData::CInstanceMeshLOD* meshes = loadInstanceMesh(instanceFile);
				m_clipmapview->instanceMeshes[iter->second] = meshes;

				for (int i = 0; i < CCellData::INSTANCE_LOD; i++)
				{
					if (meshes->meshes[i].sliceName.empty())
					{
						continue;
					}

					TMap<std::string, int>::iterator islice = m_clipmapview->instanceSliceMap.find(meshes->meshes[i].sliceName);
					if (islice != m_clipmapview->instanceSliceMap.end())
					{
						meshes->meshes[i].textureSlice = islice->second;
					}
					else
					{
						meshes->meshes[i].textureSlice = lastInstanceSlice;
						m_clipmapview->instanceSliceMap[meshes->meshes[i].sliceName] = lastInstanceSlice;
						lastInstanceSlice++;
					}
				}
			}

			// Load instance textures
			//int size = (int)instanceSliceMap.size()*materialLibrary.mapSize*materialLibrary.mapSize * 4;
			//unsigned char* data = VF_ALLOC(unsigned char, size);
			// Load instance textures
			int size = (int)m_clipmapview->instanceSliceMap.size()*mapSize*mapSize * 4;
			unsigned char* data = VF_ALLOC(unsigned char, size);
			TMap<int, String> sliceIds;
			for (TMap<String, int>::iterator i = m_clipmapview->instanceSliceMap.begin(); i != m_clipmapview->instanceSliceMap.end(); ++i)
			{
				sliceIds[i->second] = i->first;
			}
			for (TMap<int, String>::iterator i = sliceIds.begin(); i != sliceIds.end(); ++i)
			{
				char localFile[MAX_PATH];
				sprintf_s(localFile, MAX_PATH, "materials\\textures\\%s", i->second.c_str());
				/*
				char localFile[MAX_PATH];
				if (!cacheFile(i->second.c_str(), MAX_PATH, path, localFile))
				{
					notifyError(ERROR_MATERIAL_FILE_NOT_FOUND, "Missing Texture File", i->second.c_str(), false);
				}
				*/
				TGATexture tga;
				LoadTGA(&tga, localFile);
				if (tga.imageData != NULL)
				{
					if (tga.width == mapSize && tga.height == mapSize)
					{
						memcpy(&(data[i->first*mapSize*mapSize * 4]), tga.imageData, mapSize*mapSize * 4);
					}
					else
					{
						notifyError(ERROR_MATERIAL_FILE_NOT_FOUND, "Invalid Texture Size", localFile, false);
					}
					VF_FREE(tga.imageData);
				}
				else
				{
					notifyError(ERROR_MATERIAL_FILE_NOT_FOUND, "Error Loading Texture File", localFile, false);
				}
			}

			glGenTextures(1, &instanceTextureArray);
			glBindTexture(GL_TEXTURE_2D_ARRAY, instanceTextureArray);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_GENERATE_MIPMAP, GL_TRUE);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, /*GL_RGBA8*/GL_COMPRESSED_RGBA, mapSize, mapSize, (GLsizei)m_clipmapview->instanceSliceMap.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			VF_FREE(data);	
		}

		bool loadShader()
		{
			int instanceRenderIds[3] = { 0, 1, 2 };
			char* instanceRenderNames[3] = { "vertex_position", "vertex_uvt", "vertex_normal" };

			GLuint newprog = compileProgram("instanceReference.vert", "instanceReference.frag", 0, instanceRenderIds, instanceRenderNames);
			if (newprog == 0) return false;

			m_shader.glInstanceProgram = newprog;
			m_shader.uniform_instanceProgram_fade = glGetUniformLocationARB(m_shader.glInstanceProgram, "fade");
			m_shader.uniform_instanceProgram_time = glGetUniformLocationARB(m_shader.glInstanceProgram, "time");
			m_shader.uniform_instanceProgram_texture = glGetUniformLocationARB(m_shader.glInstanceProgram, "texture");
			m_shader.uniform_instanceProgram_noise = glGetUniformLocationARB(m_shader.glInstanceProgram, "noise");
			m_shader.uniform_instanceProgram_camUp = glGetUniformLocationARB(m_shader.glInstanceProgram, "camUp");
			m_shader.uniform_instanceProgram_camRight = glGetUniformLocationARB(m_shader.glInstanceProgram, "camRight");
			m_shader.uniform_instanceProgram_offset = glGetUniformLocationARB(m_shader.glInstanceProgram, "offset");
			m_shader.uniform_instanceProgram_cellSize = glGetUniformLocationARB(m_shader.glInstanceProgram, "cellSize");
			m_shader.uniform_instanceProgram_instanceNormal = glGetUniformLocationARB(m_shader.glInstanceProgram, "instanceNormal");
			m_shader.uniform_instanceProgram_localOffset = glGetUniformLocationARB(m_shader.glInstanceProgram, "localOffset");
			m_shader.uniform_instanceProgram_localCellSize = glGetUniformLocationARB(m_shader.glInstanceProgram, "localCellSize");
			m_shader.uniform_instanceProgram_localRotMatrix = glGetUniformLocationARB(m_shader.glInstanceProgram, "localRotMatrix");
			m_shader.uniform_instanceProgram_viewerPos = glGetUniformLocationARB(m_shader.glInstanceProgram, "viewerPos");
			m_shader.uniform_instanceProgram_shadowMap = glGetUniformLocationARB(m_shader.glInstanceProgram, "shadowMap");
			m_shader.uniform_instanceProgram_skylightMap = glGetUniformLocationARB(m_shader.glInstanceProgram, "skylightMap");
			m_shader.uniform_instanceProgram_applyColor = glGetUniformLocationARB(m_shader.glInstanceProgram, "applyColor");
			m_shader.uniform_instanceProgram_sunDir = glGetUniformLocationARB(m_shader.glInstanceProgram, "sunDir");
			m_shader.uniform_instanceProgram_sunColor = glGetUniformLocationARB(m_shader.glInstanceProgram, "sunColor");
			m_shader.uniform_instanceProgram_skyColor = glGetUniformLocationARB(m_shader.glInstanceProgram, "skyColor");
			m_shader.uniform_instanceProgram_ambColor1 = glGetUniformLocationARB(m_shader.glInstanceProgram, "ambColor1");
			m_shader.uniform_instanceProgram_ambColor2 = glGetUniformLocationARB(m_shader.glInstanceProgram, "ambColor2");
			m_shader.uniform_instanceProgram_ambBase = glGetUniformLocationARB(m_shader.glInstanceProgram, "ambBase");

			return true;
		}

		void bakeCellDataVBO(const CCellData* cellData)
		{
			if (cellData->instanceReferenceCount == 0) return;

			const CellId cell = cellData->cell;

			int level, xc, yc, zc;
			unpackCellId(cell, level, xc, yc, zc);

			double scale = CELL_SIZE * (1 << level);

			if (m_bakedcellinstances.find(cell) != m_bakedcellinstances.end())
				VF_DELETE m_bakedcellinstances[cell];

			BakedInstanceMesh* mesh_out_ptr = VF_NEW BakedInstanceMesh();
			m_bakedcellinstances[cell] = mesh_out_ptr;

			// --------------------------------------------------------------------
			// count how many instance vertices and faces we have in total in the cell
			unsigned int instanceVertCount = 0;
			unsigned int& instanceFaceCount = mesh_out_ptr->instanceFaceCount;

			for (int ri = 0; ri < cellData->instanceReferenceCount; ++ri)
			{
				CCellData::CInstanceMeshLOD* meshes = m_clipmapview->instanceMeshes[cellData->instanceReferences[ri].instanceIndex];
				if (meshes == NULL) continue;

				CCellData::CInstanceMesh* mesh = &meshes->meshes[meshes->lodMap[level - LOD_0]];
				if (mesh == NULL) continue;

				instanceVertCount += mesh->vertexCount;
				instanceFaceCount += mesh->faceCount;
			}


			// --------------------------------------------------------------------
			// bake

			const int vertComponents = /* vertex */ 3 + /* normal */ 3 + /* uvt */ 3;
			float* packedVertexInfo = VF_ALLOC(float, instanceVertCount * vertComponents);
			int* packedInstanceFaces = VF_ALLOC(int, instanceFaceCount * 3);

			GLuint vertOffset = 0;
			GLuint faceOffset = 0;

			for (int ri = 0; ri < cellData->instanceReferenceCount; ++ri)
			{
				const CCellData::InstanceReference& instanceReference = cellData->instanceReferences[ri];

				CCellData::CInstanceMeshLOD* meshes = m_clipmapview->instanceMeshes[instanceReference.instanceIndex];
				if (!meshes) continue;

				const CCellData::CInstanceMesh& mesh_in = meshes->meshes[meshes->lodMap[level - LOD_0]];

				const float localScale = instanceReference.instanceSize / ((float)(1 << (level - LOD_0)) * (float)CELL_SIZE);


				Vector normal;
				normal.x = instanceReference.instanceNormal.x;
				normal.y = instanceReference.instanceNormal.y;
				normal.z = instanceReference.instanceNormal.z;
				Quaternion q = Quaternion_fromVector(normal);
				Matrix rotMatrix = Quaternion_toMatrix(q);
				Matrix_rotate(&rotMatrix, normal, instanceReference.instanceRotation);

				

				for (int vi = 0; vi < mesh_in.vertexCount; ++vi)
				{

					Vector vert_local;
					vert_local.x = mesh_in.vertices[vi].x * localScale;
					vert_local.y = mesh_in.vertices[vi].y * localScale;
					vert_local.z = mesh_in.vertices[vi].z * localScale;

					Quaternion q = Quaternion_fromVector(normal);
					Matrix rotMatrix = Quaternion_toMatrix(q);
					Matrix_rotate(&rotMatrix, normal, instanceReference.instanceRotation);

					vert_local = Matrix_multiplyVector(rotMatrix, vert_local);

					vert_local.x += instanceReference.instancePosition.x;
					vert_local.y += instanceReference.instancePosition.y;
					vert_local.z += instanceReference.instancePosition.z;

					packedVertexInfo[vi * vertComponents + 0 + vertOffset] = vert_local.x;
					packedVertexInfo[vi * vertComponents + 1 + vertOffset] = vert_local.y;
					packedVertexInfo[vi * vertComponents + 2 + vertOffset] = vert_local.z;

					packedVertexInfo[vi * vertComponents + 3 + vertOffset] = mesh_in.uv[vi].u;
					packedVertexInfo[vi * vertComponents + 4 + vertOffset] = mesh_in.uv[vi].v;
					packedVertexInfo[vi * vertComponents + 5 + vertOffset] = (float)mesh_in.textureSlice;

					packedVertexInfo[vi * vertComponents + 6 + vertOffset] = instanceReference.instanceNormal.x;
					packedVertexInfo[vi * vertComponents + 7 + vertOffset] = instanceReference.instanceNormal.y;
					packedVertexInfo[vi * vertComponents + 8 + vertOffset] = instanceReference.instanceNormal.z;
				}

				for (int fi = 0; fi < mesh_in.faceCount*3; ++fi)
					packedInstanceFaces[fi + faceOffset] = mesh_in.faces[fi] + vertOffset / vertComponents;
				
				vertOffset += mesh_in.vertexCount * vertComponents;
				faceOffset += mesh_in.faceCount * 3;
			}
			





			// --------------------------------------------------------------------
			// Move instance vertices to OpenGL
			glGenVertexArrays(1, &mesh_out_ptr->vao);
			glBindVertexArray(mesh_out_ptr->vao); // Bind our Vertex Array Object so we can use it  

			// Move instance vertices to OpenGL
			glGenBuffersARB(1, &(mesh_out_ptr->vboVertices));
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, mesh_out_ptr->vboVertices);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, instanceVertCount * vertComponents * sizeof(float), packedVertexInfo, GL_STATIC_DRAW_ARB);

			GLuint idx = glGetAttribLocation(m_shader.glInstanceProgram, "vertex_position");
			glEnableVertexAttribArray(idx); // pos
			glVertexAttribPointer(idx, 3, GL_FLOAT, GL_FALSE, vertComponents * sizeof(float), 0);

			idx = glGetAttribLocation(m_shader.glInstanceProgram, "vertex_uvt");
			glEnableVertexAttribArray(idx); // tex
			glVertexAttribPointer(idx, 3, GL_FLOAT, GL_FALSE, vertComponents * sizeof(float), (GLvoid*)(3 * sizeof(GLfloat)));

			idx = glGetAttribLocation(m_shader.glInstanceProgram, "vertex_normal");
			glEnableVertexAttribArray(idx); // normal
			glVertexAttribPointer(idx, 3, GL_FLOAT, GL_FALSE, vertComponents * sizeof(float), (GLvoid*)(6 * sizeof(GLfloat)));

			// Move face indices to OpenGL
			glGenBuffersARB(1, &(mesh_out_ptr->vboInstanceFaces));
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_out_ptr->vboInstanceFaces);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, instanceFaceCount * 3 * sizeof(GLuint), packedInstanceFaces, GL_STATIC_DRAW_ARB);

			glBindVertexArray(0); // Disable our Vertex Buffer Object  

			VF_FREE(packedInstanceFaces);
			VF_FREE(packedVertexInfo);
		}

		void removeCellVBO(CellId cell)
		{
			if (m_bakedcellinstances.find(cell) != m_bakedcellinstances.end())
			{
				VF_DELETE m_bakedcellinstances[cell];
				m_bakedcellinstances.erase(cell);
			}
		}

		void renderCellInstances(Scene* scene, const CClipmapView& view)
		{
			if (!scene || scene->empty()) return;

			glUseProgram(m_shader.glInstanceProgram);
			glDisable(GL_CULL_FACE);

			Frustum frustum;
			extractFrustum(frustum);

			const float RADIUS_EXTRA = 0.7f;

			glEnable(GL_TEXTURE_2D);

			glActiveTextureARB(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, instanceTextureArray);
			glUniform1iARB(m_shader.uniform_instanceProgram_texture, 0);

			glActiveTextureARB(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, skylightDepthTextureId);
			glUniform1iARB(m_shader.uniform_instanceProgram_skylightMap, 6);

			glActiveTextureARB(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, shadowDepthTextureId);
			glUniform1iARB(m_shader.uniform_instanceProgram_shadowMap, 7);

			glErrorCheck();

			glUniform4f(m_shader.uniform_instanceProgram_sunDir, VoxelFarm::GL::sunDir.x, GL::sunDir.y, GL::sunDir.z, 0.0f);
			glUniform4f(m_shader.uniform_instanceProgram_sunColor, GL::sunColor.x, GL::sunColor.y, GL::sunColor.z, 0.0f);
			glUniform4f(m_shader.uniform_instanceProgram_skyColor, GL::skyColor.x, GL::skyColor.y, GL::skyColor.z, 0.0f);
			glUniform4f(m_shader.uniform_instanceProgram_ambColor1, GL::ambColor1.x, GL::ambColor1.y, GL::ambColor1.z, 0.0f);
			glUniform4f(m_shader.uniform_instanceProgram_ambColor2, GL::ambColor2.x, GL::ambColor2.y, GL::ambColor2.z, 0.0f);
			glUniform4f(m_shader.uniform_instanceProgram_ambBase, GL::ambBase.x, GL::ambBase.y, GL::ambBase.z, 0.0f);

			glErrorCheck();

			for (TSet<CellId>::iterator is = scene->begin(); is != scene->end(); ++is)
			{
				glErrorCheck();

				CellId cell = *is;

				// Unpack cell coordinates
				int level, xc, yc, zc;
				unpackCellId(cell, level, xc, yc, zc);
				const double scale = CELL_SIZE *  (1 << level);

				// Peform frustum test for cell
				/*
				const float x = (float)((xc + 0.5f)*scale - view.xpos);
				const float y = (float)((yc + 0.5f)*scale - view.ypos);
				const float z = (float)((zc + 0.5f)*scale - view.zpos);
				const bool visible = sphereInFrustum(frustum, x, y, z, RADIUS_EXTRA*(float)scale);
				if (!visible) continue;
				*/

				// See if there is information for the cell
				const CClipmapView::CellDataCache::const_iterator icell = view.cellCache.find(cell);
				if (icell == view.cellCache.end()) continue;

				const CGLCellData* cellData = (CGLCellData*)icell->second;

				//if (!lineMode && cellData->occluded) continue;

				if (cellData->instanceReferenceCount == 0) continue;

				glMatrixMode(GL_TEXTURE);
				// Save sunlight matrix
				glActiveTextureARB(GL_TEXTURE7);
				glPushMatrix();
				glTranslatef(
					(float)(scale*xc - lastShadowXpos),
					(float)(scale*yc - lastShadowYpos),
					(float)(scale*zc - lastShadowZpos));
				glScalef((float)scale, (float)scale, (float)scale);

				glErrorCheck();

				// Save skylight matrix
				glActiveTextureARB(GL_TEXTURE6);
				glPushMatrix();
				glTranslatef(
					(float)(scale*xc - lastShadowXpos),
					(float)(scale*yc - lastShadowYpos),
					(float)(scale*zc - lastShadowZpos));
				glScalef((float)scale, (float)scale, (float)scale);

				glErrorCheck();

				// Send time to shader
				glUniform1fARB(m_shader.uniform_instanceProgram_time, (float)view.time);

				glErrorCheck();
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();

				CCellData::CVert globalPosition;// = cellData->instanceReferences[ri].instancePosition;
				globalPosition.x = (float)(xc*scale);
				globalPosition.y = (float)(yc*scale);
				globalPosition.z = (float)(zc*scale);

				// Translate to cell orgin relative to viewer pos
				glTranslatef(
					(float)(scale*xc - view.xpos),
					(float)(scale*yc - view.ypos),
					(float)(scale*zc - view.zpos));

				// Cell vertices are in 0..1 range, scale to world cell dimensions
				glScalef((float)scale, (float)scale, (float)scale);

				glUniform1f(m_shader.uniform_instanceProgram_cellSize, (float)scale);
				glUniform3f(m_shader.uniform_instanceProgram_offset, globalPosition.x, globalPosition.y, globalPosition.z);
				glUniform3f(m_shader.uniform_instanceProgram_viewerPos, (float)view.xpos, (float)view.ypos, (float)view.zpos);

				{
					BakedInstanceMesh* bakedMesh = m_bakedcellinstances[cell];
					glBindVertexArray(bakedMesh->vao);
					glDrawElements(GL_TRIANGLES, 3 * bakedMesh->instanceFaceCount, GL_UNSIGNED_INT, 0);
					glBindVertexArray(0); // Disable our Vertex Buffer Object  
				}

				glMatrixMode(GL_TEXTURE);
				glActiveTextureARB(GL_TEXTURE7);
				glPopMatrix();
				glActiveTextureARB(GL_TEXTURE6);
				glPopMatrix();

				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}
	};
}
