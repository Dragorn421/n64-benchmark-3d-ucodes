import argparse
import sys

import bpy

args = sys.argv
try:
    i = args.index("--")
except ValueError:
    args = []
else:
    args = args[i + 1 :]

parser = argparse.ArgumentParser()
parser.add_argument("filepath")
parser.add_argument("decimate_ratio", type=float)
args = parser.parse_args(args)

obj = bpy.context.object
assert obj is not None
decimate_mod = obj.modifiers.new("Decimate", "DECIMATE")
assert isinstance(decimate_mod, bpy.types.DecimateModifier)
decimate_mod.decimate_type = "COLLAPSE"
decimate_mod.ratio = args.decimate_ratio
decimate_mod.use_collapse_triangulate = True

# DragEx currently does not apply modifiers on export (bad!), so apply it here
bpy.ops.object.modifier_apply(modifier=decimate_mod.name)

bpy.ops.simple_common_materials.simple_common_materials_to_dragex()

bpy.ops.dragex.oot_export_dlist(filepath=args.filepath)
