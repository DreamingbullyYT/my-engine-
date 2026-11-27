# Mobile UX and Accessibility

Status: PLANNED design proposal for review; not an accepted new gameplay decision yet.

## Product principle
The game may have deep simulation, but the default mobile experience must be easy to understand and operate. Complexity is available on demand, not forced on every player. Never remove simulation depth merely to simplify the interface; instead expose it progressively and automate repetitive work.

## Progressive disclosure
- Start with a small number of clear actions and essential indicators.
- Show advanced values, detailed graphs, formulas, orbital parameters, production chains, and per-citizen controls only when the player opens an advanced/details view.
- Every advanced screen has a concise plain-language summary and a clear route back to the main task.
- Avoid presenting every resource, stat, warning, or control at once.
- Use contextual explanations: explain a value where it appears, not only in a separate manual.

## Beginner and advanced play
- Default onboarding teaches one useful action at a time: inspect colony, build housing/food, assign or enable worker automation, research, build a basic rocket, launch, and reach a simple orbital objective.
- Tutorials are skippable, replayable, and do not block sandbox play.
- Basic controls use sensible defaults, recommended designs, automatic worker assignment, suggested flight maneuvers, and clear mission feasibility feedback.
- Advanced controls expose manual worker priorities, production-chain details, part-level craft design, staging configuration, orbital parameters, maneuver nodes, and detailed telemetry.
- Do not lock advanced mechanics behind arbitrary player levels. Let users open advanced controls when ready.
- Difficulty/assistance settings can change how much help the interface provides without silently changing the underlying physics or resource rules.

## Automation as a usability feature
- Provide opt-in automatic worker assignment, production priorities, cargo-route templates, recommended rocket designs, and flight assistance.
- Explain what automation will do, its resource budget, risks, and limits before enabling consequential actions.
- Allow review, pause, override, and undo where safe. Preserve the user's ability to take direct control.
- Automation must not create resources, bypass physics, or conceal important failures.
- Mayor autonomy remains gradual under D-011; this UX document does not replace that decision.

## Touch-first interaction
- Prioritize large touch targets, readable text, safe-area awareness, and one-handed reach for frequent actions where practical.
- Avoid requiring precise dragging for essential tasks; provide tap-to-select, tap-to-place, snapping, undo, and clear placement previews.
- Rocket editor supports zoom/pan, part search/categories, snap/attach feedback, symmetry tools, stage grouping, and accessible alternatives to tiny drag handles.
- Flight controls support configurable layouts, clear throttle/attitude controls, map-based planning, and optional assisted flight.
- Colony management supports contextual action sheets, filters, search, multi-select, and batch actions instead of forcing repeated navigation through deep menus.
- Important actions require confirmation when destructive or costly; routine reversible actions should remain quick.

## Information hierarchy
- Main HUD: only immediate survival/economy status and current objective/alerts.
- Context panel: selected citizen, building, vehicle, station, or body with the most relevant actions.
- Details view: full stats, inputs/outputs, graphs, history, and diagnostics.
- Advanced view: technical parameters and developer/mod diagnostics.
- Alerts are prioritized by urgency and explain cause, consequence, and a suggested next step. Avoid notification spam and non-actionable red warnings.

## Accessibility and localization
- Scalable UI/text, high-contrast options, color-independent status indicators, reduced-motion option, remappable controls, and adjustable camera/gesture sensitivity.
- Support Romanian and English at minimum, with text expansion considered in layouts.
- Avoid conveying critical information by color alone; pair icons/colors with labels or shapes.
- Provide pause at any time outside explicitly defined real-time multiplayer contexts.

## Complexity and content pacing
- Introduce systems in a dependency-aware order. Do not expose stations, fleet automation, advanced production, and interstellar expansion as simultaneous first-session tasks.
- The player can ignore optional systems until they become relevant.
- Keep the core loop understandable: sustain a colony → research/build → launch/explore → bring back useful resources/knowledge → grow the colony.
- Do not use daily login pressure or offline progression penalties.
- Measure onboarding completion, task success, menu depth, mis-taps, session performance, and user-reported confusion during device testing. Use evidence rather than assuming a screen is intuitive.

## Performance-aware UI
- Avoid rendering large live citizen lists, graphs, or fleet telemetry continuously when hidden.
- Use summaries and on-demand detail views for large populations and fleets.
- Keep touch input responsive during simulation, loading, and time warp; expose progress and allow safe cancellation where possible.

## Review questions before acceptance
- Should the first launch use a guided scenario, a free sandbox, or offer both?
- Should assisted controls be the default for rocket flight, with manual control available immediately?
- Which UI language should be selected automatically, and should the user be asked on first launch?
These are open product choices; do not silently mark them as accepted.
