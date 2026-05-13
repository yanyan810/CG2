bl_info = {
    "name": "Battle Action Exporter",
    "author": "4monthProject",
    "version": (1, 1, 0),
    "blender": (4, 0, 0),
    "location": "View3D > Sidebar > Battle Anime",
    "description": "Import battle animation JSON files and apply to armature",
    "category": "Animation",
}

import bpy
import json
import mathutils
import os


def load_json_to_action(context, obj, filepath, frame_offset, fps, scale, settings):
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception as exc:
        print(f"[BattleActionExporter] Failed to load {filepath}: {exc}")
        return

    node_anims = data.get("nodeAnimations", {})
    if not node_anims:
        return

    def get_val(klist, time, default):
        if not klist:
            return default

        last_v = klist[0]["value"]
        for key in klist:
            if key["time"] > time:
                break
            last_v = key["value"]
        return last_v

    all_times = set()
    for anim in node_anims.values():
        for ktype in ("translate", "rotate", "scale"):
            for key in anim.get(ktype, []):
                all_times.add(key["time"])

    pose_bones = obj.pose.bones

    def convert_rotation(value):
        ax, ay, az, aw = value[0], value[1], value[2], value[3]
        pattern = settings.quat_pattern

        if pattern == "P0":
            q = mathutils.Quaternion((aw, ax, ay, az))
        elif pattern == "P1":
            q = mathutils.Quaternion((aw, ax, -ay, -az))
        elif pattern == "P2":
            q = mathutils.Quaternion((aw, ax, -az, ay))
        elif pattern == "P3":
            q = mathutils.Quaternion((aw, -ax, -ay, -az))
        else:
            q = mathutils.Quaternion((aw, ax, ay, az))

        return q.normalized()

    def convert_translation(value):
        tx = -value[0]
        ty = value[1]
        tz = value[2]

        if settings.quat_pattern == "P2":
            return mathutils.Vector((tx, tz, -ty)) * scale

        return mathutils.Vector((tx, ty, tz)) * scale

    def make_local_matrix(anim, time):
        q = convert_rotation(get_val(anim.get("rotate"), time, [0, 0, 0, 1]))
        t = convert_translation(get_val(anim.get("translate"), time, [0, 0, 0]))
        s = get_val(anim.get("scale"), time, [1, 1, 1])

        loc = mathutils.Matrix.Translation(t)
        rot = q.to_matrix().to_4x4()
        scl = mathutils.Matrix.Diagonal((s[0], s[1], s[2], 1.0))
        return loc @ rot @ scl

    def rest_local_matrix(pose_bone):
        bone = pose_bone.bone
        if pose_bone.parent:
            return pose_bone.parent.bone.matrix_local.inverted() @ bone.matrix_local
        return bone.matrix_local.copy()

    for time in sorted(all_times):
        frame = (time * fps) + frame_offset

        for bone in pose_bones:
            anim = node_anims.get(bone.name)
            if not anim:
                continue

            bone.rotation_mode = "QUATERNION"
            local_matrix = make_local_matrix(anim, time)

            if settings.apply_mode == "REST_DELTA":
                # Engine JSON stores the target local SRT. Blender pose channels store
                # the delta from the armature rest pose, so remove the rest transform.
                bone.matrix_basis = rest_local_matrix(bone).inverted() @ local_matrix
            else:
                bone.location = local_matrix.to_translation()
                bone.rotation_quaternion = local_matrix.to_quaternion()
                bone.scale = local_matrix.to_scale()

            bone.keyframe_insert(data_path="location", frame=frame, group=bone.name)
            bone.keyframe_insert(data_path="rotation_quaternion", frame=frame, group=bone.name)
            bone.keyframe_insert(data_path="scale", frame=frame, group=bone.name)


def ensure_dir(filepath):
    directory = os.path.dirname(bpy.path.abspath(filepath))
    if directory:
        os.makedirs(directory, exist_ok=True)


def game_path(filepath):
    normalized = bpy.path.abspath(filepath).replace("\\", "/")
    marker = "/resources/"
    if marker in normalized:
        return "resources/" + normalized.split(marker, 1)[1]
    if normalized.startswith("resources/"):
        return normalized
    return filepath.replace("\\", "/")


def vec3(value):
    return [float(value[0]), float(value[1]), float(value[2])]


def export_dir(settings):
    return settings.sequence_export_dir or "resources/sequences"


def profile_name(settings):
    return settings.sequence_profile_name or "test_attack"


def sequence_profile_path(settings):
    return os.path.join(export_dir(settings), profile_name(settings) + ".json")


def sequence_camera_path(settings):
    return os.path.join(export_dir(settings), profile_name(settings) + "_camera.json")


def fallback_anim_path(settings, suffix):
    return os.path.join("resources", "CustomAnim", profile_name(settings) + suffix + ".json")


def player_attack_path(settings):
    return settings.sequence_player_anim_path or fallback_anim_path(settings, "_player_attack")


def enemy_damage_path(settings):
    return settings.sequence_enemy_anim_path or fallback_anim_path(settings, "_enemy_damage")


def sequence_map_path(settings):
    return os.path.join(export_dir(settings), "card_sequence_map.json")


def add_unique_name(target, name):
    if name and name not in target:
        target.append(name)


def sequence_effect_type(settings):
    if settings.sequence_map_type == "CUSTOM":
        return settings.sequence_custom_effect_type.strip()
    if settings.sequence_map_type in {"NONE", "CARD_USE"}:
        return ""
    return settings.sequence_map_type


def update_sequence_map_json(settings):
    if not settings.sequence_auto_register:
        return False

    map_type = settings.sequence_map_type
    card_id = settings.sequence_card_id
    if map_type == "NONE" and card_id <= 0:
        return False

    path = sequence_map_path(settings)
    abs_path = bpy.path.abspath(path)
    data = {
        "cardUse": [],
        "effects": {},
        "cards": {},
    }

    if os.path.exists(abs_path):
        try:
            with open(abs_path, "r", encoding="utf-8") as f:
                loaded = json.load(f)
            if isinstance(loaded, dict):
                data.update(loaded)
        except Exception as exc:
            print(f"[BattleActionExporter] Failed to read {path}: {exc}")

    if not isinstance(data.get("cardUse"), list):
        data["cardUse"] = []
    if not isinstance(data.get("effects"), dict):
        data["effects"] = {}
    if not isinstance(data.get("cards"), dict):
        data["cards"] = {}

    name = profile_name(settings)
    if map_type == "CARD_USE":
        add_unique_name(data["cardUse"], name)

    effect_type = sequence_effect_type(settings)
    if effect_type:
        effect_list = data["effects"].setdefault(effect_type, [])
        if isinstance(effect_list, list):
            add_unique_name(effect_list, name)
        else:
            data["effects"][effect_type] = [name]

    if card_id > 0:
        card_key = str(card_id)
        card_list = data["cards"].setdefault(card_key, [])
        if isinstance(card_list, list):
            add_unique_name(card_list, name)
        else:
            data["cards"][card_key] = [name]

    ensure_dir(path)
    with open(abs_path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=4)

    return True


def export_sequence_json(settings):
    profile_path = sequence_profile_path(settings)
    camera_path = sequence_camera_path(settings)
    data = {
        "cutinDuration": settings.sequence_cutin_duration,
        "cutinBgColor": [0.0, 0.0, 0.0, 0.7],
        "cutinLineColor": [1.0, 0.3, 0.1, 1.0],
        "enableCameraWork": settings.sequence_enable_camera_work,
        "approachDuration": settings.sequence_approach_duration,
        "attackWaitDuration": settings.sequence_attack_wait_duration,
        "returnDuration": settings.sequence_return_duration,
        "cameraPos": [0.0, 3.0, -10.0],
        "cameraRot": [0.2, 0.0, 0.0],
        "cameraFov": 0.8,
        "cameraAnimFile": game_path(camera_path),
        "playerPos": [0.0, 0.0, 0.0],
        "enemyPos": [0.0, 0.0, 5.0],
        "playerAttackAnim": game_path(player_attack_path(settings)),
        "playerAttackAnimStartTime": settings.sequence_player_anim_start_time,
        "enemyDamageAnim": game_path(enemy_damage_path(settings)),
        "enemyDamageAnimStartTime": settings.sequence_enemy_anim_start_time,
    }

    if settings.player_obj:
        data["playerPos"] = vec3(settings.player_obj.location)
    if settings.enemy_obj:
        data["enemyPos"] = vec3(settings.enemy_obj.location)
    if settings.camera_obj:
        pos, rot, fov = camera_state_to_engine(settings.camera_obj)
        data["cameraPos"] = pos
        data["cameraRot"] = rot
        data["cameraFov"] = fov

    ensure_dir(profile_path)
    with open(bpy.path.abspath(profile_path), "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=4)


def camera_state_to_engine(camera_obj):
    rotation = camera_obj.rotation_euler.to_matrix().to_euler("XYZ")
    fov = camera_obj.data.angle if camera_obj.type == "CAMERA" else 0.8
    return vec3(camera_obj.location), vec3(rotation), float(fov)


def collect_key_times(obj, frame_start, frame_end, action=None):
    times = {frame_start, frame_end}
    target_action = action
    if not target_action and obj and obj.animation_data:
        target_action = obj.animation_data.action
    if target_action:
        for fc in target_action.fcurves:
            for key in fc.keyframe_points:
                frame = int(round(key.co.x))
                if frame_start <= frame <= frame_end:
                    times.add(frame)
    return sorted(times)


def export_camera_json(context, settings):
    camera_obj = settings.camera_obj
    if not camera_obj or camera_obj.type != "CAMERA":
        return False

    scene = context.scene
    fps = scene.render.fps / scene.render.fps_base
    current_frame = scene.frame_current
    frames = collect_key_times(camera_obj, settings.sequence_frame_start, settings.sequence_frame_end)
    keyframes = []

    for frame in frames:
        scene.frame_set(frame)
        pos, rot, fov = camera_state_to_engine(camera_obj)
        keyframes.append({
            "time": float((frame - settings.sequence_frame_start) / fps),
            "pos": pos,
            "rot": rot,
            "fov": fov,
        })

    scene.frame_set(current_frame)

    camera_path = sequence_camera_path(settings)
    ensure_dir(camera_path)
    with open(bpy.path.abspath(camera_path), "w", encoding="utf-8") as f:
        json.dump({"loop": False, "keyframes": keyframes}, f, ensure_ascii=False, indent=4)

    return True


def export_armature_animation_json(context, obj, filepath, settings, action=None):
    if not obj or obj.type != "ARMATURE":
        return False

    scene = context.scene
    fps = scene.render.fps / scene.render.fps_base
    current_frame = scene.frame_current
    old_action = obj.animation_data.action if obj.animation_data else None
    if action:
        obj.animation_data_create()
        obj.animation_data.action = action
    frames = collect_key_times(obj, settings.sequence_frame_start, settings.sequence_frame_end, action)
    node_anims = {}

    def rest_local_matrix(pose_bone):
        bone = pose_bone.bone
        if pose_bone.parent:
            return pose_bone.parent.bone.matrix_local.inverted() @ bone.matrix_local
        return bone.matrix_local.copy()

    for pose_bone in obj.pose.bones:
        node_anims[pose_bone.name] = {
            "translate": [],
            "rotate": [],
            "scale": [],
        }

    for frame in frames:
        scene.frame_set(frame)
        time = float((frame - settings.sequence_frame_start) / fps)
        for pose_bone in obj.pose.bones:
            if settings.apply_mode == "REST_DELTA":
                local_matrix = rest_local_matrix(pose_bone) @ pose_bone.matrix_basis
            else:
                loc = mathutils.Matrix.Translation(pose_bone.location)
                rot = pose_bone.rotation_quaternion.to_matrix().to_4x4()
                scl = mathutils.Matrix.Diagonal((pose_bone.scale.x, pose_bone.scale.y, pose_bone.scale.z, 1.0))
                local_matrix = loc @ rot @ scl

            loc, rot, scale = local_matrix.decompose()
            rot.normalize()
            node_anim = node_anims[pose_bone.name]
            node_anim["translate"].append({
                "time": time,
                "value": [-float(loc.x), float(loc.y), float(loc.z)],
            })
            node_anim["rotate"].append({
                "time": time,
                "value": [float(rot.x), -float(rot.y), -float(rot.z), float(rot.w)],
            })
            node_anim["scale"].append({
                "time": time,
                "value": [float(scale.x), float(scale.y), float(scale.z)],
            })

    scene.frame_set(current_frame)
    if action and obj.animation_data:
        obj.animation_data.action = old_action

    data = {
        "duration": float((settings.sequence_frame_end - settings.sequence_frame_start) / fps),
        "nodeAnimations": node_anims,
    }

    ensure_dir(filepath)
    with open(bpy.path.abspath(filepath), "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=4)

    return True


class BattleAnimeSettings(bpy.types.PropertyGroup):
    player_obj: bpy.props.PointerProperty(name="Player", type=bpy.types.Object)
    enemy_obj: bpy.props.PointerProperty(name="Enemy", type=bpy.types.Object)
    camera_obj: bpy.props.PointerProperty(name="Camera", type=bpy.types.Object)
    player_idle_action: bpy.props.PointerProperty(name="Idle Action", type=bpy.types.Action)
    player_attack_action: bpy.props.PointerProperty(name="Attack Action", type=bpy.types.Action)
    enemy_idle_action: bpy.props.PointerProperty(name="Idle Action", type=bpy.types.Action)
    enemy_damage_action: bpy.props.PointerProperty(name="Damage Action", type=bpy.types.Action)
    player_idle_anim_path: bpy.props.StringProperty(name="Idle JSON", subtype="FILE_PATH")
    player_anim_custom_path: bpy.props.StringProperty(name="Attack JSON", subtype="FILE_PATH")
    player_anim_start_frame: bpy.props.IntProperty(name="Start Frame", default=0)
    import_scale: bpy.props.FloatProperty(name="Scale", default=1.0)
    apply_mode: bpy.props.EnumProperty(
        name="Apply Mode",
        description="How JSON local transforms are applied to Blender pose bones",
        items=[
            ("REST_DELTA", "Rest Delta", "Use for engine CustomAnim JSON"),
            ("RAW_POSE", "Raw Pose", "Old direct pose-channel import"),
        ],
        default="REST_DELTA",
    )
    quat_pattern: bpy.props.EnumProperty(
        name="Quat Pattern",
        description="Coordinate conversion pattern",
        items=[
            ("P0", "P0: No conversion", "Use JSON quaternion as-is"),
            ("P1", "P1: LH inverse", "JSON(x,y,z,w) to Blender(w,x,-y,-z)"),
            ("P2", "P2: LH + Y-up to Z-up", "Swap Y/Z axis for Blender"),
            ("P3", "P3: Invert X", "Extra X inversion"),
        ],
        default="P1",
    )
    sequence_export_dir: bpy.props.StringProperty(
        name="Export Directory",
        subtype="DIR_PATH",
        default="resources/sequences",
    )
    sequence_profile_name: bpy.props.StringProperty(
        name="Profile Name",
        default="test_attack",
    )
    sequence_player_idle_path: bpy.props.StringProperty(
        name="JSON Path",
        subtype="FILE_PATH",
        default="resources/CustomAnim/Idle.json",
    )
    sequence_player_anim_path: bpy.props.StringProperty(
        name="JSON Path",
        subtype="FILE_PATH",
        default="resources/CustomAnim/CustomAnim_attack_1.json",
    )
    sequence_enemy_idle_path: bpy.props.StringProperty(
        name="JSON Path",
        subtype="FILE_PATH",
        default="",
    )
    sequence_enemy_anim_path: bpy.props.StringProperty(
        name="JSON Path",
        subtype="FILE_PATH",
        default="resources/CustomAnim/CustomAnim_attack_received_1.json",
    )
    sequence_frame_start: bpy.props.IntProperty(name="Start Frame", default=1, min=0)
    sequence_frame_end: bpy.props.IntProperty(name="End Frame", default=120, min=1)
    sequence_cutin_duration: bpy.props.FloatProperty(name="Cutin Duration", default=0.6, min=0.0)
    sequence_approach_duration: bpy.props.FloatProperty(name="Approach Duration", default=0.3, min=0.0)
    sequence_attack_wait_duration: bpy.props.FloatProperty(name="Attack Wait Duration", default=0.5, min=0.0)
    sequence_return_duration: bpy.props.FloatProperty(name="Return Duration", default=0.3, min=0.0)
    sequence_player_anim_start_time: bpy.props.FloatProperty(name="Player Anim Start", default=0.0, min=0.0)
    sequence_enemy_anim_start_time: bpy.props.FloatProperty(name="Enemy Anim Start", default=0.0, min=0.0)
    sequence_enable_camera_work: bpy.props.BoolProperty(name="Enable Camera Work", default=True)
    sequence_auto_register: bpy.props.BoolProperty(
        name="Update Map JSON",
        description="Register this profile in resources/sequences/card_sequence_map.json when exporting",
        default=True,
    )
    sequence_map_type: bpy.props.EnumProperty(
        name="Use For",
        description="Which card/effect category should play this sequence",
        items=[
            ("NONE", "No Effect Map", "Only export files"),
            ("CARD_USE", "Card Use", "Always play before the card effect"),
            ("Damage", "Damage", "Single target damage cards"),
            ("DamageAll", "All Damage", "All enemy damage cards"),
            ("DamageCrescent", "Crescent Damage", "DamageCrescent cards"),
            ("DamageByBlock", "Block Damage", "DamageByBlock cards"),
            ("Block", "Block", "Block/self defense cards"),
            ("Heal", "Heal", "Heal/self recovery cards"),
            ("PowerBoost", "Power Boost", "PowerBoost buff cards"),
            ("NextTurnAtkUp", "Next Turn Atk Up", "NextTurnAtkUp buff cards"),
            ("Draw", "Draw", "Draw cards"),
            ("EnergyCharge", "Energy Charge", "EnergyCharge cards"),
            ("SelfDamage", "Self Damage", "SelfDamage cards"),
            ("CUSTOM", "Custom Effect", "Use the custom effect type below"),
        ],
        default="Damage",
    )
    sequence_custom_effect_type: bpy.props.StringProperty(
        name="Custom Effect",
        description="Exact effect type string used by the game, for example HealByBlock",
        default="",
    )
    sequence_card_id: bpy.props.IntProperty(
        name="Card ID",
        description="Optional: also register this profile for one specific card ID. 0 disables card-specific registration",
        default=0,
        min=0,
    )


class BATTLE_PT_export_panel(bpy.types.Panel):
    bl_label = "Battle Action Exporter"
    bl_idname = "BATTLE_PT_export_panel"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Battle Anime"

    def draw(self, context):
        layout = self.layout
        settings = context.scene.battle_anime_settings
        if not settings:
            return

        layout.prop(settings, "sequence_export_dir")
        layout.prop(settings, "sequence_profile_name")

        frame_row = layout.row(align=True)
        frame_row.prop(settings, "sequence_frame_start")
        frame_row.prop(settings, "sequence_frame_end")

        layout.separator()

        map_box = layout.box()
        map_box.label(text="Game Mapping")
        map_box.prop(settings, "sequence_auto_register")
        map_box.prop(settings, "sequence_map_type")
        if settings.sequence_map_type == "CUSTOM":
            map_box.prop(settings, "sequence_custom_effect_type")
        map_box.prop(settings, "sequence_card_id")

        layout.separator()

        player_box = layout.box()
        player_box.label(text="Player Settings")
        player_box.prop(settings, "player_obj", text="Player Object")
        player_box.label(text="-- Idle Animation --")
        player_box.prop(settings, "player_idle_action", text="Idle Action")
        player_box.prop(settings, "sequence_player_idle_path")
        player_box.label(text="-- Attack Animation --")
        player_box.prop(settings, "player_attack_action", text="Attack Action")
        player_box.prop(settings, "sequence_player_anim_path")
        player_box.prop(settings, "sequence_player_anim_start_time", text="Anim Start Frame")

        enemy_box = layout.box()
        enemy_box.label(text="Enemy Settings")
        enemy_box.prop(settings, "enemy_obj", text="Enemy Object")
        enemy_box.label(text="-- Idle Animation --")
        enemy_box.prop(settings, "enemy_idle_action", text="Idle Action")
        enemy_box.prop(settings, "sequence_enemy_idle_path")
        enemy_box.label(text="-- Damage Animation --")
        enemy_box.prop(settings, "enemy_damage_action", text="Damage Action")
        enemy_box.prop(settings, "sequence_enemy_anim_path")
        enemy_box.prop(settings, "sequence_enemy_anim_start_time", text="Anim Start Frame")

        camera_box = layout.box()
        camera_box.label(text="Camera Settings")
        camera_box.prop(settings, "camera_obj", text="Camera Object")
        camera_box.prop(settings, "sequence_enable_camera_work")

        convert_box = layout.box()
        convert_box.label(text="Import / Conversion")
        convert_box.prop(settings, "import_scale")
        convert_box.prop(settings, "apply_mode")
        convert_box.prop(settings, "quat_pattern")
        convert_box.prop(settings, "player_idle_anim_path")
        convert_box.prop(settings, "player_anim_custom_path")
        convert_box.prop(settings, "player_anim_start_frame")
        convert_box.operator("import_scene.battle_panel_anims", text="Import from Panel", icon="PLAY")
        convert_box.operator("battle.reset_pose", text="Reset Pose", icon="LOOP_BACK")

        layout.operator("export_scene.battle_sequence_json", text="Export Battle Profile", icon="EXPORT")


class IMPORT_OT_battle_panel_anims(bpy.types.Operator):
    bl_idname = "import_scene.battle_panel_anims"
    bl_label = "Import"

    def execute(self, context):
        settings = context.scene.battle_anime_settings
        obj = settings.player_obj or context.active_object
        if not obj or obj.type != "ARMATURE":
            self.report({"ERROR"}, "Select an armature in Player or as the active object.")
            return {"CANCELLED"}

        fps = context.scene.render.fps / context.scene.render.fps_base
        if not obj.animation_data:
            obj.animation_data_create()

        action = bpy.data.actions.new(name="CombinedAction")
        obj.animation_data.action = action

        if settings.player_idle_anim_path:
            load_json_to_action(context, obj, settings.player_idle_anim_path, 0, fps, settings.import_scale, settings)
        if settings.player_anim_custom_path:
            load_json_to_action(
                context,
                obj,
                settings.player_anim_custom_path,
                settings.player_anim_start_frame,
                fps,
                settings.import_scale,
                settings,
            )

        return {"FINISHED"}


class RESET_OT_battle_character(bpy.types.Operator):
    bl_idname = "battle.reset_pose"
    bl_label = "Reset Pose"

    def execute(self, context):
        settings = context.scene.battle_anime_settings
        obj = settings.player_obj or context.active_object
        if not obj:
            return {"CANCELLED"}

        if obj.animation_data:
            obj.animation_data.action = None

        obj.location = (0, 0, 0)
        obj.rotation_euler = (0, 0, 0)
        if obj.type == "ARMATURE":
            for bone in obj.pose.bones:
                bone.matrix_basis.identity()

        return {"FINISHED"}


class EXPORT_OT_battle_sequence_json(bpy.types.Operator):
    bl_idname = "export_scene.battle_sequence_json"
    bl_label = "Export Sequence JSONs"

    def execute(self, context):
        settings = context.scene.battle_anime_settings

        if settings.sequence_frame_end <= settings.sequence_frame_start:
            self.report({"ERROR"}, "End Frame must be greater than Start Frame.")
            return {"CANCELLED"}

        exported = []
        if export_camera_json(context, settings):
            exported.append("camera")
        if settings.sequence_player_idle_path:
            if export_armature_animation_json(
                context,
                settings.player_obj,
                settings.sequence_player_idle_path,
                settings,
                settings.player_idle_action,
            ):
                exported.append("player idle")
        if export_armature_animation_json(
            context,
            settings.player_obj,
            player_attack_path(settings),
            settings,
            settings.player_attack_action,
        ):
            exported.append("player animation")
        if settings.sequence_enemy_idle_path:
            if export_armature_animation_json(
                context,
                settings.enemy_obj,
                settings.sequence_enemy_idle_path,
                settings,
                settings.enemy_idle_action,
            ):
                exported.append("enemy idle")
        if export_armature_animation_json(
            context,
            settings.enemy_obj,
            enemy_damage_path(settings),
            settings,
            settings.enemy_damage_action,
        ):
            exported.append("enemy animation")

        export_sequence_json(settings)
        exported.append("sequence")
        if update_sequence_map_json(settings):
            exported.append("map")

        self.report({"INFO"}, "Exported: " + ", ".join(exported))
        return {"FINISHED"}


classes = (
    BattleAnimeSettings,
    BATTLE_PT_export_panel,
    IMPORT_OT_battle_panel_anims,
    RESET_OT_battle_character,
    EXPORT_OT_battle_sequence_json,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.battle_anime_settings = bpy.props.PointerProperty(type=BattleAnimeSettings)


def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    del bpy.types.Scene.battle_anime_settings


if __name__ == "__main__":
    register()
