#include "CantingStructure.h"

#include <algorithm>
#include <cmath>

namespace CantingCore
{
	namespace
	{
		constexpr double Pi = 3.14159265358979323846;

		double WrapRadians(double A)
		{
			while (A > Pi) A -= 2.0 * Pi;
			while (A < -Pi) A += 2.0 * Pi;
			return A;
		}

		// 0 at Edge0, 1 at Edge1, linear in between.
		double LinearStep(double Edge0, double Edge1, double X)
		{
			if (Edge1 <= Edge0) return X >= Edge1 ? 1.0 : 0.0;
			return std::min(std::max((X - Edge0) / (Edge1 - Edge0), 0.0), 1.0);
		}

		size_t Index(int X, int Y, int W)
		{
			return static_cast<size_t>(Y) * static_cast<size_t>(W) + static_cast<size_t>(X);
		}

		struct FRegions
		{
			int W = 0;
			int H = 0;
			int Num = 0;
			std::vector<int> Label;          // >= 0 region id, -1 wax
			std::vector<char> TouchesBorder;
			std::vector<long long> Area;
			std::vector<double> SumX, SumY, SumXX, SumYY, SumXY;
			std::vector<int> X0, Y0, X1, Y1;
		};

		FRegions LabelRegions(const FWaxGrid& Grid, float ToleranceTexels)
		{
			FRegions R;
			R.W = Grid.GetWidth();
			R.H = Grid.GetHeight();
			const size_t Count = static_cast<size_t>(R.W) * static_cast<size_t>(R.H);
			const uint8_t Threshold = FWaxGrid::Encode(ToleranceTexels);
			const std::vector<uint8_t>& Pixels = Grid.GetData();
			R.Label.assign(Count, -2);
			for (size_t I = 0; I < Count; ++I)
			{
				if (Pixels[I] < Threshold) R.Label[I] = -1;
			}
			std::vector<size_t> Stack;
			for (size_t Start = 0; Start < Count; ++Start)
			{
				if (R.Label[Start] != -2) continue;
				const int Id = R.Num++;
				bool bBorder = false;
				long long Area = 0;
				double SX = 0.0, SY = 0.0, SXX = 0.0, SYY = 0.0, SXY = 0.0;
				int BX0 = R.W, BY0 = R.H, BX1 = -1, BY1 = -1;
				R.Label[Start] = Id;
				Stack.assign(1, Start);
				while (!Stack.empty())
				{
					const size_t Cell = Stack.back();
					Stack.pop_back();
					const int X = static_cast<int>(Cell % static_cast<size_t>(R.W));
					const int Y = static_cast<int>(Cell / static_cast<size_t>(R.W));
					++Area;
					const double CX = X + 0.5, CY = Y + 0.5;
					SX += CX; SY += CY; SXX += CX * CX; SYY += CY * CY; SXY += CX * CY;
					BX0 = std::min(BX0, X); BY0 = std::min(BY0, Y); BX1 = std::max(BX1, X); BY1 = std::max(BY1, Y);
					if (X == 0 || Y == 0 || X == R.W - 1 || Y == R.H - 1) bBorder = true;
					const bool Has[4] = {X > 0, X < R.W - 1, Y > 0, Y < R.H - 1};
					const size_t Next[4] = {Cell - 1, Cell + 1, Cell - static_cast<size_t>(R.W), Cell + static_cast<size_t>(R.W)};
					for (int N = 0; N < 4; ++N)
					{
						if (Has[N] && R.Label[Next[N]] == -2)
						{
							R.Label[Next[N]] = Id;
							Stack.push_back(Next[N]);
						}
					}
				}
				R.TouchesBorder.push_back(bBorder ? 1 : 0);
				R.Area.push_back(Area);
				R.SumX.push_back(SX);
				R.SumY.push_back(SY);
				R.SumXX.push_back(SXX); R.SumYY.push_back(SYY); R.SumXY.push_back(SXY);
				R.X0.push_back(BX0); R.Y0.push_back(BY0); R.X1.push_back(BX1); R.Y1.push_back(BY1);
			}
			return R;
		}

		// Mask of region Id in a local box, grown by Radius texels (to reach the wall's centre line).
		std::vector<char> RegionMask(const FRegions& R, int Id, double Radius, int& OutW, int& OutH)
		{
			const int Pad = static_cast<int>(std::ceil(Radius)) + 2;
			const int OX = R.X0[Id] - Pad;
			const int OY = R.Y0[Id] - Pad;
			OutW = R.X1[Id] - R.X0[Id] + 1 + 2 * Pad;
			OutH = R.Y1[Id] - R.Y0[Id] + 1 + 2 * Pad;
			std::vector<char> Mask(static_cast<size_t>(OutW) * static_cast<size_t>(OutH), 0);
			std::vector<std::pair<int, int>> Disk;
			const int RI = static_cast<int>(std::ceil(Radius));
			for (int DY = -RI; DY <= RI; ++DY)
				for (int DX = -RI; DX <= RI; ++DX)
					if (DX * DX + DY * DY <= Radius * Radius) Disk.push_back({DX, DY});
			auto IsId = [&R, Id](int X, int Y)
			{
				return X >= 0 && Y >= 0 && X < R.W && Y < R.H && R.Label[Index(X, Y, R.W)] == Id;
			};
			for (int Y = R.Y0[Id]; Y <= R.Y1[Id]; ++Y)
			{
				for (int X = R.X0[Id]; X <= R.X1[Id]; ++X)
				{
					if (!IsId(X, Y)) continue;
					Mask[Index(X - OX, Y - OY, OutW)] = 1;
					if (Radius <= 0.0) continue;
					const bool bEdge = !IsId(X - 1, Y) || !IsId(X + 1, Y) || !IsId(X, Y - 1) || !IsId(X, Y + 1);
					if (!bEdge) continue;
					for (const auto& D : Disk) Mask[Index(X - OX + D.first, Y - OY + D.second, OutW)] = 1;
				}
			}
			return Mask;
		}

		// Outer boundary of the blob in a local mask (Moore-neighbour tracing, Jacob's stopping rule).
		std::vector<double> TraceOutline(const std::vector<char>& Mask, int W, int H)
		{
			std::vector<double> XY;
			int Start = -1;
			for (int I = 0; I < W * H && Start < 0; ++I) if (Mask[static_cast<size_t>(I)]) Start = I;
			if (Start < 0) return XY;
			auto In = [&Mask, W, H](int X, int Y) { return X >= 0 && Y >= 0 && X < W && Y < H && Mask[Index(X, Y, W)]; };
			const int DX8[8] = {1, 1, 0, -1, -1, -1, 0, 1};
			const int DY8[8] = {0, 1, 1, 1, 0, -1, -1, -1};
			const int SX = Start % W, SY = Start / W;
			int X = SX, Y = SY, Dir = 7, FirstDir = -1;
			for (int Guard = 0; Guard < 8 * W * H; ++Guard)
			{
				int D = (Dir + 6) % 8, K = 0;
				for (; K < 8; ++K, D = (D + 1) % 8) if (In(X + DX8[D], Y + DY8[D])) break;
				if (K == 8) { XY.push_back(X + 0.5); XY.push_back(Y + 0.5); break; }
				if (FirstDir < 0) FirstDir = D;
				else if (X == SX && Y == SY && D == FirstDir) break;
				XY.push_back(X + 0.5);
				XY.push_back(Y + 0.5);
				X += DX8[D];
				Y += DY8[D];
				Dir = D;
			}
			return XY;
		}
	}

	double CurvedShareOfOutline(const std::vector<double>& XY, double CornerDeg, double WallBow)
	{
		const int N = static_cast<int>(XY.size() / 2);
		if (N < 8) return 0.0;
		auto PX = [&XY](int I) { return XY[2 * static_cast<size_t>(I)]; };
		auto PY = [&XY](int I) { return XY[2 * static_cast<size_t>(I) + 1]; };

		// Resample to M evenly spaced points.
		std::vector<double> Cum(static_cast<size_t>(N) + 1, 0.0);
		for (int I = 0; I < N; ++I)
			Cum[static_cast<size_t>(I) + 1] = Cum[static_cast<size_t>(I)] + std::hypot(PX((I + 1) % N) - PX(I), PY((I + 1) % N) - PY(I));
		const double L = Cum[static_cast<size_t>(N)];
		constexpr int M = 200;
		std::vector<double> UX(M), UY(M);
		for (int J = 0, I = 0; J < M; ++J)
		{
			const double T = L * J / M;
			while (I < N - 1 && Cum[static_cast<size_t>(I) + 1] < T) ++I;
			const double F = (T - Cum[static_cast<size_t>(I)]) / std::max(1e-9, Cum[static_cast<size_t>(I) + 1] - Cum[static_cast<size_t>(I)]);
			UX[static_cast<size_t>(J)] = PX(I) + (PX((I + 1) % N) - PX(I)) * F;
			UY[static_cast<size_t>(J)] = PY(I) + (PY((I + 1) % N) - PY(I)) * F;
		}
		auto Wrap = [](int J) { return static_cast<size_t>((J % M + M) % M); };

		// Light smoothing to remove the pixel staircase.
		std::vector<double> SX(M), SY(M);
		for (int J = 0; J < M; ++J)
		{
			double AX = 0.0, AY = 0.0, AW = 0.0;
			for (int K = -6; K <= 6; ++K)
			{
				const double Wt = std::exp(-K * K / 8.0);
				AX += Wt * UX[Wrap(J + K)];
				AY += Wt * UY[Wrap(J + K)];
				AW += Wt;
			}
			SX[static_cast<size_t>(J)] = AX / AW;
			SY[static_cast<size_t>(J)] = AY / AW;
		}

		// Turning per point, summed over a short window; corners are its peaks.
		std::vector<double> Turn(M), Window(M);
		for (int J = 0; J < M; ++J)
		{
			const size_t A = Wrap(J - 1), B = Wrap(J), C = Wrap(J + 1);
			Turn[static_cast<size_t>(J)] = WrapRadians(std::atan2(SY[C] - SY[B], SX[C] - SX[B]) - std::atan2(SY[B] - SY[A], SX[B] - SX[A])) * 180.0 / Pi;
		}
		for (int J = 0; J < M; ++J)
		{
			double T = 0.0;
			for (int K = -3; K <= 3; ++K) T += Turn[Wrap(J + K)];
			Window[static_cast<size_t>(J)] = std::fabs(T);
		}
		std::vector<int> Corners;
		for (int J = 0; J < M; ++J)
		{
			if (Window[static_cast<size_t>(J)] < CornerDeg) continue;
			bool bPeak = true;
			for (int K = -8; K <= 8 && bPeak; ++K)
			{
				if (K == 0) continue;
				const double Other = Window[Wrap(J + K)];
				if (Other > Window[static_cast<size_t>(J)] || (Other == Window[static_cast<size_t>(J)] && K < 0)) bPeak = false;
			}
			if (bPeak) Corners.push_back(J);
		}
		if (Corners.size() < 2) return 1.0;   // no corners: one curved wall

		// Each wall between corners is curved if it bows away from its chord.
		double CurvedLength = 0.0, TotalLength = 0.0;
		for (size_t C = 0; C < Corners.size(); ++C)
		{
			const int A = Corners[C];
			const int B = Corners[(C + 1) % Corners.size()];
			const int Len = ((B - A) % M + M) % M;
			const double AX = SX[Wrap(A)], AY = SY[Wrap(A)], BX = SX[Wrap(B)], BY = SY[Wrap(B)];
			const double Chord = std::hypot(BX - AX, BY - AY);
			double Bow = 0.0;
			for (int K = 1; K < Len; ++K)
			{
				const size_t Q = Wrap(A + K);
				const double Dist = std::fabs((BX - AX) * (SY[Q] - AY) - (BY - AY) * (SX[Q] - AX)) / std::max(1e-9, Chord);
				Bow = std::max(Bow, Dist);
			}
			TotalLength += Len;
			if (Bow > WallBow * Chord) CurvedLength += Len;
		}
		return CurvedLength / std::max(1.0, TotalLength);
	}

	FWardReading ReadWards(const FWaxGrid& Grid, const FWardParams& Params,
		std::vector<uint8_t>* OutFilterMask, std::vector<uint8_t>* OutDeflectMask)
	{
		FWardReading Reading;
		const size_t Count = static_cast<size_t>(Grid.GetWidth()) * static_cast<size_t>(Grid.GetHeight());
		if (OutFilterMask) OutFilterMask->assign(Count, uint8_t(0));
		if (OutDeflectMask) OutDeflectMask->assign(Count, uint8_t(0));
		if (Count == 0 || Params.TexelsPerCm <= 0.0) return Reading;

		const double T = Params.TexelsPerCm;
		const double T2 = T * T;
		// A texel counts as wax within half the gap tolerance of a stroke, so gaps up to the tolerance close.
		const float Tolerance = static_cast<float>(std::min(0.5 * Params.GapToleranceCm * T, static_cast<double>(FWaxGrid::BandTexels) - 0.5));
		const FRegions R = LabelRegions(Grid, std::max(Tolerance, 0.0f));
		const long long MinArea = std::max<long long>(1, static_cast<long long>(std::ceil(Params.MinCellCm2 * T2)));
		// Grow each cell back by the tolerance that was added to the wax, plus the chosen offset.
		const double Grow = std::max(0.0, Params.OutlineOffsetCm) * T + std::max(Tolerance, 0.0f);

		std::vector<int> CellOfRegion(static_cast<size_t>(R.Num), -1);
		for (int Id = 0; Id < R.Num; ++Id)
		{
			if (R.TouchesBorder[static_cast<size_t>(Id)] || R.Area[static_cast<size_t>(Id)] < MinArea) continue;
			FCellReading Cell;
			Cell.AreaCm2 = static_cast<double>(R.Area[static_cast<size_t>(Id)]) / T2;
			Cell.CenterXCm = R.SumX[static_cast<size_t>(Id)] / static_cast<double>(R.Area[static_cast<size_t>(Id)]) / T;
			Cell.CenterYCm = R.SumY[static_cast<size_t>(Id)] / static_cast<double>(R.Area[static_cast<size_t>(Id)]) / T;
			{
				const double A = static_cast<double>(R.Area[static_cast<size_t>(Id)]);
				const double MX = R.SumX[static_cast<size_t>(Id)] / A, MY = R.SumY[static_cast<size_t>(Id)] / A;
				const double VXX = R.SumXX[static_cast<size_t>(Id)] / A - MX * MX;
				const double VYY = R.SumYY[static_cast<size_t>(Id)] / A - MY * MY;
				const double VXY = R.SumXY[static_cast<size_t>(Id)] / A - MX * MY;
				const double Tr = VXX + VYY, Disc = std::sqrt(std::max(0.0, Tr * Tr / 4.0 - (VXX * VYY - VXY * VXY)));
				Cell.Aspect = std::sqrt((Tr / 2.0 + Disc) / std::max(1e-9, Tr / 2.0 - Disc));
			}
			int MW = 0, MH = 0;
			const std::vector<char> Mask = RegionMask(R, Id, Grow, MW, MH);
			Cell.CurvedShare = CurvedShareOfOutline(TraceOutline(Mask, MW, MH), Params.CornerDeg, Params.WallBow);
			Cell.FilterShare = Cell.Aspect >= Params.BandAspect ? 0.0 : LinearStep(Params.StraightMax, Params.CurvedMin, Cell.CurvedShare);

			const double Power = std::min(Cell.AreaCm2, Params.MaxCellCm2);
			Reading.FilterPowerCm2 += Power * Cell.FilterShare;
			Reading.DeflectPowerCm2 += Power * (1.0 - Cell.FilterShare);
			if (Cell.FilterShare >= 0.75) ++Reading.FilterCells;
			else if (Cell.FilterShare <= 0.25) ++Reading.DeflectCells;
			else ++Reading.MixedCells;

			CellOfRegion[static_cast<size_t>(Id)] = static_cast<int>(Reading.Cells.size());
			Reading.Cells.push_back(Cell);
		}

		if (OutFilterMask || OutDeflectMask)
		{
			for (size_t I = 0; I < Count; ++I)
			{
				const int Id = R.Label[I];
				if (Id < 0) continue;
				const int C = CellOfRegion[static_cast<size_t>(Id)];
				if (C < 0) continue;
				const double Share = Reading.Cells[static_cast<size_t>(C)].FilterShare;
				if (OutFilterMask) (*OutFilterMask)[I] = static_cast<uint8_t>(std::lround(255.0 * Share));
				if (OutDeflectMask) (*OutDeflectMask)[I] = static_cast<uint8_t>(std::lround(255.0 * (1.0 - Share)));
			}
		}
		return Reading;
	}
}
