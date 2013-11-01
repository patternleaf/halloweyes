//
//  Eyes.cpp
//  Halloweyes
//
//  Created by Eric Miller on 10/29/13.
//
//  The eye shader is based on the Cinder GLSLLighting sample code.
//

#include "Eyes.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "Resources.h"
#include "cinder/app/AppBasic.h"

GLfloat mat_ambient[]		= { 0.8, 0.8, 0.8, 1.0 };
GLfloat mat_diffuse[]		= { 0.9, 0.9, 0.9, 1.0 };
GLfloat mat_specular[]		= { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_emission[]		= { 0.6, 0.6, 0.6, 0.0 };
GLfloat mat_shininess[]		= { 128.0 };


using namespace ci;

Eyes::Eyes(Vec3f position)
:mPos(position),
mBlinkAmt(0.0),
mBlinkPhase(BlinkPhase_none),
mNonBlinkFrames(0),
mNonEvilFrames(0),
mEvilPhase(EvilPhase_none),
mIrisPhase(IrisPhase_none),
mIrisChangedFrames(0),
mIrisOpenAmt(0.5f),
mEvilLidsAmt(0.0f)
{
	mShader = gl::GlslProg( app::loadResource( RES_VERT_GLSL ), app::loadResource( RES_FRAG_GLSL ) );
}

void Eyes::blink()
{
	if (mBlinkPhase == BlinkPhase_none) {
		mBlinkPhase = BlinkPhase_closing;
	}
}

void Eyes::update()
{
	if (mBlinkPhase == BlinkPhase_none && randFloat() < 0.01 && mNonBlinkFrames > 400) {
		mBlinkPhase = BlinkPhase_closing;
		mNonBlinkFrames = 0;
	}
	else if (mBlinkPhase == BlinkPhase_closing) {
		mBlinkAmt += 0.1;
		if (mBlinkAmt >= 1.0) {
			mBlinkAmt = 1.0;
			mBlinkPhase = BlinkPhase_opening;
		}
	}
	else if (mBlinkPhase == BlinkPhase_opening) {
		mBlinkAmt -= 0.1;
		if (mBlinkAmt <= 0.0) {
			mBlinkAmt = 0.0;
			mBlinkPhase = BlinkPhase_none;
		}
	}
	mNonBlinkFrames++;
	
	float evilChance = 1.0 - (mLookAt.distance(mPos) / 300.0);
	
	switch(mEvilPhase) {
	case EvilPhase_none:
		if (randFloat() < 0.1 * evilChance && mNonEvilFrames > 600) {
			mEvilPhase = EvilPhase_turning;
			mNonEvilFrames = 0;
		}
		else {
			mNonEvilFrames++;
		}
		break;
	case EvilPhase_turning:
		mEvilLidsAmt += 0.1;
		if (mEvilLidsAmt >= 1.0) {
			mEvilLidsAmt = 1.0;
			mEvilFrames = 0;
			mEvilPhase = EvilPhase_evil;
		}
		break;
	case EvilPhase_evil:
		if (mEvilFrames >= 300) {
			mEvilPhase = EvilPhase_healing;
		}
		mEvilFrames++;
		break;
	case EvilPhase_healing:
		mEvilLidsAmt -= 0.1;
		if (mEvilLidsAmt <= 0.0) {
			mEvilLidsAmt = 0.0;
			mEvilPhase = EvilPhase_none;
		}
		break;
	}
	
	switch(mIrisPhase) {
	case IrisPhase_none:
		if (randFloat() < 0.4 * evilChance && mIrisChangedFrames > 300) {
			mIrisChangedFrames = 0;
			mIrisDilationDestination = (randFloat() * 0.83) + 0.02;
			if (mIrisDilationDestination < mIrisOpenAmt) {
				mIrisPhase = IrisPhase_closing;
			}
			else {
				mIrisPhase = IrisPhase_dilating;
			}
		}
		else {
			mIrisChangedFrames++;
		}
		break;
	case IrisPhase_dilating:
		mIrisOpenAmt += 0.2;
		if (mIrisOpenAmt >= mIrisDilationDestination) {
			mIrisOpenAmt = mIrisDilationDestination;
			mIrisPhase = IrisPhase_none;
		}
		break;
	case IrisPhase_closing:
		mIrisOpenAmt -= 0.1;
		if (mIrisOpenAmt <= mIrisDilationDestination) {
			mIrisOpenAmt = mIrisDilationDestination;
			mIrisPhase = IrisPhase_none;
		}
		break;
	}
}

void Eyes::setLookAt(Vec3f lookAt)
{
	mLookAt = lookAt;
}

void Eyes::openIris(float amt)
{
	mIrisOpenAmt += amt;
	if (mIrisOpenAmt >= 0.85f) {
		mIrisOpenAmt = 0.85f;
	}
	if (mIrisOpenAmt <= 0.02f) {
		mIrisOpenAmt = 0.02f;
	}
}

void Eyes::evilLids(float amt)
{
	mEvilLidsAmt += amt;
	if (mEvilLidsAmt >= .99f) {
		mEvilLidsAmt = 1.0f;
	}
	if (mEvilLidsAmt <= -0.2f) {
		mEvilLidsAmt = -0.2f;
	}

}

void Eyes::draw()
{
	float eyeDist = 40.0f;
	int eyeResolution = 40;
	int lidResolution = 20;
	
	ci::ColorA color( CM_HSV, 0.4f, 0.4f, 0.4f, 1.0f );
	glMaterialfv( GL_FRONT, GL_DIFFUSE,	color );
	glMaterialfv( GL_FRONT, GL_AMBIENT,	mat_ambient );
	glMaterialfv( GL_FRONT, GL_SPECULAR, mat_specular );
	glMaterialfv( GL_FRONT, GL_SHININESS, mat_shininess );
	glMaterialfv( GL_FRONT, GL_EMISSION, mat_emission );
	
	gl::pushMatrices();
	
	gl::translate(mPos);
	
	mShader.bind();
	mShader.uniform( "NumEnabledLights", 1 );
	mShader.uniform("lookAtPosition", mLookAt);
	mShader.uniform( "irisSize", mIrisOpenAmt );

	gl::drawSphere(Vec3f(-eyeDist, -10, 0), 20, eyeResolution);
	gl::drawSphere(Vec3f(eyeDist, -10, 0), 20, eyeResolution);
	mShader.unbind();
	
	ci::ColorA eyelidColor( CM_HSV, 0.0f, 0.0f, 0.0f, 1.0f );
	GLfloat eyelidShininess[] = { 0 };
	gl::color(0.0, 0.0, 0.0);
	glMaterialfv( GL_FRONT, GL_DIFFUSE,	eyelidColor );
	glMaterialfv( GL_FRONT, GL_AMBIENT,	eyelidColor );
	glMaterialfv( GL_FRONT, GL_SPECULAR, eyelidColor );
	glMaterialfv( GL_FRONT, GL_SHININESS, eyelidShininess );
	glMaterialfv( GL_FRONT, GL_EMISSION, eyelidColor );

	float upperRotation = ((1 - mBlinkAmt) * 40.0) - 20.0;
	float lowerRotation = ((1 - mBlinkAmt) * -20.0) - 200.0;
	
	float evilRotation = mEvilLidsAmt * 20;
	
	upperRotation -= mEvilLidsAmt * 10;
	lowerRotation += mEvilLidsAmt * 10;
	
	gl::pushMatrices();
	gl::translate(Vec2f(-eyeDist, -10));
	gl::pushMatrices();
	gl::rotate(Vec3f(upperRotation, 0.0f, -evilRotation));
	drawHalfSphere(lidResolution, lidResolution, 21);
	gl::popMatrices();
	gl::rotate(Vec3f(lowerRotation, 0.0f, 0.0f));
	drawHalfSphere(lidResolution, lidResolution, 21);
	gl::popMatrices();

	gl::pushMatrices();
	gl::translate(Vec2f(eyeDist, -10));
	gl::pushMatrices();
	gl::rotate(Vec3f(upperRotation, 0.0f, evilRotation));
	drawHalfSphere(lidResolution, lidResolution, 21);
	gl::popMatrices();
	gl::rotate(Vec3f(lowerRotation, 0.0f, 0.0f));
	drawHalfSphere(lidResolution, lidResolution, 21);
	gl::popMatrices();
	
	
	gl::popMatrices();
}

// Courtesy user Sumpfratte at:
// https://www.opengl.org/discussion_boards/showthread.php/159402-half-sphere
//
void Eyes::drawHalfSphere(int scaley, int scalex, GLfloat r) {
	int i, j;
	GLfloat v[scalex*scaley][3];
	
	for (i=0; i<scalex; ++i) {
		for (j=0; j<scaley; ++j) {
			v[i*scaley+j][0]=r*cos(j*2*M_PI/scaley)*cos(i*M_PI/(2*scalex));
			v[i*scaley+j][1]=r*sin(i*M_PI/(2*scalex));
			v[i*scaley+j][2]=r*sin(j*2*M_PI/scaley)*cos(i*M_PI/(2*scalex));
		}
	}
	
	glBegin(GL_QUADS);
	for (i=0; i<scalex-1; ++i) {
		for (j=0; j<scaley; ++j) {
			glVertex3fv(v[i*scaley+j]);
			glVertex3fv(v[i*scaley+(j+1)%scaley]);
			glVertex3fv(v[(i+1)*scaley+(j+1)%scaley]);
			glVertex3fv(v[(i+1)*scaley+j]);
		}
	}
	glEnd();
}