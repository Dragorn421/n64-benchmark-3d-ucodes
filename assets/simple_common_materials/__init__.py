from pathlib import Path
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
        layout.operator(SCMToFast64Operator.bl_idname, text="To Fast64")


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


class SCMToFast64Operator(bpy.types.Operator):
    bl_idname = "simple_common_materials.simple_common_materials_to_fast64"
    bl_label = "Simple Common Materials to Fast64"

    def execute(self, context):  # type: ignore
        context.scene.gameEditorMode = "Homebrew"
        context.scene.f3d_type = "T3D"
        materials_to_convert = tuple(bpy.data.materials)
        # 1) Convert all materials to f3d materials.
        #    This is done by creating new materials and
        #    replacing the initial ones in all relevant slots
        replace_materials: dict[bpy.types.Material, bpy.types.Material] = {}
        temp_obj = None
        temp_mesh = None
        saved_active_obj = context.view_layer.objects.active
        try:
            temp_mesh = bpy.data.meshes.new("temp mesh")
            temp_obj = bpy.data.objects.new("temp obj", temp_mesh)
            context.view_layer.active_layer_collection.collection.objects.link(temp_obj)
            context.view_layer.objects.active = temp_obj
            for material in materials_to_convert:
                if not material.is_f3d:
                    existing_materials = set(bpy.data.materials)
                    bpy.ops.object.create_f3d_mat()
                    (new_material,) = set(bpy.data.materials) - existing_materials
                    new_material.name = material.name + " (F3D'd)"
                    replace_materials[material] = new_material
        finally:
            context.view_layer.objects.active = saved_active_obj
            if temp_obj is not None:
                bpy.data.objects.remove(temp_obj)
            if temp_mesh is not None:
                bpy.data.meshes.remove(temp_mesh)
        for obj in bpy.data.objects:
            for material_slot in obj.material_slots:
                if material_slot.material is not None:
                    material_slot.material = replace_materials.get(
                        material_slot.material, material_slot.material
                    )
        homebrew_presets_dir_p = Path(
            bpy.utils.user_resource(
                "SCRIPTS", path=str(Path("presets/f3d/homebrew")), create=True
            )
        )

        def f3d_preset_apply(stem):
            bpy.ops.material.f3d_preset_apply(
                filepath=str(homebrew_presets_dir_p / f"{stem}.py")
            )

        # Note this lowering could probably be improved,
        # for example by using 1-cycle instead of 2-cycle where possible.
        for material in materials_to_convert:
            mp = SCM(material)
            f3d_mat = replace_materials.get(material, material)
            with context.temp_override(material=f3d_mat):
                if mp.use_shade:
                    if mp.use_image:
                        if mp.alpha_blend == "OPAQUE":
                            f3d_preset_apply("shaded_texture")
                        elif mp.alpha_blend == "CUTOUT":
                            f3d_preset_apply("shaded_texture_cutout")
                        elif mp.alpha_blend == "TRANSPARENT":
                            f3d_preset_apply("shaded_texture_transparent")
                        else:
                            assert False
                    else:
                        if mp.alpha_blend == "OPAQUE":
                            f3d_preset_apply("shaded_solid")
                        elif mp.alpha_blend == "CUTOUT":
                            raise NotImplementedError("shaded_solid_cutout")
                        elif mp.alpha_blend == "TRANSPARENT":
                            f3d_preset_apply("shaded_solid_transparent")
                        else:
                            assert False
                else:
                    if mp.use_image:
                        if mp.alpha_blend == "OPAQUE":
                            f3d_preset_apply("unlit_texture")
                        elif mp.alpha_blend == "CUTOUT":
                            f3d_preset_apply("unlit_texture_cutout")
                        elif mp.alpha_blend == "TRANSPARENT":
                            f3d_preset_apply("unlit_texture_transparent")
                        else:
                            assert False
                    else:
                        if mp.alpha_blend == "OPAQUE":
                            f3d_preset_apply("shaded_solid")
                            f3d_mat.f3d_mat.combiner1.D = "1"
                            f3d_mat.f3d_mat.rdp_settings.g_lighting = False
                        elif mp.alpha_blend == "CUTOUT":
                            raise NotImplementedError("unlit_solid_cutout")
                        elif mp.alpha_blend == "TRANSPARENT":
                            raise NotImplementedError("unlit_solid_transparent")
                        else:
                            assert False
                if mp.use_image:
                    f3d_mat.f3d_mat.tex0.tex = mp.image
                else:
                    f3d_mat.f3d_mat.prim_color = mp.color
                material.update_tag()
        return {"FINISHED"}


classes = (
    SCMMaterialProperties,
    SCMToDragExOperator,
    SCMToFast64Operator,
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
