extends RefCounted

## One flat, dark, touch-friendly theme for the whole game UI.
## Colours live here so a later skin or mod only has to replace this file.

const PANEL := Color(0.09, 0.11, 0.15, 0.94)
const PANEL_BORDER := Color(0.28, 0.33, 0.41, 1.0)
const BUTTON := Color(0.17, 0.2, 0.27, 1.0)
const BUTTON_HOVER := Color(0.22, 0.26, 0.35, 1.0)
const BUTTON_PRESSED := Color(0.12, 0.14, 0.2, 1.0)
const ACCENT := Color(1.0, 0.8, 0.28, 1.0)
const GOOD := Color(0.45, 0.85, 0.5, 1.0)
const BAD := Color(0.95, 0.4, 0.36, 1.0)
const TEXT := Color(0.94, 0.96, 0.98, 1.0)
const TEXT_DIM := Color(0.66, 0.72, 0.8, 1.0)
const MIN_TOUCH := 44.0


static func panel_style(border_bottom_only: bool = false) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = PANEL
	style.border_color = PANEL_BORDER
	if border_bottom_only:
		style.border_width_bottom = 2
	else:
		style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.content_margin_left = 10.0
	style.content_margin_right = 10.0
	style.content_margin_top = 6.0
	style.content_margin_bottom = 6.0
	return style


static func button_style(color: Color, border: Color = PANEL_BORDER) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = color
	style.border_color = border
	style.set_border_width_all(2)
	style.set_corner_radius_all(5)
	style.content_margin_left = 10.0
	style.content_margin_right = 10.0
	style.content_margin_top = 6.0
	style.content_margin_bottom = 6.0
	return style


static func build() -> Theme:
	var theme := Theme.new()
	theme.set_default_font_size(15)

	theme.set_stylebox("panel", "PanelContainer", panel_style())
	theme.set_stylebox("panel", "Panel", panel_style())

	theme.set_stylebox("normal", "Button", button_style(BUTTON))
	theme.set_stylebox("hover", "Button", button_style(BUTTON_HOVER))
	theme.set_stylebox("pressed", "Button", button_style(BUTTON_PRESSED, ACCENT))
	theme.set_stylebox("disabled", "Button", button_style(Color(0.12, 0.14, 0.18, 0.9), Color(0.2, 0.24, 0.3)))
	theme.set_stylebox("focus", "Button", StyleBoxEmpty.new())
	theme.set_color("font_color", "Button", TEXT)
	theme.set_color("font_hover_color", "Button", TEXT)
	theme.set_color("font_pressed_color", "Button", ACCENT)
	theme.set_color("font_disabled_color", "Button", Color(0.5, 0.55, 0.62))
	theme.set_font_size("font_size", "Button", 15)

	theme.set_color("font_color", "Label", TEXT)
	theme.set_font_size("font_size", "Label", 15)
	theme.set_constant("outline_size", "Label", 0)
	return theme
