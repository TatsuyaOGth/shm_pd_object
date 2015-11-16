#pragma once

#include "ofMain.h"
#include "ofxSharedMemory.h"

#define NUM_DATA 256

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    //----------
    // data send
    //----------
    float* mSendData;
    bool mfSenderConnection;
    ofxSharedMemory<float *> mShmDataSender;
    
    
    //----------
    // data receive
    //----------
    float* mReceiveData;
    bool mfReceiverConnection;
    ofxSharedMemory<float *> mShmDataReceiver;
    
};
