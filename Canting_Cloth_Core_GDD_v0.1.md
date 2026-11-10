# CANTING CLOTH
## Full Game Design Document

**Document:** Full GDD (before scoping)
**Version:** 0.2  
**Project:** GIMERSIA 2026  
**Themes:** Melestarikan / Gotong Royong / Pulang  
**Genre:** Crafting simulation, magical tailoring, expedition simulation  
**Visual direction:** Hybrid 3D + 2D; layered 2.5D expedition presentation  
**Target vertical slice:** Two customer commissions, one as a minimum viable fallback and a third as optional stretch content.

> **Document purpose:** This is the intended design baseline, not the reduced production scope. Systems and interactions are described at the level needed to understand the game. Implementation complexity and content quantity can be reduced in a later scoping pass without silently removing the game's defining craft stages.

---

# 1. High Concept

The player runs a fantasy tailoring workshop where batik patterns function as magical inscriptions.

- Adventurers commission garments with specific requirements for their expeditions.
- Players transform fabric into garments through blueprint-based cutting, arrangement, canting, dyeing, and stitching.
- Players customize garment construction within blueprint constraints and freely design magical inscriptions on flat fabric panels.
- Inscription construction determines the garment's magical properties and effectiveness.
- Adventurers use the finished garments on simulated expeditions presented as skippable, illustrated battle chronicles.
- Returning adventurers bring rewards, materials, knowledge, stories, and potentially new blueprints or techniques.

**Core fantasy:** Become a magical batik artisan whose craftsmanship determines the fate of adventurers.

# 2. Themes and Design Pillars

## 2.1. GIMERSIA Themes

### Melestarikan: Preserving

The game celebrates the preservation and continuation of batik craft, motifs, techniques, and knowledge. Preservation is expressed through making, learning, teaching, and adapting techniques rather than collecting motifs as passive unlocks.

### Gotong Royong: Working Together

The workshop is part of a wider community. Artisans, adventurers, and other community members contribute materials, knowledge, patterns, techniques, and assistance. The player's work benefits from contributions made by others, and the player's creations help others in return.

### Pulang: Returning Home

Adventurers leave wearing garments made by the player and eventually return with the consequences of their journeys. Their returns reconnect the workshop to the wider world through stories, damaged garments, rewards, new knowledge, and new commissions.

## 2.2. Design Pillars

### P1. Craftsmanship is the primary gameplay

The workshop and the garment-making process are the main playing experience. Cloth selection, pattern layout, cutting, canting, dyeing, and stitching should each have a clear place in the production process.

### P2. Creative freedom within structural constraints

Garment construction follows acquired blueprints, but players can customize the layout and cuts within those constraints. Magical inscriptions are freeform and customizable; their construction affects their functionality.

### P3. Creations have consequences

The garments produced by the player directly influence simulated expeditions. A well-constructed garment or inscription can protect an adventurer, while a flawed or unsuitable design can contribute to injury or failure.

### P4. Preservation through practice and exchange

Batik-inspired craft, motifs, and techniques are learned, practiced, shared, and developed through interactions with artisans and adventurers. Cultural references should be researched and represented respectfully rather than assigned arbitrary meanings.

### P5. Minimal, purposeful UI

Every interface should support a meaningful action or communicate useful information. Avoid redundant panels and decorative status displays when the workshop, garment, character, or expedition can communicate the same information more clearly.

### P6. One connected production pipeline

Every craft stage should contribute to the final garment. The stages may be simplified individually, but the complete garment-making process should remain represented in the design.

# 3. Core Gameplay Loop

**Receive → Blueprint → Cloth → Arrange → Cut → Canting → Dye / Reveal → Stitch → Inspect → Deliver → Expedition → Return**

1. **Receive:** An adventurer arrives with a garment commission and expedition requirements.
2. **Blueprint:** The player selects a garment blueprint appropriate for the commission.
3. **Cloth:** The player selects the fabric to be used.
4. **Arrange:** The player lays out the blueprint's pattern pieces on the available fabric, considering placement, orientation, and material use.
5. **Cut:** The player cuts the fabric into garment panels, with customization permitted within blueprint constraints.
6. **Canting:** The player draws custom batik inscriptions onto the flat fabric panels using the canting tool.
7. **Dye / Reveal:** The player dyes the fabric and removes the resist to reveal the finished pattern.
8. **Stitch:** The prepared panels are joined according to the blueprint's seam definitions to assemble the garment.
9. **Inspect:** The player reviews the finished garment and its magical properties.
10. **Deliver:** The completed garment is given to the adventurer.
11. **Expedition:** The adventurer undertakes an expedition, resolved through simulation and presented as an illustrated, skippable chronicle.
12. **Return:** The adventurer returns with the consequences of the expedition, along with rewards, materials, knowledge, stories, or new blueprints.

The vertical slice targets two completed customer commissions. One commission is the minimum playable fallback; a third customer is optional stretch content.

# 4. Player Experience

## 4.1. Main Menu

**Purpose:** Establish the game and provide entry into the workshop.

Expected options:
- New Game.
- Continue / Load, if progression and persistence require it.
- Settings.
- Exit.

The vertical slice may use a reduced menu and does not require a full save-slot or profile-management system.

## 4.2. Workshop Introduction and Hub

The player enters a small 3D tailoring workshop that acts as the central hub for customer interactions and garment production.

The workshop contains or represents:
- Customer area / counter.
- Blueprint and fabric storage.
- Fabric layout and cutting workspace.
- Canting and dyeing workspace.
- Stitching / assembly workspace.
- Finished garment inspection area.

The workshop is presented through a fixed or controlled camera. Interaction may use contextual point-and-click selection of workstations. Whether the full game uses a stationary point-and-click workshop or a walkable workshop remains an open design decision. The vertical slice can use a fixed-camera, non-walkable implementation without deciding the final product's navigation model.

The workshop should feel like a coherent physical place. Crafting interfaces can open contextually from the relevant station rather than presenting every function as an unrelated menu.

## 4.3. Customer Commissions

An adventurer arrives and explains what they need for an upcoming expedition.

A commission can contain:

**Required information**
- Requested garment or garment category.
- Required magical effect or protection.
- Expedition destination, hazard, or expected challenge.
- Any essential garment constraints, such as lightness, coverage, or durability.

**Optional preferences**
- An additional magical effect.
- A preferred garment property or appearance.
- A request for a particular technique or motif, where appropriate.

The commission communicates the intended outcome without prescribing the exact cut or inscription. The player decides how to satisfy the request through their craft.

The player can learn about:
- The adventurer and their capabilities.
- The expedition and its likely hazards.
- The required garment and magical properties.
- Payment, trade, or other agreed rewards.

Customers in the vertical slice are authored. Procedural customer generation is outside the initial slice.

## 4.4. Blueprint and Cloth Selection

Garment silhouettes and construction rules are defined by blueprints. Blueprints may be acquired through purchases, trades, commissions, community contributions, or expedition rewards.

A blueprint defines:
- Garment silhouette and intended form.
- Required fabric pattern pieces.
- Panel outlines and editable boundaries.
- Permitted cutting and shape modifications.
- Panel relationships and seam definitions.
- Assembly orientation and garment construction rules.
- Areas or constraints relevant to inscription placement.

The player selects a blueprint appropriate for the commission and chooses the fabric to use.

Fabric properties may influence:
- Weight.
- Durability.
- Flexibility or stiffness.
- Texture, appearance, and dye response.
- Direction or grain, where relevant.
- Compatibility with particular magical techniques or inscription effects.

The number and depth of fabric properties are subject to later scope decisions. They should exist as a coherent data model even if the vertical slice uses only one or two meaningful fabric variants.

**Input:** Acquired garment blueprint and available fabric.  
**Output:** A fabric sheet and a garment pattern template ready for layout.

## 4.5. Fabric Layout, Cutting, and Arrangement

The player works in a 2D fabric workspace to prepare the garment's pattern pieces.

### Fabric Layout / Arrangement

The blueprint supplies pre-authored pattern pieces. The player arranges these pieces on a finite fabric sheet before cutting.

Arrangement can involve:
- Moving pattern pieces on the fabric sheet.
- Rotating or orienting pieces within permitted constraints.
- Fitting pieces efficiently to reduce unused fabric.
- Considering fabric direction, grain, or motif orientation where relevant.
- Preserving enough space for all required panels.

Arrangement should create a meaningful choice rather than asking the player to place pieces into fixed slots. Its potential outcomes include material efficiency, fabric waste, orientation, or other properties supported by the chosen fabric and blueprint.

### Cutting

After arranging the pattern pieces, the player cuts the fabric along their boundaries.

The blueprint provides the structural boundaries, while selected points or handles allow customization within defined limits. The player may adjust panel dimensions or contours without needing unrestricted polygon editing or arbitrary garment-shape generation.

Cutting may influence:
- Garment weight.
- Coverage.
- Durability or structural suitability.
- Available inscription area.
- Material use and leftover fabric.

The 2D pattern geometry is the source of truth for the resulting fabric panels and their corresponding 3D representation.

**Input:** Fabric sheet, blueprint pattern pieces, and player layout / cut modifications.  
**Output:** Separate 2D fabric panels with defined geometry and panel-local coordinates.

## 4.6. Canting / Inscription Design

**Canting is the primary creative and magical crafting interaction.**

The player uses a canting tool to draw custom batik patterns onto the flat fabric panels before dyeing and garment assembly.

The player can:
- Draw custom strokes and patterns.
- Connect, repeat, and arrange geometric structures.
- Modify inscription size, placement, and density.
- Create variations of learned or existing magical techniques.
- Work with the visible boundaries of each fabric panel.

The inscription is stored in relation to the 2D fabric panel rather than relying on the topology of the final assembled 3D garment. This preserves its placement through later dyeing and stitching.

The inscription is both a visual design and a functional magical construction. Its geometry and structure are interpreted by the spell system; it is not merely a cosmetic texture.

The player should receive feedback while drawing so they can understand the structural integrity and expected functionality of the inscription. Feedback may identify connected paths, enclosed regions, incomplete structures, instability, or other relevant properties.

**Input:** Flat cut fabric panels and the player's canting strokes.  
**Output:** Editable inscription data associated with the relevant fabric panels.

## 4.7. Dyeing and Pattern Reveal

After canting, the player applies dye and completes the resist-dye process to reveal the batik pattern.

The intended simplified sequence is:
1. Apply wax / resist through the canting process.
2. Apply dye to the prepared fabric.
3. Remove or resolve the resist to reveal the finished pattern.
4. Inspect the dyed panels before assembly.

Canting already represents the wax application; there is no separate duplicate waxing action.

Dye selection may influence the garment's color, appearance, and potentially magical or material properties if supported by the design. If dye is purely visual in the vertical slice, it should remain a concise finishing interaction rather than an elaborate resource-management system.

The game is a fantasy adaptation of batik craft. The process, terminology, motifs, and cultural references should be researched and reviewed with knowledgeable practitioners where possible.

**Input:** Inscribed fabric panels and selected dye.  
**Output:** Finished dyed panels with revealed batik patterns.

## 4.8. Stitching and Garment Assembly

After the panels have been inscribed and dyed, the player assembles them into the garment.

The blueprint contains pre-authored seam relationships that define which panel edges connect and how they are oriented. The player reviews the prepared panels and confirms assembly. Stitching is represented through the joining of those seams and a suitable animation or interaction; the system does not require simulation of every individual stitch unless later design work gives that interaction a meaningful purpose.

The assembled garment is generated in 3D and reflects the player's selected fabric, panel modifications, layout, and inscriptions.

The assembly system should preserve each panel's fabric appearance and inscription mapping. The 2D panel definitions remain available as the source data for their corresponding 3D garment surfaces.

**Input:** Cut, inscribed, and dyed fabric panels plus blueprint seam definitions.  
**Output:** An assembled 3D garment with corresponding panel textures, UV mapping, and construction properties.

## 4.9. Global Time and Action Economy

The workshop operates on a global action-based timeline inspired by turn-based RPG systems.

Time advances when the player commits meaningful crafting actions. It does not advance simply because the player is looking at a screen or moving between interfaces.

**Actions that may consume time include:**
- Committing a fabric layout.
- Cutting a panel or completing a cutting operation.
- Completing a canting segment or meaningful inscription operation.
- Applying a dye process.
- Committing garment assembly.
- Other significant workshop or community actions.

**Actions that do not consume time include:**
- Inspecting a commission or blueprint.
- Moving, rotating, or previewing an object without committing a change.
- Reviewing an inscription or garment preview.
- Navigating between workstations or interfaces.
- Reading information or dialogue, unless a specific event explicitly requires time to pass.

The timeline is global: opening another workstation or interface does not pause, reset, or bypass elapsed time. Actions should be grouped at a meaningful level; individual cursor movements or every tiny drawing input should not each become a separate time-consuming turn.

Action costs are intended to make players consider the time and effort of their craft without imposing continuous real-time pressure. The global timeline may interact with scheduled events, customer availability, adventurer departures, workshop activity, or other time-sensitive situations.

The exact action costs, schedule rules, and consequences remain subject to balancing. At least one observable time-dependent event should be used to validate the system in the vertical slice; otherwise, time risks becoming a decorative counter.

## 4.10. Final Inspection and Delivery

Before delivery, the player can inspect the completed garment in 3D.

Inspection communicates relevant qualities through the garment and concise supporting information, such as:
- Final cut and appearance.
- Weight, coverage, and durability.
- Fabric properties.
- Magical effect and expected behavior.
- Spell stability or known construction defects.

The inspection should make the relationship between the player's craft decisions and the finished product understandable without overwhelming the player with unnecessary statistics.

The player delivers the garment to the adventurer. The adventurer then departs for the associated expedition.

## 4.11. Expedition Simulation

The adventurer undertakes an expedition using the garment and its magical properties.

The expedition is calculated by a simulation system before its visual presentation begins. The simulation may consider:
- Adventurer attributes, experience, and condition.
- Garment construction and material properties.
- Magical effects, strength, and stability.
- Expedition hazards and environmental conditions.
- Enemy characteristics and encounter events.
- Other equipment, companions, or party modifiers, if included.

The player does not directly control the adventurer during combat. The expedition exists to demonstrate the consequences of the player's craftsmanship.

### Expedition Presentation

The expedition is presented as an illustrated, animated chronicle using a layered 2.5D visual approach.

- **Background:** 2D illustrated scenery and distant environmental layers.
- **Environment:** 3D terrain, props, and structures.
- **Characters:** 3D adventurers and enemies.
- **Effects:** 2D magical overlays and procedural inscription-based VFX.
- **Foreground:** Additional 2D layers and silhouettes for depth and composition.

Parallax and layered animation create depth without requiring a fully simulated action-RPG environment.

Important events may include:
- Adventurer movement and exploration.
- Enemy encounters.
- Attacks and incoming damage.
- Dodging, blocking, and defensive actions.
- Magical inscriptions activating.
- Healing, protection, enhancement, or other magical effects.
- Enemy defeat.
- Expedition success, failure, injury, or retreat.

The player's actual inscription should remain recognizable when its magical effect activates.

### Playback Controls

The player can:
- Watch the expedition chronicle.
- Skip the presentation and receive the already-calculated result.
- Replay the chronicle where practical.
- Optionally adjust playback speed if it improves the experience.

Watching, replaying, or skipping must not change the calculated expedition outcome. The visualizer presents the resolver's event sequence; it does not determine damage, success, or failure.

## 4.12. Adventurer Return and Rewards

After the expedition, the adventurer returns according to the calculated outcome.

Possible outcomes include:
- Successful expedition.
- Successful expedition with injuries or damaged equipment.
- Failed expedition or forced retreat.
- Death or permanent loss, if included in the broader game's consequence model.

The vertical slice does not need to implement every possible consequence, but the result should clearly reflect how the adventurer's equipment performed.

Returning adventurers may provide:
- Payment or expedition rewards.
- New fabric or crafting materials.
- New garment blueprints.
- New motifs, techniques, or inscription knowledge.
- Stories and feedback about the garment's performance.

Rewards should have a defined purpose in the progression or crafting loop. Materials that have no use in the slice should not be added merely to populate an inventory.

The return is a central emotional and thematic beat: the player sees what happened to someone who relied on their work.

# 5. Rune / Spell System

The inscription system is the primary magical mechanic of Canting Cloth. Players create magical effects by drawing custom batik patterns using the canting tool rather than selecting only from predefined spells.

## 5.1. Inscription Design

Players draw directly onto a 2D workspace representing the flat fabric panels.

The player can:
- Draw custom strokes and patterns.
- Connect and arrange geometric structures.
- Modify inscription size, placement, and density.
- Create variations of learned or existing magical techniques.
- Inspect how the inscription fits within the available panel area.

Inscriptions remain editable during the canting stage and are stored relative to their fabric panels. This keeps the inscription independent from the final 3D garment topology.

## 5.2. Spell Interpretation

The inscription is interpreted by a structured spell system that determines its mechanical properties.

Potential geometric variables include:
- Path continuity.
- Enclosed areas.
- Intersections.
- Symmetry.
- Distribution.
- Pattern density.
- Relative placement and scale.

These properties may influence:
- Magical effect or effect family.
- Strength.
- Stability.
- Coverage.
- Range or direction.
- Energy consumption or duration.
- Trigger conditions, where supported.

The exact relationship between inscription geometry and spell functionality remains subject to prototyping. The system should be learnable and legible rather than relying on opaque image recognition or arbitrary scoring.

The system should provide immediate feedback while drawing, allowing players to understand structural integrity and expected functionality. The player should be able to distinguish between a visually intricate pattern and a mechanically sound inscription.

## 5.3. Technical-Art Rendering

The player's inscription produces two distinct outputs:

1. **Mechanical output:** Determines how the garment's magical effect functions during expeditions.
2. **Visual output:** Determines the appearance and animation of the resulting magical effect.

Magical effects follow a shared visual language and general structure while allowing extensive customization based on the player's inscription.

Procedural technical art may use shaders, geometry, masks, curves, particles, and animation to generate variations without requiring an individually handcrafted VFX asset for every possible inscription.

The same inscription should remain visually recognizable on the fabric and during its activation in expeditions.

## 5.4. Garment Integration and Data Relationship

Inscription data is stored independently from the final 3D garment mesh and retains its relationship to the corresponding 2D fabric panel.

The fabric panel's geometry and UV data connect the 2D inscription to the resulting 3D garment surface. Assembly should preserve the identity and mapping of each panel.

This approach supports different garment blueprints and panel arrangements without requiring the inscription system to depend on one fixed garment mesh.

## 5.5. Learning and Preservation

Players can learn traditional or fictional magical techniques from artisans, community members, and returning adventurers.

Learned techniques may introduce:
- New inscription structures or construction rules.
- New effect families or modifiers.
- New motif references and design knowledge.
- New ways to interpret or stabilize existing inscriptions.

Traditional batik motifs and their cultural meanings should be researched and represented accurately. Fictional magical behavior should be clearly treated as the game's fantasy system rather than presented as the real historical meaning of a motif.

## 5.6. Vertical Slice Scope

The vertical slice focuses on a limited number of functional magical effect families while allowing freeform inscription designs.

The system must demonstrate:
- Freeform inscription drawing through the canting workspace.
- Inscription creation on flat fabric panels.
- Immediate structural and functional feedback.
- Conversion of inscription geometry into structured spell data.
- Meaningful differences in spell functionality.
- Visual generation based on the player's actual inscription.
- Recognition of the inscription during expedition effect activation.

The objective is to establish a system where players can create visually distinct inscriptions that produce meaningfully different magical outcomes.

# 6. Garment Construction and Data Architecture

This section describes the intended relationships between the garment-making stages. It is a design-level architecture, not a commitment to a specific engine or implementation technique.

## 6.1. Blueprint

A blueprint is a reusable garment construction definition. It contains the garment's panel templates, editable boundaries, seam relationships, assembly information, and relevant constraints.

## 6.2. Fabric Sheet

A fabric sheet represents the source material from which panels are cut. It has dimensions, material properties, appearance, and any relevant orientation or pattern direction.

## 6.3. Pattern Piece / Panel

A pattern piece defines a 2D region on the fabric sheet. Once cut, it becomes an individual fabric panel with its own geometry, local coordinate system, and UV relationship.

## 6.4. Layout and Cutting

The player places blueprint pattern pieces on the fabric sheet and commits a layout. Cutting generates the corresponding panels from the placed and permitted modified boundaries.

The implementation should constrain the editing space through blueprint-defined handles or boundaries. Unrestricted arbitrary mesh surgery is not a requirement of the design.

## 6.5. Inscription Data

Inscription strokes are stored in panel-local 2D coordinates. They are not stored solely as pixels on the final assembled garment or as coordinates tied to a single 3D mesh.

This allows each inscription to remain associated with its original fabric panel through dyeing and assembly.

## 6.6. Seam Definitions and Assembly

The blueprint defines pairs of edges or seam paths that connect panels. Stitching uses these relationships to assemble the garment in 3D.

The assembly system should preserve panel identity, texture, UV mapping, and inscription placement. A full cloth simulation is not required by the design unless later production goals justify it.

## 6.7. Finished Garment Data

The finished garment combines:
- Blueprint identity.
- Fabric and material properties.
- Panel geometry and cut modifications.
- Layout and material-use information.
- Inscription data and spell properties.
- Dye and appearance data.
- Assembly and seam data.
- Final gameplay attributes.

The final garment is the object delivered to the adventurer and evaluated by the expedition resolver.

# 7. Global Time and Action Economy

The workshop operates on a global action-based timeline inspired by turn-based RPG systems.

Time advances when the player commits meaningful actions. It does not advance merely because the player is viewing a screen, inspecting an object, or navigating between interfaces.

## 7.1. Time-Consuming Actions

Potential time-consuming actions include:
- Committing a fabric layout.
- Cutting a panel or completing a cutting operation.
- Completing a canting segment or meaningful inscription operation.
- Applying a dye process.
- Committing garment assembly.
- Performing other meaningful workshop or community activities.

The action should be grouped at a meaningful level. Cursor movement, hovering, zooming, and every tiny drawing input should not each become a separate time-consuming turn.

## 7.2. Actions That Do Not Consume Time

Examples include:
- Reading commissions or dialogue.
- Inspecting blueprints and fabrics.
- Previewing cuts or garment appearance without committing them.
- Reviewing inscription feedback.
- Navigating between workstations and interfaces.

## 7.3. Global Timeline

The timeline is shared across the entire workshop. Switching pages, stations, or interfaces does not pause or reset time. The player cannot avoid time costs by remaining inside a particular crafting screen.

Time may affect:
- Customer availability and patience.
- Adventurer departure schedules.
- Arrival of new customers or community members.
- Scheduled workshop events or deliveries.
- Other world activities that depend on the passage of time.

Commission deadlines may exist where they create interesting decisions, but should not be the sole justification for the time system.

## 7.4. Design Intent

The action economy should make the player consider the time and effort required by their craft without imposing continuous real-time pressure. The exact action costs and schedule consequences remain subject to balancing and vertical-slice testing.

# 8. Expedition System

The expedition resolver and expedition visualizer are separate systems.

## 8.1. Expedition Resolver

The resolver calculates the expedition and produces an event sequence.

It may consider:
- Adventurer attributes and experience.
- Garment construction and material properties.
- Spell functionality, strength, and stability.
- Expedition hazards and environmental conditions.
- Enemy characteristics.
- Equipment or party modifiers, if implemented.

Example events:
- Expedition started.
- Encounter entered.
- Enemy attacked.
- Defensive inscription activated.
- Damage reduced or prevented.
- Adventurer attacked or used an ability.
- Enemy defeated.
- Adventurer injured or equipment damaged.
- Expedition completed, failed, or abandoned.

Each event records the information required to present what happened, including the relevant actors, actions, effects, and outcome.

## 8.2. Expedition Visualizer

The visualizer reads the event sequence and presents it through the layered 2.5D chronicle.

The visualizer does not determine damage, success, or failure. It is a presentation layer for an already-calculated expedition.

This separation supports:
- Identical outcomes when watching or skipping.
- Replay of completed expeditions.
- Independent improvement of animations and visual effects.
- Reuse of character animations, environments, and encounter presentations.
- Combat testing without requiring the visual presentation to be complete.

## 8.3. Chronicle Presentation

The expedition should communicate the important events and, especially, the moments when the player's garment or inscription matters. It does not need to show every simulated step or calculation.

The presentation uses 2D illustrated backgrounds, 3D environment elements and characters, 2D overlays, foreground layers, and parallax. The overall scene should feel like a layered illustrated chronicle rather than a conventional directly controlled combat arena.

# 9. Progression, Rewards, and Community Exchange

Progression is driven by commissions, returning adventurers, knowledge exchange, and access to new crafting possibilities.

## 9.1. Customer Progression

Each commission introduces a garment requirement, expedition condition, or crafting challenge. Later customers may request different garment types, more specialized effects, or combinations of properties.

## 9.2. Blueprints

Blueprints unlock new garment forms and construction options. They may be purchased, traded, taught, or earned through expeditions and community interactions.

## 9.3. Materials

Materials may be obtained through payment, trade, rewards, or community contributions. Materials should have defined uses in fabric construction, dyeing, or other crafting decisions.

## 9.4. Knowledge and Techniques

Artisans and returning adventurers can contribute motifs, techniques, or knowledge. These contributions may expand the player's inscription vocabulary, improve an existing technique, or introduce new garment-making possibilities.

## 9.5. Gotong Royong in Play

Community contributions should be represented through actual exchanges of knowledge, materials, or assistance rather than dialogue alone. The vertical slice can demonstrate this with one artisan or returning adventurer who teaches or contributes a technique needed or useful for the next commission.

# 10. Visual Direction

## 10.1. Workshop

- 3D environment.
- Fixed or controlled camera for the vertical slice.
- Stylized materials, lighting, and readable workstations.
- Contextual interactions that open the relevant crafting workspace.

## 10.2. Fabric and Crafting Workspaces

- 2D fabric sheet and pattern layout.
- 2D cutting interface with visible blueprint constraints.
- Flat fabric panels for canting and dyeing.
- 2D inscription strokes that remain associated with individual panels.
- 3D garment preview showing the result of cutting, assembly, and fabric customization.

## 10.3. Expedition

- Layered 2.5D composition.
- 2D illustrated backgrounds and parallax layers.
- 3D terrain, props, adventurers, and enemies.
- 2D magical overlays and procedural inscription-based VFX.
- Foreground silhouettes or decorative layers where useful.
- Reusable character animations and event presentations.

The visual language should connect the inscription seen during crafting to the magical effect seen during the expedition.

# 11. UI and Interaction Principles

UI should exist when it provides information or enables an action that cannot be communicated more naturally through the environment or the crafting workspace.

Expected interfaces include:
1. Main menu and basic settings.
2. Customer dialogue and commission requirements.
3. Blueprint and fabric selection.
4. Fabric layout and cutting workspace.
5. Canting / inscription workspace.
6. Dye and finishing controls.
7. Stitching / assembly confirmation.
8. 3D garment inspection.
9. Expedition playback controls.
10. Expedition result and return dialogue.

Avoid:
- Redundant quest and management dashboards.
- Excessive floating numbers.
- Decorative status panels that do not support a decision.
- Unnecessary inventory complexity.
- Repeating the same information in dialogue, HUD, and separate menus.

The player should understand the commission, the crafting process, and the garment's performance without needing to consult multiple redundant interfaces.

# 12. Vertical Slice Content

**Target: Two customers. Minimum fallback: one customer. Optional stretch: a third customer.**

| Content | Minimum fallback | Target slice | Stretch |
|---|---:|---:|---:|
| Customers | 1 | 2 | 3 |
| Garment blueprints | 1 | 2 | 2–3 |
| Garment forms / models | 1 | 2 | 2–3 |
| Fabric types | 1 | 1–2 | 2–3 |
| Magical effect families | 1 | 2 | 2–3 |
| Expedition environments | 1 | 1 | 1–2 |
| Enemy types | 1 | 1–2 | 2–3 |
| Expedition outcomes | 2 | 2–3 | 3 or more |
| Custom inscriptions | Freeform drawing | Freeform drawing | Freeform drawing |

## 12.1. Customer 1: The Scout

- Requests a lightweight cloak.
- Requires protection against poison or another clearly communicated environmental hazard.
- May express one optional preference, such as improved mobility.
- Introduces the core garment pipeline and inscription design.
- Can return with a new blueprint, material, or technique that supports the next commission.

## 12.2. Customer 2: The Guardian

- Requests a more protective garment.
- Requires resistance against physical attacks or another distinct hazard.
- May express one optional defensive or utility preference.
- Demonstrates a different garment construction and/or inscription requirement.
- Provides an opportunity to demonstrate knowledge or technique received from the community or first adventurer.

## 12.3. Customer 3: The Wanderer: Stretch

- Requests a specialized garment for an unfamiliar environment.
- Introduces a less conventional requirement.
- Tests whether the player can adapt existing construction and inscription knowledge rather than simply repeat an earlier solution.

Customer dialogue, commissions, and expedition events should be authored to demonstrate the systems without requiring a large narrative campaign.

# 13. Out of Scope for the Initial Vertical Slice

The following are not required to validate the core experience:
- Player-controlled adventurer combat.
- Open-world exploration.
- Large-scale town or village simulation.
- Full economic simulation or market management.
- Procedural customer generation.
- Unlimited garment silhouettes or unrestricted garment-shape editing.
- Unlimited independently handcrafted magical effects.
- Multiplayer or cooperative player control.
- Complex relationship systems.
- Large-scale skill trees or long-term progression systems.
- Full expedition party management.
- Fully simulated cloth physics or individual stitch simulation.
- Extensive inventory and storage management.

These exclusions do not remove the corresponding design concepts from the broader game; they identify systems that are not necessary to prove the vertical slice.

# 14. Vertical Slice Definition of Done

The vertical slice must demonstrate a complete playable experience from entering the workshop to receiving an adventurer's return.

## 14.1. Entry and Commission
- [ ] Player can start a new game and enter the workshop.
- [ ] Workshop interactions are accessible through the chosen interface.
- [ ] An adventurer arrives and presents a commission.
- [ ] Commission requirements and expedition conditions are understandable.
- [ ] Player can select an appropriate garment blueprint and fabric.

## 14.2. Garment Construction
- [ ] Player can arrange blueprint pattern pieces on a fabric sheet.
- [ ] Arrangement produces a meaningful result, such as material efficiency or orientation.
- [ ] Player can customize and cut fabric within blueprint constraints.
- [ ] Cutting produces individual 2D fabric panels.
- [ ] The system retains panel geometry and panel-local coordinates.
- [ ] Player can cant custom inscriptions onto flat panels.
- [ ] Player can dye and reveal the finished patterns.
- [ ] Player can assemble panels through blueprint seam definitions.
- [ ] The assembled garment is generated and previewed in 3D.
- [ ] The finished garment preserves the appearance and inscription of each panel.

## 14.3. Magical Crafting
- [ ] Player can freely draw inscriptions using the canting workspace.
- [ ] The system provides immediate structural and functional feedback.
- [ ] Inscription geometry is converted into structured spell data.
- [ ] Different inscription constructions produce meaningfully different magical effects.
- [ ] The inscription generates its corresponding procedural visual effect.
- [ ] The inscription remains recognizable during expedition activation.

## 14.4. Time and Action Economy
- [ ] Meaningful crafting actions advance the global workshop timeline.
- [ ] Inspection, previewing, and interface navigation do not consume time.
- [ ] Action costs are consistent across crafting stages and workstations.
- [ ] Time cannot be bypassed by switching interfaces or remaining on a particular page.
- [ ] At least one observable time-dependent event demonstrates that the global timeline matters.

## 14.5. Expedition
- [ ] Player can deliver the completed garment to the adventurer.
- [ ] Expedition outcomes are calculated through the simulation system.
- [ ] Garment construction and inscription properties influence expedition results.
- [ ] The resolver generates an event sequence for the expedition.
- [ ] The event sequence is presented through the layered 2.5D visualizer.
- [ ] Player can watch or skip the expedition.
- [ ] Watching and skipping produce identical outcomes.

## 14.6. Return and Completion
- [ ] Adventurer returns according to the calculated expedition outcome.
- [ ] The player receives appropriate rewards and feedback.
- [ ] The first adventurer or another community member contributes knowledge, a technique, material, or blueprint.
- [ ] Player can complete a second customer commission for the target slice.
- [ ] The vertical slice ends with a complete and understandable experience.

**Final acceptance test:** A player unfamiliar with the game can receive a commission, select a blueprint and cloth, arrange and cut the fabric, create a custom canting inscription, dye and reveal the pattern, stitch the panels into a garment, deliver it to an adventurer, observe its consequences during an expedition, and receive the adventurer's return.

The complete sequence must be playable from beginning to end without developer intervention.

# 15. Primary Development Risks and Validation

The following systems should be prototyped and validated before extensive content production.

| Risk | Why it matters | Validation |
|---|---|---|
| Blueprint and pattern data | Establishes the reusable foundation for garment types | Create one blueprint with multiple pre-authored fabric panels and seam definitions |
| Fabric layout and arrangement | Must offer a real decision rather than a mandatory placement chore | Test arranging pattern pieces on a finite fabric sheet and evaluate material use or orientation |
| Cutting and panel geometry | Determines whether players can customize shapes without unrestricted mesh editing | Test constrained 2D boundary handles and generate the resulting panels |
| UV and panel mapping | Keeps fabric appearance and inscription placement consistent | Map a custom texture and inscription onto a panel and preserve it through assembly |
| Seam-based assembly | Connects individual panels into a coherent 3D garment | Test matching seam relationships and generating an assembled garment |
| Canting input | Determines whether freeform inscription is enjoyable and usable | Build the 2D drawing interaction on flat panels |
| Inscription interpretation | Determines whether custom batik designs produce functional spells | Convert stroke geometry into structured spell data and test different outcomes |
| Spell feedback and balance | Players must understand why an inscription works or fails | Test valid and invalid constructions with immediate feedback |
| Procedural magical rendering | Connects the player's actual design to its magical manifestation | Generate animated effects from custom inscription data |
| Global action economy | Ensures time advances consistently across all workstations | Test committed actions, free inspection, and a scheduled event across the global timeline |
| Expedition simulation | Determines whether garments meaningfully affect outcomes | Test different garments against identical expedition conditions |
| Expedition event visualization | Must communicate calculated results without becoming a separate combat system | Convert one simulated encounter into a complete replay |
| 2D/3D visual coherence | Ensures workshop, garment preview, and expedition scenes share a visual identity | Build one representative workshop, garment preview, and expedition scene |
| Batik representation | Ensures the cultural foundation is treated responsibly | Research traditional processes and motifs and consult knowledgeable practitioners where possible |

Development should validate the complete relationship between fabric construction, inscription, garment assembly, and expedition outcomes before expanding content quantity.

# 16. Open Design Decisions

The following decisions are intentionally unresolved and should be settled through design discussion or prototyping rather than silently assumed.

1. **Workshop navigation:** Fixed-camera point-and-click interaction or a walkable 3D workshop in the full game.
2. **Arrangement model:** How much freedom the player has when fitting pattern pieces onto the fabric sheet, and which consequences matter most: waste, orientation, motif placement, grain, or other fabric properties.
3. **Cutting interaction:** The final interaction model for editing panel boundaries, such as handles, constrained points, or other tools.
4. **Canting interpretation:** Whether the player selects or learns an intended magical effect family before drawing, or whether the inscription system derives more of the effect directly from its structure.
5. **Spell grammar:** Which geometric properties are mechanically meaningful and how players learn those rules.
6. **Dye significance:** Whether dye is primarily visual or also affects material and magical properties.
7. **Stitching interaction:** Whether stitching is a confirmation and animation, includes a choice of stitch technique, or has further meaningful player input.
8. **Time consequences:** Which scheduled events and availability rules make the global action economy meaningful without turning the game into a conventional deadline-management sim.
9. **Expedition consequences:** Which outcomes, including injury, retreat, equipment damage, or death, are appropriate for the intended tone and vertical slice.
10. **Cultural consultation:** Which real batik processes, motifs, and techniques are represented and how their cultural context is communicated.

# 17. Success Criteria

The vertical slice succeeds if it demonstrates the following:

**1. The craft is engaging.**  
Players find the sequence of fabric preparation, cutting, arrangement, canting, dyeing, and stitching satisfying enough to engage with beyond simply selecting a result.

**2. Creative choices matter.**  
Different fabric, layout, cut, and inscription decisions produce understandable differences in the finished garment.

**3. The inscription is meaningful.**  
Players can create visually distinct inscriptions and understand how their construction affects magical functionality.

**4. Craftsmanship has consequences.**  
Different garments and inscriptions produce meaningfully different expedition outcomes.

**5. The themes are present in play.**  
Preservation is expressed through craft and knowledge, cooperation through contributions and exchange, and homecoming through adventurers returning with the consequences of their journeys.

**6. The experience is cohesive.**  
The transition from commission to garment construction, expedition, and return feels like one complete game rather than several disconnected prototypes.

**The central experience to validate is:**

*I made this. Someone wore it. It mattered. And they came home.*
