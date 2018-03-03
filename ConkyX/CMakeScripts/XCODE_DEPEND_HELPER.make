# DO NOT EDIT
# This makefile makes sure all linkable targets are
# up-to-date with anything they link to
default:
	echo "Do not invoke directly"

# Rules to remove targets that are older than anything to which they
# link.  This forces Xcode to relink the targets from scratch.  It
# does not seem to check these dependencies itself.
PostBuild.conky.Debug:
/Users/np/ConkyX/conky-for-macOS/ConkyX/src/Debug/conky:\
	/usr/X11R6/lib/libSM.dylib\
	/usr/X11R6/lib/libICE.dylib\
	/usr/X11R6/lib/libX11.dylib\
	/usr/X11R6/lib/libXext.dylib\
	/usr/X11R6/lib/libXdamage.dylib\
	/usr/X11R6/lib/libXfixes.dylib\
	/usr/X11R6/lib/libXft.dylib\
	/usr/X11R6/lib/libXinerama.dylib
	/bin/rm -f /Users/np/ConkyX/conky-for-macOS/ConkyX/src/Debug/conky


PostBuild.conky.Release:
/Users/np/ConkyX/conky-for-macOS/ConkyX/src/Release/conky:\
	/usr/X11R6/lib/libSM.dylib\
	/usr/X11R6/lib/libICE.dylib\
	/usr/X11R6/lib/libX11.dylib\
	/usr/X11R6/lib/libXext.dylib\
	/usr/X11R6/lib/libXdamage.dylib\
	/usr/X11R6/lib/libXfixes.dylib\
	/usr/X11R6/lib/libXft.dylib\
	/usr/X11R6/lib/libXinerama.dylib
	/bin/rm -f /Users/np/ConkyX/conky-for-macOS/ConkyX/src/Release/conky


PostBuild.conky.MinSizeRel:
/Users/np/ConkyX/conky-for-macOS/ConkyX/src/MinSizeRel/conky:\
	/usr/X11R6/lib/libSM.dylib\
	/usr/X11R6/lib/libICE.dylib\
	/usr/X11R6/lib/libX11.dylib\
	/usr/X11R6/lib/libXext.dylib\
	/usr/X11R6/lib/libXdamage.dylib\
	/usr/X11R6/lib/libXfixes.dylib\
	/usr/X11R6/lib/libXft.dylib\
	/usr/X11R6/lib/libXinerama.dylib
	/bin/rm -f /Users/np/ConkyX/conky-for-macOS/ConkyX/src/MinSizeRel/conky


PostBuild.conky.RelWithDebInfo:
/Users/np/ConkyX/conky-for-macOS/ConkyX/src/RelWithDebInfo/conky:\
	/usr/X11R6/lib/libSM.dylib\
	/usr/X11R6/lib/libICE.dylib\
	/usr/X11R6/lib/libX11.dylib\
	/usr/X11R6/lib/libXext.dylib\
	/usr/X11R6/lib/libXdamage.dylib\
	/usr/X11R6/lib/libXfixes.dylib\
	/usr/X11R6/lib/libXft.dylib\
	/usr/X11R6/lib/libXinerama.dylib
	/bin/rm -f /Users/np/ConkyX/conky-for-macOS/ConkyX/src/RelWithDebInfo/conky




# For each target create a dummy ruleso the target does not have to exist
/usr/X11R6/lib/libICE.dylib:
/usr/X11R6/lib/libSM.dylib:
/usr/X11R6/lib/libX11.dylib:
/usr/X11R6/lib/libXdamage.dylib:
/usr/X11R6/lib/libXext.dylib:
/usr/X11R6/lib/libXfixes.dylib:
/usr/X11R6/lib/libXft.dylib:
/usr/X11R6/lib/libXinerama.dylib:
