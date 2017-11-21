import processing.core.*; 
import processing.data.*; 
import processing.event.*; 
import processing.opengl.*; 

import peasy.*; 
import peasy.org.apache.commons.math.*; 
import peasy.org.apache.commons.math.geometry.*; 
import peasy.*; 
import controlP5.*; 

import java.util.HashMap; 
import java.util.ArrayList; 
import java.io.File; 
import java.io.BufferedReader; 
import java.io.PrintWriter; 
import java.io.InputStream; 
import java.io.OutputStream; 
import java.io.IOException; 

public class fwView extends PApplet {





final float PX_PER_METER = 100;
public float pxToMeter(float px) {
    return px/PX_PER_METER;
}

public float meterToPx(float m) {
    return m * PX_PER_METER;
}

Robot robot;

// global states
boolean PLAYING = true;
boolean REAL_SPEED = false;

boolean DRAW_ORIGIN = true;
boolean DRAW_VEL_IND = true;
boolean DRAW_AVL_IND = true;
boolean DRAW_TRAIL = true;

public void setup() {
    
    
    // Setup 3D camera
    createCam();
     
    // Setup gui
    createGUI();
    
    // Create test robot
    robot = new Robot("test robot");
    
    // Loading simulation data
    Table simData = loadCSV("test");
    
    // Load simulation data into robot
    robot.populateSim(simData);
}

public void draw() {
    background(200);
    if (DRAW_ORIGIN) drawTestAxes();
    drawTestObjs();
    
    robot.draw();
    
    drawHUD();
}

public void drawTestObjs() {
    drawTestLights();
    fill(255, 255, 255);
    noStroke();
    rectMode(CENTER);
    rect(0, 0, meterToPx(10), meterToPx(10));
    
    // draw some grids
    stroke(200);
    strokeWeight(1);
    for (int x = -5; x <= 5; x++) {
        line(meterToPx(x), meterToPx(-5), meterToPx(x), meterToPx(5));
        line(meterToPx(-5), meterToPx(x), meterToPx(5), meterToPx(x));
    }
}

public void drawTestAxes() {
    strokeWeight(3);
    stroke(255, 0, 0);
    line(0, 0, 0, meterToPx(1), 0, 0);
    stroke(0, 255, 0);
    line(0, 0, 0, 0, meterToPx(-1), 0);
    stroke(0, 0, 255);
    line(0, 0, 0, 0, 0, meterToPx(1));
}

public void drawTestLights() {
    directionalLight(240, 220, 180, 0, 0.3f, -1);
    ambientLight(90, 130, 160);
}

public void drawHUD() {
    cam.beginHUD();
    displayGUI();
    fill(20);
    textSize(12);
    text("Time: " + String.format("%.1f", robot.getTime()) + "s", 20, 20);
    cam.endHUD();
}


PeasyCam cam;

public void createCam() {
    // Create camera instance
    cam = new PeasyCam(this, meterToPx(10));
    cam.setMinimumDistance(0.01f);
    cam.setMaximumDistance(meterToPx(50));
    cam.setSuppressRollRotationMode();
    
    // remap camera functions
    cam.setRightDragHandler(cam.getRotateDragHandler());
    cam.setCenterDragHandler(cam.getPanDragHandler());
    cam.setLeftDragHandler(null);
}


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
        .addItem("Real speed", 0);

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
// GUI Handlers

public void controlEvent(ControlEvent e) {
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
public Table loadCSV(String fname) {
    Table table = loadTable(fname + ".csv", "header");
    return table;
}
public class Robot {
    float[] times;
    float[] positionXs;
    float[] positionYs;
    float[] headings;
    float[] velocityXs;
    float[] velocityYs;
    float[] angularVs;
    
    int frame;
    int frames;
    String name;
    
    float x;
    float y;
    float heading;
    
    int startTime;
    
    boolean hasData;
    
    Robot(String name) {
        frame = 0;
        frames = 0;
        this.name = name;
        
        x = 0;
        y = 0;
        heading = 0;
        
        startTime = millis();
        
        hasData = false;
    }
    
    public void populateSim(Table tb) {
        int frames = tb.getRowCount();
        this.frames = frames;
        
        times = new float[frames];
        positionXs = new float[frames];
        positionYs = new float[frames];
        headings = new float[frames];
        velocityXs = new float[frames];
        velocityYs = new float[frames];
        angularVs = new float[frames];
        
        int currentRow = 0;
        for (TableRow row : tb.rows()) {
            if (row.getString("TYPE").equals("SIM")) {
                times[currentRow] = row.getFloat("TIME");
                positionXs[currentRow] = row.getFloat("X");
                positionYs[currentRow] = row.getFloat("Y");
                headings[currentRow] = row.getFloat("THETA");
                velocityXs[currentRow] = row.getFloat("VX");
                velocityYs[currentRow] = row.getFloat("VY");
                angularVs[currentRow] = row.getFloat("VA");
                
                currentRow++;
            }
        }
        
        if (this.positionXs.length != 0) {
            this.hasData = true;
        }
        
        println(this.positionXs.length);
    }
    
    public void draw() {
        if (!hasData) {
            pushMatrix();
            fill(255, 0, 0);
            translate(0, 0, 50);
            scale(3);
            text("No test.csv found", 0, 0);
            popMatrix();
        }
        
        pushMatrix();
        
        // x and y are in meters
        translate(meterToPx(this.x), -meterToPx(this.y), meterToPx(0.1f));
        rotateZ(this.heading);
        drawRobot();
        popMatrix();
        
        if (this.hasData) {
            if (DRAW_TRAIL) drawTrail();
            if (DRAW_VEL_IND) drawSpeedVector();
            if (DRAW_AVL_IND) drawAngularVector();
            
            if (PLAYING) {
                this.update(true);
            }
        }
    }
    
    public void update(Boolean incrementFrames) {
        if (frame < this.frames) {
            int elapsedTime = millis() - startTime;
            if (this.times[frame] * 1000 <= elapsedTime || !REAL_SPEED) {
                this.x = this.positionXs[frame];
                this.y = this.positionYs[frame];
                this.heading = this.headings[frame];
                
                if (incrementFrames) frame++;
            }
        }
    }
    
    public void drawRobot() {
        stroke(0);
        box(meterToPx(0.18f));
        translate(13, 0, 0);
        beginShape();
        fill(0xffFF9944);
        noStroke();
        vertex(0, 0);
        vertex(-2, -5);
        vertex(10, 0);
        vertex(-2, 5);
        endShape(CLOSE);
    }
    
    // Probably not the best way to do it
    public void drawTrail() {
        strokeWeight(4);
        stroke(150);
        for (int i = 0; i < frame; i++) {
            if (i > 0) {
                line(meterToPx(this.positionXs[i - 1]), -meterToPx(this.positionYs[i - 1]), 0,
                meterToPx(this.positionXs[i]), -meterToPx(this.positionYs[i]), 0);
            }
        }
    }
    
    // Draw vectors such as speed or rotation
    public void drawSpeedVector() {
        pushMatrix();
        translate(meterToPx(this.x), -meterToPx(this.y), meterToPx(0.1f));
        strokeWeight(5);
        stroke(0xff4499FF);
        
        int lastFrame = frame >= this.frames ? this.frames-1: frame;
        line(0, 0, 0, meterToPx(this.velocityXs[lastFrame]), -meterToPx(this.velocityYs[lastFrame]), 0);
        popMatrix();
    }
    
    public void drawAngularVector() {
        pushMatrix();
        translate(meterToPx(this.x), -meterToPx(this.y), meterToPx(0.12f));
        rotateZ(this.heading);
        strokeWeight(5);
        stroke(0xff66FF66);
        noFill();
        int lastFrame = frame >= this.frames ? this.frame-1: frame;
        arc(0, 0, meterToPx(0.5f), meterToPx(0.5f), 0, constrain(angularVs[lastFrame], 0, PI/2));
        popMatrix();
    }
    
    public void reset() {
        this.frame = 0;
        this.x = 0;
        this.y = 0;
        this.heading = 0;
        this.startTime = millis();
    }
    
    public int getFrame() {
        return this.frame;
    }
    
    public float getTime() {
        if (!hasData) return 0;
        
        int lastFrame = frame >= this.frames ? this.frame-1: frame;
        return this.times[lastFrame];
    }
    
    public void setFrame(int frame) {
        if (!hasData) return;
        
        println(frame);
        if (frame < 0) frame = 0;
        if (frame > frames) frame = frames;
        this.frame = frame;
        
        this.update(false);
    }
}
    public void settings() {  size(1280, 800, P3D); }
    static public void main(String[] passedArgs) {
        String[] appletArgs = new String[] { "fwView" };
        if (passedArgs != null) {
          PApplet.main(concat(appletArgs, passedArgs));
        } else {
          PApplet.main(appletArgs);
        }
    }
}
