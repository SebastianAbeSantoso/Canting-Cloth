# Canting Cloth — Final Sprint Plan (21–29 Nov)

21 Nov 2026 · 2 developers × 3 h/day

## Dates and budget

| Milestone | Date | What it means |
|---|---|---|
| Feature complete | Wed 25 Nov, end of day | Whole game playable start to end with placeholder assets. No new features after this. |
| Final assets in | Sun 29 Nov | Art and audio plugged in, final build uploaded. |
| Submission deadline | Sun 29 Nov, 23:59 WIB (= 30 Nov 00:59 WITA) | itch.io link (Windows build), 60–120 s video, description + how to play + 3 screenshots, asset credits, 2 representatives. Aim to submit by **20:00 WITA**. |

**Budget:** 21–25 Nov = 5 days × 2 people × 3 h = **30 dev-hours** for features. 26–29 Nov = **24 dev-hours** for assets, polish, video and submission. That is small, so the plan builds on what already runs (the C++ craft loop, stage HUD, canting cloth, resolver) and only adds what the judges will see.

**What the judges check first:** the video (theme fit, clear core gameplay, execution), then curators play the **first few minutes** live. So the first commission must be clear and smooth before anything else gets added.

## Roles

| | Dev A — Canting & magic | Dev B — Loop, content & presentation |
|---|---|---|
| Owns | `CantingCanvasComponent`, `CantingWaxGrid`, `CraftRules` (inscription + garment), `BP_ClothActor`, `M_BatikCloth` | `CraftLoopSubsystem`, `CraftLoopHUD` / `CraftLoopWidgets`, data tables, `BP_WorkshopController`, map, build scripts |
| Focus | The drawing decides the magic, and the player sees their drawing again on the expedition | Two commissions end to end, menu/restart, expedition chronicle, asset hook-ups, builds |

Suggested split: whoever wrote the loop/HUD code (Guntur) takes Dev B. Swap if it suits you better.

**One owner per file.** `.uasset` and `.umap` files are binary and can't be merged. Before opening an asset the other person owns, ask in chat. Pull before you start, commit + push at the end of every session.

## Day by day (feature phase)

Each cell is one 3-hour session. The ✔ line says when it's done.

| Day | Dev A (3 h) | Dev B (3 h) |
|---|---|---|
| **Sat 21** | Get canting working in PIE: the cloth looks clean before the first stroke, strokes keep the same width fast and slow, End Stroke fires when the mouse leaves the cloth. ✔ Draw a design and finish the Scout loop once. | Play the full Scout loop and list every bug. Set default map + game mode. Add a start menu (Play / Quit) and a Restart button on the end screen. Make a **packaged Windows build** with `build-itch.bat` now, to catch packaging problems early. ✔ The .exe runs start to end. |
| **Sun 22** | Drawing decides the ward: each closed shape is read as **round → poison ward (Gringsing)** or **angular → physical ward (Tambal)** using a simple roundness check (area vs. perimeter). Strength = fill vs. capacity, stability = existing closed-shape formula. ✔ Circles give a poison ward, squares a physical ward (log to screen). | Second commission, data only: `Vest_Guardian` blueprint, `Guardian` commission (Kiri, texts from the World doc), add to the queue. Resolver: physical ward reduces encounter damage. Gotong Royong payoff: the Scout's returned motif helps the Guardian's ward. ✔ Both commissions play back to back. |
| **Mon 23** | Live feedback on the Canting panel: which ward the drawing reads as, strength and stability bars, over-capacity warning. Undo last stroke. ✔ A new player can tell what their drawing will do before finishing. | Expedition chronicle v1: placeholder background, adventurer and enemy placeholders, events play one by one with an HP bar, Skip gives the same result. Clear outcome screens (Success / Injured / Retreat). ✔ Watch and Skip end in the same outcome. |
| **Tue 24** | When the ward activates on the expedition, show the **player's own drawing** as a glowing overlay (snapshot of the wax texture into a UI image). Dye reveal uses the wax, so Inspect shows the same drawing. ✔ The drawing you made is recognisable on the expedition. | Make every placeholder **swappable from one place** (data table fields, material parameters, one widget style) and write the asset list below with exact names and slots. Short first-time hints for each stage. Placeholder SFX hooks: stroke, dye, stitch, ward fires, outcome. ✔ Swapping a mesh or sound means changing one field. |
| **Wed 25** | Balance: for each commission, 3 drawing styles (none, scribble, neat closed shapes) give visibly different outcomes. Fix bugs. | Fix bugs. Packaged build. Get one person who has never seen the game to play the first 5 minutes; note where they get stuck. ✔ **Feature freeze** — tag the commit `feature-complete`. |

**Cut checkpoint — end of Mon 23.** If the round/angular reading isn't reliable by then, fall back to: effect family comes from the commission, the drawing only sets strength and stability. Dev A then moves to the Tue task early. Don't carry a half-working system into the freeze.

## Asset phase (26–29 Nov)

Assets will be finished by 29 Nov. **Ask the artists to deliver in batches** (below) instead of everything on the 29th. The video and the final build both need the final look, and a single delivery on submission day leaves no time to fix a broken import.

| Day | Batch to plug in | Dev A (3 h) | Dev B (3 h) |
|---|---|---|---|
| **Thu 26** | Batch 1: workshop room/table, cloth texture, UI font and panel frames | Fixes from Wednesday's playtest; check the cloth material still shows wax and dye correctly with the real texture | Plug in batch 1; restyle HUD and widgets with the real font/frames |
| **Fri 27** | Batch 2: cloak + vest meshes, character portraits (Ranti, Kiri, Bu Sari) | Check the drawing appears on the real garment meshes (UVs); fallback: show the dyed panels on the table at Inspect | Plug in batch 2; commission and return screens use the portraits |
| **Sat 28** | Batch 3: expedition backgrounds, enemy, ward VFX, music, SFX | Plug in VFX and audio, set volumes; packaged build + upload to itch as a test | Plug in backgrounds and enemy; record raw gameplay footage of both commissions; take screenshots |
| **Sun 29** | Anything late | Final build, upload to itch, download it and play once from itch | Cut the 60–120 s video; update itch page (description, how to play, ≥3 screenshots, credits for every third-party asset); fill the form. **Submit by 20:00 WITA.** |

If an asset batch is late, keep the placeholder and move on. A finished game with some placeholder art scores better than a broken build.

## Asset list for the artists

Agree names and formats now so plugging in is a swap, not a rebuild.

| Asset | Used in | Spec to agree |
|---|---|---|
| Workshop room + work table | Workshop map | One static mesh set, fixed camera view only |
| Cloth texture | `M_BatikCloth` | Plain undyed cloth, tileable |
| Cloak mesh, vest mesh | Stitch / Inspect | One material slot each, UVs laid out so the inscribed panel (cloak back, vest front) is one clean UV island |
| Character portraits: Ranti, Kiri, Bu Sari | Commission, Return, notices | 2D PNG, same size and framing |
| Expedition backgrounds (marsh, Guardian's route) | Chronicle | 2D layers (back / mid / front) for parallax |
| Enemy (1–2) | Chronicle | 2D sprite or simple mesh with idle + hit |
| UI font + panel frame | All widgets | One font family, one 9-slice frame |
| Music | Workshop + expedition | 2 loops |
| SFX | Stroke, dye, stitch, ward fires, success / fail | Short WAVs |

## Cut list (cut from the top if behind)

1. Undo last stroke
2. Parallax layers on the expedition (use one flat background)
3. Gotong Royong motif bonus for the Guardian (keep Bu Sari's indigo delivery instead)
4. Drawing overlay on the expedition (show a text line instead)
5. Round/angular ward reading (commission decides the family)

Never cut: both commissions playable, start/restart, the expedition outcome changing with the garment, a working packaged build.

## Daily routine

- 10 min at the start: what you'll finish today, which files you'll touch.
- Last 15 min: play the loop once, commit, push, write one line on what's left.
- Every other day (21, 23, 25, 27, 29): make a packaged Windows build. Problems found on the 29th are the ones that sink submissions.

## Before submitting

- Build downloaded from itch runs on a second PC.
- Credits screen and itch page list your team and every third-party asset with its source.
- Remove debug text, console commands and test overlays from the shipped build.
- Video shows: the commission, drawing on cloth, dye reveal, the garment, the expedition with the ward firing, and the return. Theme (Melestarikan, Gotong Royong, Pulang) visible in the first 30 s.
