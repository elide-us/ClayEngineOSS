#pragma once

#include "ClipmapView.h"

/// Elastic Box Tool
class ElasticBox
{
public:
	/// It signals whether the Elastic Box Tool is in use.
	bool Mode;
	/// It signals whether that is the first box been created.
	bool First;
	/// It is the state of the box tool
	enum EBState
	{
		/// The box is been edited
		EB_EDIT,
		/// The box is been moved
		EB_MOVE,
		/// The box is been resized
		EB_RESIZE,
		/// The box is been rotated
		EB_ROTATE
	} State;

	/// It is the purpose of the box tool
	enum EBPurpose
	{
		/// For inserting prefabs
		EBP_PREFAB,
		/// For general purpose
		EBP_GENERAL,
		/// For copy&paste operations
		EBP_COPY,
		/// For cut&paste operations
		EBP_CUT
	} Purpose;

	double PrevSize[3];

	/// It defines the scope for the box tool. Each vertex is a corner of the box.
	double Scope[8][3];
	/// Position of the box in world units
	double WorldPos[3];
	/// Size of the box in world units
	double Size[3];
	/// Rotation applied to the box
	VoxelFarm::Algebra::Quaternion Rotations;
	double Intersection[3];
	int Triangle;
	int	Corner;

	//copy & paste
	/// It remembers the size of the clipboard during the last copy/cut operation
	double CopySize[3];
	/// It is an offset of the data inside of the clipboard
	float OffsetClipboard[3];
	/// It remembers the scope of the cut operation (for later erasure)
	double CutScope[8][3];

	// zero-initialize
	ElasticBox();

	/// It initializes the Elastic Box Tool
	void beginBox(
		/// Purpose of the box tool
		ElasticBox::EBPurpose purpose,
		/// It signals whether is a new box. It is used to calculate the dimensions of the box.
		bool newBox,
		/// It is the description of an initial prefab in case the purpose was EBP_PREFAB.
		VoxelFarm::Architecture::CPrefabDesc *prefabDesc,
		/// It is the position of the box in the world
		const VoxelFarm::VoxelHitInfo& hitInfo,
		/// It is the default size of the box
		const int buildCursorSizes[3]
	);

	/// It ends the box tool by stamping the selected prefab in case the purpose was EBP_PREFAB into a block layer
	void endBox(
		/// The view object being stamped into
		VoxelFarm::CClipmapView* view,
		/// It keeps the list of all prefabs in the system
		VoxelFarm::Architecture::CArchitectureManager* architectureManager,
		/// Identifier of the prefab to be stamped
		int moduleId,
		/// Palette of materials
		VoxelFarm::Architecture::CPaletteDesc* palette,
		/// Type of the palette
		VoxelFarm::Architecture::PaletteType type,
		/// It allows to translate a material depending on the position in the world
		VoxelFarm::IMeshStamMaterialSource* materials
	);

	/// It ends the box tool by cancelling the operation
	void cancel();

	/// Copy block data from the area covered by this box into the given clipboard
	void copyBlock(
		/// The view object being stamped into
		VoxelFarm::CClipmapView* view,
		/// clipboard to copy into
		VoxelFarm::IClipboard* clipboard,
		/// whether this is a cut or copy
		bool cut
	);

	void pasteBlock(
		/// this function is really awkward, references internals of clipmapView.  $$$REI refactor further
		VoxelFarm::CClipmapView* clipmapView,
		/// Clipboard to paste from
		VoxelFarm::IClipboard* clipboard,
		/// Whether or not to paste voxels with material 0 into the target
		bool air
	);

	/// A transformation matrix that modifies (rotate and scale) the data in the clipboard.
	VoxelFarm::Algebra::Matrix previewPasteTransform();

	/// Resize the box to match the size of a different prefab
	void changeSize(VoxelFarm::Architecture::CPrefabDesc prefabDesc);

	/// It smooths the encoded surface in the voxels inside the box
	void smoothArea(VoxelFarm::CClipmapView* pView);

	/// It raises the encoded surface in the voxels inside the box scope
	void raiseArea(VoxelFarm::CClipmapView* pView);

	/// It removes data inside the box tool.
	void removeArea(VoxelFarm::CClipmapView* view, bool fullErase);

	/// It checks if the view direction ray intersects the box scope
	int rayIntersect(const double orig[3], const double dir[3]); // note: modifies internal state

	void rotateScope(float dx, float dy, float dz);
	void rotateScope(VoxelFarm::Algebra::Vector axis, double angle);
	void resizeScope(const int cursorSize[3], double minX, double minY, double minZ, double dx0, double dy0, double dz0, double dx1, double dy1, double dz1);
	void moveScope(bool boxDir, double dx, double dy, double dz);

	void mouseMoveBox(const double pos[3], const double dir[3]);
	void mouseResizeBox(const double orig[3], const double dir[3], const int cursorSize[3], bool oneWay, double minX, double minY, double minZ);
	bool checkMouseRotate(const double orig[3], const double dir[3]);
	bool checkMouseMove(const double orig[3], const double dir[3]);
	bool checkMouseResize(const double orig[3], const double dir[3]);

	/// It updates the box scope based on the box modifications (mostly an internal function)
	void updateScope();
};

