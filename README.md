# SMG2 KoopaJrShipReturns

## Bowser Jr.'s Airship in SMG2

This is the long awaited port of Bowser Jr.'s Airship to SMG2!

Below is a list of everything that's included:
- The ObjectData archives for Bowser Jr.'s Airship.
- The `KoopaJrShip` object for placing down in a level editor.
- JSON files for particle effects.

Below is a list of other things that you will need:
- A level editor.
- A particle editor with support for JSON files.
- The [CannonShellBase API module](https://github.com/SuperHackio/SMG2_CannonShellBase)
- ActionSound entries with all sound labels for KoopaJrShip and KoopaJr.
> *Note: Vanilla SMG2 already has ActionSound entries for KoopaJr, however this port adds one new ActionSound trigger, called `OjStarPieceHitEnmF`.*

## Credits
- @shibboleet - Decompilation.
- [@Super Hackio](https://github.com/SuperHackio) - Ported and enhanced SMG1 code to SMG2.

## Setup
It is adviced to reference the template mod for this port. You can use the following parameters:

### KoopaJrShip
| Parameter | Description |
| --- | --- |
| CommonPath_ID | Path ID. The path that KoopaJrShip will move back and forth on. |
| Obj_arg0 | Power Star ID. Spawns the assigned Power Star when defeated. |
| Obj_arg1 | 1st phase health. The amount of hits the Airship can take in the first phase. Default 3. |
| Obj_arg2 | 2nd phase health. The amount of hits the Airship can take in the second phase. Default 2. |

The appearance animation is controlled via a cutscene. Please reference the template mod for the setup.

At the end of each phase, KoopaJrShip can summon one of either `Kameck`, `KameckMeramera` or `KameckKuriboMini` via Generator Object ID. It will summon whichever is first in the object list.