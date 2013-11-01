//
//  Eyes.h
//  Halloweyes
//
//  Created by Eric Miller on 10/29/13.
//
//

#ifndef __Halloweyes__Eyes__
#define __Halloweyes__Eyes__

#include <iostream>
#include "cinder/app/AppBasic.h"
#include "cinder/gl/GlslProg.h"

using namespace ci;

class Eyes {
public:
	Eyes(Vec3f position);
	
	void update();
	
	// look at is actually setting the location of a "light".
	// the pupils and irises are the specular highlight of this
	// "light". it's not *quite* the same as legit lookat point, but
	// by tweaking stuff elsewhere it works itself out. :)
	void setLookAt(Vec3f lookAt);
	
	// open "pupil" i suppose, technically.
	void openIris(float amt);
	
	// evil is a verb here.
	void evilLids(float amt);
	void blink();
	
	void draw();
	
private:
	void drawHalfSphere(int scaley, int scalex, GLfloat r);
	
	typedef enum {
		BlinkPhase_none,
		BlinkPhase_closing,
		BlinkPhase_opening
	} BlinkPhase;
	
	typedef enum {
		EvilPhase_none,
		EvilPhase_turning,
		EvilPhase_evil,
		EvilPhase_healing
	} EvilPhase;
	
	typedef enum {
		IrisPhase_none,
		IrisPhase_dilating,
		IrisPhase_closing
	} IrisPhase;
	
	Vec3f			mPos;
	Vec3f			mLookAt;
	
	// @TODO: these things could all be generalized a bit
	float			mBlinkAmt;
	BlinkPhase		mBlinkPhase;
	int				mNonBlinkFrames;
	
	int				mEvilFrames;
	EvilPhase		mEvilPhase;
	float			mEvilLidsAmt;
	int				mNonEvilFrames;

	IrisPhase		mIrisPhase;
	float			mIrisOpenAmt;
	float			mIrisDilationDestination;
	int				mIrisChangedFrames;

	gl::GlslProg	mShader;
	
};

#endif /* defined(__Halloweyes__Eyes__) */
