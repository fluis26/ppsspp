tvOS Build Instructions
======================

Prerequisites:
--------------

* Xcode (from the Mac App Store) with command line tools installed
* MacPorts (from macports.org); easiest to install with their package installers
* cmake build system (from MacPorts); run "sudo port install cmake" from the command line

If you need to build ffmpeg yourself too, then you'll also need:

* gas-preprocessor; download the zip from https://github.com/mansr/gas-preprocessor, unzip and from the command line run:

        sudo cp gas-preprocessor.pl /usr/bin/
        sudo chmod +rw /usr/bin/gas-preprocessor.pl

* you may need pkg-config (from MacPorts); run "sudo port install pkgconfig" from the command line

Most of this is done from the command line:
-------------------------------------------

Change directory to wherever you want to install ppsspp (eg. "cd ~"), and then clone the main ppsspp repository:

    git clone https://github.com/hrydgard/ppsspp.git

Change directory to the newly created ppsspp directory and run:

    git submodule update --init

The above command will pull in the submodules required by PPSSPP, including the native, ffmpeg, and lang directories.  Included in the ffmpeg directory should be the necessary libs and includes for ffmpeg, so most people can skip the next command.  However, if you need to recompile ffmpeg for some reason, change directory into ffmpeg and run (this will take a while):

    ./tvos-build.sh

Change directory back up to the main ppsspp directory and do the following:

    ./b.sh --tvos-xcode

You should have an Xcode project file in the build-tvos-xcode directory named PPSSPP.xcodeproj. Open it in XCode, set up a provisioning profile and build it. 

Uploading ROMs to PPSSPP:
-------------------------------------------
tvOS version contains a GCDWebServer for uploading content to PPSSPP. Make sure your device's WiFi is turned on and connected to the same network as your computer. Check your Apple TV IP address (or Bonjour name) and open a web browser     
    
    http://[appletv-ip]

Another option is to use WebDAV.

Known issues
--------------
* ~~Sound stops working when the emulator exits to the AppleTV menu~~ Fixed by properly restarting audio 
* When B (circle) button is held longer emulator exits to the Apple TV menu - there's a temporary workaround by switching square and circle buttons (circle buttons are usually used for long press actions). The issue is that main view controller GLKViewController inherits from the UIViewController, that means that game controller B button acts as menu button. To resolve this permanently main view controller must use GCEventViewController together with the GLKViewDelagate.