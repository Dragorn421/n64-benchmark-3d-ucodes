import typing
from typing import TYPE_CHECKING

import bpy

if TYPE_CHECKING:

    @typing.overload
    def SCM(data: bpy.types.Material) -> "SCMMaterialProperties": ...


def SCM(data):
    return data.simple_common_materials


class SCMMaterialProperties(bpy.types.PropertyGroup):
    use_shade: bpy.props.BoolProperty(
        name="Shaded",
        description="Whether this material uses shading (vertex colors, lighting)",
    )
    use_image: bpy.props.BoolProperty(
        name="Textured",
        description=(
            "Whether this material uses an image texture. "
            "If not, use a plain color instead"
        ),
    )
    color: bpy.props.FloatVectorProperty(
        name="Color",
        default=(0.5, 0.5, 0.5, 1),
        subtype="COLOR",
        size=4,
        min=0,
        max=1,
    )
    image: bpy.props.PointerProperty(name="Texture", type=bpy.types.Image)
    alpha_blend: bpy.props.EnumProperty(
        name="Alpha Blend",
        description="Choose how the alpha affects the material",
        items=(
            ("OPAQUE", "Opaque", "Material is fully opaque"),
            (
                "CUTOUT",
                "Cutout",
                (
                    "Material is opaque with holes (fully transparent spots)"
                    " where the alpha is below threshold (e.g. fences)"
                ),
            ),
            (
                "TRANSPARENT",
                "Transparent",
                (
                    "Material is transparent, drawing some geometry that"
                    " can be seen through (e.g. colored glass)"
                ),
            ),
        ),
        default="OPAQUE",
    )


class SCMMaterialPanel(bpy.types.Panel):
    bl_idname = "MATERIAL_PT_simple_common_materials"
    bl_label = "Simple Common Materials"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "material"

    @classmethod
    def poll(cls, context):
        return context.material is not None

    def draw(self, context):
        layout = self.layout
        assert layout is not None
        assert context.material is not None
        mp = SCM(context.material)
        layout.prop(mp, "use_shade")
        layout.prop(mp, "use_image")
        if mp.use_image:
            layout.template_ID(mp, "image", new="image.new", open="image.open")
        else:
            layout.prop(mp, "color")
        layout.prop(mp, "alpha_blend")
        layout.separator(type="LINE")
        layout.operator(SCMToDragExOperator.bl_idname, text="To DragEx")


class SCMToDragExOperator(bpy.types.Operator):
    bl_idname = "simple_common_materials.simple_common_materials_to_dragex"
    bl_label = "Simple Common Materials to DragEx"

    def execute(self, context):  # type: ignore
        for material in bpy.data.materials:
            with context.temp_override(material=material):
                bpy.ops.dragex.set_material_mode(mode="BASIC")
                mp = SCM(material)
                material.dragex.modes.basic.texture = mp.image if mp.use_image else None
                material.dragex.modes.basic.shading = (
                    "LIGHTING" if mp.use_shade else "NONE"
                )
                material.dragex.modes.basic.alpha_blend = mp.alpha_blend
                material.dragex.modes.basic.fog = False
                if not mp.use_image:
                    bpy.ops.dragex.set_material_mode(mode="FULL")
                    material.dragex.rdp.vals.primitive_color = mp.color
                    if mp.use_shade:
                        material.dragex.rdp.combiner.rgb_A_0 = "PRIMITIVE"
                        material.dragex.rdp.combiner.rgb_B_0 = "0"
                        material.dragex.rdp.combiner.rgb_C_0 = "SHADE"
                        material.dragex.rdp.combiner.rgb_D_0 = "0"
                    else:
                        material.dragex.rdp.combiner.rgb_A_0 = "0"
                        material.dragex.rdp.combiner.rgb_B_0 = "0"
                        material.dragex.rdp.combiner.rgb_C_0 = "0"
                        material.dragex.rdp.combiner.rgb_D_0 = "PRIMITIVE"
                    if material.dragex.rdp.other_modes.cycle_type == "1CYCLE":
                        material.dragex.rdp.combiner.rgb_A_1 = (
                            material.dragex.rdp.combiner.rgb_A_0
                        )
                        material.dragex.rdp.combiner.rgb_B_1 = (
                            material.dragex.rdp.combiner.rgb_B_0
                        )
                        material.dragex.rdp.combiner.rgb_C_1 = (
                            material.dragex.rdp.combiner.rgb_C_0
                        )
                        material.dragex.rdp.combiner.rgb_D_1 = (
                            material.dragex.rdp.combiner.rgb_D_0
                        )
                material.update_tag()
        return {"FINISHED"}


classes = (
    SCMMaterialProperties,
    SCMToDragExOperator,
    SCMMaterialPanel,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.Material.simple_common_materials = bpy.props.PointerProperty(
        type=SCMMaterialProperties
    )


def unregister():
    del bpy.types.Material.simple_common_materials
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
