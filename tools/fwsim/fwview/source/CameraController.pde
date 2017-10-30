import peasy.*;

PeasyCam cam;

public void createCam() {
    // Create camera instance
    cam = new PeasyCam(this, meterToPx(10));
    cam.setMinimumDistance(0.01);
    cam.setMaximumDistance(meterToPx(50));
    cam.setSuppressRollRotationMode();
    
    // remap camera functions
    cam.setRightDragHandler(cam.getRotateDragHandler());
    cam.setCenterDragHandler(cam.getPanDragHandler());
    cam.setLeftDragHandler(null);
}