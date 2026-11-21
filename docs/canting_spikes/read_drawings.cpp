// Runs the ward reading (CantingStructure) on drawings recorded in the editor with Canting.SaveStrokes,
// exactly as UCantingCanvasComponent does, and writes a picture of each reading.
//
// Build (from docs/canting_spikes):
//   g++ -O2 -std=c++17 -I../../CantingCloth/Source/CantingCloth read_drawings.cpp
//       ../../CantingCloth/Source/CantingCloth/CantingWaxGrid.cpp ../../CantingCloth/Source/CantingCloth/CantingStructure.cpp
//       -o read_drawings
// Run:   ./read_drawings [drawings folder] [gap tolerance cm] [output folder for .ppm pictures]
//
// Pictures: wax dark brown, filter cells green, deflect cells orange, in-between cells a mix.

#include "CantingStructure.h"
#include "CantingWaxGrid.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace CantingCore;

struct Drawing
{
	std::string Name;
	double PanelW = 100.0, PanelH = 100.0;
	std::vector<std::vector<double>> Strokes;   // cm, interleaved
	std::vector<double> Radius;
};

static bool Load(const std::filesystem::path& Path, Drawing& Out)
{
	std::ifstream In(Path);
	if (!In) return false;
	Out.Name = Path.stem().string();
	std::string Line;
	while (std::getline(In, Line))
	{
		std::istringstream S(Line);
		std::string Tag;
		S >> Tag;
		if (Tag == "panel") S >> Out.PanelW >> Out.PanelH;
		else if (Tag == "stroke")
		{
			double R = 1.2, X = 0, Y = 0;
			S >> R;
			std::vector<double> XY;
			while (S >> X >> Y) { XY.push_back(X); XY.push_back(Y); }
			if (!XY.empty()) { Out.Strokes.push_back(XY); Out.Radius.push_back(R); }
		}
	}
	return true;
}

int main(int argc, char** argv)
{
	const std::string Dir = argc > 1 ? argv[1] : "../../CantingCloth/CantingDrawings";
	const double Gap = argc > 2 ? std::atof(argv[2]) : 1.0;
	const std::string OutDir = argc > 3 ? argv[3] : "";
	const double Offset = argc > 4 ? std::atof(argv[4]) : 0.0;
	const int Resolution = 512;

	std::vector<std::filesystem::path> Files;
	for (const auto& E : std::filesystem::directory_iterator(Dir))
		if (E.path().extension() == ".txt") Files.push_back(E.path());
	std::sort(Files.begin(), Files.end());

	std::printf("gap tolerance %.2f cm, outline offset %.2f cm, %d px across the panel\n", Gap, Offset, Resolution);
	std::printf("%-20s %5s %6s | %5s %7s %7s | %7s %7s | cells (area cm2 : curved share)\n",
		"drawing", "strk", "wax%", "cells", "filter", "deflect", "F power", "D power");
	for (const auto& Path : Files)
	{
		Drawing D;
		if (!Load(Path, D)) continue;
		const double TexelsPerCm = Resolution / D.PanelW;
		FWaxGrid Grid;
		Grid.Init(Resolution, static_cast<int>(std::lround(D.PanelH * TexelsPerCm)));
		for (size_t S = 0; S < D.Strokes.size(); ++S)
			Grid.StampPolyline(D.Strokes[S].data(), static_cast<int>(D.Strokes[S].size() / 2), TexelsPerCm, static_cast<float>(D.Radius[S] * TexelsPerCm));

		FWardParams P;
		P.TexelsPerCm = TexelsPerCm;
		P.BrushRadiusCm = D.Radius.empty() ? 1.2 : D.Radius[0];
		P.GapToleranceCm = Gap;
		P.OutlineOffsetCm = Offset;
		std::vector<uint8_t> FilterMask, DeflectMask;
		const FWardReading W = ReadWards(Grid, P, &FilterMask, &DeflectMask);

		long long Wax = 0;
		for (uint8_t V : Grid.GetData()) Wax += V < FWaxGrid::Encode(0.0f) ? 1 : 0;
		const double WaxShare = static_cast<double>(Wax) / static_cast<double>(Grid.GetData().size());

		std::vector<FCellReading> Cells = W.Cells;
		std::sort(Cells.begin(), Cells.end(), [](const FCellReading& A, const FCellReading& B) { return A.AreaCm2 > B.AreaCm2; });
		std::string Detail;
		for (const FCellReading& C : Cells)
		{
			char Buf[48];
			std::snprintf(Buf, sizeof Buf, " %.0f:%.2f%s", C.AreaCm2, C.CurvedShare, C.FilterShare == 0.0 && C.Aspect >= P.BandAspect ? "b" : "");
			Detail += Buf;
		}
		std::printf("%-20s %5zu %5.1f%% | %5zu %7d %7d | %7.0f %7.0f |%s\n", D.Name.c_str(), D.Strokes.size(), 100.0 * WaxShare,
			W.Cells.size(), W.FilterCells, W.DeflectCells, W.FilterPowerCm2, W.DeflectPowerCm2, Detail.c_str());

		if (!OutDir.empty())
		{
			const int GW = Grid.GetWidth(), GH = Grid.GetHeight();
			std::vector<unsigned char> Img(static_cast<size_t>(GW) * GH * 3);
			for (size_t I = 0; I < static_cast<size_t>(GW) * GH; ++I)
			{
				const double F = FilterMask[I] / 255.0, Df = DeflectMask[I] / 255.0;
				double R = 245, G = 238, B = 220;
				if (F + Df > 0) { R = 170 * F + 245 * Df; G = 220 * F + 185 * Df; B = 170 * F + 110 * Df; }
				const double Cov = std::min(std::max(0.5 - static_cast<double>(FWaxGrid::Decode(Grid.GetData()[I])), 0.0), 1.0);
				R += (70 - R) * Cov; G += (45 - G) * Cov; B += (30 - B) * Cov;
				Img[I * 3] = static_cast<unsigned char>(R); Img[I * 3 + 1] = static_cast<unsigned char>(G); Img[I * 3 + 2] = static_cast<unsigned char>(B);
			}
			std::ofstream O(OutDir + "/" + D.Name + ".ppm", std::ios::binary);
			O << "P6\n" << GW << " " << GH << "\n255\n";
			O.write(reinterpret_cast<const char*>(Img.data()), static_cast<std::streamsize>(Img.size()));
		}
	}
	return 0;
}
