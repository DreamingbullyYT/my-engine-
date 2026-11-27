-- Lua-only DMod example. No compiler or .so is required.
-- Hooks run in-process at documented engine hook points.

dream.hooks.register("godot.main_loop.process", "Before", function(ctx)
    -- Example: cap the delta exposed to this main-loop process call.
    if ctx.delta > 0.1 then
        ctx.delta = 0.1
    end
end, 10)

function on_update(delta)
    -- Called by DreamMods once per engine frame for this mod.
end
