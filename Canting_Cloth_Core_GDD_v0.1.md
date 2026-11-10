# CANTING CLOTH
## Core Game Design Document — Vertical Slice

**Document:** Core GDD / Vertical Slice Specification  
**Version:** 0.1  
**Project type:** Indie game / hackathon prototype  
**Genre:** Crafting simulation, magical tailoring, expedition simulation  
**Visual direction:** Hybrid 3D + 2D, layered 2.5D expedition presentation  
**Target experience:** One to three customer commissions, played from start to finish.

---

# 1. High Concept

The player operates a tailoring workshop in a fantasy world where batik patterns function as magical inscriptions.

Adventurers visit the workshop to commission garments that provide magical abilities during their expeditions.

The player obtains garment blueprints, customizes fabric cuts within their structural constraints, and freely designs magical inscriptions on the fabric.

These inscriptions are not cosmetic. Their construction determines the magical properties of the finished garment, directly affecting an adventurer's survival and expedition success.

After receiving their equipment, adventurers depart on expeditions. Their journeys are simulated and presented as illustrated, animated battle chronicles. Upon returning, they bring rewards, new materials, knowledge, and potentially new blueprints.

**Core fantasy:** Become a magical batik artisan whose craftsmanship determines the fate of adventurers.

# 2. Design Pillars

### P1. Craftsmanship is the primary gameplay

The workshop is the main playing space. The player spends their time designing, cutting, inscribing, dyeing, and assembling garments.

### P2. Creative freedom within structural constraints

Garment construction follows acquired blueprints. Players can customize cuts within those constraints.

Magical inscriptions are freeform and customizable. Their actual construction affects their functionality.

### P3. Creations have consequences

The garments produced by the player directly influence simulated expeditions. A well-made enchantment can save an adventurer; a flawed one can contribute to failure.

### P4. Preservation through practice and exchange

Traditional batik-inspired craft, knowledge, and techniques are preserved and expanded through interactions with artisans and adventurers.

### P5. Minimal, purposeful UI

Every interface exists to support a meaningful action. Information that can be communicated through the world, character behavior, or animation should not require an additional UI panel.

# 3. Core Gameplay Loop

**Receive → Design → Craft → Deliver → Observe → Return**

1. An adventurer arrives with a garment commission.
2. The player examines the requested garment and magical properties.
3. The player selects an available blueprint.
4. The player customizes the garment's cuts.
5. The player designs a magical inscription.
6. The player completes the batik and tailoring processes.
7. The finished garment is delivered to the adventurer.
8. The adventurer departs on an expedition.
9. The expedition is simulated and presented as a visual chronicle.
10. The adventurer returns with the consequences of the expedition.
11. The player receives rewards and progresses to the next commission.

The experience ends after one to three completed customer commissions.

# 4. Player Experience: Full Vertical Slice

## 4.1. Main Menu

**Purpose:** Establish the game and provide entry into the workshop.

Minimum interface:
- New Game
- Continue (if applicable)
- Settings
- Exit

No elaborate progression system or additional menus are required.

## 4.2. Workshop Introduction

The player enters a small 3D workshop.

The workshop is presented through a fixed or controlled camera. The player does not need to navigate a character through the environment for the vertical slice.

Interactive elements:
- Customer counter
- Blueprint and material storage
- Fabric cutting workspace
- Batik inscription workspace
- Tailoring workspace

Stations can be accessed through contextual interactions.

The workshop acts as the central hub for all player activity.

## 4.3. Customer Commission

An adventurer arrives and explains their requirements.

Each commission contains:

**Mandatory requirements**
- Requested garment or garment category.
- Required magical effect.
- Relevant expedition conditions.

**Optional requirements**
- Additional magical effect.
- Preferred garment properties.
- Other specialized requests.

The player is free to fulfill the request exactly, exceed it, or experiment with a different design.

A commission should communicate the adventurer's needs without prescribing the exact inscription the player must create.

The player can inspect:
- Adventurer profile.
- Expedition destination or hazard.
- Commission requirements.
- Available payment or trade offer.

For the vertical slice, customers are authored rather than procedurally generated.

## 4.4. Blueprint Selection

Garment silhouettes are defined by blueprints.

Blueprints are acquired through:
- Purchasing.
- Trading with adventurers.
- Receiving them through expedition rewards.

A blueprint defines:
- Garment silhouette.
- Available fabric panels.
- Structural boundaries.
- Cutting constraints.
- Garment assembly requirements.

The player cannot freely invent an arbitrary garment silhouette.

However, they can customize cuts within the blueprint's permitted parameters.

For example, a cloak blueprint might allow adjustments to its length, width, collar, or coverage.

The player should receive immediate visual feedback through a 3D garment preview.

**Vertical slice requirement:** At least one starter blueprint and one additional blueprint acquisition or exchange opportunity.

## 4.5. Garment Cutting

The garment is unfolded into a 2D fabric workspace.

The player adjusts and cuts fabric according to the selected blueprint.

Cut customization may influence:
- Garment weight.
- Coverage.
- Durability.
- Available inscription area.

The final cut is reflected in the 3D garment model.

For the vertical slice, garment customization should use a controlled set of adjustable parameters or constrained points rather than a fully procedural clothing simulation.

## 4.6. Magical Inscription / Batik Creation

**This is the primary creative mechanic and the most important system in the vertical slice.**

The player uses a 2D canvas to freely construct magical inscriptions on the fabric.

Unlike garment silhouettes, inscriptions are not restricted to predefined designs.

The player can:
- Draw custom lines and patterns.
- Arrange and connect geometric structures.
- Modify the inscription's size and placement.
- Create variations on existing magical techniques.

The inscription must have mechanical consequences.

Its underlying data should be interpreted by the spell system to determine properties such as:
- Magical effect.
- Stability.
- Strength.
- Coverage.
- Energy consumption or duration.

The exact relationship between geometry and magical functionality remains a system to prototype.

**Implementation direction:** Use a structured inscription representation that can be interpreted by the spell resolver and procedural technical-art system.

The visual appearance should be generated from the player's actual design, not substituted with an unrelated predefined effect.

For the vertical slice, implement a limited number of validated magical effect families rather than attempting unrestricted spell functionality.

## 4.7. Batik Processing and Tailoring

The player completes the physical garment-making process.

Core actions:
1. Apply wax or resist material.
2. Dye the fabric.
3. Reveal and inspect the finished inscription.
4. Stitch and assemble the garment.

The exact traditional processes may be simplified for gameplay while retaining recognizable batik craftsmanship.

Each meaningful action advances workshop time.

**Time model:**
- Time advances when the player performs an action.
- Each action has an associated time cost.
- Time does not advance simply because the player is idle.
- No real-time countdown is required for the vertical slice.

The intention is to make actions and production decisions determine the passage of time.

## 4.8. Final Inspection

Before delivery, the player can inspect the finished garment.

Show only information relevant to the final product:
- Garment appearance and cut.
- Weight and coverage.
- Durability.
- Magical effect.
- Spell stability.
- Known limitations or construction defects.

The player should be able to see the relationship between their design choices and the final product.

The garment can then be delivered.

## 4.9. Expedition Simulation

After receiving their garment, the adventurer departs.

The expedition is calculated immediately by a simulation system.

The simulation considers:
- Adventurer attributes and experience.
- Garment properties.
- Magical effects and their stability.
- Expedition hazards.
- Enemy characteristics.
- Other relevant equipment or party modifiers, where implemented.

The outcome is determined before the visual presentation begins.

The player does not directly control the adventurer during combat.

### Expedition Presentation

The expedition is presented as an illustrated, animated chronicle.

**Visual direction: layered 2.5D presentation**
- Background: 2D illustrated scenery.
- Environment: 3D terrain and objects.
- Characters: 3D adventurers and enemies.
- Effects: 2D magical overlays and procedural VFX.
- Foreground: Additional 2D visual layers for depth and composition.

The presentation uses parallax and layered animation to create depth.

Important events are visualized:
- Adventurer movement.
- Enemy encounters.
- Attacks and incoming damage.
- Dodging and blocking.
- Magical inscriptions activating.
- Healing, protection, or other magical effects.
- Enemy defeat.
- Expedition failure or retreat where applicable.

The player's inscriptions should be recognizable during activation.

### Playback Controls

The player can:
- Watch the entire expedition.
- Fast-forward.
- Skip the presentation.

Skipping does not change the simulation outcome.

The visual presentation is a representation of a completed calculation, not a separate combat system.

## 4.10. Adventurer Returns

After the expedition, the adventurer returns according to the calculated outcome.

Possible outcomes:
- Successful expedition.
- Successful expedition with injuries or damaged equipment.
- Failed expedition or forced retreat.

Death may exist in the larger game's consequence system, but does not need to be implemented in the first vertical slice unless necessary to establish its intended stakes.

The player receives:
- Payment or expedition rewards.
- New crafting materials.
- Potential blueprint exchanges.
- New techniques or knowledge.
- Feedback about the garment's performance.

The returned adventurer provides a narrative connection between the player's workshop and the world outside it.

# 5. Rune / Spell System

The spell system is a defining feature of the game.

The player is not merely selecting an existing enchantment. They are constructing its inscription.

### Input

A freeform 2D inscription canvas.

### Interpretation

A structured system interprets the inscription into mechanical properties.

Potential geometric variables:
- Path continuity.
- Enclosed areas.
- Intersections.
- Symmetry.
- Distribution.
- Pattern density.

These variables can influence spell functionality according to the selected effect family.

### Output

Two separate outputs are produced:

1. **Mechanical output:** Spell behavior, strength, stability, and other attributes.
2. **Visual output:** Procedurally generated magical appearance based on the inscription.

The technical-art system should reuse a common family of magical shapes and animations while allowing substantial variation from custom player input.

The system must not require a separate handcrafted VFX for every possible inscription.

# 6. Expedition System

The expedition resolver and expedition visualizer are separate systems.

### Resolver

Calculates the expedition and produces an event sequence.

Example events:
- Encounter started.
- Enemy attacked.
- Defensive spell activated.
- Damage reduced.
- Adventurer attacked.
- Enemy defeated.
- Expedition completed.

Each event contains the information needed by the visualizer.

### Visualizer

Reads the event sequence and presents it through the layered 2.5D chronicle.

The visualizer does not determine damage, success, or failure.

This separation ensures:
- Watching and skipping produce identical results.
- Battles can be replayed.
- Visual effects can be improved independently.
- New encounter types can reuse existing animations.
- Combat balance can be tested without requiring visual playback.

# 7. Vertical Slice Content

**Target: Two customers. Third customer is optional stretch content.**

| Content | Minimum | Target |
|---|---:|---:|
| Customers | 1 | 2 |
| Garment blueprints | 1 | 2 |
| Garment models | 1 | 2 |
| Magical effect families | 1 | 2 |
| Expedition environments | 1 | 1 |
| Enemy types | 1 | 2 |
| Expedition outcomes | 2 | 3 |
| Custom inscriptions | Freeform input | Freeform input |

### Suggested customer sequence

**Customer 1: The Scout**

- Requests a lightweight cloak.
- Requires protection against poison.
- Optionally requests improved mobility.
- Introduces basic garment construction and magical inscriptions.
- Can provide or exchange an additional garment blueprint upon returning.

**Customer 2: The Guardian**

- Requests more substantial protective clothing.
- Requires resistance against physical attacks.
- Optionally requests another defensive or utility effect.
- Introduces a different blueprint and a more demanding inscription.
- Demonstrates the relationship between garment construction, inscription design, and expedition results.

**Customer 3: The Wanderer (Stretch)**

- Requests a specialized garment for an unfamiliar environment.
- Has a less conventional requirement.
- Tests whether the player can adapt their existing knowledge rather than simply repeat previous solutions.

Customer dialogue and expedition events should be authored to demonstrate the core systems without requiring a large narrative.

# 8. Visual Direction

### Workshop

- 3D environment.
- Fixed or controlled camera.
- Stylized materials and lighting.
- 2D crafting interfaces.

### Fabric and Inscription Workspace

- 2D canvas.
- Clear fabric-panel representation.
- Freeform drawing.
- Immediate 3D preview of the garment.

### Expedition

- Layered 3D + 2D environment.
- Parallax backgrounds.
- Reusable 3D character animation.
- 2D spell overlays.
- Procedural inscription-based VFX.

The visual language should consistently connect the actual inscription to its magical manifestation.

# 9. UI Principles

UI should exist only when it provides information or enables an action that cannot be communicated more naturally through the environment.

Required interfaces:

1. Main menu.
2. Commission dialogue and requirements.
3. Blueprint selection.
4. Fabric cutting workspace.
5. Inscription workspace.
6. Material selection and crafting controls.
7. Final garment inspection.
8. Minimal expedition playback controls.
9. Expedition result and return dialogue.

Avoid:
- Unnecessary management dashboards.
- Excessive floating numbers.
- Decorative status panels.
- Redundant quest menus.
- Large inventories unrelated to the current crafting process.

The workshop and expedition scenes should communicate as much as possible through visual presentation.

# 10. Out of Scope

The following systems are deliberately excluded from the first vertical slice:

- Player-controlled adventurer combat.
- Free-roaming workshop navigation.
- Open-world exploration.
- Large-scale town or village simulation.
- Full economic simulation.
- Procedural customer generation.
- Unlimited garment silhouettes.
- Unlimited independently handcrafted magical effects.
- Multiplayer.
- Complex relationship systems.
- Large-scale progression or skill trees.
- Full expedition party management.

These may be considered after the core crafting and expedition loop has been validated.

# 11. Vertical Slice Definition of Done

The vertical slice is considered complete when the following experience works from beginning to end:

### Entry and Commission
- [ ] Player can launch a new game.
- [ ] Player enters the workshop.
- [ ] At least one adventurer arrives.
- [ ] Player understands the commission requirements.

### Garment Construction
- [ ] Player can select an acquired blueprint.
- [ ] Player can customize the garment's cut within blueprint constraints.
- [ ] Changes are visible in the 3D garment preview.

### Magical Crafting
- [ ] Player can freely draw an inscription.
- [ ] The inscription is converted into structured spell data.
- [ ] The inscription has a measurable mechanical effect.
- [ ] Its visual appearance can be reproduced by the technical-art system.
- [ ] Player can finish and assemble the garment.

### Expedition
- [ ] Player can deliver the finished garment.
- [ ] The expedition resolver calculates an outcome.
- [ ] The garment affects the expedition result.
- [ ] The expedition is represented through an illustrated 2.5D sequence.
- [ ] The player can watch, fast-forward, or skip.
- [ ] Watching and skipping produce identical results.

### Return and Completion
- [ ] The adventurer returns according to the expedition outcome.
- [ ] The player receives appropriate rewards and feedback.
- [ ] At least one additional commission can be completed in the target slice.
- [ ] The vertical slice ends with a complete and understandable result.

**Final acceptance test:**

A player unfamiliar with the game should be able to enter the workshop, receive a commission, construct a custom garment and magical inscription, send an adventurer on an expedition, observe the consequences of their craftsmanship, and receive the adventurer's return.

The complete sequence must be playable without developer intervention.

# 12. Primary Development Risks

The following should be prototyped before investing heavily in content production.

| Risk | Why it matters | Validation |
|---|---|---|
| Freeform inscription interpretation | Determines whether spellmaking is genuinely playable | Build the 2D drawing and spell-data pipeline |
| Garment customization | Arbitrary cutting can become expensive to implement | Test blueprint-constrained mesh modification |
| Rune-to-VFX generation | Central to the game's visual identity | Render custom inscriptions as animated effects |
| Spell balance | Poor inscriptions must have meaningful consequences | Test several valid and invalid constructions |
| Expedition event visualization | Must reliably communicate calculated results | Convert one simulated encounter into a replay |
| 2D/3D visual coherence | Different rendering styles may clash | Build one finished expedition scene |

# 13. Success Criteria

The vertical slice succeeds if it demonstrates three things:

**1. The crafting is enjoyable.**

The player finds designing and constructing inscriptions interesting enough to experiment beyond simply fulfilling customer requests.

**2. The crafting matters.**

Different garment cuts and inscriptions produce meaningfully different expedition outcomes.

**3. The experience is cohesive.**

The transition from customer request to crafting, expedition, and return feels like one complete game rather than several disconnected prototypes.

**The central experience to validate is:**

*I made this. Someone wore it. It mattered. And they came home.*
