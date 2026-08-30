#!/usr/bin/env python3
"""Executable model for R37N8's atomic display/geometry ownership contract."""
from dataclasses import dataclass

@dataclass
class Geometry:
    source_frame: str | None = None
    width: int = 0
    height: int = 0
    xani: int = 0
    yani: int = 0
    iani3x: int = 0
    iani3y: int = 0
    iani3z: int = 0
    iani3id: int = 0
    valid: bool = False

SPRITES = {
    "A01": Geometry("A01", 84, 122, 13, 7, -9, 4, 51, 108, True),
    "B02": Geometry("B02", 96, 131, 18, 11, -14, 3, 60, 117, True),
}

class Runtime:
    def __init__(self):
        self.current_frame = None
        self.geometry = Geometry()

    def apply(self, frame: str) -> bool:
        self.current_frame = frame
        self.geometry = Geometry(source_frame=frame)
        src = SPRITES.get(frame)
        if src is None:
            return False
        self.geometry = Geometry(**src.__dict__)
        return True

    def copy_geometry(self, frame: str) -> bool:
        self.geometry = Geometry(source_frame=frame)
        src = SPRITES.get(frame)
        if src is None:
            return False
        self.geometry = Geometry(**src.__dict__)
        return True

    def display_only_force(self, frame: str):
        self.current_frame = frame
        # Crucial safety property: never leave stale geometry owned by old frame.
        self.geometry = Geometry(source_frame=frame)


def collision_tuple(rt: Runtime):
    assert rt.geometry.valid
    g = rt.geometry
    return g.iani3x, g.iani3y, g.iani3z, g.iani3id


def main():
    rt = Runtime()
    assert rt.apply("A01")
    assert rt.current_frame == "A01"
    assert (rt.geometry.width, rt.geometry.height, rt.geometry.xani, rt.geometry.yani) == (84, 122, 13, 7)
    assert collision_tuple(rt) == (-9, 4, 51, 108)

    # Geometry may be borrowed from another frame without changing display identity.
    assert rt.copy_geometry("B02")
    assert rt.current_frame == "A01" and rt.geometry.source_frame == "B02"
    assert collision_tuple(rt) == (-14, 3, 60, 117)

    # Forced victim frame must atomically move visual identity and geometry owner.
    assert rt.apply("B02")
    assert rt.current_frame == "B02" and rt.geometry.source_frame == "B02"
    assert collision_tuple(rt) == (-14, 3, 60, 117)

    # A display-only compatibility force may not accidentally retain B02's box.
    rt.display_only_force("UNKNOWN")
    assert rt.current_frame == "UNKNOWN"
    assert not rt.geometry.valid
    assert rt.geometry.source_frame == "UNKNOWN"

    print("R37N8 frame geometry ownership model: PASS")

if __name__ == "__main__":
    main()
