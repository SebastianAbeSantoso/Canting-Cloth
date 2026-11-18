# Canting Cloth — Design Handoff

> **Primary Design Reference:** [Canting_Cloth_Core_GDD_v0.1.md](./Canting_Cloth_Core_GDD_v0.1.md) (Full GDD v0.2). Treat that document as the primary design reference, with the clarifications and decisions below taking precedence where later discussions have changed something.

---

## 1. Project Identity

- **Working Title:** Canting Cloth
- **Project:** GIMERSIA 2026
- **Genre:** Crafting simulation, magical tailoring, expedition simulation
- **Visual Direction:** Hybrid 3D + 2D, with layered 2.5D expedition presentation

**Core Fantasy:**
Become a magical batik artisan whose craftsmanship determines the fate of adventurers.

- The player operates a fantasy tailoring workshop where batik patterns function as magical inscriptions.
- Adventurers commission garments for their expeditions. The player transforms fabric into customized garments, creates magical inscriptions, and sends the finished products into the world.
- Their creations have actual mechanical consequences. An effective garment can protect an adventurer; a flawed one can contribute to injury or expedition failure.
- Adventurers eventually return, bringing stories, rewards, materials, knowledge, and potentially new blueprints.

**Central Emotional Experience:**
*I made this. Someone wore it. It mattered. And they came home.*

---

## 2. GIMERSIA Themes

The project targets all three themes:

- **Melestarikan — Preserving** *(Primary gameplay pillar)*
  Preserving batik craftsmanship, motifs, and techniques through actual practice, creation, learning, and adaptation.
- **Gotong Royong — Mutual Cooperation**
  Artisans, adventurers, and community members exchange knowledge, materials, techniques, and blueprints. Contributions should have actual gameplay consequences.
- **Pulang — Returning Home**
  Adventurers leave wearing the player's creations and return with the consequences of their journeys, connecting the workshop to the world outside.

> **Important weakness to address:** *Gotong Royong* is not yet as mechanically developed as the other two themes. It should be represented through meaningful exchange and contribution, not merely dialogue.

---

## 3. Design Pillars

1. **Craftsmanship is the primary gameplay.** The player actively transforms cloth into garments.
2. **Creative freedom within structural constraints.** Blueprints provide structure, while cutting and inscriptions allow customization.
3. **Creations have consequences.** Garment construction and magical inscriptions influence expedition outcomes.
4. **Preservation through practice and exchange.** Batik-inspired techniques and knowledge are learned and shared.
5. **Minimal, purposeful UI.** Every interface must support an actual action or meaningful information.
6. **One connected production pipeline.** Every crafting stage contributes to the finished garment.

> The sixth pillar is particularly important: do not sacrifice the defining crafting experience simply to reduce implementation effort.

---

## 4. Core Gameplay Loop

`Receive commission → Select blueprint and cloth → Arrange fabric patterns → Cut fabric panels → Canting / inscribe → Dye and reveal → Stitch / assemble → Inspect and deliver → Expedition simulation → Adventurer returns`

Rewards, knowledge, and new commissions feed back into the workshop.

The vertical slice targets **two customers**, with one as the minimum fallback and a third as optional stretch content:

- **Customer 1: The Scout**
  - Requests a lightweight cloak.
  - Needs protection against poison or another environmental hazard.
  - Introduces garment construction and basic magical inscription.
  - Can provide a blueprint, material, or technique upon returning.
- **Customer 2: The Guardian**
  - Requests more substantial protective clothing.
  - Needs resistance against physical attacks or another distinct hazard.
  - Introduces a different garment and/or magical requirement.
  - Demonstrates how the player's knowledge and craftsmanship have developed.
- **Customer 3: The Wanderer (stretch)**
  - Requests a specialized garment for an unfamiliar environment.
  - Tests whether the player can adapt existing knowledge.

---

## 5. The Garment Creation Pipeline

The player isn't simply selecting an armor item and enchanting it—they are making the garment.

**Intended order:** `Cloth → Arrange → Cut → Canting → Dye / Reveal → Stitch → Finished Garment`

### 5.1 Cloth Selection
- Select available fabric.
- Fabric can have properties such as weight, durability, flexibility, texture, and dye response.
- Different fabrics may interact differently with magical techniques.

### 5.2 Arrangement
- The player arranges blueprint pattern pieces on a finite fabric sheet.
- Potential decisions: material efficiency, pattern orientation, fabric grain, motif positioning, fabric waste.
- Arrangement must have a meaningful purpose, not be a mandatory placement chore.

### 5.3 Cutting
- The player cuts fabric into individual panels according to a blueprint.
- Blueprints provide pre-authored pattern pieces; players can modify boundaries within structural constraints.
- Cutting can affect weight, coverage, durability, and inscription area.
- Fully unrestricted polygon editing is not required.

### 5.4 Canting *(Primary creative mechanic)*
- The player draws custom batik patterns onto flat fabric panels using a canting tool (freeform strokes, geometric arrangements, connected structures, custom placement and density, learned techniques and variations).
- The inscription is not cosmetic—its geometry determines magical functionality.

### 5.5 Dyeing and Reveal
- Canting already represents wax application.
- Dye is applied to the prepared fabric, wax/resist is removed or resolved, and the finished batik pattern is revealed.
- There should not be a redundant second waxing action.

### 5.6 Stitching
- The prepared fabric panels are connected according to blueprint seam definitions to assemble the final 3D garment.
- Stitching remains part of the production pipeline, even if the vertical slice implements it through a confirmation and animation rather than individual simulated stitches.

> **Important distinction:** Simplify the implementation, not by deleting stages. Using predefined seam definitions is acceptable; removing stitching from the intended crafting experience is not.

---

## 6. Technical Architecture

The architecture should support future garment types and construction variations without requiring a complete rewrite.

### 6.1 Garment Construction (2D Pattern-Based System)

`Blueprint (pattern pieces, boundaries, seam definitions) → 2D fabric layout & cutting (editable panels, panel-local coordinates) → Canting & dyeing (persistent inscription & fabric appearance) → Seam-based assembly (connect panels, generate garment) → Final 3D garment (geometry, UVs, textures, inscriptions, stats)`

| Structure | Responsibility |
| :--- | :--- |
| `Blueprint` | Garment template and construction rules |
| `FabricSheet` | Fabric dimensions and properties |
| `PatternPiece` | 2D boundaries and editable geometry |
| `CutPanel` | Resulting fabric panel |
| `SeamDefinition` | Relationships between panel edges |
| `GarmentAssembly` | Arrangement and assembled geometry |
| `Inscription` | Canting strokes and magical data |
| `Garment` | Final product and gameplay attributes |

> **Important:** UV mapping maintains the relationship between fabric surfaces, textures, and inscriptions; it does not perform the cutting itself. The 2D panel should remain the source of truth wherever possible.

### 6.2 Spell System
Three separate responsibilities:
1. **Inscription data:** Stores the player's actual drawing.
2. **Spell interpretation:** Determines mechanical functionality.
3. **Technical-art rendering:** Converts the inscription into visual magical effects.

- **Potential geometric variables:** Path continuity, enclosed areas, intersections, symmetry, distribution, pattern density.
- **Potential spell outputs:** Magical effect, strength, stability, coverage, energy consumption or duration.
- The exact relationship between geometry and functionality is still a prototyping question. The vertical slice should support a limited number of functional effect families while permitting considerable visual variation.

### 6.3 Expedition Architecture
Two separate systems:
1. **Resolver:** Calculates expedition outcomes, evaluates adventurer attributes, garment properties, magical effects, and hazards, and produces a chronological event sequence.
2. **Visualizer:** Reads the event sequence and presents the expedition through animation. Does not calculate damage or determine success.

This separation ensures the player can skip the presentation without changing the outcome.

---

## 7. Global Time and Action Economy

The workshop operates like a turn-based RPG in which meaningful actions consume time. Time advances when the player commits an action, not when they enter a particular page.

| Action | Time |
| :--- | :--- |
| Inspect a blueprint | Free |
| Rotate a garment preview | Free |
| Switch workstations | Free |
| Commit fabric arrangement | Consumes time |
| Cut fabric | Consumes time |
| Complete a canting segment | Consumes time |
| Dye fabric | Consumes time |
| Commit assembly | Consumes time |

*(Exact costs remain undecided.)*

**Important rules:**
- The timeline is global; switching interfaces cannot bypass time.
- Cursor movement and tiny drawing inputs should not individually consume turns.
- Previewing and inspection remain free.
- Scheduled events may react to elapsed time.
- One observable time-dependent event should demonstrate the system in the vertical slice.

---

## 8. Expedition Presentation and Visual Identity

The expedition should feel like an **illustrated battle chronicle** rather than a conventional action RPG.

**Visual layers:**
- 2D illustrated backgrounds
- 3D terrain and props
- 3D adventurers and enemies
- 2D magical overlays
- Foreground layers and parallax
- Procedural inscription-based VFX

**Important events:** Movement/exploration, enemy encounters, attacks/damage, dodging/blocking, magical inscription activation, healing/protection, enemy defeat, and success/injury/failure/retreat.

- The player's actual inscription should be recognizable when activated.
- **Playback:** Watch, skip, and potentially replay (fast-forward optional).
- The resolver calculates the outcome before the presentation starts.

---

## 9. Progression and Community

Progression is driven by commissions and exchanges (payment, new fabrics/materials, garment blueprints, new magical techniques, knowledge/motifs, crafting assistance).

*Gotong Royong* should have a concrete implementation—for example, a returning adventurer bringing back a technique learned from another community, or an artisan teaching a new way of constructing a pattern. The player's knowledge grows through the community, and the player's work contributes to the community in return.

---

## 10. Vertical Slice Target

| Content | Minimum | Target |
| :--- | :--- | :--- |
| Customers | 1 | 2 |
| Garment blueprints | 1 | 2 |
| Garment models | 1 | 2 |
| Fabric types | 1 | 1–2 |
| Magical effect families | 1 | 2 |
| Expedition environments | 1 | 1 |
| Enemy types | 1 | 1–2 |
| Expedition outcomes | 2 | 2–3 |
| Inscription drawing | Freeform | Freeform |

**Complete vertical slice journey:**
Enter workshop → Receive commission → Select blueprint and cloth → Arrange and cut fabric → Create custom inscription → Dye and reveal pattern → Stitch and assemble garment → Deliver finished garment → Observe expedition → Receive adventurer's return and rewards → Complete second commission *(third customer optional)*.

---

## 11. Current Design Decisions & Open Questions

### Locked Decisions (do not reopen without concrete reason)
| Topic | Current Decision |
| :--- | :--- |
| Game identity | Magical batik tailoring |
| Primary theme | Melestarikan |
| Supporting themes | Gotong Royong and Pulang |
| Craft pipeline | Full cloth-to-garment process |
| Garment silhouettes | Blueprint-driven |
| Cutting | Customizable within constraints |
| Arrangement | Meaningful fabric-layout decisions |
| Canting | Freeform on flat fabric panels |
| Dyeing | After canting, before assembly |
| Stitching | Blueprint-defined seams, assembly animation |
| Time | Global action-based economy |
| Spellmaking | Geometry influences actual functionality |
| Combat | Simulated, not player-controlled |
| Expedition presentation | Layered 2.5D chronicle |
| Workshop | Fixed-camera 3D for the vertical slice |
| UI | Minimal and purposeful |
| Slice target | Two customers |

### Open Prototyping Questions
1. How much freedom should fabric arrangement provide?
2. Which cutting interaction is most suitable: handles, constrained points, or something else?
3. Does the player select an effect family before drawing, or does the inscription determine more of its functionality?
4. What geometric properties actually influence spell behavior?
5. Does dye affect gameplay or remain primarily visual?
6. Does stitching have additional decisions beyond assembly confirmation?
7. Which scheduled event demonstrates the global time system?
8. How severe should expedition failure and adventurer injury be?
9. How should traditional batik motifs and techniques be represented responsibly?
10. Should the final game use a walkable workshop or contextual point-and-click interaction?

---

## 12. Development Philosophy

Scope ruthlessly, **not** prematurely. Evaluate features by:
1. Is it part of the game's identity?
2. Does it provide a meaningful player decision?
3. Can its implementation be simplified?
4. Can its content quantity be reduced?
5. Can it be represented through an existing system?
6. Does it need to exist in the vertical slice?

| Feature | Bad Simplification | Better Simplification |
| :--- | :--- | :--- |
| Cutting | Remove cutting | Constrain editable panel boundaries |
| Arrangement | Remove arrangement | Use a limited fabric sheet and meaningful placement rules |
| Stitching | Remove assembly | Use predefined seams and an animation |
| Canting | Replace with rune selection | Support freeform drawing with limited effect families |
| Expedition | Remove battle presentation | Use reusable animations and precomputed events |
| Time | Remove global timeline | Use a small set of meaningful action costs and one scheduled event |

Do not confuse architectural completeness with production completeness. Preserve the complete experience while reducing the complexity of each individual system.

---

## 13. Immediate Development Priorities & Non-Goals

### Recommended Prototype Order
1. **2D fabric and pattern editor** — finite fabric sheets, arrangement, editable boundaries, cutting.
2. **Panel-to-3D pipeline** — generate fabric panels, preserve UVs, assemble via seam definitions.
3. **Canting and inscription interpreter** — draw on flat panels, extract geometric features, generate mechanical spell data.
4. **Procedural magical VFX** — make the player's actual inscription recognizable as an animated effect.
5. **One expedition simulation and replay** — verify the garment produces a visibly different outcome.
6. **Global action economy** — connect crafting actions to a shared timeline and demonstrate one scheduled event.
7. **Complete workshop integration** — connect stations, commission, finished garment, expedition, and return.

### What Not to Do Yet
Avoid prematurely committing to: a large number of garment blueprints, fully procedural clothing simulation, unlimited spell categories, complex economic management, a large expedition campaign, extensive character relationship systems, a fully walkable workshop, large-scale progression trees, or elaborate inventory management.

---

## 14. Next Discussion Goals
- Review the complete GDD for contradictions and missing mechanics.
- Ruthlessly scope the vertical slice without removing defining gameplay.
- Identify technical dependencies and implementation risks.
- Establish a realistic development roadmap.
- Validate the garment and inscription pipeline through prototypes.

> *Canting Cloth is not just a game about making magical armor. It's a game about preserving a craft, contributing something to others, and seeing the people who carry your work return home. The tailoring is the gameplay. The magic gives it consequence. The return gives it meaning.*
