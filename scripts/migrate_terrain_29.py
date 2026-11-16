#!/usr/bin/env python3
"""Idempotent level-source migration for v2.10.29 (keeps authored objects)."""
import argparse
import json
import math
import shutil
from pathlib import Path

REVISION = 29
CLEARANCE = 47.0  # 39px crouch collision capsule + 8px breathing room


def overlaps(a, b):
    return min(a[0] + a[2], b[0] + b[2]) - max(a[0], b[0])


def upgrade(d):
    if d.get('terrainRevision', 0) >= REVISION:
        return False
    ground, ledges = d['ground'], d['ledges']
    original = [r[:] for r in ledges]
    # Raise undersides of overlapping upper catwalks, never lower their decks.
    # Each pass also re-evaluates upper/upper clearances affected by its neighbor.
    for _ in range(12):
        changed = False
        for index, high in enumerate(ledges):
            limit = high[1]
            for i, low in enumerate(ground + ledges):
                if i == len(ground) + index or overlaps(high, low) < 36:
                    continue
                gap = low[1] - (high[1] + high[3])
                if high[1] < low[1] and 0 <= gap < CLEARANCE:
                    limit = min(limit, low[1] - high[3] - CLEARANCE)
            if limit < high[1] - .001:
                high[1] = round(limit, 4)
                changed = True
        if not changed:
            break
    # Keep authored stair transitions and above-deck pickups on the moved deck.
    for stair in d.get('stairs', []):
        for at in (0, 2):
            x, y = stair[at:at + 2]
            candidates = [(abs(y - old[1]), j) for j, old in enumerate(original)
                          if old[0] - 6 <= x <= old[0] + old[2] + 6 and
                          abs(y - old[1]) < 2.5 and abs(ledges[j][1] - old[1]) > .01]
            if candidates:
                _, idx = min(candidates)
                stair[at + 1] = round(y + ledges[idx][1] - original[idx][1], 4)
    for field, xi, yi in (('enemies', 1, 2), ('supplies', 1, 2), ('crates', 1, 2),
                          ('beacons', 0, 1)):
        for obj in d.get(field, []):
            x, y = obj[xi], obj[yi]
            candidates = [(abs(y - old[1]), j) for j, old in enumerate(original)
                          if old[0] <= x <= old[0] + old[2] and abs(y - old[1]) < 2.5 and
                          abs(ledges[j][1] - old[1]) > .01]
            if candidates:
                _, idx = min(candidates)
                obj[yi] = round(y + ledges[idx][1] - original[idx][1], 4)
    # Put three widely separated ladders where an elevated ledge and the lower
    # floor really share an X coordinate. No invisible/teleporting ladders.
    ladders = d.setdefault('ladders', [])
    candidates = []
    for ledge in ledges:
        for floor in ground:
            left = max(ledge[0], floor[0])
            right = min(ledge[0] + ledge[2], floor[0] + floor[2])
            height = floor[1] - ledge[1]
            if right - left < 54 or not (70 <= height <= 230):
                continue
            x = round((left + right) * .5, 3)
            if any(r[0] - 16 <= x <= r[0] + r[2] + 16 and
                   r[1] < floor[1] and r[1] + r[3] > ledge[1] + 12
                   for r in d.get('cover', [])):
                continue
            candidates.append((x, ledge[1], floor[1]))
    candidates.sort()
    extent = d['worldWidth']
    for fraction in (.22, .50, .79):
        eligible = [c for c in candidates if all(abs(c[0] - l[0]) > 180 for l in ladders)]
        if eligible:
            x, top, bottom = min(eligible, key=lambda c: abs(c[0] - extent * fraction))
            ladders.append([round(x, 4), round(top, 4), round(bottom, 4)])
    lifts = d['lifts']
    if d['cooperative']:
        # A new automatic vertical access lift well away from relay shutters.
        floor = ground[0][1]
        upper = min((r[1] for r in ledges if r[0] <= 560 <= r[0] + r[2]), default=floor - 145)
        lifts.append([540., round((floor + upper) / 2, 3), 70., 0.,
                      round((upper - floor) * .53, 3), 4.2, 0.0])
    else:
        # Service trench BELOW the main terrain baseline, accessed from a gap.
        # The shaft is open air: unlike tunneling through a solid ground rect.
        pair = None
        for left, right in zip(ground, ground[1:]):
            gap = right[0] - left[0] - left[2]
            if gap < 85:
                continue
            middle = (right[0] + left[0] + left[2]) * .5
            rating = (abs(middle - extent * .47), -gap)
            if pair is None or rating < pair[0]:
                pair = (rating, left, right, gap)
        if pair is not None:
            _, left, right, gap = pair
            edge = left[0] + left[2]
            deck_y = round(max(left[1], right[1]) + 126., 3)
            deck_x = round(edge + 9, 3)
            deck_w = round(gap - 18, 3)
            # Landing lip at the exact height of the source floor. A player can
            # reach the ladder from the floor without a pixel-perfect jump.
            landing = [round(edge - 28, 3), left[1], round(min(96, gap * .73), 3), 17.]
            ledges.append(landing)
            ledges.append([deck_x, deck_y, deck_w, 16.])
            ladder_x = round(edge + min(39, gap * .37), 3)
            ladders.append([ladder_x, left[1], deck_y])
            # The lift continuously goes up and down, covering the vertical
            # connection beside the ladder. Its deck remains one-way collision.
            lift_x = round(edge + max(38, gap * .53), 3)
            if lift_x + 51 > right[0] - 7:
                lift_x = round(right[0] - 58, 3)
            lifts.append([lift_x, round((deck_y + left[1]) * .5, 3), 50.,
                          0., round((left[1] - deck_y) * .5, 3), 4.5, .0])
    # The newly constructed access lip can uncover a narrow gap beneath a
    # pre-existing elevated platform. Re-run clearance on the FINAL geometry,
    # and preserve authored stair/ladder anchors a second time.
    before_final = [r[:] for r in ledges]
    for _ in range(12):
        changed = False
        for index, high in enumerate(ledges):
            # The last two surfaces are the engineered below-baseline shaft;
            # they must remain tied to their source-floor and ladder heights.
            if not d['cooperative'] and index >= len(ledges) - 2:
                continue
            limit = high[1]
            for i, low in enumerate(ground + ledges):
                if i == len(ground) + index or overlaps(high, low) < 36:
                    continue
                gap = low[1] - high[1] - high[3]
                if high[1] < low[1] and 0 <= gap < CLEARANCE:
                    limit = min(limit, low[1] - high[3] - CLEARANCE)
            if limit < high[1] - .001:
                high[1] = round(limit, 4)
                changed = True
        if not changed:
            break
    def adjust_endpoint(obj, xi, yi):
        x, y = obj[xi], obj[yi]
        candidates = [(abs(y - old[1]), j) for j, old in enumerate(before_final)
                      if old[0] - 6 <= x <= old[0] + old[2] + 6 and
                      abs(y - old[1]) < 2.5 and abs(ledges[j][1] - old[1]) > .01]
        if candidates:
            _, idx = min(candidates)
            obj[yi] = round(y + ledges[idx][1] - before_final[idx][1], 4)
    for stair in d.get('stairs', []):
        adjust_endpoint(stair, 0, 1)
        adjust_endpoint(stair, 2, 3)
    for ladder in ladders:
        adjust_endpoint(ladder, 0, 1)
    for field, xi, yi in (('enemies', 1, 2), ('supplies', 1, 2), ('crates', 1, 2),
                          ('beacons', 0, 1)):
        for obj in d.get(field, []):
            adjust_endpoint(obj, xi, yi)
    # If taller clearance required raising a stair's upper landing, extend
    # the ramp in X to keep it valid and human-walkable (slope <= 0.85).
    for stair in d.get('stairs', []):
        x0, y0, x1, y1 = stair
        required = abs(y1 - y0) / .84
        missing = required - (x1 - x0)
        if missing <= 0:
            continue
        def landing(x, y):
            supports = [r for r in ground + ledges
                        if r[0] - 3 <= x <= r[0] + r[2] + 3 and
                        abs(y - r[1]) < 2.5]
            return max(supports, key=lambda r: r[2]) if supports else None
        beginning, ending = landing(x0, y0), landing(x1, y1)
        if beginning and ending:
            left = min(missing, max(0., x0 - beginning[0] - 27.))
            right = min(missing - left, max(0., ending[0] + ending[2] - 27. - x1))
            stair[0] = round(x0 - left, 4)
            stair[2] = round(x1 + right, 4)
    d['terrainRevision'] = REVISION
    return True


def migrate(directory, backup=None):
    count = 0
    for file in sorted(Path(directory).glob('*.vllevel.json')):
        doc = json.loads(file.read_text())
        if not upgrade(doc):
            continue
        if backup:
            target = Path(backup) / file.name
            target.parent.mkdir(parents=True, exist_ok=True)
            if not target.exists():
                shutil.copy2(file, target)
        file.write_text(json.dumps(doc, indent=2, ensure_ascii=False) + '\n')
        count += 1
    return count


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('levels', type=Path)
    parser.add_argument('--backup', type=Path)
    args = parser.parse_args()
    print(f'VOIDLINE terrain v2.10.29: migrated {migrate(args.levels, args.backup)} level(s)')
