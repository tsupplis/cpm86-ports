# StarTrek

CP/M-86 port of the classic 1970s text Star Trek game.

## Goal

Destroy all Klingon battle cruisers before the stardate deadline runs out.

## Setup

Enter any character as a random seed when prompted. The galaxy is an 8×8 grid
of quadrants, each subdivided into an 8×8 sector grid.

## Commands

| # | Command           | Description                                      |
|---|-------------------|--------------------------------------------------|
| 1 | Warp              | Navigate to another sector or quadrant           |
| 2 | Short range scan  | Redisplay the current sector map                 |
| 3 | Long range scan   | Show klingon/base/star counts in adjacent quadrants |
| 4 | Phasers           | Fire energy beams at all Klingons in sector      |
| 5 | Torpedo           | Fire a photon torpedo along a course             |
| 6 | Galactic records  | Show the known galaxy map                        |

Enter `?` or any non-number to list commands.

## Navigation

Course is a number **1–8.9** mapping to compass headings:

```
        8 / 1
       7     2
       6     3
        5 \ 4
```

Warp factor **0–12**. Warp > 0.2 requires undamaged warp engines.  
Energy cost = `warp × 8 × 2` units per move.

## Sector map

```
.  empty space
E  Enterprise (you)
K  Klingon
B  Starbase
*  Star
```

## Status display (right of sector map)

| Field              | Meaning                          |
|--------------------|----------------------------------|
| Years              | Time remaining                   |
| Stardate           | Current stardate                  |
| Condition          | GREEN / YELLOW / RED / DOCKED    |
| Quadrant           | Current quadrant (col-row, 1-8)  |
| Sector             | Position within quadrant         |
| Energy             | Available energy units           |
| Photon torpedoes   | Remaining torpedo count          |
| Klingons left      | Galaxy-wide total                |

**Condition** RED = Klingons present · YELLOW = energy < 10% · DOCKED = adjacent to starbase (repairs and resupply).

## Tips

- Dock at a starbase (move adjacent) to restore energy, torpedoes, and repair all damage.
- Phasers divide energy equally across all Klingons in the quadrant; hit strength falls off with distance.
- Torpedoes travel in a straight line and can destroy stars and starbases — be careful.
- Space storms occur randomly during warp and may damage or repair a subsystem.
- Damaged warp engines cap speed at 0.2; other damaged systems disable their command.
