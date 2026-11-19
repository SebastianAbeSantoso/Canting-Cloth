# Canting Cloth — Core Loop Skeleton (Blueprint Guide)

> **Superseded.** The loop now runs in C++ (`UCraftLoopSubsystem`, `UCraftRules`, `UCraftSettings` in `Source/CantingCloth/`), and the Blueprint loop assets built from this guide have been retired. Keep this page only as a reference for the design (stage order, formulas, time costs). To extend the loop from Blueprint, use **Get CraftLoopSubsystem** and its actions and events, not the steps below.

Goal: one customer (**The Scout**) is playable end to end, and each stage exists and can be advanced:

`Commission → Blueprint → Cloth → Arrange → Cut → Canting → Dye/Reveal → Stitch → Inspect → Deliver → Expedition → Return → (next commission)`

**Canting** uses your existing system (`BP_WorkshopController` → `DrawWaxAtUV` → `BP_ClothActor` → `RT_CantingWax`). Every other stage starts as a **simple placeholder** that still makes a real (if small) decision and still spends time. That follows the handoff rule: *simplify the implementation, don't delete stages.*

Build in order. Each Part ends with a **✅ Test** so you always have something runnable.

---

## Part 0 — Housekeeping (5 min)

1. **Project Settings → Maps & Modes**
   - *Editor Startup Map* and *Game Default Map* → `/Game/Maps/L_Workshop`. (Right now `DefaultEngine.ini` points at the OpenWorld template.)
   - *Default GameMode* → `BP_WorkshopGameMode`.
2. Delete the duplicate `Content/L_Workshop.umap` and the stray `Content/Maps.umap`, but first check which map holds your work table setup. Keep `Content/Maps/L_Workshop`. Use *Right-click → Delete* in the Content Browser so references get fixed up.
3. `CantingCloth.uproject` now has `"EngineAssociation": "{76A2F7DD-…}"` instead of `"5.8"`. That happens when the project is re-associated with a source or registered engine build. If your teammates use the Launcher 5.8, set it back to `"5.8"` before you commit. If not, they won't be able to open it.
4. Create these folders:
   ```
   Content/Blueprints/Core      (enums, structs, GameState, function library)
   Content/Data                 (data tables)
   Content/UI                   (widgets)
   ```

---

## Part 1 — Data types

### 1.1 Enumerations (*Right-click → Blueprint → Enumeration*)

| Asset | Values (in this order) |
|---|---|
| `E_CraftStage` | Commission, SelectBlueprint, SelectCloth, Arrange, Cut, Canting, Dye, Stitch, Inspect, Deliver, Expedition, Return, Finished |
| `E_EffectFamily` | Ward_Poison, Ward_Physical |
| `E_CutStyle` | Short, Standard, Long |
| `E_ExpeditionOutcome` | Success, Injured, Retreat |
| `E_EventType` | Travel, Encounter, Hazard, InscriptionActivates, Result |

> Stage order matters: `AdvanceStage` just does "current + 1".

### 1.2 Structures (*Right-click → Blueprint → Structure*)

**`S_Fabric`**
| Var | Type | Scout default |
|---|---|---|
| DisplayName | Text | Mori Cotton |
| Weight | Float | 1.0 |
| Durability | Float | 40 |
| MagicCapacity | Float | 6.0 *(total stroke length in UV units the cloth can hold)* |
| DyeColor | Linear Color | indigo (0.08, 0.12, 0.35) |

**`S_GarmentBlueprint`**
| Var | Type | Example |
|---|---|---|
| DisplayName | Text | Light Cloak |
| BaseWeight | Float | 1.0 |
| MaxWeight | Float | 2.2 *(above this the wearer is slowed)* |
| BaseCoverage | Float | 0.7 |
| GarmentMesh | Static Mesh (Object Ref) | a cloak mesh or any placeholder plane |
| PanelCount | Integer | 2 |

**`S_Commission`**
| Var | Type | Scout |
|---|---|---|
| CustomerName | Text | Ranti the Scout |
| RequestText | Text (multi-line) | "I cross the Rawa Getih marsh in three days…" |
| AllowedBlueprints | Array of Name | [Cloak_Light] |
| RequiredEffect | E_EffectFamily | Ward_Poison |
| HazardName | Text | Marsh miasma |
| HazardStrength | Float | 45 |
| EnemyAttack | Float | 20 |
| RewardText | Text | "She brings back a *kawung* motif sketch from the river village." |
| RewardGold | Integer | 30 |

**`S_Inscription`** (filled by the cloth while you draw)
| Var | Type |
|---|---|
| StrokeCount | Integer |
| InkLength | Float |
| EnclosedShapes | Integer |
| Strength | Float (0–1) |
| Stability | Float (0–1) |

**`S_Garment`** (final product)
| Var | Type |
|---|---|
| BlueprintRow | Name |
| FabricRow | Name |
| CutStyle | E_CutStyle |
| Weight | Float |
| Coverage | Float |
| Durability | Float |
| Effect | E_EffectFamily |
| Protection | Float |
| Inscription | S_Inscription |

**`S_ExpeditionEvent`**
| Var | Type |
|---|---|
| Type | E_EventType |
| Text | Text |
| Damage | Float |
| HPAfter | Float |

### 1.3 Data Tables (*Right-click → Miscellaneous → Data Table*)

| Asset | Row Struct | Rows |
|---|---|---|
| `DT_Fabrics` | S_Fabric | `Mori_Cotton` |
| `DT_Blueprints` | S_GarmentBlueprint | `Cloak_Light` (+ `Vest_Guardian` later) |
| `DT_Commissions` | S_Commission | `Scout` (+ `Guardian` later) |

✅ **Test:** open each table and confirm the rows show up. Nothing runs yet.

---

## Part 2 — `BP_WorkshopGameState` (the loop's single source of truth)

Create a Blueprint Class → parent **GameStateBase** → `Content/Blueprints/Core/BP_WorkshopGameState`.
Open `BP_WorkshopGameMode` → *Class Defaults* → **Game State Class** = `BP_WorkshopGameState`. Also set **Player Controller Class** = `BP_WorkshopController` if it isn't set already.

### Variables
| Name | Type | Default |
|---|---|---|
| CurrentStage | E_CraftStage | Commission |
| CurrentHour | Integer | 0 |
| CommissionQueue | Array of Name | [Scout] *(add Guardian later)* |
| CommissionIndex | Integer | 0 |
| Commission | S_Commission | — |
| Garment | S_Garment | — |
| ExpeditionLog | Array of S_ExpeditionEvent | — |
| Outcome | E_ExpeditionOutcome | — |
| Gold | Integer | 0 |
| KnownMotifs | Array of Text | — *(Gotong Royong / Melestarikan payoff)* |
| bIndigoDelivered | Boolean | false |

### Event Dispatchers
- `OnStageChanged (NewStage: E_CraftStage)`
- `OnTimeChanged (Hour: Integer, ActionLabel: Text)`
- `OnNotice (Message: Text)`, for scheduled events and toasts

### Functions

**`SetStage(NewStage)`**: Set `CurrentStage` → Call `OnStageChanged(NewStage)`.

**`AdvanceStage`**: `Byte(CurrentStage) + 1` → *Byte to Enum (E_CraftStage)* → `SetStage`.
(Use the *To Byte* / *Byte to E_CraftStage* conversion nodes.)

**`SpendTime(Hours: Integer, Label: Text)`**: the **global action economy**
1. `CurrentHour += Hours`
2. Call `OnTimeChanged(CurrentHour, Label)`
3. Call `CheckScheduledEvents`

**`CheckScheduledEvents`**: the one observable scheduled event for the slice (Gotong Royong):
```
Branch (CurrentHour >= 4 AND NOT bIndigoDelivered)
  True → bIndigoDelivered = true
       → OnNotice("Bu Sari from the next street drops off a pot of fermented indigo. Deeper dye unlocked.")
```
Later, use `bIndigoDelivered` in the Dye stage to offer a second, stronger colour.

**`LoadCurrentCommission`**
```
CommissionQueue[CommissionIndex] → Get Data Table Row (DT_Commissions) → Set Commission
Reset Garment (Make S_Garment with defaults), Clear ExpeditionLog
```

**Event BeginPlay**: `LoadCurrentCommission` → `SetStage(Commission)`.
*(Delay 0.1s first if the controller's bindings aren't ready yet. Simplest fix: in the controller, bind and then call `SetStage(CurrentStage)` yourself, as shown in Part 3.)*

### Time costs (one place to tune)
| Action | Hours |
|---|---|
| Read commission / browse blueprints / rotate preview | 0 |
| Commit arrangement | 1 |
| Cut | 1 |
| Finish canting | 2 |
| Dye & reveal | 2 |
| Stitch | 2 |
| Deliver | 0 |

✅ **Test:** on BeginPlay, Print String `CurrentStage` and `Commission.CustomerName` and check they show the right values.

---

## Part 3 — Stage routing in `BP_WorkshopController`

The controller already owns mouse input. Now it also owns **which UI is visible** and **whether canting is allowed**.

### Variables
- `GS` (BP_WorkshopGameState ref)
- `StageHUD` (WBP_StageHUD ref)
- `ActivePanel` (User Widget ref)
- `bCantingEnabled` (Boolean, default **false**)

### Event BeginPlay (add to what you already have)
```
Get Game State → Cast to BP_WorkshopGameState → Set GS
Set Show Mouse Cursor = true
Set Input Mode Game And UI (Hide Cursor During Capture = false)
Create Widget WBP_StageHUD → Set StageHUD → Add to Viewport
GS → Bind Event to OnStageChanged → (custom event) HandleStageChanged
GS → Bind Event to OnTimeChanged  → StageHUD.UpdateTime
GS → Bind Event to OnNotice       → StageHUD.ShowNotice
HandleStageChanged(GS.CurrentStage)      ← catch the first stage
```

### `HandleStageChanged(NewStage)`
```
If ActivePanel is valid → Remove from Parent
bCantingEnabled = false
StageHUD.SetStageTitle(NewStage)

Switch on E_CraftStage:
  Commission      → ShowPanel(WBP_Commission)
  SelectBlueprint → ShowPanel(WBP_ChooseList, mode = Blueprint)
  SelectCloth     → ShowPanel(WBP_ChooseList, mode = Fabric)
  Arrange         → ShowPanel(WBP_Arrange)
  Cut             → ShowPanel(WBP_Cut)
  Canting         → bCantingEnabled = true ; ShowPanel(WBP_Canting) ; ClothActor.ResetInscription
  Dye             → ShowPanel(WBP_Dye)
  Stitch          → ShowPanel(WBP_Stitch)
  Inspect         → BuildGarment ; ShowPanel(WBP_Inspect)
  Deliver         → ShowPanel(WBP_Deliver)
  Expedition      → ResolveExpedition ; ShowPanel(WBP_Expedition)
  Return          → ShowPanel(WBP_Return)
  Finished        → ShowPanel(WBP_EndOfSlice)
```
`ShowPanel(Class)` = Create Widget (class pin) → Set ActivePanel → Add to Viewport.

### Gate the existing canting input
At the start of your existing mouse-draw logic (before `DeprojectMousePositionToWorld`):
```
Branch (bCantingEnabled) → False: return / do nothing
```
Also block drawing while the cursor is over a widget. Put your Confirm button in a corner away from the cloth, or check *Is Hovered* on the panel.

✅ **Test:** temporarily make the Commission panel's button call `GS.SetStage(Canting)`. Drawing should work only in that stage.

---

## Part 4 — Widgets (keep them plain for now; P5 = minimal UI)

All widgets read and write state through `Get Game State → Cast → BP_WorkshopGameState`. Put that in *Event Construct* and cache it in a variable `GS`.

### `WBP_StageHUD` (always on screen)
- Top-left: **StageTitle** text and **"Hour 3"** text.
- Bottom: a **Notice** text that fades out (Play Animation) after 4s.
- Functions: `SetStageTitle(E_CraftStage)` (use *Enum to String* for now), `UpdateTime(Hour, Label)` sets "Hour {Hour} — {Label}", and `ShowNotice(Text)`.

### `WBP_Commission`
- Shows `GS.Commission.CustomerName`, `RequestText`, `HazardName`, and Required Effect.
- Button **Accept** → `GS.AdvanceStage`.

### `WBP_ChooseList` (one reusable widget for blueprints and fabrics)
- Exposed var `bIsFabric` (Boolean, *Expose on Spawn*). In the controller, pass it from `ShowPanel`, or just make two widgets if that's simpler.
- Construct: if blueprints → loop `GS.Commission.AllowedBlueprints`. If fabrics → *Get Data Table Row Names (DT_Fabrics)*. For each name, create a `WBP_ChoiceButton` holding that Name and add it to a Vertical Box.
- On choice clicked → write `GS.Garment.BlueprintRow` or `FabricRow` (use *Set Members in S_Garment*) → `GS.AdvanceStage`.

> Each choice is free (0 hours). Browsing doesn't cost time, only committing does.

### `WBP_Arrange` (placeholder that still decides something)
- Text: "Lay the pattern along the grain or on the bias?"
- **Along grain** button: no change, durability stays as is.
- **On bias** button: Durability −10%, Coverage +5% (the cloth drapes better).
- Store as `bOnBias` (add it to S_Garment), then → `GS.SpendTime(1, "Arranged pattern")` → `GS.AdvanceStage`.
- *Later:* replace with real 2D piece placement on a finite sheet.

### `WBP_Cut`
- Three buttons: **Short / Standard / Long**. Store `GS.Garment.CutStyle` → `SpendTime(1, "Cut panels")` → `AdvanceStage`.
- Show the trade-off on hover: Short = lighter, less coverage. Long = more coverage, heavier, more cloth to inscribe.
- *Later:* replace with draggable boundary handles.

### `WBP_Canting`
- Small panel in a corner: **Ink used** progress bar = `Inscription.InkLength / Fabric.MagicCapacity`. This is your Magic Capacity Gauge, and it may go over 100% (shown red).
- Text: strokes, closed shapes.
- Button **Clear** → `ClothActor.ResetInscription` (free for now. The craft reference suggests a *nge-jos* removal that costs time, which you can add later).
- Button **Finish canting** → `ClothActor.FinalizeInscription` → `GS.SpendTime(2, "Canting")` → `GS.AdvanceStage`.
- Refresh the gauge on a 0.1s timer, or have the cloth fire an `OnInscriptionChanged` dispatcher (cleaner).

### `WBP_Dye`
- Button **Dye with soga brown** (always).
- Button **Dye with indigo** (enabled only if `GS.bIndigoDelivered`). Indigo gives +10% Stability to show the community gift matters.
- On click → `ClothActor.ApplyDye(Color)` → `SpendTime(2, "Dyed & removed wax")` → `AdvanceStage`.

### `WBP_Stitch`
- Text: "Join panels along the seams" + **Stitch** button.
- On click → controller/garment actor plays the assembly (Part 6) → `SpendTime(2, "Stitched")` → after the animation finishes, `AdvanceStage`.

### `WBP_Inspect`
- Shows Weight / Coverage / Durability / Effect / Protection from `GS.Garment`, plus a hint if Weight > MaxWeight ("Ranti may be slowed in the marsh").
- Button **Looks good** → AdvanceStage. Optional **Rotate** buttons are free.

### `WBP_Deliver`
- "Ranti tries on the cloak." **Hand over** → AdvanceStage.

### `WBP_Expedition` (the *visualizer*: it only reads the log)
- Vertical Box of lines, HP bar, **Skip** button.
- Construct: `Index = 0` → *Set Timer by Event* (1.2s, looping) → `ShowNextEvent`.
- `ShowNextEvent`: if `Index < GS.ExpeditionLog.Length`, add a text line for `ExpeditionLog[Index].Text`, set the HP bar from `HPAfter`, and `Index++`. Otherwise *Clear Timer* and show a **Continue** button.
- **Skip** → add all remaining lines at once, clear the timer, show Continue.
- **Continue** → AdvanceStage.

### `WBP_Return`
- Text depends on `GS.Outcome`:
  - Success: "Ranti returns muddy but whole…" + `Commission.RewardText`.
  - Injured: "She limps in, the cloak torn at the hem…" + half reward.
  - Retreat: "She turned back at the marsh edge…" + no motif.
- Apply rewards: `Gold += RewardGold` (or a reduced amount), and add the motif to `KnownMotifs` on Success/Injured.
- Button **Next customer** →
  ```
  CommissionIndex++
  Branch (CommissionIndex < CommissionQueue.Length)
    True  → LoadCurrentCommission → SetStage(Commission) ; ClothActor.ResetAll
    False → SetStage(Finished)
  ```

✅ **Test:** click through every stage. The hour counter should go 0 → 1 → 2 → 4 → 6 → 8, and the indigo notice should pop up once you pass hour 4.

---

## Part 5 — Inscription analysis in `BP_ClothActor`

This is the "geometry decides the magic" hook. It's cheap stroke statistics gathered inside the drawing code you already have.

### New variables
`Inscription` (S_Inscription), `StrokeStartUV` (Vector2D), `CurrentStrokeLength` (Float), `FabricRow` (Name)

### In `DrawWaxAtUV(UV)` (you already track LastUV / HasLastUV)
```
If HasLastUV == false   (this is the first dab of a new stroke)
   → StrokeStartUV = UV ; CurrentStrokeLength = 0
Else
   → Seg = Distance2D(LastUV, UV)
   → CurrentStrokeLength += Seg
   → Inscription.InkLength += Seg
(… your existing draw code, then LastUV = UV, HasLastUV = true)
```

### In `EndStroke`
```
If CurrentStrokeLength > 0.01:
   Inscription.StrokeCount += 1
   If Distance2D(LastUV, StrokeStartUV) < 0.03 AND CurrentStrokeLength > 0.12:
       Inscription.EnclosedShapes += 1          ← a closed klowong outline
HasLastUV = false   (already there)
```

### `ResetInscription`
Clear Render Target 2D (`RT_CantingWax`), reset `Inscription` to defaults, and set `HasLastUV = false`.

### `FinalizeInscription` → writes into `GS.Garment.Inscription`
```
Capacity = DT_Fabrics[GS.Garment.FabricRow].MagicCapacity
Fill     = InkLength / Capacity
Strength = Clamp(Fill, 0, 1)
Overfill = Max(Fill - 1, 0)

ClosedRatio = EnclosedShapes / Max(StrokeCount, 1)
Stability   = Clamp(0.3 + 0.7 * ClosedRatio - Overfill, 0, 1)
```
Design meaning, taken from your craft reference: **closed outlines = the wax resist holds**, so the ward is stable. Scribbling past the cloth's capacity makes it unstable (wax bleeds).

> UV distances depend on your cloth mesh's UV layout. Draw one "normal" design and Print `InkLength`, then tune `MagicCapacity` so a decent design lands around 0.7–0.9 fill.

✅ **Test:** a scribble gives high Strength and low Stability. A few neat closed circles give medium Strength and high Stability.

---

## Part 6 — Dye reveal, stitch, and building the garment

### Dye (`ApplyDye(Color)` on BP_ClothActor)
- On BeginPlay: *Create Dynamic Material Instance* from the cloth mesh's `M_BatikCloth` → store `ClothMID`.
- `ApplyDye`: `ClothMID → Set Vector Parameter Value ("DyeColor", Color)`, then animate `IsDyed` from 0 → 1 with a **Timeline** (1.5s) using *Set Scalar Parameter Value ("IsDyed", t)*.
- ⚠️ If `IsDyed` in `M_BatikCloth` is a **Static Switch**, it can't be changed at runtime. Replace it with a **Scalar Parameter** feeding the Lerp alpha that's already there. Also add a `DyeColor` Vector Parameter: dyed areas = DyeColor, waxed areas = cloth white.

### Stitch (placeholder assembly)
- Add a hidden **StaticMeshComponent** `GarmentPreview` to `BP_WorkTableActor` (or a new `BP_GarmentActor`).
- On Stitch: set its mesh to `DT_Blueprints[BlueprintRow].GarmentMesh`, set its material to the **same `ClothMID`** so your drawing shows on the garment, then run a 1.5s Timeline that scales it from 0 → 1 while the flat cloth scales 1 → 0. When the Timeline finishes → `GS.AdvanceStage`.
- It's fine for UVs not to match the real panels yet. The handoff's panel-to-3D pipeline is a later prototype.

### `BuildGarment` (controller or a Blueprint Function Library, called when entering Inspect)
```
F  = DT_Fabrics[Garment.FabricRow]
B  = DT_Blueprints[Garment.BlueprintRow]
CutCov  = Select(CutStyle: Short 0.8, Standard 1.0, Long 1.2)
CutWt   = Select(CutStyle: Short 0.8, Standard 1.0, Long 1.25)

Weight     = B.BaseWeight * F.Weight * CutWt
Coverage   = B.BaseCoverage * CutCov * (bOnBias ? 1.05 : 1.0)
Durability = F.Durability * CutCov * (bOnBias ? 0.9 : 1.0)
Effect     = Commission.RequiredEffect          (slice: effect family comes from the request; open question #3)
Protection = 100 * Inscription.Strength * Inscription.Stability * Coverage
             (+10% Stability first if indigo was used)
```
Write it all into `GS.Garment`.

---

## Part 7 — Expedition **resolver** (pure logic, no visuals)

Put `ResolveExpedition` in a **Blueprint Function Library** `BFL_Expedition` (static, testable), or in the GameState. It fills `ExpeditionLog` and `Outcome` **before** anything is shown, so Skip can't change the result.

```
HP = 100 ; Log = []

1. Travel
   Add("Ranti sets out at dawn wearing your cloak.", Travel, 0, HP)

2. Slowed check
   If Garment.Weight > Blueprint.MaxWeight:
       HP -= 10 ; Add("The heavy cloak drags in the mud. She tires.", Travel, 10, HP)

3. Encounter (physical)
   Dmg = Commission.EnemyAttack * (1 - Clamp(Garment.Durability / 100, 0, 0.8))
   HP -= Dmg ; Add("A marsh boar charges! The cloth takes the brunt.", Encounter, Dmg, HP)

4. Hazard (magical)
   If Garment.Effect == RequiredEffect AND Garment.Protection > 5:
       Add("Your inscription glows; the miasma parts around her.", InscriptionActivates, 0, HP)
       Dmg = Max(Commission.HazardStrength - Garment.Protection, 0)
   Else
       Dmg = Commission.HazardStrength
   HP -= Dmg ; Add("The miasma rolls in …", Hazard, Dmg, HP)

5. Result
   Outcome = HP >= 60 → Success ; HP > 25 → Injured ; else Retreat
   Add(result line, Result, 0, HP)
```
Store into `GS.ExpeditionLog` and `GS.Outcome`.

Sanity numbers (HazardStrength 45, EnemyAttack 20, Durability ~40):
- No inscription → 100 − 12 − 45 = **43, Injured**
- Decent closed design (Protection ~45) → **~88, Success**
- Long cut on heavy fabric + scribble → slowed + unstable → **Retreat** is possible

That gives the 2–3 visibly different outcomes the slice needs.

✅ **Test:** play the loop three times (no drawing, a scribble, a neat closed design) and confirm you get different outcomes.

---

## Part 8 — Second customer (The Guardian)

Once the Scout loop works, adding the Guardian is **data only**:
1. `DT_Blueprints`: add `Vest_Guardian` (BaseWeight 1.6, MaxWeight 3.0, BaseCoverage 0.9).
2. `DT_Commissions`: add `Guardian` (RequiredEffect Ward_Physical, AllowedBlueprints [Vest_Guardian], EnemyAttack 40, HazardStrength 20).
3. `BP_WorkshopGameState.CommissionQueue` = [Scout, Guardian].
4. In the resolver, a `Ward_Physical` effect reduces the **Encounter** damage by Protection instead of the Hazard damage (Switch on `Effect`).
5. Pay off Gotong Royong: if `KnownMotifs` contains the Scout's motif, raise the base Stability floor (0.3 → 0.4). *The Scout's return taught you something.*

---

## Where each piece will grow later

| Placeholder now | Real version (handoff §13 order) |
|---|---|
| Grain / bias buttons | 2D pattern pieces on a finite fabric sheet |
| Short/Standard/Long cut | Draggable constrained boundary handles |
| Stroke stats (length, closed loops) | Richer geometry: symmetry, intersections, density map |
| Scale-swap stitch | Seam definitions → assembled panels with preserved UVs |
| Text expedition log | Layered 2.5D chronicle reading the same event log |
| One scheduled event | Event table keyed by hour |

The data shapes in this guide (`S_Garment`, `S_Inscription`, `S_ExpeditionEvent`, resolver/visualizer split) already match the handoff architecture, so these upgrades replace one stage at a time and don't require rewiring the loop.
