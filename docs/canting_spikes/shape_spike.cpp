// Feasibility spike for the canting plan (not production code).
// Question: can sealed cells drawn by hand be classified as round / angular / long reliably enough
// to drive a spell grammar, and can dots inside a cell (isen) and nesting be counted?
//
// Rasterises jittered hand-like outlines with the repo's CantingWaxGrid, labels the enclosed regions,
// traces each region's boundary, and classifies it from circularity, corner count and aspect ratio.
// Build: g++ -O2 -std=c++17 -I../../CantingCloth/Source/CantingCloth shape_spike.cpp
//        ../../CantingCloth/Source/CantingCloth/CantingWaxGrid.cpp -o shape_spike && ./shape_spike

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
	const double kPanelW = 50.0, kPanelH = 80.0;       // cm
	const int kGridW = 512;                             // texels across
	const double kTexPerCm = kGridW / kPanelW;          // 10.24
	const float kBrushCm = 0.6f;
	const double kRoundMax = 0.045, kAngularMin = 0.075;

	struct P2 { double x, y; };

	// ---------------- hand-drawn outline generation ----------------
	std::vector<P2> polygon(int sides, double rx, double ry, double rot)
	{
		std::vector<P2> v;
		for (int i = 0; i < sides; ++i)
		{
			const double a = rot + 2 * kPi * i / sides;
			v.push_back({rx * std::cos(a), ry * std::sin(a)});
		}
		return v;
	}

	// Walks the closed outline with ~0.5 cm raw samples, adds tremor and slow wobble, rounds corners a little.
	std::vector<double> handOutline(const std::vector<P2>& verts, bool smoothShape, double cx, double cy, double size, std::mt19937& rng)
	{
		std::normal_distribution<double> tremor(0.0, 0.12);
		std::uniform_real_distribution<double> phase(0, 2 * kPi);
		const double wobA = 0.03 * size, wobF = 2 + std::uniform_int_distribution<int>(0, 2)(rng), wobP = phase(rng);
		std::vector<P2> path;
		if (smoothShape)
		{
			// verts holds (rx, ry) in verts[0]
			const double rx = verts[0].x, ry = verts[0].y;
			const int n = 400;
			for (int i = 0; i <= n; ++i) { const double a = 2 * kPi * i / n; path.push_back({rx * std::cos(a), ry * std::sin(a)}); }
		}
		else
		{
			const size_t n = verts.size();
			const double cut = std::min(0.5, 0.08 * size);   // hand-rounded corners (cm)
			for (size_t i = 0; i <= n; ++i)
			{
				const P2 a = verts[i % n], b = verts[(i + 1) % n];
				const double L = std::hypot(b.x - a.x, b.y - a.y);
				const int steps = std::max(2, int(L / 0.25));
				for (int s = 0; s <= steps; ++s)
				{
					const double t = double(s) / steps;
					if (i < n && (t * L < cut || (1 - t) * L < cut)) continue;   // leave the corner region open...
					path.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t});
				}
			}
		}
		// Resample to 0.5 cm and perturb.
		std::vector<double> xy;
		double carry = 0;
		P2 last = path[0];
		auto emit = [&](P2 p, double s)
		{
			const double w = wobA * std::sin(wobF * s / size + wobP);
			const double nx = p.x / std::max(1e-6, std::hypot(p.x, p.y)), ny = p.y / std::max(1e-6, std::hypot(p.x, p.y));
			xy.push_back(cx + p.x + nx * w + tremor(rng));
			xy.push_back(cy + p.y + ny * w + tremor(rng));
		};
		double s = 0;
		emit(last, 0);
		for (size_t i = 1; i < path.size(); ++i)
		{
			const double d = std::hypot(path[i].x - last.x, path[i].y - last.y);
			carry += d; s += d;
			if (carry >= 0.5) { emit(path[i], s); carry = 0; }
			last = path[i];
		}
		// overshoot past the start so the loop closes like a real stroke
		xy.push_back(xy[2]); xy.push_back(xy[3]);
		return xy;
	}

	// ---------------- analysis ----------------
	struct Cell
	{
		int id = 0, area = 0, islands = 0, depth = 0;
		double cx = 0, cy = 0, perimeter = 0, circularity = 0, aspect = 1;
		int corners = 0;
		double angularity = 0;   // RMS deviation of the contour from its moment-matched ellipse
		std::string cls;
	};

	struct Labels
	{
		int W = 0, H = 0;
		std::vector<int> region;      // background region id, -1 = wax
		std::vector<int> waxComp;     // wax component id, -1 = background
		std::vector<char> regionBorder;
		int numRegions = 0, numWax = 0;
	};

	Labels label(const FWaxGrid& g)
	{
		Labels L; L.W = g.GetWidth(); L.H = g.GetHeight();
		const int N = L.W * L.H;
		const uint8_t thr = FWaxGrid::Encode(0.0f);
		const auto& px = g.GetData();
		L.region.assign(N, -2); L.waxComp.assign(N, -2);
		std::vector<int> st;
		for (int i = 0; i < N; ++i) { if (px[i] < thr) L.region[i] = -1; else L.waxComp[i] = -1; }
		for (int s = 0; s < N; ++s)
		{
			if (L.region[s] != -2) continue;
			const int id = L.numRegions++;
			bool border = false;
			L.region[s] = id; st.assign(1, s);
			while (!st.empty())
			{
				const int c = st.back(); st.pop_back();
				const int x = c % L.W, y = c / L.W;
				if (x == 0 || y == 0 || x == L.W - 1 || y == L.H - 1) border = true;
				const int nb[4] = {x > 0 ? c - 1 : -1, x < L.W - 1 ? c + 1 : -1, y > 0 ? c - L.W : -1, y < L.H - 1 ? c + L.W : -1};
				for (int j : nb) if (j >= 0 && L.region[j] == -2) { L.region[j] = id; st.push_back(j); }
			}
			L.regionBorder.push_back(border ? 1 : 0);
		}
		for (int s = 0; s < N; ++s)   // wax components, 8-connected
		{
			if (L.waxComp[s] != -2) continue;
			const int id = L.numWax++;
			L.waxComp[s] = id; st.assign(1, s);
			while (!st.empty())
			{
				const int c = st.back(); st.pop_back();
				const int x = c % L.W, y = c / L.W;
				for (int dy = -1; dy <= 1; ++dy)
					for (int dx = -1; dx <= 1; ++dx)
					{
						const int nx = x + dx, ny = y + dy;
						if ((dx || dy) && nx >= 0 && ny >= 0 && nx < L.W && ny < L.H && L.waxComp[ny * L.W + nx] == -2)
						{ L.waxComp[ny * L.W + nx] = id; st.push_back(ny * L.W + nx); }
					}
			}
		}
		return L;
	}

	// Moore-neighbour trace of the outer boundary of region `id`.
	std::vector<P2> trace(const Labels& L, int id)
	{
		int start = -1;
		for (int i = 0; i < L.W * L.H && start < 0; ++i) if (L.region[i] == id) start = i;
		auto in = [&](int x, int y) { return x >= 0 && y >= 0 && x < L.W && y < L.H && L.region[y * L.W + x] == id; };
		const int dx8[8] = {1, 1, 0, -1, -1, -1, 0, 1}, dy8[8] = {0, 1, 1, 1, 0, -1, -1, -1};
		std::vector<P2> out;
		int x = start % L.W, y = start / L.W, dir = 7;
		const int sx = x, sy = y;
		for (int guard = 0; guard < 4 * L.W * L.H; ++guard)
		{
			out.push_back({x + 0.5, y + 0.5});
			int d = (dir + 6) % 8, k = 0;
			for (; k < 8; ++k, d = (d + 1) % 8) if (in(x + dx8[d], y + dy8[d])) break;
			if (k == 8) break;
			x += dx8[d]; y += dy8[d]; dir = d;
			if (x == sx && y == sy && out.size() > 2) break;
		}
		return out;
	}

	double wrapDeg(double a) { while (a > 180) a -= 360; while (a < -180) a += 360; return a; }

	// Shape from the traced inner contour, measured scale-free:
	//  - area and second moments from the contour polygon (so holes such as isen dots or inner cells don't count),
	//  - the contour is offset outward by the brush radius (Steiner formula) to measure the wall's centre line,
	//  - corners = peaks of turning on a contour resampled to 200 points and Gaussian-smoothed.
	void shapeOf(const std::vector<P2>& raw, double brushTex, Cell& c)
	{
		const int n = int(raw.size());
		double A2 = 0, Sx = 0, Sy = 0, Sxx = 0, Syy = 0, Sxy = 0, P = 0;
		for (int i = 0; i < n; ++i)
		{
			const P2 a = raw[i], b = raw[(i + 1) % n];
			const double cr = a.x * b.y - b.x * a.y;
			A2 += cr;
			Sx += (a.x + b.x) * cr; Sy += (a.y + b.y) * cr;
			Sxx += (a.x * a.x + a.x * b.x + b.x * b.x) * cr;
			Syy += (a.y * a.y + a.y * b.y + b.y * b.y) * cr;
			Sxy += (a.x * b.y + 2 * a.x * a.y + 2 * b.x * b.y + b.x * a.y) * cr;
		}
		const double A = A2 / 2;
		const double cx = Sx / (3 * A2), cy = Sy / (3 * A2);
		const double mxx = Sxx / (6 * A2) - cx * cx, myy = Syy / (6 * A2) - cy * cy, mxy = Sxy / (12 * A2) - cx * cy;
		const double tr = mxx + myy, det = mxx * myy - mxy * mxy, disc = std::sqrt(std::max(0.0, tr * tr / 4 - det));
		const double l1 = tr / 2 + disc, l2 = std::max(1e-9, tr / 2 - disc);
		const double semiA = 2 * std::sqrt(l1), semiB = 2 * std::sqrt(l2);
		c.aspect = (semiA + brushTex) / (semiB + brushTex);

		// uniform resample to M points
		const int M = 200;
		std::vector<double> cum(n + 1, 0);
		for (int i = 0; i < n; ++i) cum[i + 1] = cum[i] + std::hypot(raw[(i + 1) % n].x - raw[i].x, raw[(i + 1) % n].y - raw[i].y);
		const double L = cum[n];
		std::vector<P2> u(M);
		for (int j = 0, i = 0; j < M; ++j)
		{
			const double t = L * j / M;
			while (i < n - 1 && cum[i + 1] < t) ++i;
			const double f = (t - cum[i]) / std::max(1e-9, cum[i + 1] - cum[i]);
			const P2 a = raw[i], b = raw[(i + 1) % n];
			u[j] = {a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f};
		}
		// Gaussian smoothing, sigma 2 points, wrap-around
		std::vector<P2> sm(M);
		for (int j = 0; j < M; ++j)
		{
			double sx = 0, sy = 0, sw = 0;
			for (int k = -6; k <= 6; ++k)
			{
				const double w = std::exp(-k * k / 8.0);
				sx += w * u[(j + k + M) % M].x; sy += w * u[(j + k + M) % M].y; sw += w;
			}
			sm[j] = {sx / sw, sy / sw};
		}
		for (int j = 0; j < M; ++j) P += std::hypot(sm[(j + 1) % M].x - sm[j].x, sm[(j + 1) % M].y - sm[j].y);
		{
			// Moment-matched ellipse of the inner contour: (p-c)^T S^-1 (p-c) = 4 on its boundary.
			const double inv = 1.0 / std::max(1e-12, det);
			const double ixx = myy * inv, iyy = mxx * inv, ixy = -mxy * inv;
			double sq = 0;
			for (int j = 0; j < M; ++j)
			{
				const double dx = sm[j].x - cx, dy = sm[j].y - cy;
				const double rho = 0.5 * std::sqrt(std::max(0.0, dx * dx * ixx + 2 * dx * dy * ixy + dy * dy * iyy));
				sq += (rho - 1) * (rho - 1);
			}
			c.angularity = std::sqrt(sq / M);
		}
		const double Ac = std::fabs(A) + P * brushTex + kPi * brushTex * brushTex;
		const double Pc = P + 2 * kPi * brushTex;
		c.perimeter = Pc;
		c.circularity = 4 * kPi * Ac / (Pc * Pc);

		// turning per point, then windowed turning over +-3 points (about 3.5% of the perimeter)
		std::vector<double> turnPt(M), win(M);
		for (int j = 0; j < M; ++j)
		{
			const P2 a = sm[(j - 1 + M) % M], b = sm[j], d = sm[(j + 1) % M];
			turnPt[j] = wrapDeg((std::atan2(d.y - b.y, d.x - b.x) - std::atan2(b.y - a.y, b.x - a.x)) * 180 / kPi);
		}
		for (int j = 0; j < M; ++j)
		{
			double t = 0;
			for (int k = -3; k <= 3; ++k) t += turnPt[(j + k + M) % M];
			win[j] = std::fabs(t);
		}
		int corners = 0;
		for (int j = 0; j < M; ++j)
		{
			if (win[j] < 38.0) continue;
			bool isMax = true;
			for (int k = -8; k <= 8 && isMax; ++k)
			{
				if (k == 0) continue;
				const double o = win[(j + k + M) % M];
				if (o > win[j] || (o == win[j] && k < 0)) isMax = false;
			}
			corners += isMax ? 1 : 0;
		}
		c.corners = corners;
	}

	std::vector<Cell> analyse(const Labels& L, double minAreaCm2)
	{
		std::vector<Cell> cells;
		const int N = L.W * L.H;
		const int minArea = int(minAreaCm2 * kTexPerCm * kTexPerCm);
		std::vector<int> area(L.numRegions, 0);
		std::vector<double> mx(L.numRegions, 0), my(L.numRegions, 0), mxx(L.numRegions, 0), myy(L.numRegions, 0), mxy(L.numRegions, 0);
		for (int i = 0; i < N; ++i)
		{
			const int r = L.region[i];
			if (r < 0) continue;
			const double x = i % L.W + 0.5, y = i / L.W + 0.5;
			++area[r]; mx[r] += x; my[r] += y; mxx[r] += x * x; myy[r] += y * y; mxy[r] += x * y;
		}
		// wax component -> set of adjacent regions (for islands and nesting)
		std::vector<std::vector<int>> waxAdj(L.numWax);
		for (int i = 0; i < N; ++i)
		{
			const int w = L.waxComp[i];
			if (w < 0) continue;
			const int x = i % L.W, y = i / L.W;
			const int nb[4] = {x > 0 ? i - 1 : -1, x < L.W - 1 ? i + 1 : -1, y > 0 ? i - L.W : -1, y < L.H - 1 ? i + L.W : -1};
			for (int j : nb)
				if (j >= 0 && L.region[j] >= 0 && std::find(waxAdj[w].begin(), waxAdj[w].end(), L.region[j]) == waxAdj[w].end())
					waxAdj[w].push_back(L.region[j]);
		}
		// nesting depth: BFS over region -> wax -> region from the border-touching regions
		std::vector<int> depth(L.numRegions, -1);
		std::vector<int> q;
		for (int r = 0; r < L.numRegions; ++r) if (L.regionBorder[r]) { depth[r] = 0; q.push_back(r); }
		for (size_t qi = 0; qi < q.size(); ++qi)
		{
			const int r = q[qi];
			for (int w = 0; w < L.numWax; ++w)
			{
				if (std::find(waxAdj[w].begin(), waxAdj[w].end(), r) == waxAdj[w].end()) continue;
				for (int r2 : waxAdj[w]) if (depth[r2] < 0) { depth[r2] = depth[r] + 1; q.push_back(r2); }
			}
		}
		for (int r = 0; r < L.numRegions; ++r)
		{
			if (L.regionBorder[r] || area[r] < minArea) continue;
			Cell c; c.id = r; c.area = area[r]; c.depth = depth[r];
			c.cx = mx[r] / area[r]; c.cy = my[r] / area[r];
			const double sxx = mxx[r] / area[r] - c.cx * c.cx, syy = myy[r] / area[r] - c.cy * c.cy, sxy = mxy[r] / area[r] - c.cx * c.cy;
			const double tr = sxx + syy, det = sxx * syy - sxy * sxy, disc = std::sqrt(std::max(0.0, tr * tr / 4 - det));
			c.aspect = std::sqrt((tr / 2 + disc) / std::max(1e-9, tr / 2 - disc));
			for (int w = 0; w < L.numWax; ++w) if (waxAdj[w].size() == 1 && waxAdj[w][0] == r) ++c.islands;   // wax touching only this region
			shapeOf(trace(L, r), kBrushCm * kTexPerCm, c);
			if (c.aspect >= 3.0) c.cls = "long";
			else if (c.angularity <= kRoundMax) c.cls = "round";
			else if (c.angularity >= kAngularMin) c.cls = "angular";
			else c.cls = "mixed";
			cells.push_back(c);
		}
		return cells;
	}
}

int main()
{
	std::mt19937 rng(2026);
	struct Kind { std::string name, expect; int sides; double aspect; };
	const std::vector<Kind> kinds = {
		{"circle", "round", 0, 1.0}, {"ellipse 1.6:1", "round", 0, 1.6}, {"ellipse 2.2:1", "round", 0, 2.2},
		{"triangle", "angular", 3, 1.0}, {"square", "angular", 4, 1.0}, {"diamond", "angular", 4, 1.0},
		{"pentagon", "mixed", 5, 1.0}, {"hexagon", "mixed", 6, 1.0}, {"long oval 4:1", "long", 0, 4.0}};
	const std::vector<double> sizes = {3.0, 5.0, 8.0};   // cm, circumradius
	const int trials = 25;

	std::printf("1. Cell shape classification, hand-like jitter, brush radius %.1f cm, %d trials per cell\n", kBrushCm, trials);
	std::printf("   %-15s %-8s | size 3 cm | size 5 cm | size 8 cm | angularity mean (min..max)\n", "shape", "expect");
	int total = 0, correct = 0;
	std::map<std::string, int> confusion;
	for (const Kind& k : kinds)
	{
		std::printf("   %-15s %-8s |", k.name.c_str(), k.expect.c_str());
		double circSum = 0, cornSum = 0, aMin = 1e9, aMax = -1e9; int n = 0;
		for (double s : sizes)
		{
			int ok = 0;
			for (int t = 0; t < trials; ++t)
			{
				FWaxGrid g; g.Init(kGridW, int(kPanelH * kTexPerCm));
				std::uniform_real_distribution<double> rot(0, 2 * kPi);
				std::vector<double> xy;
				const double r = rot(rng);
				if (k.sides == 0)
				{
					std::vector<P2> rr{{s, s / k.aspect}};
					xy = handOutline(rr, true, 25, 40, s, rng);
					// rotate the ellipse
					for (size_t i = 0; i < xy.size(); i += 2)
					{
						const double x = xy[i] - 25, y = xy[i + 1] - 40;
						xy[i] = 25 + x * std::cos(r) - y * std::sin(r);
						xy[i + 1] = 40 + x * std::sin(r) + y * std::cos(r);
					}
				}
				else
				{
					const double rotation = k.name == "diamond" ? kPi / 4 + 0.1 * (rot(rng) - kPi) / kPi : r;
					xy = handOutline(polygon(k.sides, s, s, rotation), false, 25, 40, s, rng);
				}
				g.StampPolyline(xy.data(), int(xy.size() / 2), kTexPerCm, float(kBrushCm * kTexPerCm));
				const Labels L = label(g);
				const std::vector<Cell> cells = analyse(L, 2.0);
				const std::string got = cells.size() == 1 ? cells[0].cls : (cells.empty() ? "none" : "many");
				if (cells.size() == 1) { circSum += cells[0].angularity; cornSum += cells[0].corners; aMin = std::min(aMin, cells[0].angularity); aMax = std::max(aMax, cells[0].angularity); ++n; }
				ok += got == k.expect ? 1 : 0;
				++confusion[k.name + " -> " + got];
			}
			std::printf("   %2d/%2d   |", ok, trials);
			total += trials; correct += ok;
		}
		std::printf(" %5.3f (%5.3f..%5.3f)\n", n ? circSum / n : 0, n ? aMin : 0, n ? aMax : 0);
	}
	std::printf("   overall %d/%d correct (%.1f%%)\n", correct, total, 100.0 * correct / total);
	std::printf("   misclassifications:\n");
	for (const auto& [key, count] : confusion)
	{
		const std::string name = key.substr(0, key.find(" -> "));
		const std::string got = key.substr(key.find(" -> ") + 4);
		for (const Kind& k : kinds) if (k.name == name && k.expect != got) std::printf("     %-28s %d\n", key.c_str(), count);
	}

	std::printf("2. Isen dots inside a cell, and nesting depth\n");
	{
		FWaxGrid g; g.Init(kGridW, int(kPanelH * kTexPerCm));
		std::vector<P2> rr{{8, 8}};
		std::vector<double> outer = handOutline(rr, true, 25, 25, 8, rng);
		g.StampPolyline(outer.data(), int(outer.size() / 2), kTexPerCm, float(kBrushCm * kTexPerCm));
		for (int d = 0; d < 5; ++d)   // five cecek dots inside
		{
			const double a = 2 * kPi * d / 5;
			const double xy[2] = {25 + 4 * std::cos(a), 25 + 4 * std::sin(a)};
			g.StampPolyline(xy, 1, kTexPerCm, float(0.45 * kTexPerCm));
		}
		std::vector<P2> r2{{6, 6}}, r3{{3, 3}};
		std::vector<double> ring1 = handOutline(r2, true, 25, 60, 6, rng);   // concentric pair
		std::vector<double> ring2 = handOutline(r3, true, 25, 60, 3, rng);
		g.StampPolyline(ring1.data(), int(ring1.size() / 2), kTexPerCm, float(kBrushCm * kTexPerCm));
		g.StampPolyline(ring2.data(), int(ring2.size() / 2), kTexPerCm, float(kBrushCm * kTexPerCm));
		const Labels L = label(g);
		const std::vector<Cell> cells = analyse(L, 2.0);
		for (const Cell& c : cells)
			std::printf("   cell at (%4.1f, %4.1f) cm: %-8s angularity %.3f, area %6.1f cm2, islands %d, depth %d\n",
				c.cx / kTexPerCm, c.cy / kTexPerCm, c.cls.c_str(), c.angularity, c.area / (kTexPerCm * kTexPerCm), c.islands, c.depth);
	}
	return 0;
}
