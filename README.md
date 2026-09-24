# SpaceRace_01

A space-flight racing prototype built with Unreal Engine 5.8 (C++). The player pilots a
freely-moving spaceship through hand-built levels, passing a sequence of checkpoints against the
clock while dealing with planetary gravity.

## Gameplay overview

- Free 6-DOF flight: forward/backward, lateral and vertical thrust, plus pitch/yaw/roll.
- Planetary gravity pulls the ship toward nearby "Planet"-tagged actors.
- An optional flight assistant mode can auto-correct sideways/vertical drift while flying forward,
  without touching forward speed or rotation.
- Checkpoints are placed by hand in a level and must be reached in order; reaching the last one
  ends the race after a short delay.
- A cockpit HUD (built entirely in C++, no Blueprint UI) shows velocity, fuel, gravity status,
  checkpoint/planet distances and elapsed time.
- A Niagara particle effect kicks in above a configurable speed to sell the sense of motion.
- A main menu lets the player pick from any level in the project whose name starts with `LVL-`.

## Project structure

```
Source/SpaceRace_01/
  SpaceshipPawn.h / .cpp              Player-controlled ship: flight physics, gravity, collision
                                       response, engine/collision audio, space-speed particle
                                       effect, cockpit widget lifecycle.
  SpaceRaceGameMode.h / .cpp          Per-level race flow: discovers checkpoints, steps through
                                       them in order, ends the race.
  SpaceRaceGameInstance.h / .cpp      Persistent game-level controller: level discovery, showing
                                       the main menu, starting/returning from a race.
  SpaceRaceCheckpointComponent.h/.cpp Per-checkpoint data (speech text) and cosmetic spin, added
                                       to hand-placed checkpoint actors.
  CockpitDisplayWidget.h / .cpp       In-C++ HUD widget (velocity, fuel, gravity, checkpoint and
                                       planet distances, elapsed time).
  SpaceRaceMainMenuWidget.h / .cpp    Main menu widget: lists available race levels, starts one.
```

Blueprints (`BP_Spaceship`, checkpoint actors, the main menu level, etc.) sit on top of these C++
classes purely for asset assignment (meshes, sounds, the Niagara system) and level placement — all
gameplay logic lives in C++.

## Key gameplay systems

### Flight model (`ASpaceshipPawn`)

Thrust (forward/backward, lateral, vertical) and torque (pitch/yaw/roll) are applied directly to a
velocity/angular-velocity state kept on the pawn, with configurable damping, speed caps and thrust
strength — all exposed as `EditAnywhere` properties.

`EFlightAssistantMode` governs how much automatic correction is layered on top:
- `None` — fully manual, no damping/clamping/auto-alignment.
- `PureForward` (default) — same manual physics, but while the ship is thrusting forward/backward,
  lateral and vertical drift are actively countered back toward zero using the existing thrusters;
  forward speed itself is never touched, and any axis the player is actively steering on
  (A/D, Space/Ctrl) is left to manual control.

### Gravity

Actors tagged `Planet` are found once at `BeginPlay` and exert an inverse-square gravitational pull
(`GravityConstant * PlanetMass / DistanceSquared`) on the ship, accumulated separately from
thrust-driven velocity so damping never bleeds off gravity-induced motion. Planet mass is computed
from each planet's static mesh body setup.

### Collision

The ship sweeps for collisions. On a blocking hit it resets to the last known safe transform,
inverts and reduces velocity (`TranslationBounceFactor`), zeroes angular velocity, and plays a
cooldown-debounced collision sound.

### Checkpoints

`ASpaceRaceGameMode` finds every actor with a `USpaceRaceCheckpointComponent`, sorts them by the
trailing number in their (editor) actor name (`Checkpoint00`, `Checkpoint01`, ...), and activates
them one at a time; only the active checkpoint has collision/overlap enabled. The ship reports
overlaps back to the game mode, which shows the checkpoint's `SpeechText` on the cockpit HUD,
advances to the next checkpoint, and — after the last one — ends the race following
`FinishDelaySeconds`.

### Space-speed particle effect

`SpaceSpeedNiagaraComponent` (a Niagara System, assigned in `BP_Spaceship`) runs in the emitter's
own local space and is driven purely by the ship's world velocity, transformed into the
component's local space and passed in as `User.ShipVelocity`; it activates only once total speed
exceeds `SpaceSpeedEffectMinSpeed`. No world-space position/rotation overrides are done in C++ —
Niagara's own Add Velocity and Velocity-Aligned sprite rendering handle the rest.

### Main menu / level flow

`USpaceRaceGameInstance` persists across level loads. It scans a configured content directory for
maps whose asset name starts with `LVL-` (the prefix is stripped for display), shows the main menu
whenever the main menu map is the current world, and loads a chosen race level on request.

## Building / running

- Compile via Unreal Editor's Live Coding (Ctrl+Alt+F11) for iterative C++ changes while the editor
  is open, or a normal build (`Build.bat` / Visual Studio) when the editor is closed.
- Required engine modules: `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `UMG`,
  `Niagara` (see `Source/SpaceRace_01/SpaceRace_01.Build.cs`).
- Input uses Unreal's Enhanced Input system (`IMC_Spaceship` mapping context).

## Status

Core flight, gravity, collision, checkpoints, cockpit HUD, main menu/level flow and the space-speed
particle effect are implemented and working. Fuel is tracked but not yet consumed as a hard
gameplay constraint; further flight-assistant modes (velocity-hold, docking assist, ...) and
race results/progression are anticipated future additions.
