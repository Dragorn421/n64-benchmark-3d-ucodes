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

# Apparently the gltf exporter does not export evaluated objects (with applied modifiers)
# (it's optional, see export_apply argument)
bpy.ops.object.modifier_apply(modifier=decimate_mod.name)

bpy.ops.simple_common_materials.simple_common_materials_to_fast64()

bpy.ops.export_scene.gltf(
    filepath=args.filepath,
    check_existing=False,
    export_extras=True,
)
