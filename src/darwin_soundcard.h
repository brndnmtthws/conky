/*
 *  This file is part of conky.
 *
 *  conky is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  conky is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with conky.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Nickolas Pylarinos
 *
 *  NOTE:   I havent done any work in the darwin_soundcard.h!!!
 *          I just copied the following from the freebsd soundcard.h just so that conky compiles!
 *
 *  Credit goes to the developers
 */

#ifndef DARWIN_SOUNDCARD_H
#define DARWIN_SOUNDCARD_H

#define SOUND_DEVICE_NAMES	{ \
"vol", "bass", "treble", "synth", "pcm", "speaker", "line", \
"mic", "cd", "mix", "pcm2", "rec", "igain", "ogain", \
"line1", "line2", "line3", "dig1", "dig2", "dig3", \
"phin", "phout", "video", "radio", "monitor"}

#define MIXER_READ(dev)		_IOR('M', dev, int)

#endif // DARWIN_SOUNDCARD_H
