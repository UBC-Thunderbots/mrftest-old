// GUI Handlers

void controlEvent(ControlEvent e) {
	if (e.isFrom(playbackCheckbox)) {
        REAL_SPEED = (playbackCheckbox.getArrayValue(0) == 1);
	} else if (e.isFrom(displayElementCheckboxes)) {
        DRAW_ORIGIN = (displayElementCheckboxes.getArrayValue(0) == 1);
        DRAW_VEL_IND = (displayElementCheckboxes.getArrayValue(1) == 1);
        DRAW_AVL_IND = (displayElementCheckboxes.getArrayValue(2) == 1);
        DRAW_TRAIL = (displayElementCheckboxes.getArrayValue(3) == 1);
    }
}


public void pbPlay() {
    PLAYING = true;
}

public void pbPause() {
    PLAYING = false;
}

public void pbReset() {
    if (robot != null) robot.reset();
}

public void pbStepForward() {
    if (robot != null) robot.setFrame(robot.getFrame() + 1);
}

public void pbStepBackward() {
    if (robot != null) robot.setFrame(robot.getFrame() - 1);
}