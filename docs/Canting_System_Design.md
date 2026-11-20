# Canting System Design (C++)

How the canting system should work so that the player's drawing decides the magic, and what has to be built to get there.

**Status:** proposal for team review. This document covers the system and the work it needs. Scheduling and estimates belong in the development plan, not here.

**Based on:** all five markdown files in the repo (`Canting_Cloth_Core_GDD_v0.1.md`, `handoff.md`, `Canting_Craft_Reference.md`, `Canting_World_and_Characters.md`, `docs/Core_Loop_Blueprint_Guide.md`) and the C++ on `main` at `a291d4c`.

---

## 1. Goal

The player's drawing decides what the garment's magic does, how strong and steady it is, and how long it lasts. The player can see why while drawing. The same drawing appears on the finished garment and when the magic fires during the expedition.

"Done" means the GDD §14.3 checklist (Magical Crafting), the canting items in §14.2, and success criteria §17.2–17.3 all hold.

---

## 2. What the docs require

| # | Requirement | Source | Now | Work package |
|---|---|---|---|---|
| R1 | Freeform drawing on each flat cut panel, stored in panel-local coordinates | GDD 4.6, 5.1, 6.5 | One cloth plane; strokes in cm on one canvas | M4 |
| R2 | The drawing's geometry decides the magic; it is not decoration | GDD 4.6, 5.2 | Effect always equals the commission's required effect | M2 |
| R3 | Measured: path continuity, enclosed areas, intersections, symmetry, distribution, density, placement and scale | GDD 5.2 | Enclosed count and wax coverage only | M1 |
| R4 | Outputs: effect family, strength, stability, coverage, duration | GDD 5.2 | Strength and stability only | M2 |
| R5 | Two effect families in the slice, unlimited visual variety | GDD 5.6, 12 | Two enum values, picked by the commission | M2 |
| R6 | Learnable and legible, no opaque image recognition; live feedback on connected paths, enclosed regions, gaps and instability | GDD 4.6, 5.2 | A capacity bar and text counters, updated after each stroke | M3 |
| R7 | Inscription stays editable while canting | GDD 5.1 | Undo last stroke, clear all | M5 |
| R8 | Wax rules: hot wax can't be wiped; nge-jos removal costs time and leaves residue; njaplak pencil is free; klowong and isen both use capacity; coarse cloth spreads wax | Craft Ref 1, 7 | None | M5 |
| R9 | Multi-spout canting and blocking (tembokan) arrive as community contributions | Craft Ref 4, 9; GDD 9.4–9.5 | None | M5 |
| R10 | Learned motifs and techniques expand what the player can make | GDD 5.5, 9.4 | Each learned motif adds +0.1 stability to every later inscription | M6 |
| R11 | Real motif meanings stay accurate; the fantasy reading is labelled as fantasy; practitioners consulted | GDD P4, 5.5, 16.10; Craft Ref 8.3 | Motif names only | M6 + review |
| R12 | A canting segment costs time; single drawing inputs don't | GDD 4.9, 7.1 | Fixed 2 h on Finish | M2 |
| R13 | Canting is the wax; dye reveals it; no second waxing | GDD 4.7 | A timed dip whose result doesn't depend on the wax; the reveal animates `IsDyed` | M7 |
| R14 | Each panel's inscription survives dye and stitching onto the 3D garment | GDD 4.8, 5.4, 14.2 | The flat cloth shrinks; the garment keeps 6 numbers | M7 |
| R15 | Effects are generated from the player's inscription and stay recognizable during the expedition | GDD 4.11, 5.3, 14.3 | A text line says the inscription glows | M8 |
| R16 | World rule: patterns input, channel and output magic; the mechanical ground rules are still open | World 1, 6 | Not defined | D2 |
| R17 | Customers: Dutch-ish Scout with a mirror-chipped neck; Guardian Irie Kiriko, whose garment should strengthen her frozen side | World 4 | Settings still use "Ranti the Scout" and "Pak Darma the Guardian" | M0 |

---

## 3. Current state of the code

The loop scaffolding is reasonable. Stage gating and the global clock match handoff §7, and the resolver/visualizer split matches handoff §6.3. The rules are kept in a pure function library. The canting magic is a placeholder:

- `BuildGarment` sets `Effect = Commission.RequiredEffect`, so the drawing never decides what the magic does.
- Strength is wax coverage divided by the cloth's capacity, so any wax counts the same, scribble or motif.
- Stability is 0.3 plus 0.175 per closed shape, capped at 4 shapes. Four closed shapes of any kind and enough wax give full strength and stability.
- A learned motif adds 0.1 stability to every later inscription, whether or not the player draws it.
- The expedition is a fixed script (travel, encounter, hazard). The ward only checks whether its family matches.
- Time costs nothing. `CheckScheduledEvents` only handles the indigo delivery, and nothing else reads the hour. With the current costs (arrange 1 h, cut 1 h, canting 2 h) the clock always reaches hour 4 when the first canting finishes, so the indigo always arrives just before the first dye.
- Guntur's dye and stitch stages already feed the garment: dip quality changes stability by up to ±0.1, and stitch accuracy changes durability. The dip text says a long dip cracks the wax. The Craft Reference says cracks (remekan) come from bending cooled wax, and deep colour comes from repeated cold dips.
- There is one cloth surface. Arrange and Cut produce grid rectangles that are never drawn on.
- The garment stores six numbers about the inscription, not the strokes, so nothing can show the drawing later.

`UCantingCanvasComponent` has its own limits: one rectangular panel, one brush, analysis that only counts enclosed regions, and two unused texture channels.

| Code | Verdict | Notes |
|---|---|---|
| `UCraftLoopSubsystem` stage flow, clock, scheduled event | Keep | Add panel spawning, canting time cost, unlocks |
| `UCraftRules` layout, cut, dye and stitch helpers | Keep | |
| `UCraftRules::InterpretInscription` | Replace | By `UCantingInterpreter` (M2) |
| `UCraftRules::BuildGarment`, `ResolveExpedition` | Change | Use wards and charges (M2) |
| `FCraftInscription` | Change | Holds wards and per-panel data |
| `CantingWaxGrid`, `UCantingCanvasComponent` | Keep and extend | Panels, tools, layers, analysis, feedback |
| Legacy render-target path (`MeasureCloth` readback, `MaskFromPixels`, `AnalyseWaxMask`, `LegacyWaxRenderTarget`) | Remove in M4 | GPU readback and brightness guessing |
| `SetClothDrawable` (collision toggling to block drawing) | Remove in M4 | The input component checks `IsCantingActive()` instead |
| Drawing nodes in `BP_WorkshopController` and `BP_ClothActor` | Remove in M4 | Replaced by `UCantingBrushInputComponent` |
| `CraftLoopHUD` canting, inspect and expedition panels | Change | Feedback readout, tool bar, ward stats |

Screenshots in the repo show the C++ loop running in the editor (the Arrange stage), and the subsystem includes `UCantingCanvasComponent`, so it compiles. Nobody has yet checked in the editor that the canvas draws correctly.

This design changes code Guntur wrote (`CraftTypes`, `CraftRules`, the subsystem and the HUD). Agree on it with him before M2.

---

## 4. Decisions to confirm before M2

| # | Decision | Proposal | Why |
|---|---|---|---|
| D1 | Does the drawing or the player pick the effect family? (GDD 16.4) | The drawing. The commission says what's needed; the feedback shows what each cell does. | DoD 14.3 asks for different constructions to produce different effects. |
| D2 | Spell grammar and the world's ground rules (GDD 16.5, World 6) | The five rules in §5.1 | Each rule maps to a real canting practice and to "input, channel, output". |
| D3 | Does dye matter? (GDD 16.6) | Mostly visual. Keep Guntur's dip-quality term (±0.1 stability), and give each dye one small modifier: indigo +stability (as now), soga +strength. | Keeps dye a short finishing step, as GDD 4.7 asks, and keeps the existing dip stage meaningful. |
| D4 | Undo vs. "hot wax can't be wiped" (Craft Ref 1) | Free undo for the latest stroke only. Older strokes need nge-jos: 1 h and a faint residue. | Keeps drawing forgiving while respecting the craft rule. |
| D5 | Which panels take inscriptions | 1–2 per blueprint, marked in data: cloak = Back + Hood, vest = Front + Right side (her frozen arm) | Less UI and balancing; the other panels stay plain cloth. |
| D6 | Motifs in the slice | Gringsing (filter; the player's hometown motif, known from the start) and Tambal (deflect; the Scout's reward, replacing Kawung). Tembokan blocks lent by Bu Sari. No Slobog, Kawung or Parang. | Each real meaning is close to its game role (§5.5). The Scout's reward now helps the Guardian's deflect ward, so the Pulang reward matters for the next commission (GDD 9.5). Kawung and Parang were restricted court motifs. |
| D7 | Canting time | Flat 2 h per canting session (as now), plus 1 h per nge-jos | Time has no cost anywhere in the slice yet (§3), so a cost that grows with wax would reward longer work, not create a choice. Revisit once the team decides what time costs (§11). |
| D8 | Randomness on expeditions | None | Watching and skipping stay identical without extra work. |
| D9 | Kiri's frozen arm | **Decided:** her right arm. She holds the sword in her left hand, so the left side is sleeved (armguard and glove) and the right side is bare and frozen: stiff, but hard as steel and powerful. The vest's "Right side" pattern piece is the anomaly panel. | Confirmed by Sebastian. Her name works both ways: *kiri* means "left" in Indonesian, her sword hand, and her epithet Kiri means "to cut" in Japanese (World doc). The World doc doesn't state her handedness yet. |

---

## 5. Spell grammar (proposal)

### 5.1 Rules the player learns

1. **Walls hold magic.** Only sealed cells carry a ward: areas fully closed by wax that don't touch the panel edge. A gap leaks. *Craft: a broken outline lets dye seep in (garis terputus).*
2. **Walls pick the ward.** Cells walled with curves make a **filter ward** (hazards, poison, miasma). Cells walled with straight lines, and solid wax blocks, make a **deflect ward** (blows, blades). Cells with both do some of each. Pointed tips don't matter, so lens-shaped and scale-shaped cells count as curved.
3. **Filling charges it.** Isen-isen (dots and fine lines drawn with the cecek canting) fill a cell. An empty cell holds half power. A filled cell also stores a charge, and each charge is one more activation on the expedition. Isen never form walls, even where they touch one. *Craft: large outlines are filled with isen, using a finer canting than the klowong outline.*
4. **Balance steadies it.** Symmetry, even cell sizes and staying under the cloth's capacity raise stability. Leaks, loose line ends, crowded wax and nge-jos residue lower it. *Craft: overloaded cloth bleeds (nembus).*
5. **The wearer feeds it.** A ward drawn on the panel over the customer's anomaly is 1.5 times stronger: the Scout's hood at his neck, the right side panel over Kiri's frozen right arm. *World doc: accidental patterns are living circuits.*

How this maps to "patterns input, channel and output magic": the wearer's anomaly is the input (rule 5), unbroken wax lines are the channel (rules 1 and 4), and sealed cells are the output (rules 2 and 3).

The two families keep the enum names `WardPoison` and `WardPhysical`. The UI calls them "filter ward" and "deflect ward".

### 5.2 What the analysis measures

| Structure | Detected as | Used by |
|---|---|---|
| Cell | Unwaxed region that doesn't touch the panel edge, at least 4 cm². Found on a grid drawn from outline strokes only, so isen never split or dent a cell | Rules 1–3 |
| Wall shape | Measured on the wall's centre line: the cell grown by the brush radius. Corners are found first; then each wall between two corners is curved if it bows away from its chord by more than 10% of the chord's length. **Curved share** = curved wall length ÷ all wall length. A cell with no corners is all curved. | Rule 2 |
| Isen | Wax drawn with the cecek canting, measured as wax area inside each cell. Dots grazing the wall, dots that run together, and repeated clicks on one spot all count by the area they actually cover. | Rule 3 |
| Nesting | Number of wax walls between a cell and the panel edge | Later motifs (Mega Mendung) |
| Line | A stroke that encloses nothing: length, direction, loose ends (ends that touch no other wax) | Rule 4 |
| Block | Wax more than 1.5 cm from its nearest edge, from a distance transform of the wax mask. The stored distance band is only ±0.8 cm, so it can't be read from the texture. | Rule 2 (deflect) |
| Leak | A cell that closes when gaps up to 1 cm are bridged but not at 0 cm. The bridged pixels mark the gap. | Rule 1 feedback, rule 4 |
| Symmetry | Best overlap of the wax with its left–right or top–bottom mirror, or a half or quarter turn about the drawing's centre, at 128 px with 2 px tolerance | Rule 4 |
| Spread | Share of a 4 × 6 grid of squares on each inscribable panel touched by cells of that family | Ward coverage (GDD "coverage") |
| Density | Wax coverage ÷ the cloth's capacity, plus crowded spots (over 60% wax in a 5 cm window) | Rule 4 |
| Intersections | Crossing strokes. They form cells, so they need no separate rule. | — |

All seven GDD 5.2 variables are covered: continuity (leaks, loose ends), enclosed areas (cells), intersections (form cells), symmetry, distribution (spread), density, placement and scale (anomaly panel, cell size cap).

### 5.3 From structures to wards

These are starting values. M9 tunes them with the balance harness.

**Per cell *i***
- Filter share wᵢ = smoothstep(0.25, 0.5, curved shareᵢ). Deflect share = 1 − wᵢ.
- Isen needed = max(0.64 cm², 1.6% of areaᵢ), which is one cecek dot per 40 cm². Fill fᵢ = 0.5 + 0.5 × min(1, isen areaᵢ ÷ isen needed). The cell is charged if fᵢ ≥ 0.9.
- Power pᵢ = min(areaᵢ, 150 cm²) × fᵢ × panel multiplier (1.5 on the anomaly panel, otherwise 1.0). The cap stops one huge empty circle from being the best design.

**Per family** (filter uses wᵢ, deflect uses 1 − wᵢ; blocks add 0.5 × block area to deflect)
- Strength = 1 − exp(−P ÷ (0.06 × inscribable area)), where P = Σ pᵢ × share.
- Charges = 1 + floor(charged cells with share ≥ 0.5 ÷ 2), at most 5.
- Spread as in §5.2.

**Garment stability** (shared by both wards)
- 0.45 + 0.25 × symmetry + 0.10 × size regularity (1 − variation of cell areas) − 0.12 × leaks (max 3) − 0.04 × loose ends (max 5) − 1.0 × capacity overfill − 0.03 × residue marks + (dip quality − 0.5) × 0.2 + dye modifier, clamped to 0–1. The dip term is Guntur's existing rule; the dye modifier follows D3.

**Expedition**, for each hazard or attack of a matching family while charges remain:
- Damage taken = damage × (1 − Strength × (0.5 + 0.5 × Spread) × k), where k = 1 at stability ≥ 0.5, 0.5 between 0.25 and 0.5, and 0 below 0.25 (the ward fails). Each activation uses one charge. The durability block from the cloth stays as it is.
- Commissions get an event script instead of the fixed three steps. Example for the Scout: travel, miasma, boar, miasma, miasma. Three miasma events mean charges matter.

Design tensions this creates: big cells are wax-efficient but capped and need many dots; small cells give charges but use capacity fast; blocks are strong but hold no charges; drawing both families splits the power.

### 5.4 Target outcomes (M2's test bands)

| Drawing | Scout (miasma) | Guardian (raiders) |
|---|---|---|
| Nothing, or lines only | Retreat | Retreat |
| Sealed but empty curved cells | Injured | Retreat or Injured |
| Gringsing-style filled scales | Success | Injured (wrong family) |
| Tambal-style filled straight-walled cells, or tembokan blocks | Injured (wrong family) | Success on the frozen-side panel; elsewhere Injured unless very strong |
| Right family but leaking, lopsided or over capacity | Injured | Injured |

### 5.5 Motifs and techniques in the slice

| | Real meaning (Craft Ref §8) | How it plays under the rules | Game reading (labelled as fantasy) | Source in play |
|---|---|---|---|---|
| **Gringsing** | From *gering* (sick) + *sing* (not): protection from illness and misfortune. Interlocking circles or scales with centre eyes. | Small curved cells, each with a dot: filter cells that are cheap to charge | A ward against sickness and poison | The player's hometown motif, a stencil from the start |
| **Tambal** | Patchwork of triangles and squares; mending and recovery, historically draped over the sick to "patch" their health | Straight-walled cells: deflect | Mending what was broken. Kiri's frozen arm is a healed wound that became her strongest side. | The stencil the Scout brings back from the river village (replaces the kawung sketch) |
| **Tembokan** (technique) | Blocking wide areas with thick wax to keep them dye-free | Blocks, drawn with the wide spout: deflect power, no charges | Solid wax stops blows | Bu Sari lends her canting tembokan on the indigo visit |

The slice doesn't detect motifs automatically. A learned motif gives the player a pencil (njaplak) stencil to trace and a codex entry that gives the real meaning first and the fantasy reading separately. A traced stencil scores well because it follows the rules, not because it's recognized. That keeps the cultural content and the Gotong Royong payoff without a detector that has to cope with messy mouse drawings. Detection is optional (§7).

All three need cultural review before content lock (GDD 16.10). Excluded: Slobog (a mourning motif), and Kawung and Parang, which were historically restricted court motifs (*batik larangan*). Later candidates, each needing review: Truntum (scattered star blossoms), Nitik (dot grids), Mega Mendung (nested cloud outlines).

Content changes this needs: the Scout's `RewardMotif` and return texts move from Kawung to Tambal, and the Guardian's reward ("the parang line his mother wove") needs replacing for Irie Kiriko.

### 5.6 Later, not in the slice

- Channel rule: cells joined by unbroken lines share charges.
- A third family, "flow", from parallel wavy lines, for the Scout's optional mobility preference.
- Range or direction, and trigger conditions (GDD 5.2).
- Speed and temperature changing line width; blobs at stroke starts unless the spout is cleared (niup cucuk).
- Nerusi (tracing the reverse side), nyolet (spot colour inside closed cells), multi-bath mbironi.
- The returned garment showing where its ward failed.

---

## 6. Technical design

### 6.1 Data flow

```
Mouse → UCantingBrushInputComponent (on BP_WorkshopController)
      → ACantingPanelActor + UCantingCanvasComponent: strokes (cm) → wax grid → panel textures
      → on stroke end: CantingCore::AnalyseStructure → FCantingPanelStructure
      → UCantingInterpreter (grammar settings, commission context) → FCraftWard per family
      → FCraftGarment.Inscription (per-panel strokes + wards) → Dye / Stitch / Inspect
      → UCraftRules::ResolveExpedition (event script + wards) → events with ward and panel
      → chronicle + inscription overlay (panel texture rebuilt from the strokes)
```

### 6.2 Code layout

| File | Type | Job | Work package |
|---|---|---|---|
| `CantingWaxGrid.h/.cpp` | Engine-free core (exists) | Distance grid; add pencil layer, residue, downsampling | M1, M5 |
| `CantingStructure.h/.cpp` | Engine-free core (new) | Outline/isen split, regions, cells, isen area, nesting, wall shape, lines, blocks, leaks, symmetry, spread | M1 |
| `CantingMotifs.h/.cpp` | Engine-free core (new) | Motif detectors | Optional |
| `CantingTypes.h` | UE types (new) | `FCantingCell`, `FCantingPanelStructure`, `FCraftWard`, `FCraftPanelInscription` | M2 |
| `CantingInterpreter.h/.cpp` | `UBlueprintFunctionLibrary` (new) | Structures + grammar + commission → wards | M2 |
| `CantingCanvasComponent.h/.cpp` | `UActorComponent` (exists) | Tools, layers, panel bounds, analysis, feedback channels, nge-jos, input enable flag, stroke save/load | M1–M5 |
| `CantingPanelActor.h/.cpp` | `AActor` (new) | Plane mesh sized to the panel plus a canvas; one per inscribable panel | M4 |
| `CantingBrushInputComponent.h/.cpp` | `UActorComponent` (new) | Mouse trace, routing to the panel under the cursor, tool keys, undo, nge-jos | M4 |
| `CraftTypes`, `CraftRules`, `CraftSettings`, `CraftLoopSubsystem`, `CraftLoopHUD` | Existing | Grammar, tool and stencil settings; ward-based garment and resolver; panel spawning; time cost; feedback UI | M2–M8 |

Tools, motif stencils and grammar constants live in `UCraftSettings` (Project Settings), like the current content. Tuning then needs no code and no new assets. They can move to data assets later if content grows.

### 6.3 Panel textures

| Texture | R | G | B | A |
|---|---|---|---|---|
| WaxField (exists) | Wax distance | Filter cells | Deflect cells | Warnings: leak gaps, crowded spots |
| GuideField (new) | Pencil (njaplak) distance | Motif stencil | Nge-jos residue | — |

`M_BatikCloth` decodes both in one Custom HLSL node. A `ShowFeedback` parameter is 1 while canting and 0 afterwards.

The wax part of that node already works with today's component. The `WaxField` texture parameter must use Sampler Type **Linear Color**, and its R channel goes into input `D`:

```hlsl
float d = (D - 0.5) * 16.0;                        // wax distance in texels (16 = 2 x BandTexels)
float w = max(abs(ddx(d)) + abs(ddy(d)), 0.0001);
return saturate(0.5 - d / w);                      // wax coverage, anti-aliased
```

### 6.4 Input

Panels are flat, so the UV comes from intersecting the mouse ray with the panel's plane. No physics UV lookup is needed. The component ends the stroke when the cursor leaves a panel, and it ignores input unless `IsCantingActive()` is true. That replaces the collision toggling.

### 6.5 Persistence and determinism

Strokes are the only saved data. Textures and analysis are rebuilt from them. Analysis and expedition resolution use no randomness.

### 6.6 Performance budget

- Drawing: about 5 µs per `DrawAtUV` call (measured at 512 px wide).
- Analysis per stroke end: under 8 ms per panel. The spike took about 6 ms per 50 × 80 cm panel at 512 px wide, including drawing the outline. Only the changed panel is analysed. Live analysis can drop to 256 px if needed.
- Texture upload: the whole panel (about 1.6 MB for 512 × 819 RGBA8) once per stroke end.

### 6.7 Editor and art work

- Add the input component to `BP_WorkshopController` and delete the old drawing nodes there and in `BP_ClothActor`.
- Paste the Custom node into `M_BatikCloth`.
- Place an empty layout actor on the work table where the panels should appear.
- Create `M_InscriptionRune`, a UI-domain material for the expedition overlay.
- Art: one garment mesh per blueprint, with one material slot and one UV island per panel (M7).

---

## 7. Work packages

The packages are numbered in dependency order. How they are scheduled, split or cut is left to the development plan. Packages marked **core** are the smallest set where the drawing decides the magic; the others deepen it.

| | Package | Core? | Done when |
|---|---|---|---|
| M0 | Canvas in the editor, stroke recording | core | One stroke drawn in PIE shows on the cloth at the same width fast and slow, the cloth looks clean before the first stroke, the loop runs end to end, and a saved drawing loads back identically |
| M1 | Structure analysis | core | Fixture tests pass (see below) and analysis stays under 8 ms per panel |
| M2 | Wards from the drawing | core | Recorded drawings land in the §5.4 bands in an automation test; watch and skip give the same result |
| M3 | Live feedback | core | 3 playtesters who haven't seen the game can each say why a cell does or doesn't count, without being told |
| M4 | Panels | | Back and Hood (cloak) and Front and Right side (vest) can each be drawn on, and their wards combine by panel |
| M5 | Tools and wax rules | cecek only | Each tool's effect shows in the feedback; pencil is free and gone after dyeing; nge-jos costs 1 h and leaves residue |
| M6 | Motif stencils and codex | | Learning Gringsing or Tambal unlocks its stencil and codex entry; a traced stencil lands in the right §5.4 band |
| M7 | Dye, stitch and persistence | reveal only | Inspect shows the same drawing that was on the canting panels |
| M8 | Activation on the expedition | core | The chronicle shows the player's own drawing when its ward fires, and skipping still gives the same outcome |
| M9 | Balance and playtests | | GDD §17.2–17.4 hold in playtests |
| — | Motif detection (optional) | | Traced stencils are recognized 9 times in 10; ordinary drawings are falsely recognized at most once in 20 |

**Dependencies:** M1 needs M0's recorded drawings. M2 needs M1. M3 needs M1 (cell tints) and M2 (ward readout). M4 changes where strokes live, so M5–M8 are easier after it. M6 needs the pencil layer from M5. M8 needs M2.

**Smallest version that still meets the canting items of GDD §14.3:** M0–M3 and M8. That means one cloth panel, rules 1–3 from §5.1 (fill raises a cell from half to full power, no charges), stability from the existing formula, one protection value per family on the garment, and a learned motif helping only its own family. Rule 5 can be a marked zone on the single panel.

**M0. Canvas in the editor, stroke recording**
- Editor: add Canting Canvas to `BP_ClothActor`; call Draw At UV and End Stroke from its `DrawWaxAtUV` and `EndStroke` events; in `BP_WorkshopController`, also call End Stroke when the trace misses the cloth; give `M_BatikCloth` the `WaxField` parameter and the decode node from §6.3. Draw in PIE, then run `Craft.AutoPlay` in the console.
- Check: the cloth must look clean before the first stroke. If it looks waxed or noisy, the first texture upload was lost; write the clean pixels into the texture's initial data before `UpdateResource()`. This is a precaution; the problem is not confirmed for UE 5.8.
- C++: `Canting.SaveStrokes <name>` and `Canting.LoadStrokes <name>` (JSON in `Saved/Canting/`) so real drawings can be collected for tuning, and an automation test setup.
- Team: align the commission content with the World doc (names, anomalies, rewards, §5.5) and agree on D1–D9.

**M1. Structure analysis**
- C++: the `CantingStructure` core with engine-free tests, and a minimal cecek toggle so outline and isen strokes are separate from the start. A `Canting.Debug` console overlay outlines each cell and labels its walls.
- Fixtures: synthetic shapes plus at least 20 drawings recorded in M0, including Gringsing scales, Tambal patchwork and free-hand circles and squares.
- Targets: Gringsing scales and ovals read curved; Tambal cells and squares at least 6 cm across read straight; grazing, touching and repeated isen dots count by area. Thresholds (the 10% wall bow, the corner angle) are set from the recorded drawings, not the synthetic ones.

**M2. Wards from the drawing**
- C++: `CantingTypes`, grammar settings, `UCantingInterpreter`. `BuildGarment` and `ResolveExpedition` use wards and charges. Commissions get event scripts. Canting time follows D7. Inspect lists each ward.

**M3. Live feedback**
- C++: the feedback channels (per-cell tint by family, leak markers), and a HUD readout showing each ward's strength, stability and charges, plus leaks and capacity.
- Editor: extend the `M_BatikCloth` node to show the feedback channels.

**M4. Panels**
- C++: blueprint pieces get a size in cm, a role and an inscribable flag. One `ACantingPanelActor` spawns per inscribable piece after Cut, and the hem length changes the panel's height. Also `UCantingBrushInputComponent`, the anomaly panel per commission, and removal of the legacy path and collision toggling.
- Editor: add the input component, delete the old Blueprint drawing nodes, place the layout actor.

**M5. Tools and wax rules**
- C++: tool settings for klowong, cecek (dots), tembokan (wide spout) and the njaplak pencil. A tool bar with number keys, capacity cost per tool, and fabric wax spread (coarse cloth widens lines). Undo for the latest stroke only, nge-jos with time cost and residue, and Bu Sari's tembokan loan.

**M6. Motif stencils and codex**
- C++: Gringsing and Tambal stencils on the pencil layer, codex entries, and unlocking from the Scout's return.
- Team: write the codex texts and get them reviewed (GDD 16.10).

**M7. Dye, stitch and persistence**
- C++: the garment keeps per-panel strokes. The reveal is driven by the wax field, so dye shows only where there's no wax. Dye modifiers follow D3. The stitched garment shows each panel's texture. How cracks (remekan) appear depends on the dye-stage decision in §11.
- Art: garment meshes with one material slot per panel. Fallback without them: show the dyed panels laid out on the table during Inspect.

**M8. Activation on the expedition**
- C++: activation events carry the ward and panel. When a ward fires, the chronicle shows that panel's inscription as a glowing overlay, dimming as charges run out.
- Editor: `M_InscriptionRune`, a UI material for the overlay.

**M9. Balance and playtests**
- C++: a balance harness that runs recorded drawings against each commission.
- Team: playtests against GDD §17.2–17.4.

---

## 8. Testing

- **Engine-free core tests** (`CantingWaxGrid`, `CantingStructure`). They build with g++ or clang outside Unreal, so they can run on any machine or in CI.
- **Unreal automation tests** for the interpreter, the resolver, watch-vs-skip and the time cost. Run them from Tools → Session Frontend → Automation.
- **Recorded drawings** from `Canting.SaveStrokes` as fixtures for both.
- **Debug tools:** the `Canting.Debug` overlay, plus `Craft.AutoPlay` with a canned drawing per commission.

---

## 9. Risks

| Risk | Mitigation |
|---|---|
| Real mouse drawings read differently from the synthetic shapes in the spike | Record real drawings in M0 and set the thresholds from them in M1. The filter share is continuous, so borderline cells split between families instead of flipping. |
| Small straight-walled cells (under about 6 cm across) read as curved, because the line width rounds their corners away (spike v2) | Live tint per cell shows the reading while drawing; Tambal stencil cells are large enough; test a thinner klowong line (0.3 cm radius helped only a little) |
| Players don't discover the rules | Live tints per cell, leak markers, the Gringsing stencil from the start, one lesson card at the first canting |
| Time has no cost, so the global clock stays decorative (GDD 4.9 warns about this) | Flat canting cost for now (D7); the team decides what time costs (§11) |
| Two or more panels make stroke-end analysis slow | Analyse only the changed panel; drop live analysis to 256 px |
| The Unreal-side code is untested in the editor (texture upload, input) | M0 starts with one stroke drawn in PIE; M4 starts with a small in-editor input check |
| Wrong or insensitive motif use | Review codex texts and motif choices before content lock; keep fantasy readings labelled |
| Edit conflicts with Guntur on the subsystem, HUD, dye stage and `BP_WorkshopController` | Agree on this design first; one owner per file per work package |
| Scope | The smallest version in §7 |

---

## 10. Spike results

Both spikes draw outlines with the repo's `CantingWaxGrid` (0.6 cm brush radius; 0.12 cm hand tremor plus a slow wobble of 3% of the shape's size), then label the enclosed cells. Single shapes were drawn 25 times at each of 3 sizes (half-length 3, 5 and 8 cm), randomly rotated. These are synthetic strokes; real mouse input needs checking in M1. Both spikes build with g++ from `docs/canting_spikes/` (the command is at the top of each file).

### Spike v1: round vs pointed (`docs/canting_spikes/shape_spike.cpp`)

Measure: how far each cell's inner edge strays from an ellipse.

| Shape | Read as round | In between | Read as pointed | Other |
|---|---|---|---|---|
| Circle | 70 | 5 | 0 | |
| Ellipse 1.6 : 1 | 53 | 21 | 1 | |
| Ellipse 2.2 : 1 | 34 | 24 | 17 | 40 of the 41 misreads are at 3 and 5 cm |
| Triangle, square, diamond | 0 | 0 | 75 each | |
| Pentagon | 0 | 65 | 10 | |
| Hexagon | 26 | 49 | 0 | |

### Spike v2: curved vs straight walls, and isen (`docs/canting_spikes/shape_spike2.cpp`)

Gringsing scales and Kawung ovals have pointed tips, so a round-vs-pointed rule works against the motifs. Spike v2 confirms it: the v1 measure reads Gringsing-like scale cells as pointed in 158 of 160 cells, Kawung ovals as pointed or in between in all 40, and only Tambal patchwork correctly (pointed in 231 of 231).

The v2 measure (§5.2) works on the wall's centre line: it finds the corners, then checks whether each wall between corners bows (curved) or runs straight.

| Drawn | Reads as intended (v2) | v1 for comparison |
|---|---|---|
| Circle; ellipses 1.6 : 1 and 2.2 : 1; lenses 2 : 1 and 1.4 : 1 | 74–75 of 75 each, curved | 15–64 of 75 round |
| Ellipse 3 : 1 | 49 of 50 at 5 and 8 cm, curved; at 3 cm the inside closes up | 4 of 50 round |
| Gringsing-like scales (rows of 3 cm arcs, 10 drawings) | 145 of 160 cells curved, 15 in between | 0 of 160 round |
| Kawung group (4 ovals of 6 × 3 cm, 10 drawings) | 40 of 40 curved | 0 of 40 round |
| Tambal patchwork (grid plus diagonals, 10 drawings) | 188 of 231 straight, 41 in between, 2 curved | 231 of 231 pointed |
| Triangle, square, band 4 : 1 | at 8 cm: 25 of 25 straight; at 5 cm: 14–22 of 25; at 3 cm: 0–2 of 25 | 50–75 of 75 pointed |
| Rhombus 1.6 : 1, pentagon, hexagon | at 8 cm: 18–24 of 25; at 5 cm: 3–18 of 25; at 3 cm: 0–1 of 25 | |

- Overall on single shapes: v2 639 of 900 (71%) versus v1 471 of 900 (52%), counting "in between" as wrong.
- Small straight-walled shapes fail because the 1.2 cm-wide line rounds their corners until they look like one curved wall. A 0.3 cm brush radius raised the overall score to 78% but small polygons only to 3–8 of 25.
- Two earlier v2 formulations, which measured how much of the turning happens away from corners, failed even on clean polygons. The wall-bow measure is the third try.

**Isen inside a circle** (10 drawings each; one cecek dot covers 0.64 cm²):

| Scene | v1: wax pieces touching only the cell | v2: isen area inside the cell, in dots |
|---|---|---|
| 5 dots well inside | 5.0 | 4.9 |
| 5 dots, 2 grazing the wall | 3.0 | 4.5 |
| 6 dots in 3 touching pairs | 3.0 | 5.6 |
| One spot clicked 20 times | 1.0 | 0.9 |
| 3 short sawut lines, 1 touching the wall | 2.0 | 4.1 |

Under the v2 wall measure, grazing dots treated as walls barely change the cell's reading (curved share 0.97 to 0.93), so dents from touching dots are a minor issue. Counting is the real problem: raster pieces undercount grazing and touching dots, and the area method fixes that.

---

## 11. Open questions for the team

1. **What does time cost?** Right now nothing reads the clock except the indigo delivery (§3), so taking longer never hurts. Candidates: an adventurer departure hour per commission, a hazard that grows after a set hour (for example, rains reaching the marsh), or a second community visit that can be missed. Until then, canting time stays flat (D7).
2. For Guntur: should the dye stage follow the Craft Reference, where depth comes from repeated cold dips and cracks come from bending cooled wax, instead of "held too long, the wax cracks"? And do the timed dye and stitch bars stay? GDD 16.6 and 16.7 leave both open; handoff §7's "no continuous real-time pressure" is about the clock, so the bars don't break it.
3. Kiri's frozen arm is the right one (D9). Still open: should the Guardian's vest become a wrap for her bare right side, to match her character art? And should her stiff arm show up on the expedition (slow, but it takes the blows)?
4. Should the Scout's mirror-chip anomaly do more than strengthen the hood's ward, such as an illusion effect later?
5. What does Kiri bring home as her reward, now that "the parang line his mother wove" no longer fits?
6. Who reviews motif use and codex texts, and when?
7. Does Bu Sari's indigo visit also lend the tembokan, or is that a different community member? With the current costs, a tembokan arriving at hour 4 is first usable in the second commission, which is the Guardian's deflect garment.
8. UI: keep the built-in Slate HUD while the mechanics change (M0–M4), and move to UMG for visual polish once they settle. The subsystem's Blueprint events already allow that.
