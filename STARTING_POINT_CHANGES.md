# Starting Point Changes (Lab 8 to Assessment 3 base project)

This records every change made to the Lab 8 starting project (`AGP/`, UE 5.6.1) to turn it into the shared base for Assessment 3. Both group members start from this cleaned project, then build their own sub-system separately and merge by hand in Assessment 4.

The original `Source/`, `Config/` and `AGP.uproject` are backed up in `Assignment-3/_backup_Lab8_original/`.

Rule for the base: remove what the stealth extraction game does not need, and fix defects that would otherwise affect either sub-system. Do not add features here.

---

## 1. Summary

| Area | Action |
|---|---|
| Weapon pickups and pickup manager | Deleted |
| Pickup bounce and rotator components | Deleted |
| Procedural landscape | Deleted |
| Game instance | Deleted |
| Weapon rarity and weapon equipping | Removed; replaced with one fixed weapon |
| Player weapon | Player is unarmed |
| A* pathfinding | Bug fixes only (no new behaviour) |
| Navigation node | Tick disabled |
| Project config and build file | Updated to match |
| Unused levels and materials | Deleted |
| Enemy AI (`EnemyCharacter`) | **Not changed** |

---

## 2. C++ changes

### 2.1 Deleted

| Files | Reason |
|---|---|
| `Source/AGP/Pickups/` (`PickupBase`, `WeaponPickup`, `PickupManagerSubsystem`) | The game has no weapon pickups. Weapon pickups only existed to hand out random-rarity weapons, which is removed (see 2.2). The pickup manager spawned them on a timer at navigation nodes. |
| `Source/AGP/Components/PickupBounceComponent.*`, `PickupRotatorComponent.*` | Visual-only components used by the weapon pickup. Nothing else used them. |
| `Source/AGP/Landscape/ProceduralLandscape.*` | Terrain generator tied to the old outdoor map. Replaced by the building generator (Assessment 3 PCG). It also placed navigation nodes on the terrain vertices, which no longer applies. |
| `Source/AGP/AGPGameInstance.*` | Its only content was the `WeaponPickupClass` property used by the pickup manager. Nothing else depended on it. Can be re-added in Assessment 4 if a game instance is needed (e.g. for sessions). |

Note: the objective item in the final game will need an interactable pickup. `PickupBase` (box collider plus overlap event) was deleted along with the rest, so that will be written fresh when needed.

### 2.2 `Components/WeaponComponent.h` / `.cpp`: fixed weapon stats

What changed:
- Removed `EWeaponRarity` (forward declaration and member), `EWeaponType`, and `SetWeaponStats()`.
- `FWeaponStats` now has editable properties (`UPROPERTY(EditAnywhere, BlueprintReadOnly)`) with fixed defaults: accuracy 0.95, fire rate 0.5s, damage 10, magazine 10, reload 2s.
- `WeaponStats` on the component is `UPROPERTY(EditAnywhere)` (it was `VisibleAnywhere`).
- `BeginPlay()` now fills the magazine (`RoundsRemainingInMagazine = MagazineSize`).
- Added default initialisers for `RoundsRemainingInMagazine`, `TimeSinceLastShot`, `ReloadTimer`.

Why:
- The game does not have random weapon loot, so rarity and per-pickup random stats are unnecessary. A single fixed stat block is simpler, predictable for AI testing, and easy to replicate in multiplayer.
- Rarity existed only to decide whether a new pickup could replace the current weapon (`SetWeaponStats`). With no pickups, that logic has no purpose.
- Fixed existing defects: `WeaponRarity` was uninitialised, so the rarity comparison read an indeterminate value; `RoundsRemainingInMagazine` was uninitialised, so characters started with a garbage ammo count until a reload; `FWeaponStats` fields were not `UPROPERTY`, so they could not be tuned in the editor and were not visible to the garbage collector or serialisation.

### 2.3 `Characters/BaseCharacter.h` / `.cpp`: weapon created by default

What changed:
- The `UWeaponComponent` is now created in the constructor with `CreateDefaultSubobject` (it was `nullptr` and created at runtime with `NewObject` when a pickup equipped one).
- Removed `EquipWeapon()` and `SetWeaponVisibility()` (a `BlueprintImplementableEvent`).
- Removed the `EquipWeapon(HasWeapon(), FWeaponStats())` call in `BeginPlay`.
- Added `UPROPERTY(EditDefaultsOnly) bool bHasWeapon = true;`.
- `HasWeapon()` is now `const` and returns `bHasWeapon && WeaponComponent != nullptr`.
- Small tidy: `UHealthComponent::UHealthComponent::HandleApplyDamage` corrected to `UHealthComponent::HandleApplyDamage`.

Why:
- `HasWeapon()` used to mean "has picked up a weapon". With no pickups it would always be false, which meant enemies could never fire (their Engage state checks `HasWeapon()`). The weapon now exists from construction.
- `bHasWeapon` keeps armed/unarmed as a per-character setting: enemies are armed, the player is not (2.4).
- `SetWeaponVisibility` existed only to toggle a weapon mesh on equip. With no equipping it has no caller. Removing a Blueprint event also reduces Blueprint dependence, which the assessment marks.
- Components created with `CreateDefaultSubobject` are the standard Unreal pattern and are replication-friendly, unlike a runtime `NewObject`, which matters for Assessment 4.

### 2.4 `Characters/PlayerCharacter.cpp`: player unarmed

What changed: the constructor sets `bHasWeapon = false`.

Why: the player has no gun in this game (a melee knife is planned later). `Fire` and `Reload` in `BaseCharacter` already return early when `HasWeapon()` is false, so firing and reload inputs are inert.

### 2.5 `Pathfinding/PathfindingSubsystem.h` / `.cpp`: A* bug fixes

These are fixes to the lab's A*, not new features. The lab A* is provided starting code and is not claimed as original work.

| Change | Why |
|---|---|
| `FAStarNodeData` changed from a `USTRUCT()` with a `UPROPERTY` (declared in a `.cpp`) to a plain struct | Unreal Header Tool only reads headers, so the macros were ignored and misleading. The data is per-search scratch data and never needs reflection. |
| `TMap<ANavigationNode*, FAStarNodeData*>` (with `new FAStarNodeData()` per node per search, never freed) changed to `TMap<ANavigationNode*, FAStarNodeData>` by value | Memory leak: every path request leaked one allocation per node. This would grow quickly with many guards repathing on a large generated grid. |
| `ReconstructPath` takes the map by `const&` (was by value, copying it) | Avoids copying the whole map on every path. Signature in the header updated to match. |
| `FindFurthestNode` initial distance changed from `FLT_MIN` to `-FLT_MAX` | `FLT_MIN` is the smallest positive float, not the most negative. It happened to work because distances are positive, but it was incorrect. |

Not changed: the A* algorithm itself (it still uses squared distance as cost and a linear open-set scan), `PlaceProceduralNodes` (left in place for now; it is tied to the removed landscape and will be replaced by the building generator's graph builder), and all public functions.

### 2.6 `Pathfinding/NavigationNode.cpp`

What changed: `PrimaryActorTick.bCanEverTick` set to `false`.

Why: the node's `Tick` is empty. A generated building will spawn many nodes, and every ticking actor has a per-frame cost.

### 2.7 `AGP.Build.cs`

What changed: removed `"ProceduralMeshComponent"` from the private dependencies.

Why: the only user was the deleted landscape. The building generator uses instanced static meshes instead.

### 2.8 Not changed

- `Characters/EnemyCharacter.*`: the placeholder FSM (Patrol, Engage, Evade) is left for the AI sub-system owner to replace. Note its Patrol state currently walks to random nodes, and its perception senses are configured in `BP_EnemyCharacter`.
- `Components/HealthComponent.*`, `Characters/PlayerCharacter` input handling, `AGPGameModeBase`.

---

## 3. Config changes

### `Config/DefaultEngine.ini`
- `GameDefaultMap` and `EditorStartupMap` now point to `/Game/Levels/ShooterMap.ShooterMap` (they pointed to `ProceduralMap`, which was deleted). `ShooterMap` is kept as the shared placeholder test level.
- Removed `GameInstanceClass=/Game/Blueprints/BP_AGPGameInstance...` because the game instance class and Blueprint were deleted.
- Removed the `[/Script/AndroidFileServerEditor...]` block. It is Android deployment tooling with a security token that has nothing to do with this project.

---

## 4. Content changes (done in the editor)

| Asset | Why |
|---|---|
| `Blueprints/BP_WeaponPickup`, `BP_PickupRotatorComponent` (and a stray copy in `Content/`), `BP_AGPGameInstance` | Their C++ parents were deleted. |
| `Materials/WeaponPickup/` (rarity materials), `GreenMaterial`, `RedMaterial` | Used only by the weapon pickup and old maps. |
| `Materials/Landscape/` (material, instance, textures) | Used only by the procedural landscape. |
| `Levels/ProceduralMap` | Level for the deleted landscape. |
| `Levels/TestingMap` (plus its HLOD instancing asset and its `__ExternalActors__` / `__ExternalObjects__` files) | Old lab test level, not needed. |
| Weapon-visibility logic in `BP_PlayerCharacter` and `BP_EnemyCharacter` | Its C++ event was removed (2.3). |

Kept: `Levels/ShooterMap` (shared placeholder test level), `BP_PlayerCharacter`, `BP_EnemyCharacter`, `ABP_Mannequin`, `ABP_FPArms`, the `Inputs/` assets, and the Lyra and FirstPerson art content.

---

## 5. Housekeeping deletions

Deleted generated or personal files: `.idea`, `.junie`, `*.DotSettings.user`, `AGP.sln`, `Binaries`, `Saved`, `DerivedDataCache`, `Intermediate`. Unreal and the IDE regenerate these. `Intermediate` and `Binaries` have since been regenerated by the build check and should be excluded when zipping.

---

## 6. Verification

- `AGPEditor Win64 Development` builds successfully (UE 5.6.1, VS 2022 toolchain, 0 errors).
- Not yet verified in the editor: that `BP_PlayerCharacter` and `BP_EnemyCharacter` compile cleanly and that no weapon mesh is permanently visible now that `SetWeaponVisibility` is gone.

## 7. Behaviour differences to expect

- Enemies in `ShooterMap` are now armed from the start (before, they never were), so they will shoot the player in Engage.
- The player cannot shoot.
- There are no weapon pickups and no landscape; `ShooterMap` is the only level.

## 8. Known issues left for later

- Lab A* uses squared distance as cost and scans the open set linearly; it will be slow on large generated grids.
- `ANavigationNode::BeginPlay` draws persistent debug lines and spheres for every node, which will be heavy on a large graph.
- `PlaceProceduralNodes` and `PopulateNodes` still assume nodes come from a vertex grid or are pre-placed in the level.
- The unused second weapon art set (Lyra pistol or rifle, `FirstPersonContent/Weapon`) has not been removed.
