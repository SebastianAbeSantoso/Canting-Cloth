// shape_spike2.cpp: second feasibility spike for the canting plan, testing reviewer feedback.
//  Part 1: shape families. Compares
//    v1  = ellipse deviation ("angularity") of the cell's inner edge,
//    v2  = curved share of the wall centre line (cell dilated by the brush radius before tracing):
//          find corners, then count a wall between two corners as curved if it bows away from its
//          chord by more than gWallRatio x chord. Results in docs/Canting_System_Design.md section 10.
//  Part 2: isen. Compares raster islands (v1) with isen-stroke wax area inside the cell (v2),
//          for dots grazing the wall, merging dots, and repeated clicks.
// Optional args: cornerDeg wallRatio curvedMin straightMax [tremorCm wobble [brushCm]]
// Build: g++ -O2 -std=c++17 -I../../CantingCloth/Source/CantingCloth shape_spike2.cpp
//        ../../CantingCloth/Source/CantingCloth/CantingWaxGrid.cpp -o shape_spike2 && ./shape_spike2

#include "CantingWaxGrid.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace CantingCore;

namespace
{
	const double kPi = 3.14159265358979;
	const double kTex = 10.0;          // texels per cm
	double kBrushCm = 0.6;
	const double kDotCm = 0.45;
	const double kMinCellCm2 = 4.0;

	// Tunables for v2 (printed in the header).
	double gCurvedMin = 0.50;          // curved share >= this: curved (filter)
	double gStraightMax = 0.25;        // curvedness <= this: straight (deflect)
	double gCornerDeg = 38.0;          // windowed turning above this (degrees) marks a corner
	double gWallRatio = 0.10;          // wall is curved if max deviation from its chord > this x chord
	int gLastCorners = 0;
	double gTremorCm = 0.12;           // hand tremor per 0.5 cm sample
	double gWobble = 0.03;             // slow wobble, share of shape size

	struct P2 { double x, y; };

	struct Stroke
	{
		std::vector<double> xy;        // cm, interleaved
		double radiusCm = kBrushCm;
		bool isen = false;
	};

	double wrapRad(double a) { while (a > kPi) a -= 2 * kPi; while (a < -kPi) a += 2 * kPi; return a; }

	// ---------------- drawing ----------------

	// Resamples a path to 0.5 cm and adds hand tremor (0.12 cm) and a slow wobble (3% of size) along the path normal.
	Stroke handStroke(const std::vector<P2>& path, double sizeCm, P2 at, double rot, bool closed, std::mt19937& rng)
	{
		std::normal_distribution<double> tremor(0.0, std::max(1e-9, gTremorCm));
		std::uniform_real_distribution<double> phase(0, 2 * kPi);
		const double wobA = gWobble * sizeCm, wobF = 2 + std::uniform_int_distribution<int>(0, 2)(rng), wobP = phase(rng);
		Stroke s;
		const double c = std::cos(rot), sn = std::sin(rot);
		auto emit = [&](P2 p, P2 tangent, double arc)
		{
			const double tl = std::max(1e-9, std::hypot(tangent.x, tangent.y));
			const double nx = -tangent.y / tl, ny = tangent.x / tl;
			const double w = wobA * std::sin(wobF * arc / sizeCm + wobP);
			const double x = p.x + nx * w + tremor(rng), y = p.y + ny * w + tremor(rng);
			s.xy.push_back(at.x + x * c - y * sn);
			s.xy.push_back(at.y + x * sn + y * c);
		};
		double arc = 0, sinceLast = 0;
		emit(path[0], {path[1].x - path[0].x, path[1].y - path[0].y}, 0);
		for (size_t i = 1; i < path.size(); ++i)
		{
			const double d = std::hypot(path[i].x - path[i - 1].x, path[i].y - path[i - 1].y);
			arc += d; sinceLast += d;
			if (sinceLast >= 0.5 || i + 1 == path.size())
			{
				emit(path[i], {path[i].x - path[i - 1].x, path[i].y - path[i - 1].y}, arc);
				sinceLast = 0;
			}
		}
		if (closed && s.xy.size() >= 4)   // overshoot past the start, as a real closing stroke does
		{
			s.xy.push_back(s.xy[2]);
			s.xy.push_back(s.xy[3]);
		}
		return s;
	}

	std::vector<P2> ellipsePath(double a, double b)
	{
		std::vector<P2> p;
		for (int i = 0; i <= 600; ++i) { const double t = 2 * kPi * i / 600; p.push_back({a * std::cos(t), b * std::sin(t)}); }
		return p;
	}

	// Closed polygon through the vertices, corners slightly rounded by hand (chamfer of `cut` cm).
	std::vector<P2> polygonPath(const std::vector<P2>& v, double cut)
	{
		std::vector<P2> p;
		const size_t n = v.size();
		for (size_t i = 0; i < n; ++i)
		{
			const P2 a = v[i], b = v[(i + 1) % n];
			const double L = std::hypot(b.x - a.x, b.y - a.y);
			const int steps = std::max(4, int(L / 0.05));
			for (int k = 0; k <= steps; ++k)
			{
				const double t = double(k) / steps;
				if (t * L < cut || (1 - t) * L < cut) continue;
				p.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t});
			}
		}
		p.push_back(p[0]);
		return p;
	}

	std::vector<P2> regularPolygon(int n, double R, double phase)
	{
		std::vector<P2> v;
		for (int i = 0; i < n; ++i) v.push_back({R * std::cos(phase + 2 * kPi * i / n), R * std::sin(phase + 2 * kPi * i / n)});
		return v;
	}

	// Lens (vesica-like): two circular arcs between tips (+-halfLen, 0), widest at (0, +-halfWidth).
	std::vector<P2> lensPath(double halfLen, double halfWidth)
	{
		const double yc = (halfWidth * halfWidth - halfLen * halfLen) / (2 * halfWidth);   // centre of the upper arc's circle
		const double R = halfWidth - yc;
		const double a0 = std::atan2(0 - yc, halfLen), a1 = std::atan2(0 - yc, -halfLen);  // right tip, left tip
		std::vector<P2> p;
		const int n = 300;
		for (int i = 0; i <= n; ++i) { const double a = a0 + (a1 - a0) * i / n; p.push_back({R * std::cos(a), yc + R * std::sin(a)}); }   // upper arc, right to left
		for (int i = 1; i <= n; ++i) { const double a = a1 + (a0 - a1) * i / n; p.push_back({R * std::cos(a), -(yc + R * std::sin(a))}); } // lower arc, left to right
		return p;
	}

	// ---------------- analysis ----------------

	struct Regions
	{
		int W = 0, H = 0, num = 0;
		std::vector<int> label;              // >= 0 background region, -1 wax
		std::vector<char> border;
		std::vector<int> area;
		std::vector<int> x0, y0, x1, y1;      // bounding boxes
	};

	Regions labelRegions(const FWaxGrid& g)
	{
		Regions L; L.W = g.GetWidth(); L.H = g.GetHeight();
		const size_t N = size_t(L.W) * L.H;
		const uint8_t thr = FWaxGrid::Encode(0.0f);
		const auto& px = g.GetData();
		L.label.assign(N, -2);
		for (size_t i = 0; i < N; ++i) if (px[i] < thr) L.label[i] = -1;
		std::vector<int> st;
		for (size_t s = 0; s < N; ++s)
		{
			if (L.label[s] != -2) continue;
			const int id = L.num++;
			bool border = false; int area = 0, bx0 = L.W, by0 = L.H, bx1 = -1, by1 = -1;
			L.label[s] = id; st.assign(1, int(s));
			while (!st.empty())
			{
				const int c = st.back(); st.pop_back(); ++area;
				const int x = c % L.W, y = c / L.W;
				bx0 = std::min(bx0, x); by0 = std::min(by0, y); bx1 = std::max(bx1, x); by1 = std::max(by1, y);
				if (x == 0 || y == 0 || x == L.W - 1 || y == L.H - 1) border = true;
				const int nb[4] = {x > 0 ? c - 1 : -1, x < L.W - 1 ? c + 1 : -1, y > 0 ? c - L.W : -1, y < L.H - 1 ? c + L.W : -1};
				for (int j : nb) if (j >= 0 && L.label[j] == -2) { L.label[j] = id; st.push_back(j); }
			}
			L.border.push_back(border); L.area.push_back(area);
			L.x0.push_back(bx0); L.y0.push_back(by0); L.x1.push_back(bx1); L.y1.push_back(by1);
		}
		return L;
	}

	// Moore-neighbour trace of the outer boundary of the blob in a local mask (Jacob's stopping criterion).
	std::vector<P2> traceMask(const std::vector<char>& m, int w, int h)
	{
		int start = -1;
		for (int i = 0; i < w * h && start < 0; ++i) if (m[i]) start = i;
		std::vector<P2> out;
		if (start < 0) return out;
		auto in = [&](int x, int y) { return x >= 0 && y >= 0 && x < w && y < h && m[size_t(y) * w + x]; };
		const int dx8[8] = {1, 1, 0, -1, -1, -1, 0, 1}, dy8[8] = {0, 1, 1, 1, 0, -1, -1, -1};
		const int sx = start % w, sy = start / w;
		int x = sx, y = sy, dir = 7, firstDir = -1;
		for (int guard = 0; guard < 8 * w * h; ++guard)
		{
			int d = (dir + 6) % 8, k = 0;
			for (; k < 8; ++k, d = (d + 1) % 8) if (in(x + dx8[d], y + dy8[d])) break;
			if (k == 8) { out.push_back({x + 0.5, y + 0.5}); break; }
			if (firstDir < 0) firstDir = d;
			else if (x == sx && y == sy && d == firstDir) break;
			out.push_back({x + 0.5, y + 0.5});
			x += dx8[d]; y += dy8[d]; dir = d;
		}
		return out;
	}

	// Local mask of region `id`, optionally dilated by r texels. Origin of the box in (ox, oy).
	std::vector<char> regionMask(const Regions& L, int id, double r, int& ox, int& oy, int& w, int& h)
	{
		const int pad = int(std::ceil(r)) + 2;
		ox = L.x0[id] - pad; oy = L.y0[id] - pad;
		w = L.x1[id] - L.x0[id] + 1 + 2 * pad; h = L.y1[id] - L.y0[id] + 1 + 2 * pad;
		std::vector<char> m(size_t(w) * h, 0);
		std::vector<std::pair<int, int>> disk;
		const int R = int(std::ceil(r));
		for (int dy = -R; dy <= R; ++dy) for (int dx = -R; dx <= R; ++dx) if (dx * dx + dy * dy <= r * r) disk.push_back({dx, dy});
		auto isId = [&](int x, int y) { return x >= 0 && y >= 0 && x < L.W && y < L.H && L.label[size_t(y) * L.W + x] == id; };
		for (int y = L.y0[id]; y <= L.y1[id]; ++y)
			for (int x = L.x0[id]; x <= L.x1[id]; ++x)
			{
				if (!isId(x, y)) continue;
				m[size_t(y - oy) * w + (x - ox)] = 1;
				if (r <= 0) continue;
				const bool edge = !isId(x - 1, y) || !isId(x + 1, y) || !isId(x, y - 1) || !isId(x, y + 1);
				if (!edge) continue;
				for (const auto& [dx, dy] : disk) m[size_t(y - oy + dy) * w + (x - ox + dx)] = 1;
			}
		return m;
	}

	// v1: RMS radial deviation of a contour from its moment-matched ellipse (0 = perfect ellipse).
	double angularity(const std::vector<P2>& raw)
	{
		const int n = int(raw.size());
		if (n < 8) return 1;
		double A2 = 0, Sx = 0, Sy = 0, Sxx = 0, Syy = 0, Sxy = 0;
		for (int i = 0; i < n; ++i)
		{
			const P2 a = raw[i], b = raw[(i + 1) % n];
			const double cr = a.x * b.y - b.x * a.y;
			A2 += cr; Sx += (a.x + b.x) * cr; Sy += (a.y + b.y) * cr;
			Sxx += (a.x * a.x + a.x * b.x + b.x * b.x) * cr;
			Syy += (a.y * a.y + a.y * b.y + b.y * b.y) * cr;
			Sxy += (a.x * b.y + 2 * a.x * a.y + 2 * b.x * b.y + b.x * a.y) * cr;
		}
		const double cx = Sx / (3 * A2), cy = Sy / (3 * A2);
		const double mxx = Sxx / (6 * A2) - cx * cx, myy = Syy / (6 * A2) - cy * cy, mxy = Sxy / (12 * A2) - cx * cy;
		const double det = mxx * myy - mxy * mxy;
		if (det <= 1e-12) return 1;
		const double ixx = myy / det, iyy = mxx / det, ixy = -mxy / det;
		double sq = 0;
		for (int i = 0; i < n; ++i)
		{
			const double dx = raw[i].x - cx, dy = raw[i].y - cy;
			const double rho = 0.5 * std::sqrt(std::max(0.0, dx * dx * ixx + 2 * dx * dy * ixy + dy * dy * iyy));
			sq += (rho - 1) * (rho - 1);
		}
		return std::sqrt(sq / n);
	}

	// v2: share of the wall length that is curved. The contour is split at corners (peaks of turning, as in spike v1),
	// then each wall between two corners is curved if its largest deviation from its chord exceeds gWallRatio x chord.
	// A contour without corners is one curved wall.
	double curvedness(const std::vector<P2>& raw)
	{
		const int n = int(raw.size());
		if (n < 8) return 0;
		std::vector<double> cum(n + 1, 0);
		for (int i = 0; i < n; ++i) cum[i + 1] = cum[i] + std::hypot(raw[(i + 1) % n].x - raw[i].x, raw[(i + 1) % n].y - raw[i].y);
		const double L = cum[n];
		const int M = 200;
		std::vector<P2> u(M);
		for (int j = 0, i = 0; j < M; ++j)
		{
			const double t = L * j / M;
			while (i < n - 1 && cum[i + 1] < t) ++i;
			const double f = (t - cum[i]) / std::max(1e-9, cum[i + 1] - cum[i]);
			const P2 a = raw[i], b = raw[(i + 1) % n];
			u[j] = {a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f};
		}
		std::vector<P2> sm(M);
		for (int j = 0; j < M; ++j)
		{
			double sx = 0, sy = 0, sw = 0;
			for (int k = -6; k <= 6; ++k) { const double w = std::exp(-k * k / 8.0); sx += w * u[(j + k + M) % M].x; sy += w * u[(j + k + M) % M].y; sw += w; }
			sm[j] = {sx / sw, sy / sw};
		}
		std::vector<double> tp(M), win(M);
		for (int j = 0; j < M; ++j)
		{
			const P2 a = sm[(j - 1 + M) % M], b = sm[j], d = sm[(j + 1) % M];
			tp[j] = wrapRad(std::atan2(d.y - b.y, d.x - b.x) - std::atan2(b.y - a.y, b.x - a.x)) * 180 / kPi;
		}
		for (int j = 0; j < M; ++j) { double t = 0; for (int k = -3; k <= 3; ++k) t += tp[(j + k + M) % M]; win[j] = std::fabs(t); }
		std::vector<int> corners;
		for (int j = 0; j < M; ++j)
		{
			if (win[j] < gCornerDeg) continue;
			bool isMax = true;
			for (int k = -8; k <= 8 && isMax; ++k)
			{
				if (k == 0) continue;
				const double o = win[(j + k + M) % M];
				if (o > win[j] || (o == win[j] && k < 0)) isMax = false;
			}
			if (isMax) corners.push_back(j);
		}
		gLastCorners = int(corners.size());
		if (corners.size() < 2) return 1.0;   // no corners (or one): a single curved wall
		double curvedLen = 0, totalLen = 0;
		for (size_t c = 0; c < corners.size(); ++c)
		{
			const int a = corners[c], b = corners[(c + 1) % corners.size()];
			const int len = (b - a + M) % M;
			const P2 A = sm[a], B = sm[b];
			const double chord = std::hypot(B.x - A.x, B.y - A.y);
			double dev = 0;
			for (int k = 1; k < len; ++k)
			{
				const P2 q = sm[(a + k) % M];
				const double cross = std::fabs((B.x - A.x) * (q.y - A.y) - (B.y - A.y) * (q.x - A.x)) / std::max(1e-9, chord);
				dev = std::max(dev, cross);
			}
			totalLen += len;
			if (dev > gWallRatio * chord) curvedLen += len;
		}
		return curvedLen / std::max(1.0, totalLen);
	}

	struct Cell
	{
		int id = 0;
		double areaCm2 = 0;
		double angInner = 0;      // v1 measure
		double curvMid = 0;       // v2 measure
		int islands = 0;          // v1 isen: wax pieces touching only this cell (on the full grid)
		double isenCm2 = 0;       // v2 isen: isen-stroke wax area inside the cell (cells from outline strokes only)
	};

	std::string familyV1(double ang) { return ang <= 0.045 ? "round" : (ang >= 0.075 ? "pointed" : "mixed"); }
	std::string familyV2(double cv) { return cv >= gCurvedMin ? "curved" : (cv <= gStraightMax ? "straight" : "mixed"); }

	FWaxGrid raster(const std::vector<Stroke>& strokes, int W, int H, int which /*0 all, 1 outline only, 2 isen only*/)
	{
		FWaxGrid g; g.Init(W, H);
		for (const Stroke& s : strokes)
		{
			if ((which == 1 && s.isen) || (which == 2 && !s.isen)) continue;
			g.StampPolyline(s.xy.data(), int(s.xy.size() / 2), kTex, float(s.radiusCm * kTex));
		}
		return g;
	}

	// Analyses a drawing. Cells come from outline strokes only (isen never form walls) unless `isenAsWalls`.
	std::vector<Cell> analyse(const std::vector<Stroke>& strokes, int W, int H, bool isenAsWalls = false)
	{
		const FWaxGrid all = raster(strokes, W, H, 0);
		const FWaxGrid outline = isenAsWalls ? all : raster(strokes, W, H, 1);
		const FWaxGrid isen = raster(strokes, W, H, 2);
		const Regions L = labelRegions(outline);
		const Regions LA = labelRegions(all);
		const uint8_t thr = FWaxGrid::Encode(0.0f);

		// v1 islands on the full grid: 8-connected wax pieces adjacent to exactly one background region.
		std::map<int, int> islandsByRegionAll;
		{
			const size_t N = size_t(W) * H;
			std::vector<int> comp(N, -1);
			std::vector<int> st;
			for (size_t s = 0; s < N; ++s)
			{
				if (LA.label[s] != -1 || comp[s] >= 0) continue;
				const int id = int(s);
				std::vector<int> adj;
				comp[s] = id; st.assign(1, int(s));
				while (!st.empty())
				{
					const int c = st.back(); st.pop_back();
					const int x = c % W, y = c / W;
					for (int dy = -1; dy <= 1; ++dy)
						for (int dx = -1; dx <= 1; ++dx)
						{
							const int nx = x + dx, ny = y + dy;
							if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
							const int j = ny * W + nx;
							const int lab = LA.label[j];
							if (lab == -1 && comp[j] < 0) { comp[j] = id; st.push_back(j); }
							else if (lab >= 0 && std::abs(dx) + std::abs(dy) == 1 && std::find(adj.begin(), adj.end(), lab) == adj.end()) adj.push_back(lab);
						}
				}
				if (adj.size() == 1) ++islandsByRegionAll[adj[0]];
			}
		}

		std::vector<Cell> cells;
		const int minArea = int(kMinCellCm2 * kTex * kTex);
		for (int r = 0; r < L.num; ++r)
		{
			if (L.border[r] || L.area[r] < minArea) continue;
			Cell c; c.id = r; c.areaCm2 = L.area[r] / (kTex * kTex);
			int ox, oy, w, h;
			const std::vector<char> inner = regionMask(L, r, 0.0, ox, oy, w, h);
			c.angInner = angularity(traceMask(inner, w, h));
			const std::vector<char> mid = regionMask(L, r, kBrushCm * kTex, ox, oy, w, h);
			c.curvMid = curvedness(traceMask(mid, w, h));
			// v2 isen: isen wax inside this cell
			long long isenPx = 0;
			for (int y = L.y0[r]; y <= L.y1[r]; ++y)
				for (int x = L.x0[r]; x <= L.x1[r]; ++x)
				{
					const size_t i = size_t(y) * W + x;
					if (L.label[i] == r && isen.GetData()[i] < thr) ++isenPx;
				}
			c.isenCm2 = isenPx / (kTex * kTex);
			// v1 islands: the full-grid region that contains this cell's first pixel
			for (int y = L.y0[r], found = 0; y <= L.y1[r] && !found; ++y)
				for (int x = L.x0[r]; x <= L.x1[r]; ++x)
				{
					const size_t i = size_t(y) * W + x;
					if (L.label[i] == r && LA.label[i] >= 0) { c.islands = islandsByRegionAll[LA.label[i]]; found = 1; break; }
				}
			cells.push_back(c);
		}
		return cells;
	}

	Stroke dot(double x, double y) { Stroke s; s.xy = {x, y}; s.radiusCm = kDotCm; s.isen = true; return s; }
}

int main(int argc, char** argv)
{
	if (argc >= 5) { gCornerDeg = atof(argv[1]); gWallRatio = atof(argv[2]); gCurvedMin = atof(argv[3]); gStraightMax = atof(argv[4]); }
	if (argc >= 7) { gTremorCm = atof(argv[5]); gWobble = atof(argv[6]); }
	if (argc >= 8) { kBrushCm = atof(argv[7]); }
	std::mt19937 rng(2026);
	std::uniform_real_distribution<double> rot(0, 2 * kPi);
	const int trials = 25;
	const std::vector<double> sizes = {3.0, 5.0, 8.0};   // cm, half of the shape's longest extent

	std::printf("Settings: corner %.0f deg, curved wall if deviation > %.2f x chord, curved >= %.2f, straight <= %.2f, brush %.1f cm, tremor %.2f cm, wobble %.2f\n",
		gCornerDeg, gWallRatio, gCurvedMin, gStraightMax, kBrushCm, gTremorCm, gWobble);

	// ---------------- Part 1: single cells ----------------
	struct Kind { std::string name, want; };
	const std::vector<Kind> kinds = {
		{"circle", "curved"}, {"ellipse 1.6:1", "curved"}, {"ellipse 2.2:1", "curved"}, {"ellipse 3:1", "curved"},
		{"lens 2:1", "curved"}, {"lens 1.4:1", "curved"},
		{"triangle", "straight"}, {"square", "straight"}, {"rhombus 1.6:1", "straight"}, {"pentagon", "straight"},
		{"hexagon", "straight"}, {"band 4:1", "straight"}};
	std::printf("\n1. Single closed cells, %d hand-drawn trials per size (sizes = half-length 3 / 5 / 8 cm)\n", trials);
	std::printf("   %-14s %-9s | v2 correct by size | v2 curvedness mean (min..max) | v1 (inner edge) reads\n", "shape", "want");
	int ok2 = 0, ok1 = 0, total = 0;
	for (const Kind& k : kinds)
	{
		int bySize[3] = {0, 0, 0};
		double cmin = 9, cmax = -9, csum = 0; int cn = 0;
		std::map<std::string, int> v1reads;
		for (int si = 0; si < 3; ++si)
		{
			const double s = sizes[si];
			for (int t = 0; t < trials; ++t)
			{
				std::vector<P2> path;
				if (k.name == "circle") path = ellipsePath(s, s);
				else if (k.name == "ellipse 1.6:1") path = ellipsePath(s, s / 1.6);
				else if (k.name == "ellipse 2.2:1") path = ellipsePath(s, s / 2.2);
				else if (k.name == "ellipse 3:1") path = ellipsePath(s, s / 3.0);
				else if (k.name == "lens 2:1") path = lensPath(s, s / 2.0);
				else if (k.name == "lens 1.4:1") path = lensPath(s, s / 1.4);
				else if (k.name == "triangle") path = polygonPath(regularPolygon(3, s, 0), std::min(0.4, 0.08 * s));
				else if (k.name == "square") path = polygonPath(regularPolygon(4, s, kPi / 4), std::min(0.4, 0.08 * s));
				else if (k.name == "rhombus 1.6:1") path = polygonPath({{s, 0}, {0, s / 1.6}, {-s, 0}, {0, -s / 1.6}}, std::min(0.4, 0.08 * s));
				else if (k.name == "pentagon") path = polygonPath(regularPolygon(5, s, 0), std::min(0.4, 0.08 * s));
				else if (k.name == "hexagon") path = polygonPath(regularPolygon(6, s, 0), std::min(0.4, 0.08 * s));
				else if (k.name == "band 4:1") path = polygonPath({{s, s / 4}, {-s, s / 4}, {-s, -s / 4}, {s, -s / 4}}, std::min(0.4, 0.08 * s));
				const std::vector<Stroke> strokes = {handStroke(path, s, {15, 15}, rot(rng), true, rng)};
				const std::vector<Cell> cells = analyse(strokes, 300, 300);
				if (cells.size() != 1) { v1reads["(no single cell)"]++; continue; }
				const Cell& c = cells[0];
				const bool good = familyV2(c.curvMid) == k.want;
				bySize[si] += good; ok2 += good;
				const std::string r1 = familyV1(c.angInner);
				v1reads[r1]++;
				ok1 += (k.want == "curved" && r1 == "round") || (k.want == "straight" && r1 == "pointed");
				cmin = std::min(cmin, c.curvMid); cmax = std::max(cmax, c.curvMid); csum += c.curvMid; ++cn;
			}
		}
		total += 3 * trials;
		std::string v1s;
		for (const auto& [name, count] : v1reads) v1s += name + " " + std::to_string(count) + "  ";
		std::printf("   %-14s %-9s |   %2d / %2d / %2d     | %.2f (%.2f..%.2f)             | %s\n",
			k.name.c_str(), k.want.c_str(), bySize[0], bySize[1], bySize[2], cn ? csum / cn : 0, cn ? cmin : 0, cn ? cmax : 0, v1s.c_str());
	}
	std::printf("   v2 correct %d / %d (%.1f%%); v1 correct %d / %d (%.1f%%; 'mixed' counts as wrong)\n",
		ok2, total, 100.0 * ok2 / total, ok1, total, 100.0 * ok1 / total);

	// ---------------- Part 1b: patterns ----------------
	std::printf("\n1b. Patterns (10 drawings each). Cells found, and how each reads (v2 | v1)\n");
	auto report = [](const char* name, const std::vector<std::vector<Cell>>& runs)
	{
		std::map<std::string, int> r2, r1; int n = 0; double asum = 0;
		for (const auto& cells : runs) for (const Cell& c : cells) { r2[familyV2(c.curvMid)]++; r1[familyV1(c.angInner)]++; ++n; asum += c.areaCm2; }
		std::string s2, s1;
		for (const auto& [k, v] : r2) s2 += k + " " + std::to_string(v) + " ";
		for (const auto& [k, v] : r1) s1 += k + " " + std::to_string(v) + " ";
		std::printf("   %-34s %4.1f cells/drawing, mean %5.1f cm2 | v2: %-30s | v1: %s\n", name, double(n) / runs.size(), n ? asum / n : 0, s2.c_str(), s1.c_str());
	};
	for (double rho : {2.0, 3.0})   // Gringsing-like fish scales: rows of semicircle arcs, alternate rows offset
	{
		std::vector<std::vector<Cell>> runs;
		for (int t = 0; t < 10; ++t)
		{
			std::vector<Stroke> strokes;
			const int rows = 5, cols = 5;
			for (int r = 0; r < rows; ++r)
			{
				std::vector<P2> path;
				const double y0 = r * rho, off = (r % 2) * rho;
				for (int c = 0; c < cols; ++c)
				{
					const double cx = off + 2 * rho * c;
					for (int i = (c == 0 ? 0 : 1); i <= 60; ++i) { const double a = kPi - kPi * i / 60; path.push_back({cx + rho * std::cos(a), y0 + rho * std::sin(a)}); }
				}
				strokes.push_back(handStroke(path, 2 * rho, {8, 8}, 0.0, false, rng));
			}
			runs.push_back(analyse(strokes, 400, 400));
		}
		char name[64]; std::snprintf(name, sizeof name, "Gringsing scales, arc radius %.0f cm", rho);
		report(name, runs);
	}
	{   // Kawung: four ovals (2:1) around a centre, long axes pointing at it
		std::vector<std::vector<Cell>> runs;
		for (int t = 0; t < 10; ++t)
		{
			std::vector<Stroke> strokes;
			const double a = 3.0, b = 1.5;
			for (int q = 0; q < 4; ++q)
			{
				const double ang = q * kPi / 2;
				strokes.push_back(handStroke(ellipsePath(a, b), a, {20 + (a + 0.3) * std::cos(ang), 20 + (a + 0.3) * std::sin(ang)}, ang, true, rng));
			}
			runs.push_back(analyse(strokes, 400, 400));
		}
		report("Kawung group, 4 ovals 6 x 3 cm", runs);
	}
	{   // Tambal: patchwork of straight lines (grid plus diagonals)
		std::vector<std::vector<Cell>> runs;
		std::uniform_real_distribution<double> jit(-0.6, 0.6);
		for (int t = 0; t < 10; ++t)
		{
			std::vector<Stroke> strokes;
			for (int i = 0; i <= 4; ++i)
			{
				const double p = 5 + 6 * i + jit(rng);
				strokes.push_back(handStroke({{p, 4}, {p + jit(rng), 30}}, 6, {0, 0}, 0, false, rng));
				strokes.push_back(handStroke({{4, p}, {30, p + jit(rng)}}, 6, {0, 0}, 0, false, rng));
			}
			strokes.push_back(handStroke({{5, 5}, {29, 29}}, 6, {0, 0}, 0, false, rng));
			strokes.push_back(handStroke({{5, 29}, {29, 5}}, 6, {0, 0}, 0, false, rng));
			runs.push_back(analyse(strokes, 350, 350));
		}
		report("Tambal patchwork (grid + diagonals)", runs);
	}

	// ---------------- Part 2: isen ----------------
	std::printf("\n2. Isen inside a circle (drawn radius 4 cm). One dot covers %.2f cm2\n", kPi * kDotCm * kDotCm);
	std::printf("   %-34s | v1 islands | v2 isen area (dots) | v2 curvedness: isen excluded / isen as walls\n", "scene");
	const double dotArea = kPi * kDotCm * kDotCm;
	struct Scene { const char* name; std::vector<Stroke> extra; };
	const double inner = 4.0 - kBrushCm;   // inner edge radius
	std::vector<Scene> scenes;
	{
		Scene s{"5 dots, well inside", {}};
		for (int i = 0; i < 5; ++i) s.extra.push_back(dot(15 + 2.0 * std::cos(2 * kPi * i / 5), 15 + 2.0 * std::sin(2 * kPi * i / 5)));
		scenes.push_back(s);
	}
	{
		Scene s{"5 dots, 2 grazing the wall", {}};
		for (int i = 0; i < 3; ++i) s.extra.push_back(dot(15 + 2.0 * std::cos(2 * kPi * i / 5), 15 + 2.0 * std::sin(2 * kPi * i / 5)));
		for (int i = 3; i < 5; ++i) { const double rr = inner - 0.25; s.extra.push_back(dot(15 + rr * std::cos(2 * kPi * i / 5), 15 + rr * std::sin(2 * kPi * i / 5))); }
		scenes.push_back(s);
	}
	{
		Scene s{"6 dots in 3 touching pairs", {}};
		for (int i = 0; i < 3; ++i)
		{
			const double a = 2 * kPi * i / 3, cx = 15 + 1.8 * std::cos(a), cy = 15 + 1.8 * std::sin(a);
			s.extra.push_back(dot(cx - 0.35, cy)); s.extra.push_back(dot(cx + 0.35, cy));
		}
		scenes.push_back(s);
	}
	{
		Scene s{"1 spot clicked 20 times", {}};
		for (int i = 0; i < 20; ++i) s.extra.push_back(dot(15.5, 14.5));
		scenes.push_back(s);
	}
	{
		Scene s{"3 sawut lines, 1 touching the wall", {}};
		Stroke a; a.isen = true; a.radiusCm = 0.25; a.xy = {13.5, 14.0, 15.0, 13.5};
		Stroke b; b.isen = true; b.radiusCm = 0.25; b.xy = {14.5, 16.5, 16.0, 16.0};
		Stroke c; c.isen = true; c.radiusCm = 0.25; c.xy = {17.0, 15.0, 15 + inner + 0.1, 15.2};
		s.extra = {a, b, c};
		scenes.push_back(s);
	}
	for (const Scene& sc : scenes)
	{
		double isl = 0, area = 0, cvEx = 0, cvWall = 0; int n = 0;
		for (int t = 0; t < 10; ++t)
		{
			std::vector<Stroke> strokes = {handStroke(ellipsePath(4, 4), 4, {15, 15}, rot(rng), true, rng)};
			strokes.insert(strokes.end(), sc.extra.begin(), sc.extra.end());
			const std::vector<Cell> ex = analyse(strokes, 300, 300, false);
			const std::vector<Cell> wall = analyse(strokes, 300, 300, true);
			if (ex.size() != 1 || wall.empty()) continue;
			// with isen as walls, the main cell is the largest one
			const Cell* big = &wall[0];
			for (const Cell& c : wall) if (c.areaCm2 > big->areaCm2) big = &c;
			isl += ex[0].islands; area += ex[0].isenCm2; cvEx += ex[0].curvMid; cvWall += big->curvMid; ++n;
		}
		if (!n) { std::printf("   %-34s | (no single cell)\n", sc.name); continue; }
		std::printf("   %-34s |    %4.1f    |   %5.2f cm2 (%4.1f)  |   %.2f / %.2f\n",
			sc.name, isl / n, area / n, area / n / dotArea, cvEx / n, cvWall / n);
	}
	return 0;
}
