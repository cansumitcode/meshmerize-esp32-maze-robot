"""Validate circuit definitions and the project's frozen GPIO map (stdlib only)."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
diagram = json.loads((ROOT / "diagram.json").read_text(encoding="utf-8-sig"))
assert diagram["version"] == 1
parts = diagram["parts"]
ids = [p["id"] for p in parts]
assert len(ids) == len(set(ids)), "Duplicate component IDs"
definitions = {}
for file in ROOT.glob("*.chip.json"):
    model = json.loads(file.read_text(encoding="utf-8-sig"))
    pins = [p for p in model["pins"] if p]
    assert len(pins) == len(set(pins)), f"Duplicate pins: {file.name}"
    assert file.with_suffix(".c").exists(), f"Missing implementation: {file.name}"
    definitions["chip-" + file.name.removesuffix(".chip.json")] = set(pins)
assert len(definitions) == 4, "Expected four custom component types"
definitions.update({
    "wokwi-esp32-devkit-v1": set("VIN GND.1 GND.2 D13 D12 D14 D27 D26 D25 D33 D32 D35 D34 VN VP EN 3V3 D15 D2 D4 RX2 TX2 D5 D18 D19 D21 RX0 TX0 D22 D23".split()),
    "wokwi-pushbutton": {"1.l", "1.r", "2.l", "2.r"},
    "wokwi-slide-switch": {"1", "2", "3"},
    "wokwi-led": {"A", "C"},
    "wokwi-resistor": {"1", "2"},
    "wokwi-text": set(),
})
by_id = {p["id"]: p for p in parts}
edges = set()
for connection in diagram["connections"]:
    assert len(connection) == 4
    a, b, color, route = connection
    assert isinstance(route, list)
    for endpoint in (a, b):
        component, pin = endpoint.split(":", 1)
        if component == "$serialMonitor":
            assert pin in {"RX", "TX"}
            continue
        assert component in by_id, f"Unknown component: {component}"
        kind = by_id[component]["type"]
        assert pin in definitions[kind], f"Invalid pin: {endpoint}"
    edges.add(frozenset((a, b)))

def connected(a, b):
    assert frozenset((a, b)) in edges, f"Missing required wire {a} -> {b}"

for i, pin in enumerate(["VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26"], 1):
    connected(f"ir:D{i}", f"esp:{pin}")
for signal, pin in {"AIN1": "RX2", "AIN2": "TX2", "PWMA": "D18", "BIN1": "D19", "BIN2": "D21", "PWMB": "D22", "STBY": "D23"}.items():
    connected(f"driver:{signal}", f"esp:{pin}")
for a, b in [("driver:AO1", "leftMotor:M1"), ("driver:AO2", "leftMotor:M2"),
             ("driver:BO1", "rightMotor:M1"), ("driver:BO2", "rightMotor:M2"),
             ("esp:3V3", "driver:VCC"), ("supply:OUT5V", "driver:VM"),
             ("esp:3V3", "ir:VCC"), ("esp:3V3", "ir:IR"),
             ("esp:D23", "ir:TRIG"), ("esp:D13", "start:1.l"),
             ("esp:D14", "cal:1.l"), ("esp:D27", "rLed:1"),
             ("rLed:2", "finish:A"), ("finish:C", "esp:GND.1")]:
    connected(a, b)
for ground in ["ir:GND", "driver:GND", "supply:GND", "start:2.l", "cal:2.l"]:
    connected(ground, "esp:GND.1")
assert by_id["rLed"]["attrs"]["value"] == "220"
assert by_id["rightMotor"]["attrs"]["reverse"] == "1"
print(f"PASS: {len(parts)} diagram parts, {len(edges)} connections, 4 custom models; frozen GPIO map verified")
