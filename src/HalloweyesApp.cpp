//
//  Halloweyes.cpp
//  Halloweyes
//
//  Creepy eyes that follow movement captured by a Kinect.
//
//  Best effect if the eyes are projected on a big screen above
//  passers-by.
//
//  Created by Eric Miller on 10/29/13.
//
//

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"

#include "CinderFreenect.h"
#include "CinderOpenCV.h"

#include "Eyes.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define EYES_Z_LOC 200.0

class HalloweyesApp : public AppBasic {
  public:
	void prepareSettings( Settings* settings );
	void setup();
	void update();
	void updateTarget();
	void draw();
	
	void mouseDown(MouseEvent event);
	void mouseMove(MouseEvent event);
	void mouseWheel(MouseEvent event);
	void resize();
	
	KinectRef		mKinect;
	
	// capture a few frames on startup; subtract them from later frames to
	// remove stationary items from consideration as targets.
	cv::Mat			mBaseImage;
	int				mBaseAvgCount;
	
	// frames from the kinect's depth sensor
	ImageSourceRef	mDepthImage;
	ImageSourceRef	mLastDepthImage;
	
	// the blobbified image -- basis of tracking
	gl::Texture		mBlobTexture;

	Vec3f			mMouseLoc;
	CameraPersp		mCam;
	
	// though we're only using one pair, i played around with multiple eyes, so
	// we've got a vector of them.
	vector<Eyes>	mEyes;

	params::InterfaceGl mParams;
	
	float mThreshold, mBlobMin, mBlobMax;
	
	// the "target" where the eyes look. using pos/vel/acc
	// to make the motion smooth.
	Vec3f mScreenTargetPos, mScreenTargetVel, mScreenTargetAccel;
};

void HalloweyesApp::prepareSettings( Settings* settings )
{
	settings->setWindowSize( 1280, 900 );
}

void HalloweyesApp::setup()
{
	console() << Kinect::getNumDevices() << " kinects";
	if (Kinect::getNumDevices() > 0) {
		mKinect = Kinect::create();
	}
	
	mEyes.push_back(Eyes(Vec3f(0, 50, EYES_Z_LOC)));
	
	
	// tweak these as needed.
	mThreshold = 40.0f;
	mBlobMin = 80.0f;
	mBlobMax = 300.0f;
	
	mParams = params::InterfaceGl( "Tracking", Vec2i( 150, 300 ) );
	mParams.addParam( "Threshold", &mThreshold, "min=0.0 max=255.0 step=1.0 keyIncr=s keyDecr=w" );
	mParams.addParam( "Blob Minimum Radius", &mBlobMin, "min=1.0 max=200.0 step=1.0 keyIncr=e keyDecr=d" );
	mParams.addParam( "Blob Maximum Radius", &mBlobMax, "min=1.0 max=200.0 step=1.0 keyIncr=r keyDecr=f" );
	
	mBaseAvgCount = 0;
	
	mMouseLoc = Vec3f::zero();
	
	gl::enableDepthWrite();
	gl::enableDepthRead();
	gl::enableAlphaBlending();
	
	glEnable( GL_LIGHTING );
	
	resize();
}

void HalloweyesApp::update()
{
	if (mKinect) {
		if( mKinect->checkNewDepthFrame() ) {
			if (mDepthImage) {
				mLastDepthImage = mDepthImage;
			}
			mDepthImage = mKinect->getDepthImage();
			if (mBaseImage.empty()) {
				mBaseImage = toOcv(Channel8u(mDepthImage));
			}
		}
		updateTarget();
	}
	
	Vec3f eyeTarget, targetBase;
	if (mDepthImage) {
		targetBase = mScreenTargetPos;
	}
	else {
		targetBase = mMouseLoc;
	}

	// targetBase is in normalized [0, 1] coordinates. convert to eyeball space.
	//
	// you'll need to tweak these numbers based on the kinect's location and
	// orientation wrt the projected eyes. for the numbers used here, the kinect
	// was mounted above a patio and the eyes were projected on a screen
	// in a window of the house. when a person walked towards the window, it corresponded
	// to a low y coordinate, so with this setup, as y drops to zero, the eyes converge
	// a bit -- looking down.
	//
	// x axis: left/right (you may need to reverse the sign depending on your setup)
	// y axis: up/down
	// z axis: converge/stare straight
	//
	// the general scale of the scene will determine the specific constants used.
	//
	eyeTarget.x = targetBase.x * -260 + 130;
	eyeTarget.y = targetBase.y * -80.0;
	eyeTarget.z = EYES_Z_LOC - 100.0 - ((1 - targetBase.y) * 250);
	
	for (vector<Eyes>::iterator it = mEyes.begin(); it != mEyes.end(); it++) {
		it->setLookAt(eyeTarget);
		it->update();
	}

}

void HalloweyesApp::updateTarget()
{
	Vec3f currentCenter;
	mScreenTargetVel += mScreenTargetAccel;
	mScreenTargetVel *= 0.2f;
	mScreenTargetPos += mScreenTargetVel;
	
	mScreenTargetAccel = Vec3f::zero();
	
	if (mDepthImage) {
		
		
		// adapted from Andy Berg's blob tracking code posted here:
		// http://forum.libcinder.org/topic/simple-hand-tracking-with-kinect-opencv
		
		cv::Mat input(toOcv(Channel8u(mDepthImage))), subtracted, combined, blurred, thresholded, thresholded2, output;

		// on launch, make sure there is no one/nothing moving on the stage.
		// we collect a few frames which we will then subtract to take out stationary items which might otherwise
		// be considered blobs.
		if (mBaseAvgCount == 0) {
			mBaseImage = input;
			mBaseAvgCount++;
		}
		else if (mBaseAvgCount < 3) {
			cv::addWeighted(input, 0.5f, mBaseImage, 0.5f, 0.0, mBaseImage);
			mBaseAvgCount++;
		}
		
		// add in a bit of the previous frame--in my experience this helped stabilize blob detection.
		if (mLastDepthImage) {
			cv::addWeighted(input, 0.8f, toOcv(Channel8u(mLastDepthImage)), 0.2, 0.0, combined);
		}
		else {
			combined = input;
		}
		
		// subtract the base image from launch -- we should only be working with deltas
		// from the static, empty stage.
		cv::subtract(combined, mBaseImage, subtracted);
		
		cv::blur(subtracted, blurred, cv::Size(10,10));
		
		// make two thresholded images: one to display and one
		// to pass to find contours since its process alters the image
		cv::threshold( blurred, thresholded, mThreshold, 255, CV_THRESH_BINARY);
		cv::threshold( blurred, thresholded2, mThreshold, 255, CV_THRESH_BINARY);
		
		// 2d vector to store the found contours
		vector<vector<cv::Point> > contours;
		// find em
		cv::findContours(thresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		
		// convert theshold image to color for output
		// so we can draw blobs on it
		cv::cvtColor( thresholded2, output, CV_GRAY2RGB );
		
		int nBlobs = 0;
		Vec2f imageCenter(input.cols / 2.0f, input.rows / 2.0f);
		currentCenter.x = imageCenter.x;
		currentCenter.y = imageCenter.y;

		// loop the stored contours
		for (vector<vector<cv::Point> >::iterator it=contours.begin() ; it < contours.end(); it++ ){
			
			// center and radius for current blob
			cv::Point2f center;
			float radius;
			// convert the contour point to a matrix
			vector<cv::Point> pts = *it;
			cv::Mat pointsMatrix = cv::Mat(pts);
			// pass to min enclosing circle to make the blob
			cv::minEnclosingCircle(pointsMatrix, center, radius);
			
			if (radius > mBlobMin && radius < mBlobMax) {
				// rather than the circle center we're going to use the "centroid" of the blob.
				// the circle center may be sufficient if performance is suffering.
				cv::Moments moments = cv::moments(pointsMatrix);
				center.x = (int)(moments.m10 / moments.m00);
				center.y = (int)(moments.m01 / moments.m00);
				currentCenter.x += (center.x - imageCenter.x);
				currentCenter.y += (center.y - imageCenter.y);

				// draw little circles to debug blob centroids
				cv::circle(output, center, 5, cv::Scalar(255, 0, 0));
				nBlobs++;
			}
		}
		
		// simple average of all the blob centers
		if (nBlobs > 0) {
			currentCenter.x /= nBlobs;
			currentCenter.y /= nBlobs;
		}

		// big circle to debug eye target.
		cv::circle(output, cv::Point2f(currentCenter.x, currentCenter.y), 10, cv::Scalar(0, 0, 255));

		// normalize to range [0, 1]
		currentCenter.x /= input.cols;
		currentCenter.y /= input.rows;
		
		mBlobTexture = gl::Texture(fromOcv(output));
	}
	
	// add some acceleration towards new center.
	mScreenTargetAccel += (currentCenter - mScreenTargetPos) * 0.4;
}

void HalloweyesApp::mouseDown(MouseEvent event)
{
	for (vector<Eyes>::iterator it = mEyes.begin(); it != mEyes.end(); it++) {
		it->blink();
	}
}

void HalloweyesApp::mouseMove( MouseEvent event )
{
	mMouseLoc = Vec3f(
					  ((float)event.getX() / getWindowWidth()),
					  ((float)event.getY() / getWindowHeight()),
					  100);
}

void HalloweyesApp::mouseWheel(MouseEvent event)
{
	for (vector<Eyes>::iterator it = mEyes.begin(); it != mEyes.end(); it++) {
		it->openIris(event.getWheelIncrement() * 0.1);
		it->evilLids(event.getWheelIncrement() * 0.1);
	}
}

void HalloweyesApp::resize()
{
	mCam.lookAt(Vec3f::zero(), Vec3f(0.0f, 0.0f, 100.0f));
	mCam.setPerspective( 45, getWindowAspectRatio(), 1, 5000 );
}

void HalloweyesApp::draw()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
#if DEBUG
	gl::setMatricesWindow(getWindowWidth(), getWindowHeight());

	mParams.draw();
	
	gl::color(1.0, 1.0, 1.0);
	gl::pushMatrices();
	gl::scale(0.65, 0.65);
	
	if( mDepthImage ) {
		gl::Texture depthTex(mDepthImage);
		gl::translate(Vec2f(0, 800));
		gl::draw(depthTex);

		if (mBlobTexture) {
			gl::translate(Vec2f(depthTex.getWidth(), 0));
			gl::draw(mBlobTexture);
		}

		if (!mBaseImage.empty()) {
			gl::translate(Vec2f(mBlobTexture.getWidth(), 0));
			gl::draw(fromOcv(mBaseImage));
		}
	}
	gl::popMatrices();
	
#endif
	gl::setMatrices(mCam);
	
	GLfloat staticLightPosition[] = { -5000, 1000, -9000, 1.0 };
	glEnable( GL_LIGHT0 );
	glLightfv( GL_LIGHT0, GL_POSITION, staticLightPosition );
	glLightf( GL_LIGHT0, GL_CONSTANT_ATTENUATION, 2.0f );
	
	for (vector<Eyes>::iterator it = mEyes.begin(); it != mEyes.end(); it++) {
		it->draw();
	}

}

CINDER_APP_BASIC( HalloweyesApp, RendererGl )
