/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include <string.h>
#include <strings.h>

typedef struct _builtinColor {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned short name;
} BuiltinColor;

static const char BuiltinColorNames[] = {
    "alice blue\0"
    "AliceBlue\0"
    "antique white\0"
    "AntiqueWhite\0"
    "AntiqueWhite1\0"
    "AntiqueWhite2\0"
    "AntiqueWhite3\0"
    "AntiqueWhite4\0"
    "aquamarine\0"
    "aquamarine1\0"
    "aquamarine2\0"
    "aquamarine3\0"
    "aquamarine4\0"
    "azure\0"
    "azure1\0"
    "azure2\0"
    "azure3\0"
    "azure4\0"
    "beige\0"
    "bisque\0"
    "bisque1\0"
    "bisque2\0"
    "bisque3\0"
    "bisque4\0"
    "black\0"
    "blanched almond\0"
    "BlanchedAlmond\0"
    "blue\0"
    "blue violet\0"
    "blue1\0"
    "blue2\0"
    "blue3\0"
    "blue4\0"
    "BlueViolet\0"
    "brown\0"
    "brown1\0"
    "brown2\0"
    "brown3\0"
    "brown4\0"
    "burlywood\0"
    "burlywood1\0"
    "burlywood2\0"
    "burlywood3\0"
    "burlywood4\0"
    "cadet blue\0"
    "CadetBlue\0"
    "CadetBlue1\0"
    "CadetBlue2\0"
    "CadetBlue3\0"
    "CadetBlue4\0"
    "chartreuse\0"
    "chartreuse1\0"
    "chartreuse2\0"
    "chartreuse3\0"
    "chartreuse4\0"
    "chocolate\0"
    "chocolate1\0"
    "chocolate2\0"
    "chocolate3\0"
    "chocolate4\0"
    "coral\0"
    "coral1\0"
    "coral2\0"
    "coral3\0"
    "coral4\0"
    "cornflower blue\0"
    "CornflowerBlue\0"
    "cornsilk\0"
    "cornsilk1\0"
    "cornsilk2\0"
    "cornsilk3\0"
    "cornsilk4\0"
    "cyan\0"
    "cyan1\0"
    "cyan2\0"
    "cyan3\0"
    "cyan4\0"
    "dark blue\0"
    "dark cyan\0"
    "dark goldenrod\0"
    "dark gray\0"
    "dark green\0"
    "dark grey\0"
    "dark khaki\0"
    "dark magenta\0"
    "dark olive green\0"
    "dark orange\0"
    "dark orchid\0"
    "dark red\0"
    "dark salmon\0"
    "dark sea green\0"
    "dark slate blue\0"
    "dark slate gray\0"
    "dark slate grey\0"
    "dark turquoise\0"
    "dark violet\0"
    "DarkBlue\0"
    "DarkCyan\0"
    "DarkGoldenrod\0"
    "DarkGoldenrod1\0"
    "DarkGoldenrod2\0"
    "DarkGoldenrod3\0"
    "DarkGoldenrod4\0"
    "DarkGray\0"
    "DarkGreen\0"
    "DarkGrey\0"
    "DarkKhaki\0"
    "DarkMagenta\0"
    "DarkOliveGreen\0"
    "DarkOliveGreen1\0"
    "DarkOliveGreen2\0"
    "DarkOliveGreen3\0"
    "DarkOliveGreen4\0"
    "DarkOrange\0"
    "DarkOrange1\0"
    "DarkOrange2\0"
    "DarkOrange3\0"
    "DarkOrange4\0"
    "DarkOrchid\0"
    "DarkOrchid1\0"
    "DarkOrchid2\0"
    "DarkOrchid3\0"
    "DarkOrchid4\0"
    "DarkRed\0"
    "DarkSalmon\0"
    "DarkSeaGreen\0"
    "DarkSeaGreen1\0"
    "DarkSeaGreen2\0"
    "DarkSeaGreen3\0"
    "DarkSeaGreen4\0"
    "DarkSlateBlue\0"
    "DarkSlateGray\0"
    "DarkSlateGray1\0"
    "DarkSlateGray2\0"
    "DarkSlateGray3\0"
    "DarkSlateGray4\0"
    "DarkSlateGrey\0"
    "DarkTurquoise\0"
    "DarkViolet\0"
    "deep pink\0"
    "deep sky blue\0"
    "DeepPink\0"
    "DeepPink1\0"
    "DeepPink2\0"
    "DeepPink3\0"
    "DeepPink4\0"
    "DeepSkyBlue\0"
    "DeepSkyBlue1\0"
    "DeepSkyBlue2\0"
    "DeepSkyBlue3\0"
    "DeepSkyBlue4\0"
    "dim gray\0"
    "dim grey\0"
    "DimGray\0"
    "DimGrey\0"
    "dodger blue\0"
    "DodgerBlue\0"
    "DodgerBlue1\0"
    "DodgerBlue2\0"
    "DodgerBlue3\0"
    "DodgerBlue4\0"
    "firebrick\0"
    "firebrick1\0"
    "firebrick2\0"
    "firebrick3\0"
    "firebrick4\0"
    "floral white\0"
    "FloralWhite\0"
    "forest green\0"
    "ForestGreen\0"
    "gainsboro\0"
    "ghost white\0"
    "GhostWhite\0"
    "gold\0"
    "gold1\0"
    "gold2\0"
    "gold3\0"
    "gold4\0"
    "goldenrod\0"
    "goldenrod1\0"
    "goldenrod2\0"
    "goldenrod3\0"
    "goldenrod4\0"
    "gray\0"
    "gray0\0"
    "gray1\0"
    "gray10\0"
    "gray100\0"
    "gray11\0"
    "gray12\0"
    "gray13\0"
    "gray14\0"
    "gray15\0"
    "gray16\0"
    "gray17\0"
    "gray18\0"
    "gray19\0"
    "gray2\0"
    "gray20\0"
    "gray21\0"
    "gray22\0"
    "gray23\0"
    "gray24\0"
    "gray25\0"
    "gray26\0"
    "gray27\0"
    "gray28\0"
    "gray29\0"
    "gray3\0"
    "gray30\0"
    "gray31\0"
    "gray32\0"
    "gray33\0"
    "gray34\0"
    "gray35\0"
    "gray36\0"
    "gray37\0"
    "gray38\0"
    "gray39\0"
    "gray4\0"
    "gray40\0"
    "gray41\0"
    "gray42\0"
    "gray43\0"
    "gray44\0"
    "gray45\0"
    "gray46\0"
    "gray47\0"
    "gray48\0"
    "gray49\0"
    "gray5\0"
    "gray50\0"
    "gray51\0"
    "gray52\0"
    "gray53\0"
    "gray54\0"
    "gray55\0"
    "gray56\0"
    "gray57\0"
    "gray58\0"
    "gray59\0"
    "gray6\0"
    "gray60\0"
    "gray61\0"
    "gray62\0"
    "gray63\0"
    "gray64\0"
    "gray65\0"
    "gray66\0"
    "gray67\0"
    "gray68\0"
    "gray69\0"
    "gray7\0"
    "gray70\0"
    "gray71\0"
    "gray72\0"
    "gray73\0"
    "gray74\0"
    "gray75\0"
    "gray76\0"
    "gray77\0"
    "gray78\0"
    "gray79\0"
    "gray8\0"
    "gray80\0"
    "gray81\0"
    "gray82\0"
    "gray83\0"
    "gray84\0"
    "gray85\0"
    "gray86\0"
    "gray87\0"
    "gray88\0"
    "gray89\0"
    "gray9\0"
    "gray90\0"
    "gray91\0"
    "gray92\0"
    "gray93\0"
    "gray94\0"
    "gray95\0"
    "gray96\0"
    "gray97\0"
    "gray98\0"
    "gray99\0"
    "green\0"
    "green yellow\0"
    "green1\0"
    "green2\0"
    "green3\0"
    "green4\0"
    "GreenYellow\0"
    "grey\0"
    "grey0\0"
    "grey1\0"
    "grey10\0"
    "grey100\0"
    "grey11\0"
    "grey12\0"
    "grey13\0"
    "grey14\0"
    "grey15\0"
    "grey16\0"
    "grey17\0"
    "grey18\0"
    "grey19\0"
    "grey2\0"
    "grey20\0"
    "grey21\0"
    "grey22\0"
    "grey23\0"
    "grey24\0"
    "grey25\0"
    "grey26\0"
    "grey27\0"
    "grey28\0"
    "grey29\0"
    "grey3\0"
    "grey30\0"
    "grey31\0"
    "grey32\0"
    "grey33\0"
    "grey34\0"
    "grey35\0"
    "grey36\0"
    "grey37\0"
    "grey38\0"
    "grey39\0"
    "grey4\0"
    "grey40\0"
    "grey41\0"
    "grey42\0"
    "grey43\0"
    "grey44\0"
    "grey45\0"
    "grey46\0"
    "grey47\0"
    "grey48\0"
    "grey49\0"
    "grey5\0"
    "grey50\0"
    "grey51\0"
    "grey52\0"
    "grey53\0"
    "grey54\0"
    "grey55\0"
    "grey56\0"
    "grey57\0"
    "grey58\0"
    "grey59\0"
    "grey6\0"
    "grey60\0"
    "grey61\0"
    "grey62\0"
    "grey63\0"
    "grey64\0"
    "grey65\0"
    "grey66\0"
    "grey67\0"
    "grey68\0"
    "grey69\0"
    "grey7\0"
    "grey70\0"
    "grey71\0"
    "grey72\0"
    "grey73\0"
    "grey74\0"
    "grey75\0"
    "grey76\0"
    "grey77\0"
    "grey78\0"
    "grey79\0"
    "grey8\0"
    "grey80\0"
    "grey81\0"
    "grey82\0"
    "grey83\0"
    "grey84\0"
    "grey85\0"
    "grey86\0"
    "grey87\0"
    "grey88\0"
    "grey89\0"
    "grey9\0"
    "grey90\0"
    "grey91\0"
    "grey92\0"
    "grey93\0"
    "grey94\0"
    "grey95\0"
    "grey96\0"
    "grey97\0"
    "grey98\0"
    "grey99\0"
    "honeydew\0"
    "honeydew1\0"
    "honeydew2\0"
    "honeydew3\0"
    "honeydew4\0"
    "hot pink\0"
    "HotPink\0"
    "HotPink1\0"
    "HotPink2\0"
    "HotPink3\0"
    "HotPink4\0"
    "indian red\0"
    "IndianRed\0"
    "IndianRed1\0"
    "IndianRed2\0"
    "IndianRed3\0"
    "IndianRed4\0"
    "ivory\0"
    "ivory1\0"
    "ivory2\0"
    "ivory3\0"
    "ivory4\0"
    "khaki\0"
    "khaki1\0"
    "khaki2\0"
    "khaki3\0"
    "khaki4\0"
    "lavender\0"
    "lavender blush\0"
    "LavenderBlush\0"
    "LavenderBlush1\0"
    "LavenderBlush2\0"
    "LavenderBlush3\0"
    "LavenderBlush4\0"
    "lawn green\0"
    "LawnGreen\0"
    "lemon chiffon\0"
    "LemonChiffon\0"
    "LemonChiffon1\0"
    "LemonChiffon2\0"
    "LemonChiffon3\0"
    "LemonChiffon4\0"
    "light blue\0"
    "light coral\0"
    "light cyan\0"
    "light goldenrod\0"
    "light goldenrod yellow\0"
    "light gray\0"
    "light green\0"
    "light grey\0"
    "light pink\0"
    "light salmon\0"
    "light sea green\0"
    "light sky blue\0"
    "light slate blue\0"
    "light slate gray\0"
    "light slate grey\0"
    "light steel blue\0"
    "light yellow\0"
    "LightBlue\0"
    "LightBlue1\0"
    "LightBlue2\0"
    "LightBlue3\0"
    "LightBlue4\0"
    "LightCoral\0"
    "LightCyan\0"
    "LightCyan1\0"
    "LightCyan2\0"
    "LightCyan3\0"
    "LightCyan4\0"
    "LightGoldenrod\0"
    "LightGoldenrod1\0"
    "LightGoldenrod2\0"
    "LightGoldenrod3\0"
    "LightGoldenrod4\0"
    "LightGoldenrodYellow\0"
    "LightGray\0"
    "LightGreen\0"
    "LightGrey\0"
    "LightPink\0"
    "LightPink1\0"
    "LightPink2\0"
    "LightPink3\0"
    "LightPink4\0"
    "LightSalmon\0"
    "LightSalmon1\0"
    "LightSalmon2\0"
    "LightSalmon3\0"
    "LightSalmon4\0"
    "LightSeaGreen\0"
    "LightSkyBlue\0"
    "LightSkyBlue1\0"
    "LightSkyBlue2\0"
    "LightSkyBlue3\0"
    "LightSkyBlue4\0"
    "LightSlateBlue\0"
    "LightSlateGray\0"
    "LightSlateGrey\0"
    "LightSteelBlue\0"
    "LightSteelBlue1\0"
    "LightSteelBlue2\0"
    "LightSteelBlue3\0"
    "LightSteelBlue4\0"
    "LightYellow\0"
    "LightYellow1\0"
    "LightYellow2\0"
    "LightYellow3\0"
    "LightYellow4\0"
    "lime green\0"
    "LimeGreen\0"
    "linen\0"
    "magenta\0"
    "magenta1\0"
    "magenta2\0"
    "magenta3\0"
    "magenta4\0"
    "maroon\0"
    "maroon1\0"
    "maroon2\0"
    "maroon3\0"
    "maroon4\0"
    "medium aquamarine\0"
    "medium blue\0"
    "medium orchid\0"
    "medium purple\0"
    "medium sea green\0"
    "medium slate blue\0"
    "medium spring green\0"
    "medium turquoise\0"
    "medium violet red\0"
    "MediumAquamarine\0"
    "MediumBlue\0"
    "MediumOrchid\0"
    "MediumOrchid1\0"
    "MediumOrchid2\0"
    "MediumOrchid3\0"
    "MediumOrchid4\0"
    "MediumPurple\0"
    "MediumPurple1\0"
    "MediumPurple2\0"
    "MediumPurple3\0"
    "MediumPurple4\0"
    "MediumSeaGreen\0"
    "MediumSlateBlue\0"
    "MediumSpringGreen\0"
    "MediumTurquoise\0"
    "MediumVioletRed\0"
    "midnight blue\0"
    "MidnightBlue\0"
    "mint cream\0"
    "MintCream\0"
    "misty rose\0"
    "MistyRose\0"
    "MistyRose1\0"
    "MistyRose2\0"
    "MistyRose3\0"
    "MistyRose4\0"
    "moccasin\0"
    "navajo white\0"
    "NavajoWhite\0"
    "NavajoWhite1\0"
    "NavajoWhite2\0"
    "NavajoWhite3\0"
    "NavajoWhite4\0"
    "navy\0"
    "navy blue\0"
    "NavyBlue\0"
    "old lace\0"
    "OldLace\0"
    "olive drab\0"
    "OliveDrab\0"
    "OliveDrab1\0"
    "OliveDrab2\0"
    "OliveDrab3\0"
    "OliveDrab4\0"
    "orange\0"
    "orange red\0"
    "orange1\0"
    "orange2\0"
    "orange3\0"
    "orange4\0"
    "OrangeRed\0"
    "OrangeRed1\0"
    "OrangeRed2\0"
    "OrangeRed3\0"
    "OrangeRed4\0"
    "orchid\0"
    "orchid1\0"
    "orchid2\0"
    "orchid3\0"
    "orchid4\0"
    "pale goldenrod\0"
    "pale green\0"
    "pale turquoise\0"
    "pale violet red\0"
    "PaleGoldenrod\0"
    "PaleGreen\0"
    "PaleGreen1\0"
    "PaleGreen2\0"
    "PaleGreen3\0"
    "PaleGreen4\0"
    "PaleTurquoise\0"
    "PaleTurquoise1\0"
    "PaleTurquoise2\0"
    "PaleTurquoise3\0"
    "PaleTurquoise4\0"
    "PaleVioletRed\0"
    "PaleVioletRed1\0"
    "PaleVioletRed2\0"
    "PaleVioletRed3\0"
    "PaleVioletRed4\0"
    "papaya whip\0"
    "PapayaWhip\0"
    "peach puff\0"
    "PeachPuff\0"
    "PeachPuff1\0"
    "PeachPuff2\0"
    "PeachPuff3\0"
    "PeachPuff4\0"
    "peru\0"
    "pink\0"
    "pink1\0"
    "pink2\0"
    "pink3\0"
    "pink4\0"
    "plum\0"
    "plum1\0"
    "plum2\0"
    "plum3\0"
    "plum4\0"
    "powder blue\0"
    "PowderBlue\0"
    "purple\0"
    "purple1\0"
    "purple2\0"
    "purple3\0"
    "purple4\0"
    "red\0"
    "red1\0"
    "red2\0"
    "red3\0"
    "red4\0"
    "rosy brown\0"
    "RosyBrown\0"
    "RosyBrown1\0"
    "RosyBrown2\0"
    "RosyBrown3\0"
    "RosyBrown4\0"
    "royal blue\0"
    "RoyalBlue\0"
    "RoyalBlue1\0"
    "RoyalBlue2\0"
    "RoyalBlue3\0"
    "RoyalBlue4\0"
    "saddle brown\0"
    "SaddleBrown\0"
    "salmon\0"
    "salmon1\0"
    "salmon2\0"
    "salmon3\0"
    "salmon4\0"
    "sandy brown\0"
    "SandyBrown\0"
    "sea green\0"
    "SeaGreen\0"
    "SeaGreen1\0"
    "SeaGreen2\0"
    "SeaGreen3\0"
    "SeaGreen4\0"
    "seashell\0"
    "seashell1\0"
    "seashell2\0"
    "seashell3\0"
    "seashell4\0"
    "sienna\0"
    "sienna1\0"
    "sienna2\0"
    "sienna3\0"
    "sienna4\0"
    "sky blue\0"
    "SkyBlue\0"
    "SkyBlue1\0"
    "SkyBlue2\0"
    "SkyBlue3\0"
    "SkyBlue4\0"
    "slate blue\0"
    "slate gray\0"
    "slate grey\0"
    "SlateBlue\0"
    "SlateBlue1\0"
    "SlateBlue2\0"
    "SlateBlue3\0"
    "SlateBlue4\0"
    "SlateGray\0"
    "SlateGray1\0"
    "SlateGray2\0"
    "SlateGray3\0"
    "SlateGray4\0"
    "SlateGrey\0"
    "snow\0"
    "snow1\0"
    "snow2\0"
    "snow3\0"
    "snow4\0"
    "spring green\0"
    "SpringGreen\0"
    "SpringGreen1\0"
    "SpringGreen2\0"
    "SpringGreen3\0"
    "SpringGreen4\0"
    "steel blue\0"
    "SteelBlue\0"
    "SteelBlue1\0"
    "SteelBlue2\0"
    "SteelBlue3\0"
    "SteelBlue4\0"
    "tan\0"
    "tan1\0"
    "tan2\0"
    "tan3\0"
    "tan4\0"
    "thistle\0"
    "thistle1\0"
    "thistle2\0"
    "thistle3\0"
    "thistle4\0"
    "tomato\0"
    "tomato1\0"
    "tomato2\0"
    "tomato3\0"
    "tomato4\0"
    "turquoise\0"
    "turquoise1\0"
    "turquoise2\0"
    "turquoise3\0"
    "turquoise4\0"
    "violet\0"
    "violet red\0"
    "VioletRed\0"
    "VioletRed1\0"
    "VioletRed2\0"
    "VioletRed3\0"
    "VioletRed4\0"
    "wheat\0"
    "wheat1\0"
    "wheat2\0"
    "wheat3\0"
    "wheat4\0"
    "white\0"
    "white smoke\0"
    "WhiteSmoke\0"
    "yellow\0"
    "yellow green\0"
    "yellow1\0"
    "yellow2\0"
    "yellow3\0"
    "yellow4\0"
    "YellowGreen\0"};

static const BuiltinColor BuiltinColors[] = {
    {240, 248, 255, 0},    /* alice blue */
    {240, 248, 255, 11},   /* AliceBlue */
    {250, 235, 215, 21},   /* antique white */
    {250, 235, 215, 35},   /* AntiqueWhite */
    {255, 239, 219, 48},   /* AntiqueWhite1 */
    {238, 223, 204, 62},   /* AntiqueWhite2 */
    {205, 192, 176, 76},   /* AntiqueWhite3 */
    {139, 131, 120, 90},   /* AntiqueWhite4 */
    {127, 255, 212, 104},  /* aquamarine */
    {127, 255, 212, 115},  /* aquamarine1 */
    {118, 238, 198, 127},  /* aquamarine2 */
    {102, 205, 170, 139},  /* aquamarine3 */
    {69, 139, 116, 151},   /* aquamarine4 */
    {240, 255, 255, 163},  /* azure */
    {240, 255, 255, 169},  /* azure1 */
    {224, 238, 238, 176},  /* azure2 */
    {193, 205, 205, 183},  /* azure3 */
    {131, 139, 139, 190},  /* azure4 */
    {245, 245, 220, 197},  /* beige */
    {255, 228, 196, 203},  /* bisque */
    {255, 228, 196, 210},  /* bisque1 */
    {238, 213, 183, 218},  /* bisque2 */
    {205, 183, 158, 226},  /* bisque3 */
    {139, 125, 107, 234},  /* bisque4 */
    {0, 0, 0, 242},        /* black */
    {255, 235, 205, 248},  /* blanched almond */
    {255, 235, 205, 264},  /* BlanchedAlmond */
    {0, 0, 255, 279},      /* blue */
    {138, 43, 226, 284},   /* blue violet */
    {0, 0, 255, 296},      /* blue1 */
    {0, 0, 238, 302},      /* blue2 */
    {0, 0, 205, 308},      /* blue3 */
    {0, 0, 139, 314},      /* blue4 */
    {138, 43, 226, 320},   /* BlueViolet */
    {165, 42, 42, 331},    /* brown */
    {255, 64, 64, 337},    /* brown1 */
    {238, 59, 59, 344},    /* brown2 */
    {205, 51, 51, 351},    /* brown3 */
    {139, 35, 35, 358},    /* brown4 */
    {222, 184, 135, 365},  /* burlywood */
    {255, 211, 155, 375},  /* burlywood1 */
    {238, 197, 145, 386},  /* burlywood2 */
    {205, 170, 125, 397},  /* burlywood3 */
    {139, 115, 85, 408},   /* burlywood4 */
    {95, 158, 160, 419},   /* cadet blue */
    {95, 158, 160, 430},   /* CadetBlue */
    {152, 245, 255, 440},  /* CadetBlue1 */
    {142, 229, 238, 451},  /* CadetBlue2 */
    {122, 197, 205, 462},  /* CadetBlue3 */
    {83, 134, 139, 473},   /* CadetBlue4 */
    {127, 255, 0, 484},    /* chartreuse */
    {127, 255, 0, 495},    /* chartreuse1 */
    {118, 238, 0, 507},    /* chartreuse2 */
    {102, 205, 0, 519},    /* chartreuse3 */
    {69, 139, 0, 531},     /* chartreuse4 */
    {210, 105, 30, 543},   /* chocolate */
    {255, 127, 36, 553},   /* chocolate1 */
    {238, 118, 33, 564},   /* chocolate2 */
    {205, 102, 29, 575},   /* chocolate3 */
    {139, 69, 19, 586},    /* chocolate4 */
    {255, 127, 80, 597},   /* coral */
    {255, 114, 86, 603},   /* coral1 */
    {238, 106, 80, 610},   /* coral2 */
    {205, 91, 69, 617},    /* coral3 */
    {139, 62, 47, 624},    /* coral4 */
    {100, 149, 237, 631},  /* cornflower blue */
    {100, 149, 237, 647},  /* CornflowerBlue */
    {255, 248, 220, 662},  /* cornsilk */
    {255, 248, 220, 671},  /* cornsilk1 */
    {238, 232, 205, 681},  /* cornsilk2 */
    {205, 200, 177, 691},  /* cornsilk3 */
    {139, 136, 120, 701},  /* cornsilk4 */
    {0, 255, 255, 711},    /* cyan */
    {0, 255, 255, 716},    /* cyan1 */
    {0, 238, 238, 722},    /* cyan2 */
    {0, 205, 205, 728},    /* cyan3 */
    {0, 139, 139, 734},    /* cyan4 */
    {0, 0, 139, 740},      /* dark blue */
    {0, 139, 139, 750},    /* dark cyan */
    {184, 134, 11, 760},   /* dark goldenrod */
    {169, 169, 169, 775},  /* dark gray */
    {0, 100, 0, 785},      /* dark green */
    {169, 169, 169, 796},  /* dark grey */
    {189, 183, 107, 806},  /* dark khaki */
    {139, 0, 139, 817},    /* dark magenta */
    {85, 107, 47, 830},    /* dark olive green */
    {255, 140, 0, 847},    /* dark orange */
    {153, 50, 204, 859},   /* dark orchid */
    {139, 0, 0, 871},      /* dark red */
    {233, 150, 122, 880},  /* dark salmon */
    {143, 188, 143, 892},  /* dark sea green */
    {72, 61, 139, 907},    /* dark slate blue */
    {47, 79, 79, 923},     /* dark slate gray */
    {47, 79, 79, 939},     /* dark slate grey */
    {0, 206, 209, 955},    /* dark turquoise */
    {148, 0, 211, 970},    /* dark violet */
    {0, 0, 139, 982},      /* DarkBlue */
    {0, 139, 139, 991},    /* DarkCyan */
    {184, 134, 11, 1000},  /* DarkGoldenrod */
    {255, 185, 15, 1014},  /* DarkGoldenrod1 */
    {238, 173, 14, 1029},  /* DarkGoldenrod2 */
    {205, 149, 12, 1044},  /* DarkGoldenrod3 */
    {139, 101, 8, 1059},   /* DarkGoldenrod4 */
    {169, 169, 169, 1074}, /* DarkGray */
    {0, 100, 0, 1083},     /* DarkGreen */
    {169, 169, 169, 1093}, /* DarkGrey */
    {189, 183, 107, 1102}, /* DarkKhaki */
    {139, 0, 139, 1112},   /* DarkMagenta */
    {85, 107, 47, 1124},   /* DarkOliveGreen */
    {202, 255, 112, 1139}, /* DarkOliveGreen1 */
    {188, 238, 104, 1155}, /* DarkOliveGreen2 */
    {162, 205, 90, 1171},  /* DarkOliveGreen3 */
    {110, 139, 61, 1187},  /* DarkOliveGreen4 */
    {255, 140, 0, 1203},   /* DarkOrange */
    {255, 127, 0, 1214},   /* DarkOrange1 */
    {238, 118, 0, 1226},   /* DarkOrange2 */
    {205, 102, 0, 1238},   /* DarkOrange3 */
    {139, 69, 0, 1250},    /* DarkOrange4 */
    {153, 50, 204, 1262},  /* DarkOrchid */
    {191, 62, 255, 1273},  /* DarkOrchid1 */
    {178, 58, 238, 1285},  /* DarkOrchid2 */
    {154, 50, 205, 1297},  /* DarkOrchid3 */
    {104, 34, 139, 1309},  /* DarkOrchid4 */
    {139, 0, 0, 1321},     /* DarkRed */
    {233, 150, 122, 1329}, /* DarkSalmon */
    {143, 188, 143, 1340}, /* DarkSeaGreen */
    {193, 255, 193, 1353}, /* DarkSeaGreen1 */
    {180, 238, 180, 1367}, /* DarkSeaGreen2 */
    {155, 205, 155, 1381}, /* DarkSeaGreen3 */
    {105, 139, 105, 1395}, /* DarkSeaGreen4 */
    {72, 61, 139, 1409},   /* DarkSlateBlue */
    {47, 79, 79, 1423},    /* DarkSlateGray */
    {151, 255, 255, 1437}, /* DarkSlateGray1 */
    {141, 238, 238, 1452}, /* DarkSlateGray2 */
    {121, 205, 205, 1467}, /* DarkSlateGray3 */
    {82, 139, 139, 1482},  /* DarkSlateGray4 */
    {47, 79, 79, 1497},    /* DarkSlateGrey */
    {0, 206, 209, 1511},   /* DarkTurquoise */
    {148, 0, 211, 1525},   /* DarkViolet */
    {255, 20, 147, 1536},  /* deep pink */
    {0, 191, 255, 1546},   /* deep sky blue */
    {255, 20, 147, 1560},  /* DeepPink */
    {255, 20, 147, 1569},  /* DeepPink1 */
    {238, 18, 137, 1579},  /* DeepPink2 */
    {205, 16, 118, 1589},  /* DeepPink3 */
    {139, 10, 80, 1599},   /* DeepPink4 */
    {0, 191, 255, 1609},   /* DeepSkyBlue */
    {0, 191, 255, 1621},   /* DeepSkyBlue1 */
    {0, 178, 238, 1634},   /* DeepSkyBlue2 */
    {0, 154, 205, 1647},   /* DeepSkyBlue3 */
    {0, 104, 139, 1660},   /* DeepSkyBlue4 */
    {105, 105, 105, 1673}, /* dim gray */
    {105, 105, 105, 1682}, /* dim grey */
    {105, 105, 105, 1691}, /* DimGray */
    {105, 105, 105, 1699}, /* DimGrey */
    {30, 144, 255, 1707},  /* dodger blue */
    {30, 144, 255, 1719},  /* DodgerBlue */
    {30, 144, 255, 1730},  /* DodgerBlue1 */
    {28, 134, 238, 1742},  /* DodgerBlue2 */
    {24, 116, 205, 1754},  /* DodgerBlue3 */
    {16, 78, 139, 1766},   /* DodgerBlue4 */
    {178, 34, 34, 1778},   /* firebrick */
    {255, 48, 48, 1788},   /* firebrick1 */
    {238, 44, 44, 1799},   /* firebrick2 */
    {205, 38, 38, 1810},   /* firebrick3 */
    {139, 26, 26, 1821},   /* firebrick4 */
    {255, 250, 240, 1832}, /* floral white */
    {255, 250, 240, 1845}, /* FloralWhite */
    {34, 139, 34, 1857},   /* forest green */
    {34, 139, 34, 1870},   /* ForestGreen */
    {220, 220, 220, 1882}, /* gainsboro */
    {248, 248, 255, 1892}, /* ghost white */
    {248, 248, 255, 1904}, /* GhostWhite */
    {255, 215, 0, 1915},   /* gold */
    {255, 215, 0, 1920},   /* gold1 */
    {238, 201, 0, 1926},   /* gold2 */
    {205, 173, 0, 1932},   /* gold3 */
    {139, 117, 0, 1938},   /* gold4 */
    {218, 165, 32, 1944},  /* goldenrod */
    {255, 193, 37, 1954},  /* goldenrod1 */
    {238, 180, 34, 1965},  /* goldenrod2 */
    {205, 155, 29, 1976},  /* goldenrod3 */
    {139, 105, 20, 1987},  /* goldenrod4 */
    {190, 190, 190, 1998}, /* gray */
    {0, 0, 0, 2003},       /* gray0 */
    {3, 3, 3, 2009},       /* gray1 */
    {26, 26, 26, 2015},    /* gray10 */
    {255, 255, 255, 2022}, /* gray100 */
    {28, 28, 28, 2030},    /* gray11 */
    {31, 31, 31, 2037},    /* gray12 */
    {33, 33, 33, 2044},    /* gray13 */
    {36, 36, 36, 2051},    /* gray14 */
    {38, 38, 38, 2058},    /* gray15 */
    {41, 41, 41, 2065},    /* gray16 */
    {43, 43, 43, 2072},    /* gray17 */
    {46, 46, 46, 2079},    /* gray18 */
    {48, 48, 48, 2086},    /* gray19 */
    {5, 5, 5, 2093},       /* gray2 */
    {51, 51, 51, 2099},    /* gray20 */
    {54, 54, 54, 2106},    /* gray21 */
    {56, 56, 56, 2113},    /* gray22 */
    {59, 59, 59, 2120},    /* gray23 */
    {61, 61, 61, 2127},    /* gray24 */
    {64, 64, 64, 2134},    /* gray25 */
    {66, 66, 66, 2141},    /* gray26 */
    {69, 69, 69, 2148},    /* gray27 */
    {71, 71, 71, 2155},    /* gray28 */
    {74, 74, 74, 2162},    /* gray29 */
    {8, 8, 8, 2169},       /* gray3 */
    {77, 77, 77, 2175},    /* gray30 */
    {79, 79, 79, 2182},    /* gray31 */
    {82, 82, 82, 2189},    /* gray32 */
    {84, 84, 84, 2196},    /* gray33 */
    {87, 87, 87, 2203},    /* gray34 */
    {89, 89, 89, 2210},    /* gray35 */
    {92, 92, 92, 2217},    /* gray36 */
    {94, 94, 94, 2224},    /* gray37 */
    {97, 97, 97, 2231},    /* gray38 */
    {99, 99, 99, 2238},    /* gray39 */
    {10, 10, 10, 2245},    /* gray4 */
    {102, 102, 102, 2251}, /* gray40 */
    {105, 105, 105, 2258}, /* gray41 */
    {107, 107, 107, 2265}, /* gray42 */
    {110, 110, 110, 2272}, /* gray43 */
    {112, 112, 112, 2279}, /* gray44 */
    {115, 115, 115, 2286}, /* gray45 */
    {117, 117, 117, 2293}, /* gray46 */
    {120, 120, 120, 2300}, /* gray47 */
    {122, 122, 122, 2307}, /* gray48 */
    {125, 125, 125, 2314}, /* gray49 */
    {13, 13, 13, 2321},    /* gray5 */
    {127, 127, 127, 2327}, /* gray50 */
    {130, 130, 130, 2334}, /* gray51 */
    {133, 133, 133, 2341}, /* gray52 */
    {135, 135, 135, 2348}, /* gray53 */
    {138, 138, 138, 2355}, /* gray54 */
    {140, 140, 140, 2362}, /* gray55 */
    {143, 143, 143, 2369}, /* gray56 */
    {145, 145, 145, 2376}, /* gray57 */
    {148, 148, 148, 2383}, /* gray58 */
    {150, 150, 150, 2390}, /* gray59 */
    {15, 15, 15, 2397},    /* gray6 */
    {153, 153, 153, 2403}, /* gray60 */
    {156, 156, 156, 2410}, /* gray61 */
    {158, 158, 158, 2417}, /* gray62 */
    {161, 161, 161, 2424}, /* gray63 */
    {163, 163, 163, 2431}, /* gray64 */
    {166, 166, 166, 2438}, /* gray65 */
    {168, 168, 168, 2445}, /* gray66 */
    {171, 171, 171, 2452}, /* gray67 */
    {173, 173, 173, 2459}, /* gray68 */
    {176, 176, 176, 2466}, /* gray69 */
    {18, 18, 18, 2473},    /* gray7 */
    {179, 179, 179, 2479}, /* gray70 */
    {181, 181, 181, 2486}, /* gray71 */
    {184, 184, 184, 2493}, /* gray72 */
    {186, 186, 186, 2500}, /* gray73 */
    {189, 189, 189, 2507}, /* gray74 */
    {191, 191, 191, 2514}, /* gray75 */
    {194, 194, 194, 2521}, /* gray76 */
    {196, 196, 196, 2528}, /* gray77 */
    {199, 199, 199, 2535}, /* gray78 */
    {201, 201, 201, 2542}, /* gray79 */
    {20, 20, 20, 2549},    /* gray8 */
    {204, 204, 204, 2555}, /* gray80 */
    {207, 207, 207, 2562}, /* gray81 */
    {209, 209, 209, 2569}, /* gray82 */
    {212, 212, 212, 2576}, /* gray83 */
    {214, 214, 214, 2583}, /* gray84 */
    {217, 217, 217, 2590}, /* gray85 */
    {219, 219, 219, 2597}, /* gray86 */
    {222, 222, 222, 2604}, /* gray87 */
    {224, 224, 224, 2611}, /* gray88 */
    {227, 227, 227, 2618}, /* gray89 */
    {23, 23, 23, 2625},    /* gray9 */
    {229, 229, 229, 2631}, /* gray90 */
    {232, 232, 232, 2638}, /* gray91 */
    {235, 235, 235, 2645}, /* gray92 */
    {237, 237, 237, 2652}, /* gray93 */
    {240, 240, 240, 2659}, /* gray94 */
    {242, 242, 242, 2666}, /* gray95 */
    {245, 245, 245, 2673}, /* gray96 */
    {247, 247, 247, 2680}, /* gray97 */
    {250, 250, 250, 2687}, /* gray98 */
    {252, 252, 252, 2694}, /* gray99 */
    {0, 255, 0, 2701},     /* green */
    {173, 255, 47, 2707},  /* green yellow */
    {0, 255, 0, 2720},     /* green1 */
    {0, 238, 0, 2727},     /* green2 */
    {0, 205, 0, 2734},     /* green3 */
    {0, 139, 0, 2741},     /* green4 */
    {173, 255, 47, 2748},  /* GreenYellow */
    {190, 190, 190, 2760}, /* grey */
    {0, 0, 0, 2765},       /* grey0 */
    {3, 3, 3, 2771},       /* grey1 */
    {26, 26, 26, 2777},    /* grey10 */
    {255, 255, 255, 2784}, /* grey100 */
    {28, 28, 28, 2792},    /* grey11 */
    {31, 31, 31, 2799},    /* grey12 */
    {33, 33, 33, 2806},    /* grey13 */
    {36, 36, 36, 2813},    /* grey14 */
    {38, 38, 38, 2820},    /* grey15 */
    {41, 41, 41, 2827},    /* grey16 */
    {43, 43, 43, 2834},    /* grey17 */
    {46, 46, 46, 2841},    /* grey18 */
    {48, 48, 48, 2848},    /* grey19 */
    {5, 5, 5, 2855},       /* grey2 */
    {51, 51, 51, 2861},    /* grey20 */
    {54, 54, 54, 2868},    /* grey21 */
    {56, 56, 56, 2875},    /* grey22 */
    {59, 59, 59, 2882},    /* grey23 */
    {61, 61, 61, 2889},    /* grey24 */
    {64, 64, 64, 2896},    /* grey25 */
    {66, 66, 66, 2903},    /* grey26 */
    {69, 69, 69, 2910},    /* grey27 */
    {71, 71, 71, 2917},    /* grey28 */
    {74, 74, 74, 2924},    /* grey29 */
    {8, 8, 8, 2931},       /* grey3 */
    {77, 77, 77, 2937},    /* grey30 */
    {79, 79, 79, 2944},    /* grey31 */
    {82, 82, 82, 2951},    /* grey32 */
    {84, 84, 84, 2958},    /* grey33 */
    {87, 87, 87, 2965},    /* grey34 */
    {89, 89, 89, 2972},    /* grey35 */
    {92, 92, 92, 2979},    /* grey36 */
    {94, 94, 94, 2986},    /* grey37 */
    {97, 97, 97, 2993},    /* grey38 */
    {99, 99, 99, 3000},    /* grey39 */
    {10, 10, 10, 3007},    /* grey4 */
    {102, 102, 102, 3013}, /* grey40 */
    {105, 105, 105, 3020}, /* grey41 */
    {107, 107, 107, 3027}, /* grey42 */
    {110, 110, 110, 3034}, /* grey43 */
    {112, 112, 112, 3041}, /* grey44 */
    {115, 115, 115, 3048}, /* grey45 */
    {117, 117, 117, 3055}, /* grey46 */
    {120, 120, 120, 3062}, /* grey47 */
    {122, 122, 122, 3069}, /* grey48 */
    {125, 125, 125, 3076}, /* grey49 */
    {13, 13, 13, 3083},    /* grey5 */
    {127, 127, 127, 3089}, /* grey50 */
    {130, 130, 130, 3096}, /* grey51 */
    {133, 133, 133, 3103}, /* grey52 */
    {135, 135, 135, 3110}, /* grey53 */
    {138, 138, 138, 3117}, /* grey54 */
    {140, 140, 140, 3124}, /* grey55 */
    {143, 143, 143, 3131}, /* grey56 */
    {145, 145, 145, 3138}, /* grey57 */
    {148, 148, 148, 3145}, /* grey58 */
    {150, 150, 150, 3152}, /* grey59 */
    {15, 15, 15, 3159},    /* grey6 */
    {153, 153, 153, 3165}, /* grey60 */
    {156, 156, 156, 3172}, /* grey61 */
    {158, 158, 158, 3179}, /* grey62 */
    {161, 161, 161, 3186}, /* grey63 */
    {163, 163, 163, 3193}, /* grey64 */
    {166, 166, 166, 3200}, /* grey65 */
    {168, 168, 168, 3207}, /* grey66 */
    {171, 171, 171, 3214}, /* grey67 */
    {173, 173, 173, 3221}, /* grey68 */
    {176, 176, 176, 3228}, /* grey69 */
    {18, 18, 18, 3235},    /* grey7 */
    {179, 179, 179, 3241}, /* grey70 */
    {181, 181, 181, 3248}, /* grey71 */
    {184, 184, 184, 3255}, /* grey72 */
    {186, 186, 186, 3262}, /* grey73 */
    {189, 189, 189, 3269}, /* grey74 */
    {191, 191, 191, 3276}, /* grey75 */
    {194, 194, 194, 3283}, /* grey76 */
    {196, 196, 196, 3290}, /* grey77 */
    {199, 199, 199, 3297}, /* grey78 */
    {201, 201, 201, 3304}, /* grey79 */
    {20, 20, 20, 3311},    /* grey8 */
    {204, 204, 204, 3317}, /* grey80 */
    {207, 207, 207, 3324}, /* grey81 */
    {209, 209, 209, 3331}, /* grey82 */
    {212, 212, 212, 3338}, /* grey83 */
    {214, 214, 214, 3345}, /* grey84 */
    {217, 217, 217, 3352}, /* grey85 */
    {219, 219, 219, 3359}, /* grey86 */
    {222, 222, 222, 3366}, /* grey87 */
    {224, 224, 224, 3373}, /* grey88 */
    {227, 227, 227, 3380}, /* grey89 */
    {23, 23, 23, 3387},    /* grey9 */
    {229, 229, 229, 3393}, /* grey90 */
    {232, 232, 232, 3400}, /* grey91 */
    {235, 235, 235, 3407}, /* grey92 */
    {237, 237, 237, 3414}, /* grey93 */
    {240, 240, 240, 3421}, /* grey94 */
    {242, 242, 242, 3428}, /* grey95 */
    {245, 245, 245, 3435}, /* grey96 */
    {247, 247, 247, 3442}, /* grey97 */
    {250, 250, 250, 3449}, /* grey98 */
    {252, 252, 252, 3456}, /* grey99 */
    {240, 255, 240, 3463}, /* honeydew */
    {240, 255, 240, 3472}, /* honeydew1 */
    {224, 238, 224, 3482}, /* honeydew2 */
    {193, 205, 193, 3492}, /* honeydew3 */
    {131, 139, 131, 3502}, /* honeydew4 */
    {255, 105, 180, 3512}, /* hot pink */
    {255, 105, 180, 3521}, /* HotPink */
    {255, 110, 180, 3529}, /* HotPink1 */
    {238, 106, 167, 3538}, /* HotPink2 */
    {205, 96, 144, 3547},  /* HotPink3 */
    {139, 58, 98, 3556},   /* HotPink4 */
    {205, 92, 92, 3565},   /* indian red */
    {205, 92, 92, 3576},   /* IndianRed */
    {255, 106, 106, 3586}, /* IndianRed1 */
    {238, 99, 99, 3597},   /* IndianRed2 */
    {205, 85, 85, 3608},   /* IndianRed3 */
    {139, 58, 58, 3619},   /* IndianRed4 */
    {255, 255, 240, 3630}, /* ivory */
    {255, 255, 240, 3636}, /* ivory1 */
    {238, 238, 224, 3643}, /* ivory2 */
    {205, 205, 193, 3650}, /* ivory3 */
    {139, 139, 131, 3657}, /* ivory4 */
    {240, 230, 140, 3664}, /* khaki */
    {255, 246, 143, 3670}, /* khaki1 */
    {238, 230, 133, 3677}, /* khaki2 */
    {205, 198, 115, 3684}, /* khaki3 */
    {139, 134, 78, 3691},  /* khaki4 */
    {230, 230, 250, 3698}, /* lavender */
    {255, 240, 245, 3707}, /* lavender blush */
    {255, 240, 245, 3722}, /* LavenderBlush */
    {255, 240, 245, 3736}, /* LavenderBlush1 */
    {238, 224, 229, 3751}, /* LavenderBlush2 */
    {205, 193, 197, 3766}, /* LavenderBlush3 */
    {139, 131, 134, 3781}, /* LavenderBlush4 */
    {124, 252, 0, 3796},   /* lawn green */
    {124, 252, 0, 3807},   /* LawnGreen */
    {255, 250, 205, 3817}, /* lemon chiffon */
    {255, 250, 205, 3831}, /* LemonChiffon */
    {255, 250, 205, 3844}, /* LemonChiffon1 */
    {238, 233, 191, 3858}, /* LemonChiffon2 */
    {205, 201, 165, 3872}, /* LemonChiffon3 */
    {139, 137, 112, 3886}, /* LemonChiffon4 */
    {173, 216, 230, 3900}, /* light blue */
    {240, 128, 128, 3911}, /* light coral */
    {224, 255, 255, 3923}, /* light cyan */
    {238, 221, 130, 3934}, /* light goldenrod */
    {250, 250, 210, 3950}, /* light goldenrod yellow */
    {211, 211, 211, 3973}, /* light gray */
    {144, 238, 144, 3984}, /* light green */
    {211, 211, 211, 3996}, /* light grey */
    {255, 182, 193, 4007}, /* light pink */
    {255, 160, 122, 4018}, /* light salmon */
    {32, 178, 170, 4031},  /* light sea green */
    {135, 206, 250, 4047}, /* light sky blue */
    {132, 112, 255, 4062}, /* light slate blue */
    {119, 136, 153, 4079}, /* light slate gray */
    {119, 136, 153, 4096}, /* light slate grey */
    {176, 196, 222, 4113}, /* light steel blue */
    {255, 255, 224, 4130}, /* light yellow */
    {173, 216, 230, 4143}, /* LightBlue */
    {191, 239, 255, 4153}, /* LightBlue1 */
    {178, 223, 238, 4164}, /* LightBlue2 */
    {154, 192, 205, 4175}, /* LightBlue3 */
    {104, 131, 139, 4186}, /* LightBlue4 */
    {240, 128, 128, 4197}, /* LightCoral */
    {224, 255, 255, 4208}, /* LightCyan */
    {224, 255, 255, 4218}, /* LightCyan1 */
    {209, 238, 238, 4229}, /* LightCyan2 */
    {180, 205, 205, 4240}, /* LightCyan3 */
    {122, 139, 139, 4251}, /* LightCyan4 */
    {238, 221, 130, 4262}, /* LightGoldenrod */
    {255, 236, 139, 4277}, /* LightGoldenrod1 */
    {238, 220, 130, 4293}, /* LightGoldenrod2 */
    {205, 190, 112, 4309}, /* LightGoldenrod3 */
    {139, 129, 76, 4325},  /* LightGoldenrod4 */
    {250, 250, 210, 4341}, /* LightGoldenrodYellow */
    {211, 211, 211, 4362}, /* LightGray */
    {144, 238, 144, 4372}, /* LightGreen */
    {211, 211, 211, 4383}, /* LightGrey */
    {255, 182, 193, 4393}, /* LightPink */
    {255, 174, 185, 4403}, /* LightPink1 */
    {238, 162, 173, 4414}, /* LightPink2 */
    {205, 140, 149, 4425}, /* LightPink3 */
    {139, 95, 101, 4436},  /* LightPink4 */
    {255, 160, 122, 4447}, /* LightSalmon */
    {255, 160, 122, 4459}, /* LightSalmon1 */
    {238, 149, 114, 4472}, /* LightSalmon2 */
    {205, 129, 98, 4485},  /* LightSalmon3 */
    {139, 87, 66, 4498},   /* LightSalmon4 */
    {32, 178, 170, 4511},  /* LightSeaGreen */
    {135, 206, 250, 4525}, /* LightSkyBlue */
    {176, 226, 255, 4538}, /* LightSkyBlue1 */
    {164, 211, 238, 4552}, /* LightSkyBlue2 */
    {141, 182, 205, 4566}, /* LightSkyBlue3 */
    {96, 123, 139, 4580},  /* LightSkyBlue4 */
    {132, 112, 255, 4594}, /* LightSlateBlue */
    {119, 136, 153, 4609}, /* LightSlateGray */
    {119, 136, 153, 4624}, /* LightSlateGrey */
    {176, 196, 222, 4639}, /* LightSteelBlue */
    {202, 225, 255, 4654}, /* LightSteelBlue1 */
    {188, 210, 238, 4670}, /* LightSteelBlue2 */
    {162, 181, 205, 4686}, /* LightSteelBlue3 */
    {110, 123, 139, 4702}, /* LightSteelBlue4 */
    {255, 255, 224, 4718}, /* LightYellow */
    {255, 255, 224, 4730}, /* LightYellow1 */
    {238, 238, 209, 4743}, /* LightYellow2 */
    {205, 205, 180, 4756}, /* LightYellow3 */
    {139, 139, 122, 4769}, /* LightYellow4 */
    {50, 205, 50, 4782},   /* lime green */
    {50, 205, 50, 4793},   /* LimeGreen */
    {250, 240, 230, 4803}, /* linen */
    {255, 0, 255, 4809},   /* magenta */
    {255, 0, 255, 4817},   /* magenta1 */
    {238, 0, 238, 4826},   /* magenta2 */
    {205, 0, 205, 4835},   /* magenta3 */
    {139, 0, 139, 4844},   /* magenta4 */
    {176, 48, 96, 4853},   /* maroon */
    {255, 52, 179, 4860},  /* maroon1 */
    {238, 48, 167, 4868},  /* maroon2 */
    {205, 41, 144, 4876},  /* maroon3 */
    {139, 28, 98, 4884},   /* maroon4 */
    {102, 205, 170, 4892}, /* medium aquamarine */
    {0, 0, 205, 4910},     /* medium blue */
    {186, 85, 211, 4922},  /* medium orchid */
    {147, 112, 219, 4936}, /* medium purple */
    {60, 179, 113, 4950},  /* medium sea green */
    {123, 104, 238, 4967}, /* medium slate blue */
    {0, 250, 154, 4985},   /* medium spring green */
    {72, 209, 204, 5005},  /* medium turquoise */
    {199, 21, 133, 5022},  /* medium violet red */
    {102, 205, 170, 5040}, /* MediumAquamarine */
    {0, 0, 205, 5057},     /* MediumBlue */
    {186, 85, 211, 5068},  /* MediumOrchid */
    {224, 102, 255, 5081}, /* MediumOrchid1 */
    {209, 95, 238, 5095},  /* MediumOrchid2 */
    {180, 82, 205, 5109},  /* MediumOrchid3 */
    {122, 55, 139, 5123},  /* MediumOrchid4 */
    {147, 112, 219, 5137}, /* MediumPurple */
    {171, 130, 255, 5150}, /* MediumPurple1 */
    {159, 121, 238, 5164}, /* MediumPurple2 */
    {137, 104, 205, 5178}, /* MediumPurple3 */
    {93, 71, 139, 5192},   /* MediumPurple4 */
    {60, 179, 113, 5206},  /* MediumSeaGreen */
    {123, 104, 238, 5221}, /* MediumSlateBlue */
    {0, 250, 154, 5237},   /* MediumSpringGreen */
    {72, 209, 204, 5255},  /* MediumTurquoise */
    {199, 21, 133, 5271},  /* MediumVioletRed */
    {25, 25, 112, 5287},   /* midnight blue */
    {25, 25, 112, 5301},   /* MidnightBlue */
    {245, 255, 250, 5314}, /* mint cream */
    {245, 255, 250, 5325}, /* MintCream */
    {255, 228, 225, 5335}, /* misty rose */
    {255, 228, 225, 5346}, /* MistyRose */
    {255, 228, 225, 5356}, /* MistyRose1 */
    {238, 213, 210, 5367}, /* MistyRose2 */
    {205, 183, 181, 5378}, /* MistyRose3 */
    {139, 125, 123, 5389}, /* MistyRose4 */
    {255, 228, 181, 5400}, /* moccasin */
    {255, 222, 173, 5409}, /* navajo white */
    {255, 222, 173, 5422}, /* NavajoWhite */
    {255, 222, 173, 5434}, /* NavajoWhite1 */
    {238, 207, 161, 5447}, /* NavajoWhite2 */
    {205, 179, 139, 5460}, /* NavajoWhite3 */
    {139, 121, 94, 5473},  /* NavajoWhite4 */
    {0, 0, 128, 5486},     /* navy */
    {0, 0, 128, 5491},     /* navy blue */
    {0, 0, 128, 5501},     /* NavyBlue */
    {253, 245, 230, 5510}, /* old lace */
    {253, 245, 230, 5519}, /* OldLace */
    {107, 142, 35, 5527},  /* olive drab */
    {107, 142, 35, 5538},  /* OliveDrab */
    {192, 255, 62, 5548},  /* OliveDrab1 */
    {179, 238, 58, 5559},  /* OliveDrab2 */
    {154, 205, 50, 5570},  /* OliveDrab3 */
    {105, 139, 34, 5581},  /* OliveDrab4 */
    {255, 165, 0, 5592},   /* orange */
    {255, 69, 0, 5599},    /* orange red */
    {255, 165, 0, 5610},   /* orange1 */
    {238, 154, 0, 5618},   /* orange2 */
    {205, 133, 0, 5626},   /* orange3 */
    {139, 90, 0, 5634},    /* orange4 */
    {255, 69, 0, 5642},    /* OrangeRed */
    {255, 69, 0, 5652},    /* OrangeRed1 */
    {238, 64, 0, 5663},    /* OrangeRed2 */
    {205, 55, 0, 5674},    /* OrangeRed3 */
    {139, 37, 0, 5685},    /* OrangeRed4 */
    {218, 112, 214, 5696}, /* orchid */
    {255, 131, 250, 5703}, /* orchid1 */
    {238, 122, 233, 5711}, /* orchid2 */
    {205, 105, 201, 5719}, /* orchid3 */
    {139, 71, 137, 5727},  /* orchid4 */
    {238, 232, 170, 5735}, /* pale goldenrod */
    {152, 251, 152, 5750}, /* pale green */
    {175, 238, 238, 5761}, /* pale turquoise */
    {219, 112, 147, 5776}, /* pale violet red */
    {238, 232, 170, 5792}, /* PaleGoldenrod */
    {152, 251, 152, 5806}, /* PaleGreen */
    {154, 255, 154, 5816}, /* PaleGreen1 */
    {144, 238, 144, 5827}, /* PaleGreen2 */
    {124, 205, 124, 5838}, /* PaleGreen3 */
    {84, 139, 84, 5849},   /* PaleGreen4 */
    {175, 238, 238, 5860}, /* PaleTurquoise */
    {187, 255, 255, 5874}, /* PaleTurquoise1 */
    {174, 238, 238, 5889}, /* PaleTurquoise2 */
    {150, 205, 205, 5904}, /* PaleTurquoise3 */
    {102, 139, 139, 5919}, /* PaleTurquoise4 */
    {219, 112, 147, 5934}, /* PaleVioletRed */
    {255, 130, 171, 5948}, /* PaleVioletRed1 */
    {238, 121, 159, 5963}, /* PaleVioletRed2 */
    {205, 104, 137, 5978}, /* PaleVioletRed3 */
    {139, 71, 93, 5993},   /* PaleVioletRed4 */
    {255, 239, 213, 6008}, /* papaya whip */
    {255, 239, 213, 6020}, /* PapayaWhip */
    {255, 218, 185, 6031}, /* peach puff */
    {255, 218, 185, 6042}, /* PeachPuff */
    {255, 218, 185, 6052}, /* PeachPuff1 */
    {238, 203, 173, 6063}, /* PeachPuff2 */
    {205, 175, 149, 6074}, /* PeachPuff3 */
    {139, 119, 101, 6085}, /* PeachPuff4 */
    {205, 133, 63, 6096},  /* peru */
    {255, 192, 203, 6101}, /* pink */
    {255, 181, 197, 6106}, /* pink1 */
    {238, 169, 184, 6112}, /* pink2 */
    {205, 145, 158, 6118}, /* pink3 */
    {139, 99, 108, 6124},  /* pink4 */
    {221, 160, 221, 6130}, /* plum */
    {255, 187, 255, 6135}, /* plum1 */
    {238, 174, 238, 6141}, /* plum2 */
    {205, 150, 205, 6147}, /* plum3 */
    {139, 102, 139, 6153}, /* plum4 */
    {176, 224, 230, 6159}, /* powder blue */
    {176, 224, 230, 6171}, /* PowderBlue */
    {160, 32, 240, 6182},  /* purple */
    {155, 48, 255, 6189},  /* purple1 */
    {145, 44, 238, 6197},  /* purple2 */
    {125, 38, 205, 6205},  /* purple3 */
    {85, 26, 139, 6213},   /* purple4 */
    {255, 0, 0, 6221},     /* red */
    {255, 0, 0, 6225},     /* red1 */
    {238, 0, 0, 6230},     /* red2 */
    {205, 0, 0, 6235},     /* red3 */
    {139, 0, 0, 6240},     /* red4 */
    {188, 143, 143, 6245}, /* rosy brown */
    {188, 143, 143, 6256}, /* RosyBrown */
    {255, 193, 193, 6266}, /* RosyBrown1 */
    {238, 180, 180, 6277}, /* RosyBrown2 */
    {205, 155, 155, 6288}, /* RosyBrown3 */
    {139, 105, 105, 6299}, /* RosyBrown4 */
    {65, 105, 225, 6310},  /* royal blue */
    {65, 105, 225, 6321},  /* RoyalBlue */
    {72, 118, 255, 6331},  /* RoyalBlue1 */
    {67, 110, 238, 6342},  /* RoyalBlue2 */
    {58, 95, 205, 6353},   /* RoyalBlue3 */
    {39, 64, 139, 6364},   /* RoyalBlue4 */
    {139, 69, 19, 6375},   /* saddle brown */
    {139, 69, 19, 6388},   /* SaddleBrown */
    {250, 128, 114, 6400}, /* salmon */
    {255, 140, 105, 6407}, /* salmon1 */
    {238, 130, 98, 6415},  /* salmon2 */
    {205, 112, 84, 6423},  /* salmon3 */
    {139, 76, 57, 6431},   /* salmon4 */
    {244, 164, 96, 6439},  /* sandy brown */
    {244, 164, 96, 6451},  /* SandyBrown */
    {46, 139, 87, 6462},   /* sea green */
    {46, 139, 87, 6472},   /* SeaGreen */
    {84, 255, 159, 6481},  /* SeaGreen1 */
    {78, 238, 148, 6491},  /* SeaGreen2 */
    {67, 205, 128, 6501},  /* SeaGreen3 */
    {46, 139, 87, 6511},   /* SeaGreen4 */
    {255, 245, 238, 6521}, /* seashell */
    {255, 245, 238, 6530}, /* seashell1 */
    {238, 229, 222, 6540}, /* seashell2 */
    {205, 197, 191, 6550}, /* seashell3 */
    {139, 134, 130, 6560}, /* seashell4 */
    {160, 82, 45, 6570},   /* sienna */
    {255, 130, 71, 6577},  /* sienna1 */
    {238, 121, 66, 6585},  /* sienna2 */
    {205, 104, 57, 6593},  /* sienna3 */
    {139, 71, 38, 6601},   /* sienna4 */
    {135, 206, 235, 6609}, /* sky blue */
    {135, 206, 235, 6618}, /* SkyBlue */
    {135, 206, 255, 6626}, /* SkyBlue1 */
    {126, 192, 238, 6635}, /* SkyBlue2 */
    {108, 166, 205, 6644}, /* SkyBlue3 */
    {74, 112, 139, 6653},  /* SkyBlue4 */
    {106, 90, 205, 6662},  /* slate blue */
    {112, 128, 144, 6673}, /* slate gray */
    {112, 128, 144, 6684}, /* slate grey */
    {106, 90, 205, 6695},  /* SlateBlue */
    {131, 111, 255, 6705}, /* SlateBlue1 */
    {122, 103, 238, 6716}, /* SlateBlue2 */
    {105, 89, 205, 6727},  /* SlateBlue3 */
    {71, 60, 139, 6738},   /* SlateBlue4 */
    {112, 128, 144, 6749}, /* SlateGray */
    {198, 226, 255, 6759}, /* SlateGray1 */
    {185, 211, 238, 6770}, /* SlateGray2 */
    {159, 182, 205, 6781}, /* SlateGray3 */
    {108, 123, 139, 6792}, /* SlateGray4 */
    {112, 128, 144, 6803}, /* SlateGrey */
    {255, 250, 250, 6813}, /* snow */
    {255, 250, 250, 6818}, /* snow1 */
    {238, 233, 233, 6824}, /* snow2 */
    {205, 201, 201, 6830}, /* snow3 */
    {139, 137, 137, 6836}, /* snow4 */
    {0, 255, 127, 6842},   /* spring green */
    {0, 255, 127, 6855},   /* SpringGreen */
    {0, 255, 127, 6867},   /* SpringGreen1 */
    {0, 238, 118, 6880},   /* SpringGreen2 */
    {0, 205, 102, 6893},   /* SpringGreen3 */
    {0, 139, 69, 6906},    /* SpringGreen4 */
    {70, 130, 180, 6919},  /* steel blue */
    {70, 130, 180, 6930},  /* SteelBlue */
    {99, 184, 255, 6940},  /* SteelBlue1 */
    {92, 172, 238, 6951},  /* SteelBlue2 */
    {79, 148, 205, 6962},  /* SteelBlue3 */
    {54, 100, 139, 6973},  /* SteelBlue4 */
    {210, 180, 140, 6984}, /* tan */
    {255, 165, 79, 6988},  /* tan1 */
    {238, 154, 73, 6993},  /* tan2 */
    {205, 133, 63, 6998},  /* tan3 */
    {139, 90, 43, 7003},   /* tan4 */
    {216, 191, 216, 7008}, /* thistle */
    {255, 225, 255, 7016}, /* thistle1 */
    {238, 210, 238, 7025}, /* thistle2 */
    {205, 181, 205, 7034}, /* thistle3 */
    {139, 123, 139, 7043}, /* thistle4 */
    {255, 99, 71, 7052},   /* tomato */
    {255, 99, 71, 7059},   /* tomato1 */
    {238, 92, 66, 7067},   /* tomato2 */
    {205, 79, 57, 7075},   /* tomato3 */
    {139, 54, 38, 7083},   /* tomato4 */
    {64, 224, 208, 7091},  /* turquoise */
    {0, 245, 255, 7101},   /* turquoise1 */
    {0, 229, 238, 7112},   /* turquoise2 */
    {0, 197, 205, 7123},   /* turquoise3 */
    {0, 134, 139, 7134},   /* turquoise4 */
    {238, 130, 238, 7145}, /* violet */
    {208, 32, 144, 7152},  /* violet red */
    {208, 32, 144, 7163},  /* VioletRed */
    {255, 62, 150, 7173},  /* VioletRed1 */
    {238, 58, 140, 7184},  /* VioletRed2 */
    {205, 50, 120, 7195},  /* VioletRed3 */
    {139, 34, 82, 7206},   /* VioletRed4 */
    {245, 222, 179, 7217}, /* wheat */
    {255, 231, 186, 7223}, /* wheat1 */
    {238, 216, 174, 7230}, /* wheat2 */
    {205, 186, 150, 7237}, /* wheat3 */
    {139, 126, 102, 7244}, /* wheat4 */
    {255, 255, 255, 7251}, /* white */
    {245, 245, 245, 7257}, /* white smoke */
    {245, 245, 245, 7269}, /* WhiteSmoke */
    {255, 255, 0, 7280},   /* yellow */
    {154, 205, 50, 7287},  /* yellow green */
    {255, 255, 0, 7300},   /* yellow1 */
    {238, 238, 0, 7308},   /* yellow2 */
    {205, 205, 0, 7316},   /* yellow3 */
    {139, 139, 0, 7324},   /* yellow4 */
    {154, 205, 50, 7332},  /* YellowGreen */
};

#define NUM_BUILTIN_COLORS (sizeof(BuiltinColors) / sizeof(BuiltinColors[0]))

int OsLookupColor(const char *name, unsigned int len, unsigned short *pred,
                  unsigned short *pgreen, unsigned short *pblue) {
  const BuiltinColor *c;
  int low, mid, high;
  int r;

  low = 0;
  high = NUM_BUILTIN_COLORS - 1;
  while (high >= low) {
    mid = (low + high) / 2;
    c = &BuiltinColors[mid];
    r = strncasecmp(&BuiltinColorNames[c->name], name, len);
    if (r == 0 && len == strlen(&BuiltinColorNames[c->name])) {
      *pred = c->red;
      *pgreen = c->green;
      *pblue = c->blue;
      return 1;
    }
    if (r < 0)
      low = mid + 1;
    else
      high = mid - 1;
  }
  return 0;
}
