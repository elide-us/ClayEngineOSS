#include "pch.h"
#include "ElasticBox.h"

#include "BoxMeshStamp.h"
#include "Intersections.h"
#include "ClipboardData.h"

static const int sBoxTriangles[12][3] =
{
	//side0 plane xy
	{ 5, 0, 1 },
	{ 4, 0, 5 },
	//side1 plane xy
	{ 2, 3, 6 },
	{ 6, 3, 7 },
	//side2 plane xz
	{ 1, 0, 2 },
	{ 2, 0, 3 },
	//side3 plane xz
	{ 6, 4, 5 },
	{ 7, 4, 6 },
	//side4 plane yz
	{ 3, 0, 7 },
	{ 7, 0, 4 },
	//side5 plane yz
	{ 5, 1, 6 },
	{ 6, 1, 2 }
};


ElasticBox::ElasticBox()
{
	for (int c = 0; c < 3; c++)
	{
		OffsetClipboard[c] = 0.0f;
		CopySize[c] = 0.0;
		Intersection[c] = 0.0;
		Size[c] = 0.0;
		WorldPos[c] = 0.0;
		PrevSize[c] = 0.0;
	}

	Mode = false;
	First = true;
	Corner = 0;
	Triangle = 0;
	Rotations = VoxelFarm::Algebra::Quaternion();
	Purpose = EBP_GENERAL;
	State = EB_EDIT;

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			CutScope[i][j] = 0.0;
			Scope[i][j] = 0.0;
		}
	}
}

void ElasticBox::beginBox(ElasticBox::EBPurpose purpose, bool newBox, VoxelFarm::Architecture::CPrefabDesc *prefabDesc, const VoxelFarm::VoxelHitInfo& hitInfo,
						  const int buildCursorSizes[3])
{
	if (Mode)
	{
		return;
	}

	Purpose = purpose;

	if (newBox || First)
	{
		if (!hitInfo.valid)
		{
			return;
		}

		int hlevel, hxc, hyc, hzc;
		VoxelFarm::unpackCellId(hitInfo.cell, hlevel, hxc, hyc, hzc);
		double hscale = VoxelFarm::CELL_SIZE*(1 << hlevel);
		double blockSize = hscale / VoxelFarm::BLOCK_DIMENSION;

		if (First || Purpose == ElasticBox::EBP_GENERAL)
		{
			if (Purpose == ElasticBox::EBP_PREFAB)
			{
				Size[0] = prefabDesc->xsize * 10;
				Size[1] = prefabDesc->ysize * 10;
				Size[2] = prefabDesc->zsize * 10;
			}
			else
			{
				Size[0] = buildCursorSizes[0] * blockSize;
				Size[1] = buildCursorSizes[1] * blockSize;
				Size[2] = buildCursorSizes[2] * blockSize;
			}

			PrevSize[0] = Size[0];
			PrevSize[1] = Size[1];
			PrevSize[2] = Size[2];
		}

		WorldPos[0] = hscale*(hxc + (double)hitInfo.voxel[0] / VoxelFarm::BLOCK_DIMENSION) + Size[0] / 2.0;
		WorldPos[1] = hscale*(hyc + (double)hitInfo.voxel[1] / VoxelFarm::BLOCK_DIMENSION) + Size[1] / 2.0;
		WorldPos[2] = hscale*(hzc + (double)hitInfo.voxel[2] / VoxelFarm::BLOCK_DIMENSION) + Size[2] / 2.0;

		Rotations = VoxelFarm::Algebra::Quaternion_identity();
		updateScope();

		Mode = true;
		First = false;
	}
	else
	{
		Mode = true;
		First = false;

		Size[0] = PrevSize[0];
		Size[1] = PrevSize[1];
		Size[2] = PrevSize[2];
		updateScope();

		return;
	}
}

void ElasticBox::removeArea(VoxelFarm::CClipmapView* view, bool fullErase)
{
	view->clearBoxArea(Scope, fullErase);
}

void ElasticBox::copyBlock(VoxelFarm::CClipmapView* view, VoxelFarm::IClipboard* clipboard, bool cut)
{
	if (!Mode || (Purpose != ElasticBox::EBP_GENERAL && Purpose != ElasticBox::EBP_COPY && Purpose != ElasticBox::EBP_CUT))
	{
		return;
	}

	double cellSize = VoxelFarm::CELL_SIZE*(1 << VoxelFarm::LOD_0);
	double voxelSize = cellSize / VoxelFarm::BLOCK_DIMENSION;

	double minBoxX = Scope[0][0];
	double minBoxY = Scope[0][1];
	double minBoxZ = Scope[0][2];

	double maxBoxX = Scope[0][0];
	double maxBoxY = Scope[0][1];
	double maxBoxZ = Scope[0][2];

	for (int i = 1; i<8; i++)
	{
		if (minBoxX > Scope[i][0])
		{
			minBoxX = Scope[i][0];
		}
		if (minBoxY > Scope[i][1])
		{
			minBoxY = Scope[i][1];
		}
		if (minBoxZ > Scope[i][2])
		{
			minBoxZ = Scope[i][2];
		}

		if (maxBoxX <Scope[i][0])
		{
			maxBoxX = Scope[i][0];
		}
		if (maxBoxY < Scope[i][1])
		{
			maxBoxY = Scope[i][1];
		}
		if (maxBoxZ < Scope[i][2])
		{
			maxBoxZ = Scope[i][2];
		}
	}

	int minX = (int)(minBoxX / voxelSize);
	int minY = (int)(minBoxY / voxelSize);
	int minZ = (int)(minBoxZ / voxelSize);

	int maxX = (int)(maxBoxX / voxelSize);
	int maxY = (int)(maxBoxY / voxelSize);
	int maxZ = (int)(maxBoxZ / voxelSize);

	double worldPos[3] = { minBoxX, minBoxY, minBoxZ };
	float boxKernel[8][3];

	for (int i = 0; i<8; i++)
	{
		boxKernel[i][0] = (float)(Scope[i][0] - minBoxX);
		boxKernel[i][1] = (float)(Scope[i][1] - minBoxY);
		boxKernel[i][2] = (float)(Scope[i][2] - minBoxZ);
	}

	VoxelFarm::CBoxMesh scope(boxKernel);
	double offX, offY, offZ;
	view->copyBlock(clipboard, worldPos, &scope, offX, offY, offZ);

	//save copy state
	if (cut)
	{
		static_assert(sizeof(Scope) == 8 * 3 * sizeof(double), "memcpy copying wrong size?");
		memcpy(&CutScope, &Scope, 8 * 3 * sizeof(double));
		Purpose = ElasticBox::EBP_CUT;
	}
	else
	{
		Purpose = ElasticBox::EBP_COPY;
	}

	OffsetClipboard[0] = (float)(minBoxX - offX);
	OffsetClipboard[1] = (float)(minBoxY - offY);
	OffsetClipboard[2] = (float)(minBoxZ - offZ);

	Rotations = VoxelFarm::Algebra::Quaternion_identity();
	Size[0] = maxBoxX - minBoxX;
	Size[1] = maxBoxY - minBoxY;
	Size[2] = maxBoxZ - minBoxZ;

	updateScope();

	CopySize[0] = Size[0];
	CopySize[1] = Size[1];
	CopySize[2] = Size[2];
}

VoxelFarm::Algebra::Matrix ElasticBox::previewPasteTransform()
{
	VoxelFarm::Algebra::Matrix transform = VoxelFarm::Algebra::Matrix_identity();

	if (!Mode || (Purpose != ElasticBox::EBP_COPY && Purpose != ElasticBox::EBP_CUT))
	{
		return transform;
	}

	//rotate
	transform = VoxelFarm::Algebra::Quaternion_toMatrix(Rotations);

	//translate box offset
	Matrix_translate(&transform, -OffsetClipboard[0], -OffsetClipboard[1], -OffsetClipboard[2]);

	//scale
	float factorX = (float)(Size[0] / CopySize[0]);
	float factorY = (float)(Size[1] / CopySize[1]);
	float factorZ = (float)(Size[2] / CopySize[2]);

	Matrix_scale(&transform, factorX, factorY, factorZ);

	return transform;
}

void ElasticBox::pasteBlock(VoxelFarm::CClipmapView* clipmapView, VoxelFarm::IClipboard* clipboard, bool air)
{
	if (Mode)
	{
		if (Purpose != ElasticBox::EBP_COPY && Purpose != ElasticBox::EBP_CUT)
		{
			return;
		}

		VoxelFarm::Algebra::Matrix transform = previewPasteTransform();
		// Keep a set to store all cells that changed
		VoxelFarm::TSet<VoxelFarm::CellId> changedCells;

		clipmapView->enterCriticalSection();

		clipmapView->blockData->beginChanges();

		// If we were a cut (as opposed to a copy), remove the original on paste
		if (Purpose == ElasticBox::EBP_CUT)
		{
			clipmapView->enterCriticalSection();

			clipmapView->blockData->beginChanges();

			// If we were a cut (as opposed to a copy), remove the original on paste
			if (Purpose == ElasticBox::EBP_CUT)
			{
				VoxelFarm::TMap<VoxelFarm::CellId, VoxelFarm::CCellData::CInstanceEditionVolume> volumes;
				VoxelFarm::ClearBoxArea(clipmapView->blockData, CutScope, true, &changedCells, volumes);
				Purpose = ElasticBox::EBP_COPY; // further pastes can use the original source
			}

			VoxelFarm::TMap<VoxelFarm::CellId, VoxelFarm::CCellData::CInstanceEditionVolume> volumes;
			// Call global paste function
			VoxelFarm::pasteFromClipboard(clipmapView->blockData, clipboard, Scope[0], air, transform, &changedCells, volumes);

			clipmapView->processModifiedCells(changedCells);

			// Track end of changes for UNDO
			clipmapView->blockData->endChanges();

			clipmapView->leaveCriticalSection();

			VoxelFarm::TMap<VoxelFarm::CellId, VoxelFarm::CCellData::CInstanceEditionVolume> volumesClear;
			ClearBoxArea(clipmapView->blockData, CutScope, true, &changedCells, volumesClear);
			Purpose = ElasticBox::EBP_COPY; // further pastes can use the original source
		}

		// Call global paste function
		VoxelFarm::TMap<VoxelFarm::CellId, VoxelFarm::CCellData::CInstanceEditionVolume> volumes;
		VoxelFarm::pasteFromClipboard(clipmapView->blockData, clipboard, Scope[0], air, transform, &changedCells, volumes);

		clipmapView->processModifiedCells(changedCells);

		// Track end of changes for UNDO
		clipmapView->blockData->endChanges();

		clipmapView->leaveCriticalSection();
	}
	else
	{
		int sizeX, sizeY, sizeZ;
		clipboard->getSize(sizeX, sizeY, sizeZ);

		if (sizeX>0 && sizeY>0 && sizeZ>0)
		{
			double cellSize = VoxelFarm::CELL_SIZE*(1 << VoxelFarm::LOD_0);
			double voxelSize = cellSize / VoxelFarm::BLOCK_DIMENSION;

			int buildCursorSizes[3];
			clipmapView->getBuildCursorSize(buildCursorSizes);
			beginBox(ElasticBox::EBP_GENERAL, true, NULL, clipmapView->hitInfo, buildCursorSizes);
			Rotations = VoxelFarm::Algebra::Quaternion_identity();

			Size[0] = sizeX * voxelSize;
			Size[1] = sizeY * voxelSize;
			Size[2] = sizeZ * voxelSize;

			updateScope();

			CopySize[0] = Size[0];
			CopySize[1] = Size[1];
			CopySize[2] = Size[2];
			Purpose = ElasticBox::EBP_COPY;
		}
	}
}

void ElasticBox::smoothArea(VoxelFarm::CClipmapView* view)
{
	if (!Mode)
	{
		return;
	}

	/*
	VoxelFarm::debugPoints.clear();
	VoxelFarm::debugLines.clear();
	*/

	double cellSize = VoxelFarm::CELL_SIZE*(1 << VoxelFarm::LOD_0);
	double voxelSize = cellSize / VoxelFarm::BLOCK_DIMENSION;

	int cursorSizeX = (int)(Size[0] / voxelSize) + 1;
	int cursorSizeY = (int)(Size[1] / voxelSize) + 1;
	int cursorSizeZ = (int)(Size[2] / voxelSize) + 1;

	double minBoxX = Scope[0][0];
	double minBoxY = Scope[0][1];
	double minBoxZ = Scope[0][2];

	for (int i = 1; i<8; i++)
	{
		if (minBoxX > Scope[i][0])
		{
			minBoxX = Scope[i][0];
		}
		if (minBoxY > Scope[i][1])
		{
			minBoxY = Scope[i][1];
		}
		if (minBoxZ > Scope[i][2])
		{
			minBoxZ = Scope[i][2];
		}
	}

	int boxCellX = (int)(minBoxX / cellSize);
	int boxCellY = (int)(minBoxY / cellSize);
	int boxCellZ = (int)(minBoxZ / cellSize);

	VoxelFarm::CellId boxCellId = VoxelFarm::packCellId(VoxelFarm::LOD_0, boxCellX, boxCellY, boxCellZ);

	int boxVoxelX = (int)((minBoxX - (double)boxCellX * cellSize) / voxelSize);
	int boxVoxelY = (int)((minBoxY - (double)boxCellY * cellSize) / voxelSize);
	int boxVoxelZ = (int)((minBoxZ - (double)boxCellZ * cellSize) / voxelSize);

	int buildCursorSizes[3] = { cursorSizeX, cursorSizeY, cursorSizeZ };
	int hitCell[3] = { boxVoxelX, boxVoxelY, boxVoxelZ };

	/*
	VoxelFarm::DebugPoint pt;
	pt.type = 1;
	pt.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	pt.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	pt.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	VoxelFarm::debugPoints.push_back(pt);

	pt.x += cursorSizeX*voxelSize;
	pt.y += cursorSizeY*voxelSize;
	pt.z += cursorSizeZ*voxelSize;
	VoxelFarm::debugPoints.push_back(pt);

	DebugLine dl;
	dl.p0.type = 1;
	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	dl.p1.type = 1;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);

	dl.p0.x = boxCellX*cellSize + boxVoxelX*voxelSize + cursorSizeX*voxelSize;
	dl.p0.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p0.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	dl.p1.x = boxCellX*cellSize + boxVoxelX*voxelSize;
	dl.p1.y = boxCellY*cellSize + boxVoxelY*voxelSize + cursorSizeY*voxelSize;
	dl.p1.z = boxCellZ*cellSize + boxVoxelZ*voxelSize + cursorSizeZ*voxelSize;
	VoxelFarm::debugLines.push_back(dl);
	*/

	view->smoothBlock(buildCursorSizes, boxCellId, hitCell, 15, 100.0f);
}

void ElasticBox::raiseArea(VoxelFarm::CClipmapView* pView)
{
	if (!Mode)
	{
		return;
	}

	double cellSize = VoxelFarm::CELL_SIZE*(1 << VoxelFarm::LOD_0);
	double voxelSize = cellSize / VoxelFarm::BLOCK_DIMENSION;

	int cursorSizeX = (int)(Size[0] / voxelSize);
	int cursorSizeY = (int)(Size[1] / voxelSize);
	int cursorSizeZ = (int)(Size[2] / voxelSize);

	if ((cursorSizeX == 0) || (cursorSizeY == 0) || (cursorSizeZ == 0))
	{
		return;
	}

	double minBoxX = Scope[0][0];
	double minBoxY = Scope[0][1];
	double minBoxZ = Scope[0][2];

	for (int i = 1; i<8; i++)
	{
		if (minBoxX > Scope[i][0])
		{
			minBoxX = Scope[i][0];
		}
		if (minBoxY > Scope[i][1])
		{
			minBoxY = Scope[i][1];
		}
		if (minBoxZ > Scope[i][2])
		{
			minBoxZ = Scope[i][2];
		}
	}

	minBoxX += 2 * voxelSize;
	minBoxY += 2 * voxelSize;
	minBoxZ += 2 * voxelSize;

	int boxCellX = (int)(minBoxX / cellSize);
	int boxCellY = (int)(minBoxY / cellSize);
	int boxCellZ = (int)(minBoxZ / cellSize);

	VoxelFarm::CellId boxCellId = VoxelFarm::packCellId(VoxelFarm::LOD_0, boxCellX, boxCellY, boxCellZ);

	int boxVoxelX = (int)((minBoxX - (double)boxCellX * cellSize) / voxelSize);
	int boxVoxelY = (int)((minBoxY - (double)boxCellY * cellSize) / voxelSize);
	int boxVoxelZ = (int)((minBoxZ - (double)boxCellZ * cellSize) / voxelSize);

	int buildCursorSizes[3] = { cursorSizeX, cursorSizeY, cursorSizeZ };
	int hitCell[3] = { boxVoxelX, boxVoxelY, boxVoxelZ };

	pView->raiseBlock(buildCursorSizes, boxCellId, hitCell, 15, 0.01f);
}


void ElasticBox::changeSize(VoxelFarm::Architecture::CPrefabDesc prefabDesc)
{
	if (Purpose == ElasticBox::EBP_PREFAB)
	{
		if (Mode)
		{
			Size[0] = PrevSize[0];
			Size[1] = PrevSize[1];
			Size[2] = PrevSize[2];

			if (Size[0] < prefabDesc.minxsize * 10)
			{
				Size[0] = prefabDesc.minxsize * 10;
			}
			if (Size[1] < prefabDesc.minysize * 10)
			{
				Size[1] = prefabDesc.minysize * 10;
			}
			if (Size[2] < prefabDesc.minzsize * 10)
			{
				Size[2] = prefabDesc.minzsize * 10;
			}
		}
		else
		{
			Size[0] = prefabDesc.xsize * 10;
			Size[1] = prefabDesc.ysize * 10;
			Size[2] = prefabDesc.zsize * 10;
		}

		updateScope();
	}
}

void ElasticBox::updateScope()
{
	VoxelFarm::Algebra::Matrix transformations = VoxelFarm::Algebra::Quaternion_toMatrix(Rotations);

	float centerX = (float)(Size[0] / 2.f);
	float centerY = (float)(Size[1] / 2.f);
	float centerZ = (float)(Size[2] / 2.f);

	Matrix_translate(&transformations, -centerX, -centerY, -centerZ);

	//rotate
	for (int i = 0; i < 8; i++)
	{
		//scale
		VoxelFarm::Algebra::Vector v = VoxelFarm::Algebra::Vector_withValues(
										   (float)(Size[0] * VoxelFarm::VoxelCorners[i][0]),
										   (float)(Size[1] * VoxelFarm::VoxelCorners[i][1]),
										   (float)(Size[2] * VoxelFarm::VoxelCorners[i][2]));

		//move the center & rotate
		v = Matrix_multiplyVector(transformations, v);

		Scope[i][0] = v.x;
		Scope[i][1] = v.y;
		Scope[i][2] = v.z;
	}

	//translate
	for (int i = 0; i < 8; i++)
	{
		Scope[i][0] += WorldPos[0];
		Scope[i][1] += WorldPos[1];
		Scope[i][2] += WorldPos[2];
	}
}

void ElasticBox::rotateScope(float dx, float dy, float dz)
{
	if (!Mode)
	{
		return;
	}

	VoxelFarm::Algebra::Vector axisX = VoxelFarm::Algebra::Vector_withValues(1.0f, 0.0f, 0.0f);
	VoxelFarm::Algebra::Vector axisY = VoxelFarm::Algebra::Vector_withValues(0.0f, 1.0f, 0.0f);
	VoxelFarm::Algebra::Vector axisZ = VoxelFarm::Algebra::Vector_withValues(0.0f, 0.0f, 1.0f);

	VoxelFarm::Algebra::Quaternion_rotate(&Rotations, axisX, dx*VoxelFarm::Algebra::piover180f);
	VoxelFarm::Algebra::Quaternion_rotate(&Rotations, axisY, dy*VoxelFarm::Algebra::piover180f);
	VoxelFarm::Algebra::Quaternion_rotate(&Rotations, axisZ, dz*VoxelFarm::Algebra::piover180f);

	updateScope();
}

void ElasticBox::rotateScope(VoxelFarm::Algebra::Vector axis, double angle)
{
	if (!Mode)
	{
		return;
	}

	VoxelFarm::Algebra::Vector srcAxis;
	int boxPlane = Triangle / 4;

	if (boxPlane == 0) //z
	{
		srcAxis = VoxelFarm::Algebra::Vector_withValues(0.0f, 0.0f, 1.0f);
	}
	else if (boxPlane == 1) //y
	{
		srcAxis = VoxelFarm::Algebra::Vector_withValues(0.0f, 1.0f, 0.0f);
	}
	else //x
	{
		srcAxis = VoxelFarm::Algebra::Vector_withValues(1.0f, 0.0f, 0.0f);
	}

	VoxelFarm::Algebra::Matrix rotate_transform = VoxelFarm::Algebra::Quaternion_toMatrix(Rotations);
	VoxelFarm::Algebra::Vector boxAxis = Matrix_multiplyVector(rotate_transform, srcAxis);

	float dotPrd = VoxelFarm::Algebra::Vector_dot(axis, boxAxis);
	bool invert = dotPrd < 0;

	if (invert)
	{
		angle *= -1;
	}

	VoxelFarm::Algebra::Quaternion_rotate(&Rotations, axis, (float)angle*VoxelFarm::Algebra::piover180f);

	updateScope();
}

void ElasticBox::resizeScope(const int cursorSize[3], double minX, double minY, double minZ, double dx0, double dy0, double dz0, double dx1, double dy1,
							 double dz1)
{
	if (!Mode)
	{
		return;
	}

	float centerX = 0.0f;
	float centerY = 0.0f;
	float centerZ = 0.0f;

	if (Purpose != ElasticBox::EBP_PREFAB)
	{
		double blockSize = ((double)(1 << VoxelFarm::LOD_0)*VoxelFarm::CELL_SIZE / VoxelFarm::BLOCK_DIMENSION) / 10;
		minX = cursorSize[0] * blockSize;
		minY = cursorSize[1] * blockSize;
		minZ = cursorSize[2] * blockSize;
	}

	if (Size[0] + dx0 + dx1 >= minX * 10)
	{
		centerX -= (float)dx0 / 2.0f;
		centerX += (float)dx1 / 2.0f;
		Size[0] += dx0 + dx1;
	}

	if (Size[1] + dy0 + dy1 >= minY * 10)
	{
		centerY -= (float)dy0 / 2.0f;
		centerY += (float)dy1 / 2.0f;
		Size[1] += dy0 + dy1;
	}

	if (Size[2] + dz0 + dz1 >= minZ * 10)
	{
		centerZ -= (float)dz0 / 2.0f;
		centerZ += (float)dz1 / 2.0f;
		Size[2] += dz0 + dz1;
	}

	//recalculate center
	VoxelFarm::Algebra::Vector center = VoxelFarm::Algebra::Vector_withValues(centerX, centerY, centerZ);
	VoxelFarm::Algebra::Matrix rotate_transform = VoxelFarm::Algebra::Quaternion_toMatrix(Rotations);
	center = Matrix_multiplyVector(rotate_transform, center);
	WorldPos[0] += center.x;
	WorldPos[1] += center.y;
	WorldPos[2] += center.z;

	updateScope();

	PrevSize[0] = Size[0];
	PrevSize[1] = Size[1];
	PrevSize[2] = Size[2];
}


void ElasticBox::moveScope(bool boxDir, double dx, double dy, double dz)
{
	if (!Mode)
	{
		return;
	}

	if (boxDir)
	{
		VoxelFarm::Algebra::Vector center = VoxelFarm::Algebra::Vector_withValues((float)dx, (float)dy, (float)dz);
		VoxelFarm::Algebra::Matrix rotate_transform = VoxelFarm::Algebra::Quaternion_toMatrix(Rotations);
		center = Matrix_multiplyVector(rotate_transform, center);
		WorldPos[0] += center.x;
		WorldPos[1] += center.y;
		WorldPos[2] += center.z;
	}
	else
	{
		WorldPos[0] += dx;
		WorldPos[1] += dy;
		WorldPos[2] += dz;
	}

	updateScope();
}

void ElasticBox::cancel()
{
	Mode = false;
}

void ElasticBox::endBox(
	VoxelFarm::CClipmapView* view,
	VoxelFarm::Architecture::CArchitectureManager* architectureManager,
	int moduleId,
	VoxelFarm::Architecture::CPaletteDesc* palette,
	VoxelFarm::Architecture::PaletteType type,
	VoxelFarm::IMeshStamMaterialSource* materials)
{
	Mode = false;
	PrevSize[0] = Size[0];
	PrevSize[1] = Size[1];
	PrevSize[2] = Size[2];

	view->stampModel(Size, Rotations, Scope[0], architectureManager, moduleId, palette, type, materials);
}

void ElasticBox::mouseMoveBox(const double orig[3], const double dir[3])
{
	if (!Mode || (State != ElasticBox::EB_MOVE))
	{
		return;
	}

	// Compute face normal of intersected triangle
	double pt1[3] =
	{
		Scope[sBoxTriangles[Triangle][0]][0],
		Scope[sBoxTriangles[Triangle][0]][1],
		Scope[sBoxTriangles[Triangle][0]][2]
	};
	double pt2[3] =
	{
		Scope[sBoxTriangles[Triangle][1]][0],
		Scope[sBoxTriangles[Triangle][1]][1],
		Scope[sBoxTriangles[Triangle][1]][2]
	};
	double pt3[3] =
	{
		Scope[sBoxTriangles[Triangle][2]][0],
		Scope[sBoxTriangles[Triangle][2]][1],
		Scope[sBoxTriangles[Triangle][2]][2]
	};

	double px, py, pz;
	bool intersection = VoxelFarm::intersectPointPlane(orig, dir, pt1, pt2, pt3, &px, &py, &pz);
	if (intersection)
	{
		VoxelFarm::Algebra::Vector moveDir = VoxelFarm::Algebra::Vector_withValues(
				(float)(px - Intersection[0]),
				(float)(py - Intersection[1]),
				(float)(pz - Intersection[2]));

		VoxelFarm::Algebra::Vector edgeX = VoxelFarm::Algebra::Vector_withValues(1.0f, 0.0f, 0.0f);
		VoxelFarm::Algebra::Vector edgeY = VoxelFarm::Algebra::Vector_withValues(0.0f, 1.0f, 0.0f);
		VoxelFarm::Algebra::Vector edgeZ = VoxelFarm::Algebra::Vector_withValues(0.0f, 0.0f, 1.0f);

		VoxelFarm::Algebra::Matrix rotate_transform = VoxelFarm::Algebra::Quaternion_toMatrix(Rotations);

		edgeX = Matrix_multiplyVector(rotate_transform, edgeX);
		edgeY = Matrix_multiplyVector(rotate_transform, edgeY);
		edgeZ = Matrix_multiplyVector(rotate_transform, edgeZ);

		double cx = VoxelFarm::Algebra::Vector_dot(moveDir, edgeX);
		double cy = VoxelFarm::Algebra::Vector_dot(moveDir, edgeY);
		double cz = VoxelFarm::Algebra::Vector_dot(moveDir, edgeZ);

		moveScope(true, cx, cy, cz);

		Intersection[0] = px;
		Intersection[1] = py;
		Intersection[2] = pz;
	}
}

void ElasticBox::mouseResizeBox(const double orig[3], const double dir[3], const int cursorSize[3], bool oneWay, double minX, double minY, double minZ)
{
	double hscale = VoxelFarm::CELL_SIZE*(1 << VoxelFarm::LOD_0);
	double blockSize = hscale / VoxelFarm::BLOCK_DIMENSION;
	if (minX == 0)
	{
		minX = cursorSize[0] * blockSize;
	}
	if (minY == 0)
	{
		minY = cursorSize[1] * blockSize;
	}
	if (minZ == 0)
	{
		minZ = cursorSize[2] * blockSize;
	}

	if (!Mode || (State != ElasticBox::EB_RESIZE))
	{
		return;
	}

	// Compute face normal of intersected triangle
	double pt1[3] =
	{
		Scope[sBoxTriangles[Triangle][0]][0],
		Scope[sBoxTriangles[Triangle][0]][1],
		Scope[sBoxTriangles[Triangle][0]][2]
	};
	double pt2[3] =
	{
		Scope[sBoxTriangles[Triangle][1]][0],
		Scope[sBoxTriangles[Triangle][1]][1],
		Scope[sBoxTriangles[Triangle][1]][2]
	};
	double pt3[3] =
	{
		Scope[sBoxTriangles[Triangle][2]][0],
		Scope[sBoxTriangles[Triangle][2]][1],
		Scope[sBoxTriangles[Triangle][2]][2]
	};

	float sizeX = 0.0f;
	float sizeY = 0.0f;
	float sizeZ = 0.0f;
	double px, py, pz;
	bool intersection = VoxelFarm::intersectPointPlane(orig, dir, pt1, pt2, pt3, &px, &py, &pz);
	if (intersection)
	{
		VoxelFarm::Algebra::Vector resize = VoxelFarm::Algebra::Vector_withValues(
												(float)(px - Intersection[0]),
												(float)(py - Intersection[1]),
												(float)(pz - Intersection[2]));

		VoxelFarm::Algebra::Vector edgeX = VoxelFarm::Algebra::Vector_withValues(1.0f, 0.0f, 0.0f);
		VoxelFarm::Algebra::Vector edgeY = VoxelFarm::Algebra::Vector_withValues(0.0f, 1.0f, 0.0f);
		VoxelFarm::Algebra::Vector edgeZ = VoxelFarm::Algebra::Vector_withValues(0.0f, 0.0f, 1.0f);

		VoxelFarm::Algebra::Matrix rotate_transform = VoxelFarm::Algebra::Quaternion_toMatrix(Rotations);

		edgeX = Matrix_multiplyVector(rotate_transform, edgeX);
		edgeY = Matrix_multiplyVector(rotate_transform, edgeY);
		edgeZ = Matrix_multiplyVector(rotate_transform, edgeZ);

		sizeX = VoxelFarm::Algebra::Vector_dot(resize, edgeX);
		sizeY = VoxelFarm::Algebra::Vector_dot(resize, edgeY);
		sizeZ = VoxelFarm::Algebra::Vector_dot(resize, edgeZ);

		Intersection[0] = px;
		Intersection[1] = py;
		Intersection[2] = pz;

		double sizeX0 = 0.0;
		double sizeX1 = 0.0;
		double sizeY0 = 0.0;
		double sizeY1 = 0.0;
		double sizeZ0 = 0.0;
		double sizeZ1 = 0.0;
		if (VoxelFarm::VoxelCorners[Corner][0] == 0)
		{
			sizeX0 = -sizeX;
		}
		else
		{
			sizeX1 = sizeX;
		}

		if (VoxelFarm::VoxelCorners[Corner][1] == 0)
		{
			sizeY0 = -sizeY;
		}
		else
		{
			sizeY1 = sizeY;
		}

		if (VoxelFarm::VoxelCorners[Corner][2] == 0)
		{
			sizeZ0 = -sizeZ;
		}
		else
		{
			sizeZ1 = sizeZ;
		}

		if (oneWay)
		{
			if (abs(sizeX) >= abs(sizeY) && abs(sizeX) >= abs(sizeZ))
			{
				sizeY0 = 0.0;
				sizeY1 = 0.0;
				sizeZ0 = 0.0;
				sizeZ1 = 0.0;
			}
			else if (abs(sizeY) >= abs(sizeX) && abs(sizeY) >= abs(sizeZ))
			{
				sizeX0 = 0.0;
				sizeX1 = 0.0;
				sizeZ0 = 0.0;
				sizeZ1 = 0.0;
			}
			else if (abs(sizeZ) >= abs(sizeX) && abs(sizeZ) >= abs(sizeY))
			{
				sizeX0 = 0.0;
				sizeX1 = 0.0;
				sizeY0 = 0.0;
				sizeY1 = 0.0;
			}
		}

		resizeScope(cursorSize, minX, minY, minZ, sizeX0, sizeY0, sizeZ0, sizeX1, sizeY1, sizeZ1);
	}
}

int ElasticBox::rayIntersect(const double orig[3], const double dir[3])
{
	if (!Mode || (State != ElasticBox::EB_EDIT))
	{
		return -1;
	}

	//iterate all triangles in the box
	int intersectTriangle = -1;

	for (int i = 0; i<12; i++)
	{
		// Compute face normal
		double pt1[3] =
		{
			Scope[sBoxTriangles[i][0]][0],
			Scope[sBoxTriangles[i][0]][1],
			Scope[sBoxTriangles[i][0]][2]
		};
		double pt2[3] =
		{
			Scope[sBoxTriangles[i][1]][0],
			Scope[sBoxTriangles[i][1]][1],
			Scope[sBoxTriangles[i][1]][2]
		};
		double pt3[3] =
		{
			Scope[sBoxTriangles[i][2]][0],
			Scope[sBoxTriangles[i][2]][1],
			Scope[sBoxTriangles[i][2]][2]
		};
		double v1[3] =
		{
			pt2[0] - pt1[0],
			pt2[1] - pt1[1],
			pt2[2] - pt1[2]
		};
		double v2[3] =
		{
			pt3[0] - pt1[0],
			pt3[1] - pt1[1],
			pt3[2] - pt1[2]
		};
		double normal[3];
		VF_CROSS(normal, v1, v2);
		double dotp = VF_DOT(dir, normal);

		double px, py, pz;
		double t, u, v;

		// See if the triangle intersects ray
		bool intersection = VoxelFarm::intersectPointTriangle(orig, dir, pt1, pt2, pt3, &px, &py, &pz, &t, &u, &v) && (t > 0);

		if (intersection)
		{
			intersectTriangle = i;

			Triangle = i;

			Intersection[0] = px;
			Intersection[1] = py;
			Intersection[2] = pz;

			double distX = px - orig[0];
			double distY = py - orig[1];
			double distZ = pz - orig[2];

			if (dotp > 0.0)
			{
				break;
			}
		}
	}

	return intersectTriangle;
}


bool ElasticBox::checkMouseRotate(const double orig[3], const double dir[3])
{
	int intersect = rayIntersect(orig, dir);

	if (intersect != -1)
	{
		State = ElasticBox::EB_ROTATE;
	}

	return (intersect != -1);
}


bool ElasticBox::checkMouseMove(const double orig[3], const double dir[3])
{
	int intersect = rayIntersect(orig, dir);

	if (intersect != -1)
	{
		State = ElasticBox::EB_MOVE;
	}

	return (intersect != -1);
}

bool ElasticBox::checkMouseResize(const double orig[3], const double dir[3])
{
	int intersect = rayIntersect(orig, dir);

	if (intersect != -1)
	{
		State = ElasticBox::EB_RESIZE;

		double pt1[3] =
		{
			Scope[sBoxTriangles[Triangle][0]][0],
			Scope[sBoxTriangles[Triangle][0]][1],
			Scope[sBoxTriangles[Triangle][0]][2]
		};
		double pt2[3] =
		{
			Scope[sBoxTriangles[Triangle][1]][0],
			Scope[sBoxTriangles[Triangle][1]][1],
			Scope[sBoxTriangles[Triangle][1]][2]
		};
		double pt3[3] =
		{
			Scope[sBoxTriangles[Triangle][2]][0],
			Scope[sBoxTriangles[Triangle][2]][1],
			Scope[sBoxTriangles[Triangle][2]][2]
		};

		double deltaX = Intersection[0] - pt1[0];
		double deltaY = Intersection[1] - pt1[1];
		double deltaZ = Intersection[2] - pt1[2];
		double dist1 = deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ;

		deltaX = Intersection[0] - pt2[0];
		deltaY = Intersection[1] - pt2[1];
		deltaZ = Intersection[2] - pt2[2];
		double dist2 = deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ;

		deltaX = Intersection[0] - pt3[0];
		deltaY = Intersection[1] - pt3[1];
		deltaZ = Intersection[2] - pt3[2];
		double dist3 = deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ;

		int pt = 0;
		if (dist1 < dist2)
		{
			if (dist1 < dist3)
			{
				pt = 0;
			}
			else
			{
				pt = 2;
			}
		}
		else
		{
			if (dist2 < dist3)
			{
				pt = 1;
			}
			else
			{
				pt = 2;
			}
		}

		Corner = sBoxTriangles[Triangle][pt];
	}

	return (intersect != -1);
}

