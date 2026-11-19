#include "CantingWaxGrid.h"

#include <algorithm>
#include <cmath>

namespace CantingCore
{
	namespace
	{
		// Long segments are split so each piece's bounding box stays tight on diagonals.
		constexpr float MaxPieceTexels = 16.0f;

		float ClampF(float Value, float Lo, float Hi)
		{
			return Value < Lo ? Lo : (Value > Hi ? Hi : Value);
		}

		size_t ToIndex(int X, int Y, int RowWidth)
		{
			return static_cast<size_t>(Y) * static_cast<size_t>(RowWidth) + static_cast<size_t>(X);
		}
	}

	void FRectI::Merge(const FRectI& Other)
	{
		if (Other.IsEmpty())
		{
			return;
		}
		if (IsEmpty())
		{
			*this = Other;
			return;
		}
		X0 = std::min(X0, Other.X0);
		Y0 = std::min(Y0, Other.Y0);
		X1 = std::max(X1, Other.X1);
		Y1 = std::max(Y1, Other.Y1);
	}

	bool FRectI::Intersects(const FRectI& Other) const
	{
		return X0 < Other.X1 && Other.X0 < X1 && Y0 < Other.Y1 && Other.Y0 < Y1;
	}

	FRectI FRectI::ClippedTo(const FRectI& Other) const
	{
		return FRectI{std::max(X0, Other.X0), std::max(Y0, Other.Y0), std::min(X1, Other.X1), std::min(Y1, Other.Y1)};
	}

	void FWaxGrid::Init(int InWidth, int InHeight)
	{
		Width = std::max(InWidth, 0);
		Height = std::max(InHeight, 0);
		Pixels.assign(static_cast<size_t>(Width) * static_cast<size_t>(Height), uint8_t(255));
	}

	uint8_t FWaxGrid::Encode(float SignedDistanceTexels)
	{
		const float Normalized = SignedDistanceTexels / BandTexels * 0.5f + 0.5f;
		return static_cast<uint8_t>(ClampF(Normalized * 255.0f + 0.5f, 0.0f, 255.0f));
	}

	float FWaxGrid::Decode(uint8_t Value)
	{
		return (static_cast<float>(Value) / 255.0f - 0.5f) * 2.0f * BandTexels;
	}

	void FWaxGrid::Clear(const FRectI& Rect)
	{
		const FRectI R = Rect.ClippedTo(FullRect());
		if (R.IsEmpty())
		{
			return;
		}
		for (int Y = R.Y0; Y < R.Y1; ++Y)
		{
			uint8_t* Row = &Pixels[ToIndex(0, Y, Width)];
			std::fill(Row + R.X0, Row + R.X1, uint8_t(255));
		}
	}

	FRectI FWaxGrid::StampSegment(float AX, float AY, float AR, float BX, float BY, float BR, const FRectI* Clip)
	{
		const float DX = BX - AX;
		const float DY = BY - AY;
		const float Length = std::sqrt(DX * DX + DY * DY);
		const int NumPieces = std::max(1, static_cast<int>(std::ceil(Length / MaxPieceTexels)));

		FRectI Touched;
		float PX = AX;
		float PY = AY;
		float PR = AR;
		for (int Piece = 1; Piece <= NumPieces; ++Piece)
		{
			const float T = static_cast<float>(Piece) / static_cast<float>(NumPieces);
			const float NX = AX + DX * T;
			const float NY = AY + DY * T;
			const float NR = AR + (BR - AR) * T;
			Touched.Merge(StampPiece(PX, PY, PR, NX, NY, NR, Clip));
			PX = NX;
			PY = NY;
			PR = NR;
		}
		return Touched;
	}

	FRectI FWaxGrid::StampPolyline(const double* XY, int NumPoints, double Scale, float RadiusTexels, const FRectI* Clip)
	{
		FRectI Touched;
		if (XY == nullptr || NumPoints <= 0)
		{
			return Touched;
		}
		auto PointX = [XY, Scale](int Index) { return static_cast<float>(XY[2 * Index] * Scale); };
		auto PointY = [XY, Scale](int Index) { return static_cast<float>(XY[2 * Index + 1] * Scale); };

		if (NumPoints == 1)
		{
			return StampSegment(PointX(0), PointY(0), RadiusTexels, PointX(0), PointY(0), RadiusTexels, Clip);
		}
		for (int Index = 1; Index < NumPoints; ++Index)
		{
			Touched.Merge(StampSegment(PointX(Index - 1), PointY(Index - 1), RadiusTexels, PointX(Index), PointY(Index), RadiusTexels, Clip));
		}
		return Touched;
	}

	FRectI FWaxGrid::PolylineBounds(const double* XY, int NumPoints, double Scale, float RadiusTexels)
	{
		if (XY == nullptr || NumPoints <= 0)
		{
			return FRectI{};
		}
		double MinX = XY[0];
		double MaxX = XY[0];
		double MinY = XY[1];
		double MaxY = XY[1];
		for (int Index = 1; Index < NumPoints; ++Index)
		{
			MinX = std::min(MinX, XY[2 * Index]);
			MaxX = std::max(MaxX, XY[2 * Index]);
			MinY = std::min(MinY, XY[2 * Index + 1]);
			MaxY = std::max(MaxY, XY[2 * Index + 1]);
		}
		const double Pad = static_cast<double>(RadiusTexels + BandTexels);
		return FRectI{
			static_cast<int>(std::floor(MinX * Scale - Pad)),
			static_cast<int>(std::floor(MinY * Scale - Pad)),
			static_cast<int>(std::ceil(MaxX * Scale + Pad)) + 1,
			static_cast<int>(std::ceil(MaxY * Scale + Pad)) + 1};
	}

	FRectI FWaxGrid::StampPiece(float AX, float AY, float AR, float BX, float BY, float BR, const FRectI* Clip)
	{
		const float Pad = std::max(AR, BR) + BandTexels;
		FRectI R{
			static_cast<int>(std::floor(std::min(AX, BX) - Pad)),
			static_cast<int>(std::floor(std::min(AY, BY) - Pad)),
			static_cast<int>(std::ceil(std::max(AX, BX) + Pad)) + 1,
			static_cast<int>(std::ceil(std::max(AY, BY) + Pad)) + 1};
		R = R.ClippedTo(FullRect());
		if (Clip != nullptr)
		{
			R = R.ClippedTo(*Clip);
		}
		if (R.IsEmpty())
		{
			return FRectI{};
		}

		const float BAX = BX - AX;
		const float BAY = BY - AY;
		const float InvLenSq = 1.0f / std::max(BAX * BAX + BAY * BAY, 1e-6f);
		for (int Y = R.Y0; Y < R.Y1; ++Y)
		{
			uint8_t* Row = &Pixels[ToIndex(0, Y, Width)];
			const float PAY = static_cast<float>(Y) + 0.5f - AY;
			for (int X = R.X0; X < R.X1; ++X)
			{
				const float PAX = static_cast<float>(X) + 0.5f - AX;
				const float H = ClampF((PAX * BAX + PAY * BAY) * InvLenSq, 0.0f, 1.0f);
				const float OffX = PAX - BAX * H;
				const float OffY = PAY - BAY * H;
				const float SignedDistance = std::sqrt(OffX * OffX + OffY * OffY) - (AR + (BR - AR) * H);
				const uint8_t Encoded = Encode(SignedDistance);
				if (Encoded < Row[X])
				{
					Row[X] = Encoded;
				}
			}
		}
		return R;
	}

	FRegionResult AnalyseRegions(const FWaxGrid& Grid, float ToleranceTexels, int MinAreaTexels, std::vector<uint8_t>& OutSealedMask)
	{
		FRegionResult Result;
		const int GridW = Grid.GetWidth();
		const int GridH = Grid.GetHeight();
		const size_t Count = static_cast<size_t>(GridW) * static_cast<size_t>(GridH);
		OutSealedMask.assign(Count, uint8_t(0));
		if (Count == 0)
		{
			return Result;
		}

		const std::vector<uint8_t>& Pixels = Grid.GetData();
		const uint8_t WaxThreshold = FWaxGrid::Encode(ToleranceTexels);
		const uint8_t CoverThreshold = FWaxGrid::Encode(0.0f);

		// -2 = wax, -1 = unvisited background, >= 0 = region id
		std::vector<int> Label(Count, -1);
		for (size_t Index = 0; Index < Count; ++Index)
		{
			if (Pixels[Index] < WaxThreshold)
			{
				Label[Index] = -2;
			}
			if (Pixels[Index] < CoverThreshold)
			{
				++Result.WaxTexels;
			}
		}

		std::vector<long long> RegionArea;
		std::vector<uint8_t> RegionTouchesBorder;
		std::vector<size_t> Stack;
		for (size_t Start = 0; Start < Count; ++Start)
		{
			if (Label[Start] != -1)
			{
				continue;
			}
			const int Id = static_cast<int>(RegionArea.size());
			long long Area = 0;
			bool bBorder = false;
			Label[Start] = Id;
			Stack.assign(1, Start);
			while (!Stack.empty())
			{
				const size_t Cell = Stack.back();
				Stack.pop_back();
				const int CX = static_cast<int>(Cell % static_cast<size_t>(GridW));
				const int CY = static_cast<int>(Cell / static_cast<size_t>(GridW));
				++Area;
				if (CX == 0 || CY == 0 || CX == GridW - 1 || CY == GridH - 1)
				{
					bBorder = true;
				}
				const bool HasNeighbour[4] = {CX > 0, CX < GridW - 1, CY > 0, CY < GridH - 1};
				const size_t Neighbour[4] = {
					HasNeighbour[0] ? Cell - 1 : Cell,
					HasNeighbour[1] ? Cell + 1 : Cell,
					HasNeighbour[2] ? Cell - static_cast<size_t>(GridW) : Cell,
					HasNeighbour[3] ? Cell + static_cast<size_t>(GridW) : Cell};
				for (int N = 0; N < 4; ++N)
				{
					if (HasNeighbour[N] && Label[Neighbour[N]] == -1)
					{
						Label[Neighbour[N]] = Id;
						Stack.push_back(Neighbour[N]);
					}
				}
			}
			RegionArea.push_back(Area);
			RegionTouchesBorder.push_back(bBorder ? uint8_t(1) : uint8_t(0));
		}

		std::vector<uint8_t> IsEnclosed(RegionArea.size(), uint8_t(0));
		for (size_t Id = 0; Id < RegionArea.size(); ++Id)
		{
			if (RegionTouchesBorder[Id] == 0 && RegionArea[Id] >= MinAreaTexels)
			{
				IsEnclosed[Id] = 1;
				++Result.EnclosedRegions;
				Result.EnclosedTexels += RegionArea[Id];
			}
		}
		for (size_t Index = 0; Index < Count; ++Index)
		{
			if (Label[Index] >= 0 && IsEnclosed[static_cast<size_t>(Label[Index])] != 0)
			{
				OutSealedMask[Index] = 255;
			}
		}
		return Result;
	}

	double PolylineLength(const double* XY, int NumPoints)
	{
		double Total = 0.0;
		for (int Index = 1; Index < NumPoints; ++Index)
		{
			const double DX = XY[2 * Index] - XY[2 * Index - 2];
			const double DY = XY[2 * Index + 1] - XY[2 * Index - 1];
			Total += std::sqrt(DX * DX + DY * DY);
		}
		return Total;
	}
}
