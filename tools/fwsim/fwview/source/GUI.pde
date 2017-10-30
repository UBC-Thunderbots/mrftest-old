import controlP5.*;

ControlP5 cp5;
Accordion accordion;

CheckBox playbackCheckbox;
CheckBox displayElementCheckboxes;

public void createGUI() {
    cp5 = new ControlP5(this);
    cp5.setAutoDraw(false);
    
    // Sizing variables
    int sx, sy, px, py, oy;
    sx = 100; 
    sy = 16;
    oy = (int)(sy * 1.4f);
    
    int gui_w = 200;
    int gui_x = width - gui_w;
    int gui_y = 0;
    
    // Playback group, contains all controls for playback
    Group group_replay = cp5.addGroup("Replay");
    {
        group_replay.setHeight(20).setSize(gui_w, 40).setBackgroundColor(color(0, 150))
        .setColorBackground(color(0, 150));
        group_replay.getCaptionLabel().align(CENTER, CENTER);
        
        px = 4;
        py = 4;
           
        // Basic control buttons
        int bsx = (gui_w - 12) / 5;
        cp5.addButton("Play").setGroup(group_replay).plugTo(this, "pbPlay").setSize(bsx, 16).setPosition(px, py);
        cp5.addButton("Pause").setGroup(group_replay).plugTo(this, "pbPause").setSize(bsx, 16).setPosition(px += bsx + 2, py);
        cp5.addButton("<").setGroup(group_replay).plugTo(this, "pbStepBackward").setSize(bsx, 16).setPosition(px += bsx + 2, py);
        cp5.addButton(">").setGroup(group_replay).plugTo(this, "pbStepForward").setSize(bsx, 16).setPosition(px += bsx + 2, py);
        cp5.addButton("Reset").setGroup(group_replay).plugTo(this, "pbReset").setSize(bsx, 16).setPosition(px += bsx + 2, py);
        
        // Sliders
        px = 4;
        py += 18;
        playbackCheckbox = cp5.addCheckBox("playbackCheckboxes").setGroup(group_replay)
        .setPosition(px, py).setSize(16, 16).setItemsPerRow(1).setSpacingRow(2)
        .addItem("Real speed", 0).activate(0);

        // cp5.addSlider("Playback").setGroup(group_replay).setSize(sx, sy).setPosition(px, py).setRange(0, 1).setValue(0).plugTo(this, "pbSetPlayback");
    }
    
    // View group, contains all controls for visualization
    Group group_visual = cp5.addGroup("Visual");
    {
        group_visual.setHeight(20).setSize(gui_w, 40).setBackgroundColor(color(0, 150)).setColorBackground(color(0, 150));
        group_visual.getCaptionLabel().align(CENTER, CENTER);
        
        px = 4;
        py = 4;
        displayElementCheckboxes = cp5.addCheckBox("displayElementCheckboxes").setGroup(group_visual)
        .setPosition(px, py).setSize(16, 16).setItemsPerRow(1).setSpacingRow(2)
        .addItem("Origin", 0).activate(0)
        .addItem("Velocity Indicator", 1).activate(1)
        .addItem("Angular V Indicator", 2).activate(2)
        .addItem("Trail", 2).activate(3);
    }
    
    // Create new accordion
    accordion = cp5.addAccordion("acc").setPosition(gui_x, gui_y).setWidth(gui_w).setSize(gui_w, height)
    .setCollapseMode(Accordion.MULTI)
    .addItem(group_replay)
    .addItem(group_visual)
    .open(0, 1);
}

public void displayGUI() {
    noLights();
    cp5.draw();
}