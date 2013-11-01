halloweyes
==========

Creepy Cinder and Kinect-powered eyes that follow movement.

For Halloween 2013 I decided to make a little interactive installation on my front porch.

![Eyes in window](/images/house-eyes.jpg)

Requires:

- [Cinder](http://libcinder.org)
- [OpenCV](http://opencv.org)
- [OpenKinect](http://openkinect.org)

And a Kinect, of course. Also a projector if you want to project the eyes onto something big.

This repo contains the XCode project for Mac. I haven't tested on other platforms but it should work to the extent that Cinder works. The OpenCV and Cinder-Freenect blocks are included in the repo, but for later versions of Cinder they may need to be replaced.

This was a quick-n-dirty project and there are a lot of constants in the code that are just hard-coded for my front porch. I used rear projection, for example, so the x-axis is reversed. Also the kinect was mounted above:

![Kinect mounted](/images/kinect-mounting.jpg)

Anyway, those constants would need to be modified depending on the orientation and location of the eyes and the kinect. Everything could be paramterized and made more general and user-friendly ... someday ... :)

Oh, if you don't have a kinect the program will still run—the eyes will track the mouse (though with x-axis reversed).