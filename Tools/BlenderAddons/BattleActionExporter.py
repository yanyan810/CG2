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
import math
import mathutils
import os
import time


_last_live_camera_export_time = 0.0


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


def sequence_coord_mode(settings):
    return getattr(settings, "sequence_coord_mode", "ZUP_TO_YUP") if settings else "ZUP_TO_YUP"


def blender_to_engine_basis(settings):
    mode = sequence_coord_mode(settings)
    if mode == "RAW":
        return mathutils.Matrix.Identity(3)
    if mode == "X_FLIP":
        return mathutils.Matrix((
            (-1.0, 0.0, 0.0),
            (0.0, 1.0, 0.0),
            (0.0, 0.0, 1.0),
        ))
    if mode == "X_FLIP_ZUP_TO_YUP":
        return mathutils.Matrix((
            (-1.0, 0.0, 0.0),
            (0.0, 0.0, 1.0),
            (0.0, 1.0, 0.0),
        ))
    return mathutils.Matrix((
        (1.0, 0.0, 0.0),
        (0.0, 0.0, 1.0),
        (0.0, 1.0, 0.0),
    ))


def vec3_to_engine(value, settings):
    converted = blender_to_engine_basis(settings) @ mathutils.Vector((value[0], value[1], value[2]))
    return vec3(converted)


def character_position_basis(settings):
    mode = sequence_coord_mode(settings)
    flip_x = getattr(settings, "sequence_flip_character_position_x", False)

    if mode == "RAW":
        return mathutils.Matrix.Identity(3)
    if mode == "X_FLIP":
        return mathutils.Matrix((
            (-1.0 if flip_x else 1.0, 0.0, 0.0),
            (0.0, 1.0, 0.0),
            (0.0, 0.0, 1.0),
        ))
    if mode == "X_FLIP_ZUP_TO_YUP" or mode == "ZUP_TO_YUP":
        return mathutils.Matrix((
            (-1.0 if flip_x else 1.0, 0.0, 0.0),
            (0.0, 0.0, 1.0),
            (0.0, 1.0, 0.0),
        ))
    return blender_to_engine_basis(settings)


def character_position_to_engine(value, settings):
    converted = character_position_basis(settings) @ mathutils.Vector((value[0], value[1], value[2]))
    return vec3(converted)


def object_world_bounds(obj):
    if not obj:
        return None

    depsgraph = bpy.context.evaluated_depsgraph_get()
    candidates = []

    if obj.type == "MESH":
        candidates.append(obj)

    for child in obj.children_recursive:
        if child.type == "MESH":
            candidates.append(child)

    for mesh_obj in bpy.data.objects:
        if mesh_obj.type != "MESH" or mesh_obj in candidates:
            continue
        if any(mod.type == "ARMATURE" and mod.object == obj for mod in mesh_obj.modifiers):
            candidates.append(mesh_obj)

    points = []
    for candidate in candidates:
        try:
            eval_obj = candidate.evaluated_get(depsgraph)
            matrix = eval_obj.matrix_world
            for corner in eval_obj.bound_box:
                points.append(matrix @ mathutils.Vector(corner))
        except Exception:
            continue

    if not points:
        return None

    min_v = mathutils.Vector((
        min(point.x for point in points),
        min(point.y for point in points),
        min(point.z for point in points),
    ))
    max_v = mathutils.Vector((
        max(point.x for point in points),
        max(point.y for point in points),
        max(point.z for point in points),
    ))
    return min_v, max_v


def object_sequence_position(obj, settings):
    source = getattr(settings, "sequence_character_position_source", "ORIGIN")
    bounds = object_world_bounds(obj) if source != "ORIGIN" else None

    if bounds:
        min_v, max_v = bounds
        center = (min_v + max_v) * 0.5
        if source == "BOUNDS_BOTTOM_CENTER":
            return mathutils.Vector((center.x, center.y, min_v.z))
        return center

    return obj.matrix_world.translation


def euler_to_engine(value, settings):
    basis = blender_to_engine_basis(settings)
    rot = value.to_matrix()
    converted = basis @ rot @ basis.inverted()
    return vec3(converted.to_euler("XYZ"))


def character_euler_to_engine(value, settings):
    converted = euler_to_engine(value, settings)
    if getattr(settings, "sequence_flip_character_forward", False):
        converted[1] += 3.141592653589793
    converted[1] += math.radians(getattr(settings, "sequence_character_yaw_offset_deg", 0.0))
    return converted


def engine_euler_from_matrix(matrix):
    import math

    sy = max(-1.0, min(1.0, -matrix[0][2]))
    y = math.asin(sy)
    cy = math.cos(y)

    if abs(cy) > 1e-5:
        x = math.atan2(matrix[1][2], matrix[2][2])
        z = math.atan2(matrix[0][1], matrix[0][0])
    else:
        x = math.atan2(-matrix[2][1], matrix[1][1])
        z = 0.0

    return [float(x), float(y), float(z)]


def camera_euler_to_engine(value, settings):
    import math
    rot = value.to_matrix()
    mode = sequence_coord_mode(settings)

    if mode == "X_FLIP_ZUP_TO_YUP":
        # Blender カメラ local -Z 方向をワールド座標に変換
        look_world = rot @ mathutils.Vector((0.0, 0.0, -1.0))
        up_world   = rot @ mathutils.Vector((0.0, 1.0,  0.0))

        # X_FLIP_ZUP_TO_YUP: Blender(bx, by, bz) → Engine(-bx, bz, by)
        lx = -look_world.x
        ly =  look_world.z
        lz =  look_world.y

        ux = -up_world.x
        uy =  up_world.z
        uz =  up_world.y

        # Pitch / Yaw を look ベクトルから算出
        horiz = math.sqrt(lx * lx + lz * lz)
        pitch = math.atan2(-ly, horiz)   # X 軸回転
        yaw   = math.atan2(lx, lz)       # Y 軸回転

        # Roll: up ベクトルを使って算出
        # カメラの right ベクトル = look × world_up（near-zenith 時は uz を使う）
        roll = 0.0
        if horiz > 1e-4:
            right_x = math.cos(yaw)
            right_z = -math.sin(yaw)
            roll = math.atan2(ux * right_z - uz * right_x, uy)

        return [pitch, yaw, roll]
    else:
        basis = blender_to_engine_basis(settings)
        camera_local_to_engine = mathutils.Matrix((
            (1.0, 0.0,  0.0),
            (0.0, 1.0,  0.0),
            (0.0, 0.0, -1.0),
        ))
        converted = basis @ rot @ camera_local_to_engine
        return vec3(converted.to_euler("XYZ"))


def camera_world_matrix_to_engine(camera_obj, settings):
    basis = blender_to_engine_basis(settings)
    world = camera_obj.matrix_world
    rot = world.to_3x3()

    right = basis @ (rot @ mathutils.Vector((1.0, 0.0, 0.0)))
    up = basis @ (rot @ mathutils.Vector((0.0, 1.0, 0.0)))
    forward = basis @ (rot @ mathutils.Vector((0.0, 0.0, -1.0)))
    pos = basis @ world.translation

    return [
        float(right.x), float(right.y), float(right.z), 0.0,
        float(up.x), float(up.y), float(up.z), 0.0,
        float(forward.x), float(forward.y), float(forward.z), 0.0,
        float(pos.x), float(pos.y), float(pos.z), 1.0,
    ]



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
        data["playerPos"] = character_position_to_engine(object_sequence_position(settings.player_obj, settings), settings)
        # プレイヤーの向き（Y軸=上方向周りの回転 = YawのみEuler XYZ で Z成分を使う）
        player_euler = settings.player_obj.rotation_euler
        data["playerRot"] = character_euler_to_engine(player_euler, settings)
    if settings.enemy_obj:
        data["enemyPos"] = character_position_to_engine(object_sequence_position(settings.enemy_obj, settings), settings)
    if settings.camera_obj:
        pos, rot, fov, world_matrix = camera_state_to_engine(settings.camera_obj, settings)
        data["cameraPos"] = pos
        data["cameraRot"] = rot
        data["cameraFov"] = fov

    ensure_dir(profile_path)
    with open(bpy.path.abspath(profile_path), "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=4)


def camera_state_to_engine(camera_obj, settings):
    import math
    rotation = camera_obj.rotation_euler.to_matrix().to_euler("XYZ")
    if camera_obj.type == "CAMERA":
        # Blender camera.data.angle は水平FoV (ラジアン)
        # ゲームエンジンは垂直FoV (SetFovY) を使うため変換が必要
        # ゲーム解像度 1280x720 (16:9) のアスペクト比で変換
        horizontal_fov = camera_obj.data.angle
        aspect = 1280.0 / 720.0  # ゲームの解像度に合わせる
        fov = 2.0 * math.atan(math.tan(horizontal_fov / 2.0) / aspect)
    else:
        fov = 0.45  # デフォルト垂直FoV
    return (
        vec3_to_engine(camera_obj.location, settings),
        camera_euler_to_engine(rotation, settings),
        float(fov),
        camera_world_matrix_to_engine(camera_obj, settings),
    )


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
        pos, rot, fov, world_matrix = camera_state_to_engine(camera_obj, settings)
        keyframes.append({
            "time": float((frame - settings.sequence_frame_start) / fps),
            "pos": pos,
            "rot": rot,
            "fov": fov,
            "worldMatrix": world_matrix,
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

    def is_side_bone(pose_bone):
        bone = pose_bone.bone
        center_x = (bone.head_local.x + bone.tail_local.x) * 0.5
        return abs(center_x) > 0.05

    def export_bone_translation(pose_bone, loc):
        if getattr(settings, "sequence_convert_character_animation_lh", True):
            value = [-float(loc.x), float(loc.y), float(loc.z)]
        else:
            value = [float(loc.x), float(loc.y), float(loc.z)]

        if getattr(settings, "sequence_mirror_character_animation_x", False) and is_side_bone(pose_bone):
            value[0] = -value[0]
        return value

    def export_bone_rotation(pose_bone, rot):
        if getattr(settings, "sequence_convert_character_animation_lh", True):
            value = [float(rot.x), -float(rot.y), -float(rot.z), float(rot.w)]
        else:
            value = [float(rot.x), float(rot.y), float(rot.z), float(rot.w)]

        if getattr(settings, "sequence_mirror_character_animation_x", False) and is_side_bone(pose_bone):
            value[1] = -value[1]
            value[2] = -value[2]
        return value

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
                "value": export_bone_translation(pose_bone, loc),
            })
            node_anim["rotate"].append({
                "time": time,
                "value": export_bone_rotation(pose_bone, rot),
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
    sequence_live_camera_export: bpy.props.BoolProperty(
        name="Live Camera Export",
        description="Continuously export the selected camera JSON while editing in Blender",
        default=False,
    )
    sequence_flip_character_forward: bpy.props.BoolProperty(
        name="Flip Character Forward",
        description="Add 180 degrees around engine Y when the engine character faces opposite to Blender",
        default=False,
    )
    sequence_character_yaw_offset_deg: bpy.props.FloatProperty(
        name="Character Yaw Offset",
        description="Extra engine Y rotation in degrees for player/enemy facing",
        default=0.0,
        soft_min=-180.0,
        soft_max=180.0,
    )
    sequence_flip_character_position_x: bpy.props.BoolProperty(
        name="Flip Character Position X",
        description="Mirror exported player/enemy X positions. Keep off when moving right in Blender should move right in the game",
        default=False,
    )
    sequence_mirror_character_animation_x: bpy.props.BoolProperty(
        name="Mirror Character Animation X",
        description="Mirror local bone animation on X. Turn off when right and left arms are swapped in the game",
        default=False,
    )
    sequence_convert_character_animation_lh: bpy.props.BoolProperty(
        name="Convert Character Animation LH",
        description="Export bone animation in the same left-hand format as engine-loaded glTF animations",
        default=True,
    )
    sequence_character_position_source: bpy.props.EnumProperty(
        name="Character Position Source",
        description="How player/enemy positions are exported to the sequence JSON",
        items=[
            ("ORIGIN", "Object Origin", "Use the selected object's world origin"),
            ("BOUNDS_CENTER", "Bounds Center", "Use the visual center of the mesh bounds"),
            ("BOUNDS_BOTTOM_CENTER", "Bounds Bottom Center", "Use the visual bottom-center of the mesh bounds"),
        ],
        default="BOUNDS_BOTTOM_CENTER",
    )
    sequence_coord_mode: bpy.props.EnumProperty(
        name="Sequence Coord Mode",
        description="Coordinate conversion for sequence object and camera transforms",
        items=[
            ("ZUP_TO_YUP", "Blender Z-up -> Engine Y-up", "Export Blender (X,Y,Z) as engine (X,Z,Y)"),
            ("X_FLIP_ZUP_TO_YUP", "Z-up -> Y-up + Flip X (DirectX)", "Export Blender (X,Y,Z) as engine (-X,Z,Y) — correct for DirectX left-hand coord"),
            ("X_FLIP", "Legacy X Flip", "Export Blender (X,Y,Z) as engine (-X,Y,Z)"),
            ("RAW", "Raw", "Export Blender transforms without coordinate conversion"),
        ],
        default="X_FLIP_ZUP_TO_YUP",
    )
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
        camera_box.prop(settings, "sequence_live_camera_export")
        camera_box.prop(settings, "sequence_flip_character_forward")
        camera_box.prop(settings, "sequence_character_yaw_offset_deg")
        camera_box.prop(settings, "sequence_flip_character_position_x")
        camera_box.prop(settings, "sequence_mirror_character_animation_x")
        camera_box.prop(settings, "sequence_convert_character_animation_lh")
        camera_box.prop(settings, "sequence_character_position_source")
        camera_box.prop(settings, "sequence_coord_mode")

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


def live_camera_export_handler(scene):
    global _last_live_camera_export_time

    settings = getattr(scene, "battle_anime_settings", None)
    if not settings or not settings.sequence_live_camera_export:
        return
    if not settings.camera_obj or settings.camera_obj.type != "CAMERA":
        return

    now = time.monotonic()
    if now - _last_live_camera_export_time < 0.2:
        return

    _last_live_camera_export_time = now
    export_camera_json(bpy.context, settings)
    export_sequence_json(settings)


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
    if live_camera_export_handler not in bpy.app.handlers.depsgraph_update_post:
        bpy.app.handlers.depsgraph_update_post.append(live_camera_export_handler)


def unregister():
    if live_camera_export_handler in bpy.app.handlers.depsgraph_update_post:
        bpy.app.handlers.depsgraph_update_post.remove(live_camera_export_handler)
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    del bpy.types.Scene.battle_anime_settings


if __name__ == "__main__":
    register()
