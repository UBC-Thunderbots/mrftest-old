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
    
    void populateSim(Table tb) {
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
        translate(meterToPx(this.x), meterToPx(this.y), meterToPx(0.1));
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
        box(meterToPx(0.18));
        translate(13, 0, 0);
        beginShape();
        fill(#FF9944);
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
                line(meterToPx(this.positionXs[i - 1]), meterToPx(this.positionYs[i - 1]), 0,
                meterToPx(this.positionXs[i]), meterToPx(this.positionYs[i]), 0);
            }
        }
    }
    
    // Draw vectors such as speed or rotation
    public void drawSpeedVector() {
        pushMatrix();
        translate(meterToPx(this.x), meterToPx(this.y), meterToPx(0.1));
        strokeWeight(5);
        stroke(#4499FF);
        
        int lastFrame = frame >= this.frames ? this.frames-1: frame;
        line(0, 0, 0, meterToPx(this.velocityXs[lastFrame]), meterToPx(this.velocityYs[lastFrame]), 0);
        popMatrix();
    }
    
    public void drawAngularVector() {
        pushMatrix();
        translate(meterToPx(this.x), meterToPx(this.y), meterToPx(0.12));
        rotateZ(this.heading);
        strokeWeight(5);
        stroke(#66FF66);
        noFill();
        int lastFrame = frame >= this.frames ? this.frame-1: frame;
        arc(0, 0, meterToPx(0.5), meterToPx(0.5), 0, constrain(angularVs[lastFrame], 0, PI/2));
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