import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\probe_greatsword_python_api.txt"
LINES = []


def log(message):
    line = f"[ProbeGSPython] {message}"
    LINES.append(line)
    unreal.log(line)


def fmt(value):
    if value is None:
        return "<none>"
    if hasattr(value, "get_path_name"):
        try:
            return value.get_path_name()
        except Exception:
            pass
    return str(value)


def safe_get(obj, prop_name):
    try:
        return obj.get_editor_property(prop_name)
    except Exception as exc:
        return f"<error:{exc}>"


def dump_type_info():
    for type_name in [
        "GameplayTag",
        "GameplayTagContainer",
        "ActionCombatActionDefinition",
        "ActionCombatTransitionDefinition",
        "ActionCombatAttackAdvanceSettings",
        "ActionCombatLyraInputBinding",
        "ActionCombatMeleeTraceProfile",
        "ActionCombatTracePoint",
        "AnimNotifyEvent",
        "AnimMontageFactory",
    ]:
        obj = getattr(unreal, type_name, None)
        log(f"type {type_name} exists={obj is not None}")
        if obj is None:
            continue

        sample = obj
        try:
            if isinstance(obj, type):
                sample = obj()
        except Exception as exc:
            log(f"  instantiate failed: {exc}")
            sample = obj

        names = [name for name in dir(sample) if not name.startswith("_")]
        log(f"  names={names[:80]}")


def inspect_style_asset():
    asset_path = "/Game/1dev/OS/GDHOneHanded/PrimaryAttack_GDHOneHanded_Auto"
    style = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not style:
        log(f"FAILED load style {asset_path}")
        return

    log(f"style={fmt(style)} class={style.get_class().get_name()}")
    actions = safe_get(style, "actions")
    transitions = safe_get(style, "transitions")
    log(f"actions_type={type(actions)} count={len(actions) if isinstance(actions, list) else actions}")
    if isinstance(actions, list):
        for index, action in enumerate(actions[:8]):
            log(
                "  action[{0}] tag={1} montage={2} queue=({3},{4}) commit={5} trace={6} window={7} poise={8} advance={9}".format(
                    index,
                    safe_get(action, "action_tag"),
                    fmt(safe_get(action, "montage")),
                    safe_get(action, "queue_window_starts_at_normalized_time"),
                    safe_get(action, "queue_window_closes_at_normalized_time"),
                    safe_get(action, "chain_commit_at_normalized_time"),
                    safe_get(action, "trace_source_id"),
                    safe_get(action, "hit_window_name"),
                    safe_get(action, "poise_damage"),
                    safe_get(action, "attack_advance"),
                )
            )

    log(f"transitions_type={type(transitions)} count={len(transitions) if isinstance(transitions, list) else transitions}")
    if isinstance(transitions, list):
        for index, transition in enumerate(transitions[:12]):
            log(
                "  transition[{0}] from={1} cmd={2} to={3} req={4} blocked={5}".format(
                    index,
                    safe_get(transition, "from_action_tag"),
                    safe_get(transition, "command_tag"),
                    safe_get(transition, "to_action_tag"),
                    safe_get(transition, "required_held_input_tags"),
                    safe_get(transition, "blocked_held_input_tags"),
                )
            )


def inspect_montage_asset():
    montage_path = "/Game/1dev/OS/GDHOneHanded/AM_GDH_OneHanded_Light01"
    montage = unreal.EditorAssetLibrary.load_asset(montage_path)
    if not montage:
        log(f"FAILED load montage {montage_path}")
        return

    log(f"montage={fmt(montage)} class={montage.get_class().get_name()}")
    for prop_name in ["skeleton", "slot_anim_tracks", "notifies", "composite_sections"]:
        value = safe_get(montage, prop_name)
        if isinstance(value, list):
            log(f"  {prop_name}.count={len(value)}")
            for index, item in enumerate(value[:8]):
                log(f"    [{index}] {item}")
        else:
            log(f"  {prop_name}={value}")


def inspect_weapon_bp():
    bp = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/Weapon/B_MeeleWeapon_Test")
    if not bp:
        log("FAILED load weapon bp")
        return

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    log(f"weapon_bp handles={len(handles)}")
    for handle in handles:
        data = subsystem.k2_find_subobject_data_from_handle(handle)
        if not data:
            continue
        try:
            obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
        except Exception:
            obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if not obj:
            continue
        log(f"  obj={fmt(obj)} class={obj.get_class().get_name()}")
        if obj.get_name() == "MeleeTraceComponent":
            for prop_name in ["trace_source_id", "trace_source_component", "default_trace_profile"]:
                log(f"    {prop_name}={safe_get(obj, prop_name)}")


def inspect_hero_blueprint_graph():
    bp = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/DragonKnightRuntime/B_Test_Hero_DragonRuntime")
    if not bp:
        log("FAILED load hero bp")
        return

    try:
        graphs = bp.get_all_graphs()
    except Exception as exc:
        log(f"hero_bp get_all_graphs failed: {exc}")
        return

    log(f"hero_bp graph_count={len(graphs)}")
    interesting_terms = [
        "SetBaseStyle",
        "PrimaryAttack_GDHOneHanded_Auto",
        "B_MeeleWeapon_Test",
        "InputTag.Combat.Attack.Primary",
        "InputTag.Combat.Modifier.Shift",
        "Combat.Input.Held.Shift",
    ]

    for graph in graphs:
        nodes = graph.get_nodes()
        log(f"  graph={graph.get_name()} node_count={len(nodes)}")
        for node in nodes:
            node_dump = str(node)
            title = node.get_name()
            matched = any(term in node_dump or term in title for term in interesting_terms)
            if not matched:
                try:
                    pins = node.pins
                    for pin in pins:
                        if any(term in str(pin.default_value) or term in str(pin.default_object) for term in interesting_terms):
                            matched = True
                            break
                except Exception:
                    pass

            if not matched:
                continue

            log(f"    node={title} class={node.get_class().get_name()}")
            try:
                for pin in node.pins:
                    log(
                        "      pin={0} category={1} subcat_obj={2} default_value={3} default_object={4}".format(
                            pin.pin_name,
                            pin.pin_type.pin_category,
                            fmt(pin.pin_type.pin_subcategory_object),
                            pin.default_value,
                            fmt(pin.default_object),
                        )
                    )
            except Exception as exc:
                log(f"      pin dump failed: {exc}")


def main():
    dump_type_info()
    inspect_style_asset()
    inspect_montage_asset()
    inspect_weapon_bp()
    inspect_hero_blueprint_graph()

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
