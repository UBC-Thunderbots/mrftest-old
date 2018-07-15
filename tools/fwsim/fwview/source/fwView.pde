import peasy.*;
import peasy.org.apache.commons.math.*;
import peasy.org.apache.commons.math.geometry.*;

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

void setup() {
    size(1280, 800, P3D);
    
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

void draw() {
    background(200);
    if (DRAW_ORIGIN) drawTestAxes();
    drawTestObjs();
    
    robot.draw();
    
    drawHUD();
}

void drawTestObjs() {
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

void drawTestAxes() {
    strokeWeight(3);
    stroke(255, 0, 0);
    line(0, 0, 0, meterToPx(1), 0, 0);
    stroke(0, 255, 0);
    line(0, 0, 0, 0, meterToPx(-1), 0);
    stroke(0, 0, 255);
    line(0, 0, 0, 0, 0, meterToPx(1));
}

void drawTestLights() {
    directionalLight(240, 220, 180, 0, 0.3, -1);
    ambientLight(90, 130, 160);
}

void drawHUD() {
    cam.beginHUD();
    displayGUI();
    fill(20);
    textSize(12);
    text("Time: " + String.format("%.1f", robot.getTime()) + "s", 20, 20);
    cam.endHUD();
}