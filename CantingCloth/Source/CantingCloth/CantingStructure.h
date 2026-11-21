// Reads which ward a wax drawing makes: finds the sealed cells and measures whether their walls are curved
// (filter ward) or straight (deflect ward). See docs/Canting_System_Design.md, sections 5.1 and 5.2.
//
// Uses only the C++ standard library so it can be tested outside Unreal.

#pragma once

#include "CantingWaxGrid.h"

#include <cstdint>
#include <vector>

namespace CantingCore
{
	// Defaults were tuned on the drawings in CantingCloth/CantingDrawings (docs/canting_spikes/read_drawings.cpp).
	struct FWardParams
	{
		double TexelsPerCm = 5.12;
		/** Wax line half-width; the wall's centre line is this far outside the cell's edge. */
		double BrushRadiusCm = 1.2;
		/** Gaps in an outline up to this width still count as closed. At most 1.5 cm (the stored distance band). */
		double GapToleranceCm = 1.0;
		/** Smaller cells are ignored (such as the hole left inside a fat dot). */
		double MinCellCm2 = 4.0;
		/** A cell counts for at most this much area, so one huge outline can't carry the whole ward. */
		double MaxCellCm2 = 60.0;
		/** Curved share at or above this reads fully curved; at or below StraightMax fully straight; linear in between. */
		double CurvedMin = 0.45;
		double StraightMax = 0.25;
		/** Where the wall shape is measured: 0 = the cell's inner edge, BrushRadiusCm = the wall's centre line. */
		double OutlineOffsetCm = 0.3;
		/** Cells at least this many times longer than wide are bands and read straight, whatever their walls do. */
		double BandAspect = 3.0;
		/** Turning (degrees) over about 3.5% of the outline that marks a corner. */
		double CornerDeg = 38.0;
		/** A wall between two corners is curved if it bows away from its chord by more than this share of the chord. */
		double WallBow = 0.15;
	};

	struct FCellReading
	{
		double AreaCm2 = 0.0;
		double CenterXCm = 0.0;
		double CenterYCm = 0.0;
		/** Share of the wall length that is curved: 1 = all curves, 0 = all straight walls. */
		double CurvedShare = 0.0;
		/** Length over width, from the cell's spread (1 = round or square). */
		double Aspect = 1.0;
		/** How much of this cell's power goes to the filter ward (the rest goes to deflect). */
		double FilterShare = 0.0;
	};

	struct FWardReading
	{
		std::vector<FCellReading> Cells;
		/** Sum over cells of min(area, MaxCellCm2) x family share. */
		double FilterPowerCm2 = 0.0;
		double DeflectPowerCm2 = 0.0;
		/** Cells that read mostly one way (share >= 0.75) or in between. */
		int FilterCells = 0;
		int DeflectCells = 0;
		int MixedCells = 0;
	};

	/**
	 * Reads the wards in a wax grid. A cell is unwaxed area that doesn't touch the grid border.
	 * If given, OutFilterMask and OutDeflectMask are resized to the grid and hold 255 x the cell's
	 * share of each family inside every cell (0 outside cells), for tinting the cloth.
	 */
	FWardReading ReadWards(const FWaxGrid& Grid, const FWardParams& Params,
		std::vector<uint8_t>* OutFilterMask = nullptr, std::vector<uint8_t>* OutDeflectMask = nullptr);

	/** Curved share of a closed outline given as interleaved (x, y) points. Exposed for tests. */
	double CurvedShareOfOutline(const std::vector<double>& XY, double CornerDeg, double WallBow);
}
