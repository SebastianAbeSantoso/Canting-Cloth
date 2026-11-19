// Canting wax distance grid: the engine-independent core of the canting system.
//
// Stores, per texel, the signed distance to the nearest wax stroke (negative inside wax),
// clamped to +/- BandTexels and packed into one byte: 0 = deep inside, about 128 = wax edge, 255 = no wax nearby.
// Overlapping strokes keep the smaller distance, so line width never depends on cursor speed or overlap.
//
// Uses only the C++ standard library so the math can be tested outside Unreal.

#pragma once

#include <cstdint>
#include <vector>

namespace CantingCore
{
	// Integer rectangle in texels, half-open: [X0, X1) x [Y0, Y1).
	struct FRectI
	{
		int X0 = 0;
		int Y0 = 0;
		int X1 = 0;
		int Y1 = 0;

		bool IsEmpty() const { return X1 <= X0 || Y1 <= Y0; }
		void Merge(const FRectI& Other);
		bool Intersects(const FRectI& Other) const;
		FRectI ClippedTo(const FRectI& Other) const;
	};

	class FWaxGrid
	{
	public:
		// Half-width of the stored distance band. M_BatikCloth decodes with the same value:
		// distance = (R - 0.5) * 2 * BandTexels, i.e. (R - 0.5) * 16.
		static constexpr float BandTexels = 8.0f;

		void Init(int InWidth, int InHeight);
		bool IsValid() const { return Width > 0 && Height > 0; }
		int GetWidth() const { return Width; }
		int GetHeight() const { return Height; }
		FRectI FullRect() const { return FRectI{0, 0, Width, Height}; }
		const std::vector<uint8_t>& GetData() const { return Pixels; }

		static uint8_t Encode(float SignedDistanceTexels);
		static float Decode(uint8_t Value);

		// Resets a rectangle to "no wax".
		void Clear(const FRectI& Rect);

		// Stamps a capsule from A (radius AR) to B (radius BR), all in texels. Returns the texels touched.
		FRectI StampSegment(float AX, float AY, float AR, float BX, float BY, float BR, const FRectI* Clip = nullptr);

		// Stamps a polyline. XY holds NumPoints interleaved (x, y) pairs in any unit; Scale converts them to texels.
		// A single point stamps a round dot.
		FRectI StampPolyline(const double* XY, int NumPoints, double Scale, float RadiusTexels, const FRectI* Clip = nullptr);

		// Texels a polyline can affect (not clipped to the grid).
		static FRectI PolylineBounds(const double* XY, int NumPoints, double Scale, float RadiusTexels);

	private:
		FRectI StampPiece(float AX, float AY, float AR, float BX, float BY, float BR, const FRectI* Clip);

		int Width = 0;
		int Height = 0;
		std::vector<uint8_t> Pixels;
	};

	struct FRegionResult
	{
		int EnclosedRegions = 0;       // background regions fully walled in by wax
		long long EnclosedTexels = 0;  // total area of those regions
		long long WaxTexels = 0;       // texels covered by wax
	};

	// Flood-fills the unwaxed background (4-connected, so diagonal wax lines seal).
	// A region counts as enclosed if it never touches the grid border and has at least MinAreaTexels texels.
	// Texels count as wax when their distance is below ToleranceTexels, so a tolerance of T forgives gaps up to 2*T wide.
	// OutSealedMask is resized to the grid and set to 255 inside enclosed regions, 0 elsewhere.
	FRegionResult AnalyseRegions(const FWaxGrid& Grid, float ToleranceTexels, int MinAreaTexels, std::vector<uint8_t>& OutSealedMask);

	// Total length of a polyline, in the units of XY.
	double PolylineLength(const double* XY, int NumPoints);
}
