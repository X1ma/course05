extends CanvasLayer


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	AudioManager.force_stop_all_sounds()


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	AudioManager.force_stop_all_sounds()


func _on_nextlevel_gui_input(event: InputEvent) -> void:
	if (event is InputEventMouseButton && event.pressed && event.button_index == 1):
		if TransitionScene.prevscene == "res://scenes/levels/level1.tscn":
			TransitionScene.transition_effect("res://scenes/levels/level2.tscn")
		elif TransitionScene.prevscene == "res://scenes/levels/level2.tscn":
			TransitionScene.transition_effect("res://scenes/levels/level3.tscn")
		elif TransitionScene.prevscene == "res://scenes/levels/level3.tscn":
			TransitionScene.transition_effect("res://scenes/levels/level4.tscn")
		elif TransitionScene.prevscene == "res://scenes/levels/level4.tscn":
			TransitionScene.transition_effect("res://scenes/levels/level5.tscn")


func _on_replay_gui_input(event: InputEvent) -> void:
	if (event is InputEventMouseButton && event.pressed && event.button_index == 1):
		TransitionScene.transition_effect(TransitionScene.prevscene)


func _on_mainmenu_gui_input(event: InputEvent) -> void:
	if (event is InputEventMouseButton && event.pressed && event.button_index == 1):
		TransitionScene.reset_prevscene()
		TransitionScene.transition_effect("res://scenes/menus_screens/menu.tscn")
