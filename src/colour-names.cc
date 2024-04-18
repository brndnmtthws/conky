/* C++ code produced by gperf version 3.1 */
/* Command-line: gperf --ignore-case -LC++ -Zcolor_name_hash -t -7 -m1 -C -E colour-names.gperf.in  */
/* Computed positions: -k'1,3,5-9,12-15,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "colour-names.gperf.in"
struct rgb { const char *name; uint8_t red; uint8_t green; uint8_t blue; };
/* maximum key range = 3483, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_STRCMP
#define GPERF_CASE_STRCMP 1
static int
gperf_case_strcmp (const char *s1, const char *s2)
{
  for (;;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 != 0 && c1 == c2)
        continue;
      return (int)c1 - (int)c2;
    }
}
#endif

class color_name_hash
{
private:
  static inline unsigned int hash (const char *str, size_t len);
public:
  static const struct rgb *in_word_set (const char *str, size_t len);
};

inline unsigned int
color_name_hash::hash (const char *str, size_t len)
{
  static const unsigned short asso_values[] =
    {
      3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489,
      3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489,
      3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489,
      3489, 3489,  244, 3489, 3489, 3489, 3489, 3489, 3489, 3489,
      3489, 3489, 3489, 3489, 3489, 3489, 3489, 3489,  228,   18,
         7,    2,    0, 1006,  943,  889,  754,  654, 3489, 3489,
      3489, 3489, 3489, 3489, 3489,   42,   20,  368,   74,    0,
       196,    1,  426,  628,    2,  443,  122,  446,   26,   95,
       356,   12,   17,    0,  125,  204,  969,  680, 3489,  276,
      3489, 3489, 3489, 3489, 3489, 3489, 3489,   42,   20,  368,
        74,    0,  196,    1,  426,  628,    2,  443,  122,  446,
        26,   95,  356,   12,   17,    0,  125,  204,  969,  680,
      3489,  276, 3489, 3489, 3489, 3489, 3489, 3489
    };
  unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[static_cast<unsigned char>(str[14])];
      /*FALLTHROUGH*/
      case 14:
        hval += asso_values[static_cast<unsigned char>(str[13])];
      /*FALLTHROUGH*/
      case 13:
        hval += asso_values[static_cast<unsigned char>(str[12])];
      /*FALLTHROUGH*/
      case 12:
        hval += asso_values[static_cast<unsigned char>(str[11])];
      /*FALLTHROUGH*/
      case 11:
      case 10:
      case 9:
        hval += asso_values[static_cast<unsigned char>(str[8])];
      /*FALLTHROUGH*/
      case 8:
        hval += asso_values[static_cast<unsigned char>(str[7])];
      /*FALLTHROUGH*/
      case 7:
        hval += asso_values[static_cast<unsigned char>(str[6])];
      /*FALLTHROUGH*/
      case 6:
        hval += asso_values[static_cast<unsigned char>(str[5])];
      /*FALLTHROUGH*/
      case 5:
        hval += asso_values[static_cast<unsigned char>(str[4])];
      /*FALLTHROUGH*/
      case 4:
      case 3:
        hval += asso_values[static_cast<unsigned char>(str[2])];
      /*FALLTHROUGH*/
      case 2:
      case 1:
        hval += asso_values[static_cast<unsigned char>(str[0])];
        break;
    }
  return hval + asso_values[static_cast<unsigned char>(str[len - 1])];
}

const struct rgb *
color_name_hash::in_word_set (const char *str, size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 752,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 22,
      MIN_HASH_VALUE = 6,
      MAX_HASH_VALUE = 3488
    };

  static const struct rgb wordlist[] =
    {
      {""}, {""}, {""}, {""}, {""}, {""},
#line 331 "colour-names.gperf.in"
      {"grey4", 10, 10, 10},
#line 336 "colour-names.gperf.in"
      {"grey44", 112, 112, 112},
      {""},
#line 325 "colour-names.gperf.in"
      {"grey34", 87, 87, 87},
#line 320 "colour-names.gperf.in"
      {"grey3", 8, 8, 8},
#line 335 "colour-names.gperf.in"
      {"grey43", 110, 110, 110},
      {""},
#line 324 "colour-names.gperf.in"
      {"grey33", 84, 84, 84},
#line 314 "colour-names.gperf.in"
      {"grey24", 61, 61, 61},
      {""}, {""}, {""},
#line 313 "colour-names.gperf.in"
      {"grey23", 59, 59, 59},
      {""},
#line 309 "colour-names.gperf.in"
      {"grey2", 5, 5, 5},
#line 334 "colour-names.gperf.in"
      {"grey42", 107, 107, 107},
      {""},
#line 323 "colour-names.gperf.in"
      {"grey32", 82, 82, 82},
      {""},
#line 303 "colour-names.gperf.in"
      {"grey14", 36, 36, 36},
      {""}, {""},
#line 312 "colour-names.gperf.in"
      {"grey22", 56, 56, 56},
#line 302 "colour-names.gperf.in"
      {"grey13", 33, 33, 33},
      {""}, {""}, {""},
#line 293 "colour-names.gperf.in"
      {"green4", 0, 139, 0},
      {""}, {""}, {""},
#line 292 "colour-names.gperf.in"
      {"green3", 0, 205, 0},
      {""},
#line 301 "colour-names.gperf.in"
      {"grey12", 31, 31, 31},
      {""}, {""},
#line 297 "colour-names.gperf.in"
      {"grey1", 3, 3, 3},
#line 333 "colour-names.gperf.in"
      {"grey41", 105, 105, 105},
      {""},
#line 322 "colour-names.gperf.in"
      {"grey31", 79, 79, 79},
      {""},
#line 291 "colour-names.gperf.in"
      {"green2", 0, 238, 0},
#line 222 "colour-names.gperf.in"
      {"gray4", 10, 10, 10},
#line 227 "colour-names.gperf.in"
      {"gray44", 112, 112, 112},
#line 311 "colour-names.gperf.in"
      {"grey21", 54, 54, 54},
#line 216 "colour-names.gperf.in"
      {"gray34", 87, 87, 87},
#line 211 "colour-names.gperf.in"
      {"gray3", 8, 8, 8},
#line 226 "colour-names.gperf.in"
      {"gray43", 110, 110, 110},
      {""},
#line 215 "colour-names.gperf.in"
      {"gray33", 84, 84, 84},
#line 205 "colour-names.gperf.in"
      {"gray24", 61, 61, 61},
      {""},
#line 288 "colour-names.gperf.in"
      {"green", 0, 255, 0},
      {""},
#line 204 "colour-names.gperf.in"
      {"gray23", 59, 59, 59},
#line 300 "colour-names.gperf.in"
      {"grey11", 28, 28, 28},
#line 200 "colour-names.gperf.in"
      {"gray2", 5, 5, 5},
#line 225 "colour-names.gperf.in"
      {"gray42", 107, 107, 107},
      {""},
#line 214 "colour-names.gperf.in"
      {"gray32", 82, 82, 82},
      {""},
#line 194 "colour-names.gperf.in"
      {"gray14", 36, 36, 36},
      {""},
#line 290 "colour-names.gperf.in"
      {"green1", 0, 255, 0},
#line 203 "colour-names.gperf.in"
      {"gray22", 56, 56, 56},
#line 193 "colour-names.gperf.in"
      {"gray13", 33, 33, 33},
      {""}, {""},
#line 706 "colour-names.gperf.in"
      {"SpringGreen4", 0, 139, 69},
#line 675 "colour-names.gperf.in"
      {"sienna4", 139, 71, 38},
      {""}, {""},
#line 705 "colour-names.gperf.in"
      {"SpringGreen3", 0, 205, 102},
#line 674 "colour-names.gperf.in"
      {"sienna3", 205, 104, 57},
      {""},
#line 192 "colour-names.gperf.in"
      {"gray12", 31, 31, 31},
      {""}, {""},
#line 188 "colour-names.gperf.in"
      {"gray1", 3, 3, 3},
#line 224 "colour-names.gperf.in"
      {"gray41", 105, 105, 105},
      {""},
#line 213 "colour-names.gperf.in"
      {"gray31", 79, 79, 79},
#line 704 "colour-names.gperf.in"
      {"SpringGreen2", 0, 238, 118},
#line 673 "colour-names.gperf.in"
      {"sienna2", 238, 121, 66},
      {""}, {""},
#line 202 "colour-names.gperf.in"
      {"gray21", 54, 54, 54},
      {""},
#line 665 "colour-names.gperf.in"
      {"SeaGreen4", 46, 139, 87},
#line 638 "colour-names.gperf.in"
      {"red4", 139, 0, 0},
      {""},
#line 637 "colour-names.gperf.in"
      {"red3", 205, 0, 0},
#line 664 "colour-names.gperf.in"
      {"SeaGreen3", 67, 205, 128},
#line 702 "colour-names.gperf.in"
      {"SpringGreen", 0, 255, 127},
#line 700 "colour-names.gperf.in"
      {"snow4", 139, 137, 137},
      {""},
#line 636 "colour-names.gperf.in"
      {"red2", 238, 0, 0},
#line 191 "colour-names.gperf.in"
      {"gray11", 28, 28, 28},
#line 699 "colour-names.gperf.in"
      {"snow3", 205, 201, 201},
      {""}, {""}, {""},
#line 663 "colour-names.gperf.in"
      {"SeaGreen2", 78, 238, 148},
      {""},
#line 703 "colour-names.gperf.in"
      {"SpringGreen1", 0, 255, 127},
#line 672 "colour-names.gperf.in"
      {"sienna1", 255, 130, 71},
      {""},
#line 635 "colour-names.gperf.in"
      {"red1", 255, 0, 0},
#line 698 "colour-names.gperf.in"
      {"snow2", 238, 233, 233},
      {""},
#line 671 "colour-names.gperf.in"
      {"sienna", 160, 82, 45},
      {""}, {""},
#line 661 "colour-names.gperf.in"
      {"SeaGreen", 46, 139, 87},
      {""},
#line 660 "colour-names.gperf.in"
      {"sea green", 46, 139, 87},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 180 "colour-names.gperf.in"
      {"gold4", 139, 117, 0},
      {""},
#line 662 "colour-names.gperf.in"
      {"SeaGreen1", 84, 255, 159},
      {""},
#line 179 "colour-names.gperf.in"
      {"gold3", 205, 173, 0},
      {""}, {""}, {""},
#line 697 "colour-names.gperf.in"
      {"snow1", 255, 250, 250},
      {""}, {""}, {""}, {""}, {""},
#line 178 "colour-names.gperf.in"
      {"gold2", 238, 201, 0},
      {""},
#line 572 "colour-names.gperf.in"
      {"orange", 255, 165, 0},
#line 577 "colour-names.gperf.in"
      {"orange4", 139, 90, 0},
      {""},
#line 41 "colour-names.gperf.in"
      {"brown4", 139, 35, 35},
      {""},
#line 576 "colour-names.gperf.in"
      {"orange3", 205, 133, 0},
      {""},
#line 40 "colour-names.gperf.in"
      {"brown3", 205, 51, 51},
      {""}, {""}, {""},
#line 717 "colour-names.gperf.in"
      {"tan4", 139, 90, 43},
      {""},
#line 716 "colour-names.gperf.in"
      {"tan3", 205, 133, 63},
      {""},
#line 575 "colour-names.gperf.in"
      {"orange2", 238, 154, 0},
      {""},
#line 39 "colour-names.gperf.in"
      {"brown2", 238, 59, 59},
#line 715 "colour-names.gperf.in"
      {"tan2", 238, 154, 73},
      {""},
#line 177 "colour-names.gperf.in"
      {"gold1", 255, 215, 0},
      {""}, {""}, {""},
#line 634 "colour-names.gperf.in"
      {"red", 255, 0, 0},
      {""},
#line 107 "colour-names.gperf.in"
      {"DarkGreen", 0, 100, 0},
      {""},
#line 37 "colour-names.gperf.in"
      {"brown", 165, 42, 42},
#line 714 "colour-names.gperf.in"
      {"tan1", 255, 165, 79},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 713 "colour-names.gperf.in"
      {"tan", 210, 180, 140},
#line 574 "colour-names.gperf.in"
      {"orange1", 255, 165, 0},
      {""},
#line 38 "colour-names.gperf.in"
      {"brown1", 255, 64, 64},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 132 "colour-names.gperf.in"
      {"DarkSeaGreen4", 105, 139, 105},
      {""}, {""}, {""},
#line 131 "colour-names.gperf.in"
      {"DarkSeaGreen3", 155, 205, 155},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 176 "colour-names.gperf.in"
      {"gold", 255, 215, 0},
      {""}, {""},
#line 130 "colour-names.gperf.in"
      {"DarkSeaGreen2", 180, 238, 180},
#line 507 "colour-names.gperf.in"
      {"linen", 250, 240, 230},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 128 "colour-names.gperf.in"
      {"DarkSeaGreen", 143, 188, 143},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 129 "colour-names.gperf.in"
      {"DarkSeaGreen1", 193, 255, 193},
      {""},
#line 30 "colour-names.gperf.in"
      {"blue", 0, 0, 255},
#line 35 "colour-names.gperf.in"
      {"blue4", 0, 0, 139},
#line 22 "colour-names.gperf.in"
      {"bisque", 255, 228, 196},
#line 26 "colour-names.gperf.in"
      {"bisque4", 139, 125, 107},
      {""},
#line 34 "colour-names.gperf.in"
      {"blue3", 0, 0, 205},
      {""},
#line 25 "colour-names.gperf.in"
      {"bisque3", 205, 183, 158},
      {""}, {""}, {""},
#line 582 "colour-names.gperf.in"
      {"OrangeRed4", 139, 37, 0},
      {""},
#line 581 "colour-names.gperf.in"
      {"OrangeRed3", 205, 55, 0},
      {""},
#line 33 "colour-names.gperf.in"
      {"blue2", 0, 0, 238},
      {""},
#line 24 "colour-names.gperf.in"
      {"bisque2", 238, 213, 183},
#line 580 "colour-names.gperf.in"
      {"OrangeRed2", 238, 64, 0},
      {""}, {""}, {""},
#line 657 "colour-names.gperf.in"
      {"salmon4", 139, 76, 57},
#line 16 "colour-names.gperf.in"
      {"azure", 240, 255, 255},
#line 20 "colour-names.gperf.in"
      {"azure4", 131, 139, 139},
      {""},
#line 656 "colour-names.gperf.in"
      {"salmon3", 205, 112, 84},
      {""},
#line 19 "colour-names.gperf.in"
      {"azure3", 193, 205, 205},
#line 579 "colour-names.gperf.in"
      {"OrangeRed1", 255, 69, 0},
      {""}, {""}, {""}, {""}, {""},
#line 126 "colour-names.gperf.in"
      {"DarkRed", 139, 0, 0},
#line 655 "colour-names.gperf.in"
      {"salmon2", 238, 130, 98},
#line 32 "colour-names.gperf.in"
      {"blue1", 0, 0, 255},
#line 18 "colour-names.gperf.in"
      {"azure2", 224, 238, 238},
#line 23 "colour-names.gperf.in"
      {"bisque1", 255, 228, 196},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 653 "colour-names.gperf.in"
      {"salmon", 250, 128, 114},
      {""}, {""}, {""}, {""}, {""},
#line 295 "colour-names.gperf.in"
      {"grey", 190, 190, 190},
#line 116 "colour-names.gperf.in"
      {"DarkOrange", 255, 140, 0},
#line 120 "colour-names.gperf.in"
      {"DarkOrange4", 139, 69, 0},
      {""},
#line 119 "colour-names.gperf.in"
      {"DarkOrange3", 205, 102, 0},
#line 654 "colour-names.gperf.in"
      {"salmon1", 255, 140, 105},
      {""},
#line 17 "colour-names.gperf.in"
      {"azure1", 240, 255, 255},
      {""},
#line 118 "colour-names.gperf.in"
      {"DarkOrange2", 238, 118, 0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 117 "colour-names.gperf.in"
      {"DarkOrange1", 255, 127, 0},
#line 474 "colour-names.gperf.in"
      {"LightGreen", 144, 238, 144},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 578 "colour-names.gperf.in"
      {"OrangeRed", 255, 69, 0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 186 "colour-names.gperf.in"
      {"gray", 190, 190, 190},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
#line 185 "colour-names.gperf.in"
      {"goldenrod4", 139, 105, 20},
      {""},
#line 184 "colour-names.gperf.in"
      {"goldenrod3", 205, 155, 29},
      {""}, {""}, {""}, {""},
#line 183 "colour-names.gperf.in"
      {"goldenrod2", 238, 180, 34},
      {""}, {""}, {""},
#line 486 "colour-names.gperf.in"
      {"LightSeaGreen", 32, 178, 170},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 182 "colour-names.gperf.in"
      {"goldenrod1", 255, 193, 37},
      {""},
#line 652 "colour-names.gperf.in"
      {"SaddleBrown", 139, 69, 19},
      {""}, {""}, {""}, {""},
#line 701 "colour-names.gperf.in"
      {"spring green", 0, 255, 127},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 621 "colour-names.gperf.in"
      {"pink4", 139, 99, 108},
#line 694 "colour-names.gperf.in"
      {"SlateGray4", 108, 123, 139},
#line 84 "colour-names.gperf.in"
      {"dark green", 0, 100, 0},
#line 693 "colour-names.gperf.in"
      {"SlateGray3", 159, 182, 205},
#line 620 "colour-names.gperf.in"
      {"pink3", 205, 145, 158},
      {""},
#line 172 "colour-names.gperf.in"
      {"ForestGreen", 34, 139, 34},
      {""},
#line 692 "colour-names.gperf.in"
      {"SlateGray2", 185, 211, 238},
      {""},
#line 685 "colour-names.gperf.in"
      {"SlateBlue", 106, 90, 205},
#line 689 "colour-names.gperf.in"
      {"SlateBlue4", 71, 60, 139},
      {""},
#line 688 "colour-names.gperf.in"
      {"SlateBlue3", 105, 89, 205},
#line 619 "colour-names.gperf.in"
      {"pink2", 238, 169, 184},
      {""}, {""}, {""},
#line 687 "colour-names.gperf.in"
      {"SlateBlue2", 122, 103, 238},
#line 691 "colour-names.gperf.in"
      {"SlateGray1", 198, 226, 255},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 79 "colour-names.gperf.in"
      {"cyan4", 0, 139, 139},
#line 686 "colour-names.gperf.in"
      {"SlateBlue1", 131, 111, 255},
      {""},
#line 181 "colour-names.gperf.in"
      {"goldenrod", 218, 165, 32},
#line 78 "colour-names.gperf.in"
      {"cyan3", 0, 205, 205},
      {""}, {""}, {""},
#line 618 "colour-names.gperf.in"
      {"pink1", 255, 181, 197},
      {""}, {""}, {""}, {""}, {""},
#line 77 "colour-names.gperf.in"
      {"cyan2", 0, 238, 238},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 75 "colour-names.gperf.in"
      {"cyan", 0, 255, 255},
      {""}, {""}, {""}, {""},
#line 99 "colour-names.gperf.in"
      {"DarkBlue", 0, 0, 139},
      {""}, {""}, {""}, {""}, {""},
#line 76 "colour-names.gperf.in"
      {"cyan1", 0, 255, 255},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 296 "colour-names.gperf.in"
      {"grey0", 0, 0, 0},
#line 332 "colour-names.gperf.in"
      {"grey40", 102, 102, 102},
      {""},
#line 321 "colour-names.gperf.in"
      {"grey30", 77, 77, 77},
      {""}, {""}, {""}, {""},
#line 310 "colour-names.gperf.in"
      {"grey20", 51, 51, 51},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 708 "colour-names.gperf.in"
      {"SteelBlue", 70, 130, 180},
#line 712 "colour-names.gperf.in"
      {"SteelBlue4", 54, 100, 139},
      {""},
#line 711 "colour-names.gperf.in"
      {"SteelBlue3", 79, 148, 205},
#line 298 "colour-names.gperf.in"
      {"grey10", 26, 26, 26},
      {""},
#line 573 "colour-names.gperf.in"
      {"orange red", 255, 69, 0},
      {""},
#line 710 "colour-names.gperf.in"
      {"SteelBlue2", 92, 172, 238},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 709 "colour-names.gperf.in"
      {"SteelBlue1", 99, 184, 255},
      {""}, {""}, {""}, {""},
#line 629 "colour-names.gperf.in"
      {"purple", 160, 32, 240},
#line 633 "colour-names.gperf.in"
      {"purple4", 85, 26, 139},
      {""},
#line 187 "colour-names.gperf.in"
      {"gray0", 0, 0, 0},
#line 223 "colour-names.gperf.in"
      {"gray40", 102, 102, 102},
#line 632 "colour-names.gperf.in"
      {"purple3", 125, 38, 205},
#line 212 "colour-names.gperf.in"
      {"gray30", 77, 77, 77},
#line 91 "colour-names.gperf.in"
      {"dark red", 139, 0, 0},
      {""}, {""}, {""},
#line 201 "colour-names.gperf.in"
      {"gray20", 51, 51, 51},
#line 67 "colour-names.gperf.in"
      {"coral4", 139, 62, 47},
      {""}, {""},
#line 631 "colour-names.gperf.in"
      {"purple2", 145, 44, 238},
#line 66 "colour-names.gperf.in"
      {"coral3", 205, 91, 69},
      {""}, {""}, {""},
#line 159 "colour-names.gperf.in"
      {"DodgerBlue", 30, 144, 255},
#line 163 "colour-names.gperf.in"
      {"DodgerBlue4", 16, 78, 139},
#line 189 "colour-names.gperf.in"
      {"gray10", 26, 26, 26},
#line 162 "colour-names.gperf.in"
      {"DodgerBlue3", 24, 116, 205},
      {""},
#line 89 "colour-names.gperf.in"
      {"dark orange", 255, 140, 0},
#line 65 "colour-names.gperf.in"
      {"coral2", 238, 106, 80},
      {""},
#line 161 "colour-names.gperf.in"
      {"DodgerBlue2", 28, 134, 238},
      {""}, {""},
#line 597 "colour-names.gperf.in"
      {"PaleGreen4", 84, 139, 84},
      {""},
#line 596 "colour-names.gperf.in"
      {"PaleGreen3", 124, 205, 124},
      {""}, {""}, {""},
#line 630 "colour-names.gperf.in"
      {"purple1", 155, 48, 255},
#line 595 "colour-names.gperf.in"
      {"PaleGreen2", 144, 238, 144},
#line 160 "colour-names.gperf.in"
      {"DodgerBlue1", 30, 144, 255},
#line 651 "colour-names.gperf.in"
      {"saddle brown", 139, 69, 19},
      {""}, {""}, {""}, {""}, {""},
#line 445 "colour-names.gperf.in"
      {"light green", 144, 238, 144},
      {""},
#line 64 "colour-names.gperf.in"
      {"coral1", 255, 114, 86},
#line 594 "colour-names.gperf.in"
      {"PaleGreen1", 154, 255, 154},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 593 "colour-names.gperf.in"
      {"PaleGreen", 152, 251, 152},
      {""}, {""}, {""}, {""},
#line 158 "colour-names.gperf.in"
      {"dodger blue", 30, 144, 255},
      {""}, {""},
#line 626 "colour-names.gperf.in"
      {"plum4", 139, 102, 139},
#line 105 "colour-names.gperf.in"
      {"DarkGoldenrod4", 139, 101, 8},
      {""}, {""},
#line 625 "colour-names.gperf.in"
      {"plum3", 205, 150, 205},
#line 104 "colour-names.gperf.in"
      {"DarkGoldenrod3", 205, 149, 12},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 624 "colour-names.gperf.in"
      {"plum2", 238, 174, 238},
#line 103 "colour-names.gperf.in"
      {"DarkGoldenrod2", 238, 173, 14},
#line 616 "colour-names.gperf.in"
      {"peru", 205, 133, 63},
      {""}, {""}, {""}, {""},
#line 565 "colour-names.gperf.in"
      {"OldLace", 253, 245, 230},
      {""}, {""}, {""},
#line 684 "colour-names.gperf.in"
      {"slate grey", 112, 128, 144},
#line 517 "colour-names.gperf.in"
      {"maroon4", 139, 28, 98},
      {""}, {""}, {""},
#line 516 "colour-names.gperf.in"
      {"maroon3", 205, 41, 144},
      {""},
#line 133 "colour-names.gperf.in"
      {"DarkSlateBlue", 72, 61, 139},
      {""}, {""}, {""},
#line 623 "colour-names.gperf.in"
      {"plum1", 255, 187, 255},
#line 102 "colour-names.gperf.in"
      {"DarkGoldenrod1", 255, 185, 15},
#line 456 "colour-names.gperf.in"
      {"LightBlue", 173, 216, 230},
#line 460 "colour-names.gperf.in"
      {"LightBlue4", 104, 131, 139},
#line 515 "colour-names.gperf.in"
      {"maroon2", 238, 48, 167},
#line 459 "colour-names.gperf.in"
      {"LightBlue3", 154, 192, 205},
      {""}, {""},
#line 677 "colour-names.gperf.in"
      {"SkyBlue", 135, 206, 235},
#line 681 "colour-names.gperf.in"
      {"SkyBlue4", 74, 112, 139},
#line 458 "colour-names.gperf.in"
      {"LightBlue2", 178, 223, 238},
      {""}, {""},
#line 680 "colour-names.gperf.in"
      {"SkyBlue3", 108, 166, 205},
      {""},
#line 513 "colour-names.gperf.in"
      {"maroon", 176, 48, 96},
      {""},
#line 449 "colour-names.gperf.in"
      {"light sea green", 32, 178, 170},
      {""}, {""},
#line 695 "colour-names.gperf.in"
      {"SlateGrey", 112, 128, 144},
#line 457 "colour-names.gperf.in"
      {"LightBlue1", 191, 239, 255},
      {""},
#line 679 "colour-names.gperf.in"
      {"SkyBlue2", 126, 192, 238},
      {""}, {""},
#line 514 "colour-names.gperf.in"
      {"maroon1", 255, 52, 179},
      {""}, {""},
#line 676 "colour-names.gperf.in"
      {"sky blue", 135, 206, 235},
      {""},
#line 683 "colour-names.gperf.in"
      {"slate gray", 112, 128, 144},
      {""},
#line 63 "colour-names.gperf.in"
      {"coral", 255, 127, 80},
      {""}, {""}, {""}, {""},
#line 101 "colour-names.gperf.in"
      {"DarkGoldenrod", 184, 134, 11},
      {""}, {""},
#line 682 "colour-names.gperf.in"
      {"slate blue", 106, 90, 205},
      {""}, {""}, {""},
#line 678 "colour-names.gperf.in"
      {"SkyBlue1", 135, 206, 255},
#line 506 "colour-names.gperf.in"
      {"LimeGreen", 50, 205, 50},
#line 512 "colour-names.gperf.in"
      {"magenta4", 139, 0, 139},
      {""}, {""}, {""},
#line 511 "colour-names.gperf.in"
      {"magenta3", 205, 0, 205},
#line 21 "colour-names.gperf.in"
      {"beige", 245, 245, 220},
      {""}, {""}, {""}, {""}, {""},
#line 110 "colour-names.gperf.in"
      {"DarkMagenta", 139, 0, 139},
      {""},
#line 391 "colour-names.gperf.in"
      {"grey94", 240, 240, 240},
#line 510 "colour-names.gperf.in"
      {"magenta2", 238, 0, 238},
#line 690 "colour-names.gperf.in"
      {"SlateGray", 112, 128, 144},
#line 171 "colour-names.gperf.in"
      {"forest green", 34, 139, 34},
#line 390 "colour-names.gperf.in"
      {"grey93", 237, 237, 237},
#line 140 "colour-names.gperf.in"
      {"DarkTurquoise", 0, 206, 209},
      {""}, {""},
#line 108 "colour-names.gperf.in"
      {"DarkGrey", 169, 169, 169},
      {""}, {""}, {""}, {""}, {""},
#line 389 "colour-names.gperf.in"
      {"grey92", 235, 235, 235},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 509 "colour-names.gperf.in"
      {"magenta1", 255, 0, 255},
      {""}, {""},
#line 93 "colour-names.gperf.in"
      {"dark sea green", 143, 188, 143},
      {""},
#line 508 "colour-names.gperf.in"
      {"magenta", 255, 0, 255},
#line 80 "colour-names.gperf.in"
      {"dark blue", 0, 0, 139},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 388 "colour-names.gperf.in"
      {"grey91", 232, 232, 232},
      {""}, {""}, {""}, {""}, {""},
#line 282 "colour-names.gperf.in"
      {"gray94", 240, 240, 240},
      {""}, {""}, {""},
#line 281 "colour-names.gperf.in"
      {"gray93", 237, 237, 237},
      {""},
#line 564 "colour-names.gperf.in"
      {"old lace", 253, 245, 230},
#line 299 "colour-names.gperf.in"
      {"grey100", 255, 255, 255},
#line 106 "colour-names.gperf.in"
      {"DarkGray", 169, 169, 169},
#line 138 "colour-names.gperf.in"
      {"DarkSlateGray4", 82, 139, 139},
#line 495 "colour-names.gperf.in"
      {"LightSteelBlue", 176, 196, 222},
#line 499 "colour-names.gperf.in"
      {"LightSteelBlue4", 110, 123, 139},
#line 658 "colour-names.gperf.in"
      {"sandy brown", 244, 164, 96},
#line 137 "colour-names.gperf.in"
      {"DarkSlateGray3", 121, 205, 205},
#line 280 "colour-names.gperf.in"
      {"gray92", 235, 235, 235},
#line 498 "colour-names.gperf.in"
      {"LightSteelBlue3", 162, 181, 205},
      {""},
#line 448 "colour-names.gperf.in"
      {"light salmon", 255, 160, 122},
#line 670 "colour-names.gperf.in"
      {"seashell4", 139, 134, 130},
#line 707 "colour-names.gperf.in"
      {"steel blue", 70, 130, 180},
      {""}, {""},
#line 669 "colour-names.gperf.in"
      {"seashell3", 205, 197, 191},
#line 136 "colour-names.gperf.in"
      {"DarkSlateGray2", 141, 238, 238},
      {""},
#line 497 "colour-names.gperf.in"
      {"LightSteelBlue2", 188, 210, 238},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 668 "colour-names.gperf.in"
      {"seashell2", 238, 229, 222},
      {""}, {""}, {""},
#line 279 "colour-names.gperf.in"
      {"gray91", 232, 232, 232},
      {""},
#line 471 "colour-names.gperf.in"
      {"LightGoldenrod4", 139, 129, 76},
      {""}, {""}, {""},
#line 470 "colour-names.gperf.in"
      {"LightGoldenrod3", 205, 190, 112},
      {""}, {""},
#line 135 "colour-names.gperf.in"
      {"DarkSlateGray1", 151, 255, 255},
      {""},
#line 496 "colour-names.gperf.in"
      {"LightSteelBlue1", 202, 225, 255},
      {""},
#line 190 "colour-names.gperf.in"
      {"gray100", 255, 255, 255},
      {""}, {""},
#line 469 "colour-names.gperf.in"
      {"LightGoldenrod2", 238, 220, 130},
      {""},
#line 667 "colour-names.gperf.in"
      {"seashell1", 255, 245, 238},
      {""}, {""}, {""},
#line 380 "colour-names.gperf.in"
      {"grey84", 214, 214, 214},
      {""}, {""}, {""},
#line 379 "colour-names.gperf.in"
      {"grey83", 212, 212, 212},
#line 53 "colour-names.gperf.in"
      {"chartreuse", 127, 255, 0},
#line 57 "colour-names.gperf.in"
      {"chartreuse4", 69, 139, 0},
      {""},
#line 56 "colour-names.gperf.in"
      {"chartreuse3", 102, 205, 0},
#line 646 "colour-names.gperf.in"
      {"RoyalBlue", 65, 105, 225},
#line 650 "colour-names.gperf.in"
      {"RoyalBlue4", 39, 64, 139},
      {""},
#line 649 "colour-names.gperf.in"
      {"RoyalBlue3", 58, 95, 205},
#line 55 "colour-names.gperf.in"
      {"chartreuse2", 118, 238, 0},
#line 378 "colour-names.gperf.in"
      {"grey82", 209, 209, 209},
#line 589 "colour-names.gperf.in"
      {"pale green", 152, 251, 152},
#line 468 "colour-names.gperf.in"
      {"LightGoldenrod1", 255, 236, 139},
#line 648 "colour-names.gperf.in"
      {"RoyalBlue2", 67, 110, 238},
#line 696 "colour-names.gperf.in"
      {"snow", 255, 250, 250},
#line 504 "colour-names.gperf.in"
      {"LightYellow4", 139, 139, 122},
      {""}, {""}, {""},
#line 503 "colour-names.gperf.in"
      {"LightYellow3", 205, 205, 180},
#line 54 "colour-names.gperf.in"
      {"chartreuse1", 127, 255, 0},
      {""}, {""}, {""},
#line 647 "colour-names.gperf.in"
      {"RoyalBlue1", 72, 118, 255},
      {""}, {""}, {""}, {""},
#line 502 "colour-names.gperf.in"
      {"LightYellow2", 238, 238, 209},
      {""},
#line 446 "colour-names.gperf.in"
      {"light grey", 211, 211, 211},
#line 377 "colour-names.gperf.in"
      {"grey81", 207, 207, 207},
#line 727 "colour-names.gperf.in"
      {"tomato4", 139, 54, 38},
      {""}, {""}, {""},
#line 726 "colour-names.gperf.in"
      {"tomato3", 205, 79, 57},
#line 271 "colour-names.gperf.in"
      {"gray84", 214, 214, 214},
      {""}, {""}, {""},
#line 270 "colour-names.gperf.in"
      {"gray83", 212, 212, 212},
      {""}, {""}, {""},
#line 744 "colour-names.gperf.in"
      {"wheat4", 139, 126, 102},
#line 725 "colour-names.gperf.in"
      {"tomato2", 238, 92, 66},
      {""},
#line 467 "colour-names.gperf.in"
      {"LightGoldenrod", 238, 221, 130},
#line 743 "colour-names.gperf.in"
      {"wheat3", 205, 186, 150},
#line 501 "colour-names.gperf.in"
      {"LightYellow1", 255, 255, 224},
#line 269 "colour-names.gperf.in"
      {"gray82", 209, 209, 209},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 742 "colour-names.gperf.in"
      {"wheat2", 238, 216, 174},
      {""},
#line 475 "colour-names.gperf.in"
      {"LightGrey", 211, 211, 211},
      {""},
#line 617 "colour-names.gperf.in"
      {"pink", 255, 192, 203},
      {""}, {""},
#line 127 "colour-names.gperf.in"
      {"DarkSalmon", 233, 150, 122},
      {""},
#line 724 "colour-names.gperf.in"
      {"tomato1", 255, 99, 71},
      {""}, {""},
#line 100 "colour-names.gperf.in"
      {"DarkCyan", 0, 139, 139},
#line 444 "colour-names.gperf.in"
      {"light gray", 211, 211, 211},
#line 268 "colour-names.gperf.in"
      {"gray81", 207, 207, 207},
      {""}, {""},
#line 666 "colour-names.gperf.in"
      {"seashell", 255, 245, 238},
#line 549 "colour-names.gperf.in"
      {"MistyRose", 255, 228, 225},
#line 553 "colour-names.gperf.in"
      {"MistyRose4", 139, 125, 123},
      {""},
#line 552 "colour-names.gperf.in"
      {"MistyRose3", 205, 183, 181},
#line 741 "colour-names.gperf.in"
      {"wheat1", 255, 231, 186},
#line 439 "colour-names.gperf.in"
      {"light blue", 173, 216, 230},
      {""}, {""},
#line 551 "colour-names.gperf.in"
      {"MistyRose2", 238, 213, 210},
      {""}, {""}, {""}, {""},
#line 58 "colour-names.gperf.in"
      {"chocolate", 210, 105, 30},
#line 62 "colour-names.gperf.in"
      {"chocolate4", 139, 69, 19},
      {""},
#line 61 "colour-names.gperf.in"
      {"chocolate3", 205, 102, 29},
      {""}, {""},
#line 550 "colour-names.gperf.in"
      {"MistyRose1", 255, 228, 225},
      {""},
#line 60 "colour-names.gperf.in"
      {"chocolate2", 238, 118, 33},
#line 644 "colour-names.gperf.in"
      {"RosyBrown4", 139, 105, 105},
#line 505 "colour-names.gperf.in"
      {"lime green", 50, 205, 50},
#line 643 "colour-names.gperf.in"
      {"RosyBrown3", 205, 155, 155},
      {""},
#line 473 "colour-names.gperf.in"
      {"LightGray", 211, 211, 211},
#line 485 "colour-names.gperf.in"
      {"LightSalmon4", 139, 87, 66},
#line 413 "colour-names.gperf.in"
      {"IndianRed4", 139, 58, 58},
#line 642 "colour-names.gperf.in"
      {"RosyBrown2", 238, 180, 180},
#line 412 "colour-names.gperf.in"
      {"IndianRed3", 205, 85, 85},
#line 484 "colour-names.gperf.in"
      {"LightSalmon3", 205, 129, 98},
#line 59 "colour-names.gperf.in"
      {"chocolate1", 255, 127, 36},
      {""},
#line 492 "colour-names.gperf.in"
      {"LightSlateBlue", 132, 112, 255},
#line 411 "colour-names.gperf.in"
      {"IndianRed2", 238, 99, 99},
      {""}, {""},
#line 432 "colour-names.gperf.in"
      {"LawnGreen", 124, 252, 0},
      {""},
#line 641 "colour-names.gperf.in"
      {"RosyBrown1", 255, 193, 193},
#line 483 "colour-names.gperf.in"
      {"LightSalmon2", 238, 149, 114},
      {""},
#line 571 "colour-names.gperf.in"
      {"OliveDrab4", 105, 139, 34},
      {""},
#line 570 "colour-names.gperf.in"
      {"OliveDrab3", 154, 205, 50},
#line 410 "colour-names.gperf.in"
      {"IndianRed1", 255, 106, 106},
#line 640 "colour-names.gperf.in"
      {"RosyBrown", 188, 143, 143},
      {""},
#line 723 "colour-names.gperf.in"
      {"tomato", 255, 99, 71},
#line 569 "colour-names.gperf.in"
      {"OliveDrab2", 179, 238, 58},
      {""},
#line 481 "colour-names.gperf.in"
      {"LightSalmon", 255, 160, 122},
#line 369 "colour-names.gperf.in"
      {"grey74", 189, 189, 189},
      {""}, {""}, {""},
#line 368 "colour-names.gperf.in"
      {"grey73", 186, 186, 186},
#line 82 "colour-names.gperf.in"
      {"dark goldenrod", 184, 134, 11},
#line 461 "colour-names.gperf.in"
      {"LightCoral", 240, 128, 128},
      {""},
#line 568 "colour-names.gperf.in"
      {"OliveDrab1", 192, 255, 62},
#line 567 "colour-names.gperf.in"
      {"OliveDrab", 107, 142, 35},
#line 482 "colour-names.gperf.in"
      {"LightSalmon1", 255, 160, 122},
      {""}, {""}, {""},
#line 367 "colour-names.gperf.in"
      {"grey72", 184, 184, 184},
      {""}, {""}, {""},
#line 85 "colour-names.gperf.in"
      {"dark grey", 169, 169, 169},
      {""}, {""}, {""}, {""}, {""},
#line 87 "colour-names.gperf.in"
      {"dark magenta", 139, 0, 139},
      {""},
#line 48 "colour-names.gperf.in"
      {"CadetBlue", 95, 158, 160},
#line 52 "colour-names.gperf.in"
      {"CadetBlue4", 83, 134, 139},
      {""},
#line 51 "colour-names.gperf.in"
      {"CadetBlue3", 122, 197, 205},
      {""}, {""}, {""}, {""},
#line 50 "colour-names.gperf.in"
      {"CadetBlue2", 142, 229, 238},
      {""},
#line 366 "colour-names.gperf.in"
      {"grey71", 181, 181, 181},
      {""}, {""},
#line 740 "colour-names.gperf.in"
      {"wheat", 245, 222, 179},
      {""}, {""},
#line 260 "colour-names.gperf.in"
      {"gray74", 189, 189, 189},
      {""}, {""},
#line 49 "colour-names.gperf.in"
      {"CadetBlue1", 152, 245, 255},
#line 259 "colour-names.gperf.in"
      {"gray73", 186, 186, 186},
      {""},
#line 409 "colour-names.gperf.in"
      {"IndianRed", 205, 92, 92},
#line 139 "colour-names.gperf.in"
      {"DarkSlateGrey", 47, 79, 79},
#line 149 "colour-names.gperf.in"
      {"DeepSkyBlue", 0, 191, 255},
#line 153 "colour-names.gperf.in"
      {"DeepSkyBlue4", 0, 104, 139},
      {""}, {""},
#line 358 "colour-names.gperf.in"
      {"grey64", 163, 163, 163},
#line 152 "colour-names.gperf.in"
      {"DeepSkyBlue3", 0, 154, 205},
#line 258 "colour-names.gperf.in"
      {"gray72", 184, 184, 184},
#line 27 "colour-names.gperf.in"
      {"black", 0, 0, 0},
#line 357 "colour-names.gperf.in"
      {"grey63", 161, 161, 161},
      {""},
#line 83 "colour-names.gperf.in"
      {"dark gray", 169, 169, 169},
      {""}, {""}, {""},
#line 173 "colour-names.gperf.in"
      {"gainsboro", 220, 220, 220},
#line 151 "colour-names.gperf.in"
      {"DeepSkyBlue2", 0, 178, 238},
      {""}, {""},
#line 356 "colour-names.gperf.in"
      {"grey62", 158, 158, 158},
      {""}, {""}, {""}, {""}, {""},
#line 466 "colour-names.gperf.in"
      {"LightCyan4", 122, 139, 139},
      {""},
#line 465 "colour-names.gperf.in"
      {"LightCyan3", 180, 205, 205},
      {""},
#line 257 "colour-names.gperf.in"
      {"gray71", 181, 181, 181},
      {""}, {""},
#line 464 "colour-names.gperf.in"
      {"LightCyan2", 209, 238, 238},
      {""}, {""}, {""}, {""},
#line 92 "colour-names.gperf.in"
      {"dark salmon", 233, 150, 122},
#line 150 "colour-names.gperf.in"
      {"DeepSkyBlue1", 0, 191, 255},
      {""},
#line 94 "colour-names.gperf.in"
      {"dark slate blue", 72, 61, 139},
#line 355 "colour-names.gperf.in"
      {"grey61", 156, 156, 156},
#line 134 "colour-names.gperf.in"
      {"DarkSlateGray", 47, 79, 79},
#line 463 "colour-names.gperf.in"
      {"LightCyan1", 224, 255, 255},
      {""}, {""}, {""},
#line 249 "colour-names.gperf.in"
      {"gray64", 163, 163, 163},
      {""}, {""},
#line 462 "colour-names.gperf.in"
      {"LightCyan", 224, 255, 255},
#line 248 "colour-names.gperf.in"
      {"gray63", 161, 161, 161},
      {""}, {""},
#line 487 "colour-names.gperf.in"
      {"LightSkyBlue", 135, 206, 250},
#line 491 "colour-names.gperf.in"
      {"LightSkyBlue4", 96, 123, 139},
      {""}, {""}, {""},
#line 490 "colour-names.gperf.in"
      {"LightSkyBlue3", 141, 182, 205},
#line 418 "colour-names.gperf.in"
      {"ivory4", 139, 139, 131},
#line 247 "colour-names.gperf.in"
      {"gray62", 158, 158, 158},
#line 718 "colour-names.gperf.in"
      {"thistle", 216, 191, 216},
#line 722 "colour-names.gperf.in"
      {"thistle4", 139, 123, 139},
#line 417 "colour-names.gperf.in"
      {"ivory3", 205, 205, 193},
#line 622 "colour-names.gperf.in"
      {"plum", 221, 160, 221},
#line 442 "colour-names.gperf.in"
      {"light goldenrod", 238, 221, 130},
#line 721 "colour-names.gperf.in"
      {"thistle3", 205, 181, 205},
#line 347 "colour-names.gperf.in"
      {"grey54", 138, 138, 138},
#line 489 "colour-names.gperf.in"
      {"LightSkyBlue2", 164, 211, 238},
#line 645 "colour-names.gperf.in"
      {"royal blue", 65, 105, 225},
      {""},
#line 346 "colour-names.gperf.in"
      {"grey53", 135, 135, 135},
      {""},
#line 416 "colour-names.gperf.in"
      {"ivory2", 238, 238, 224},
      {""}, {""},
#line 720 "colour-names.gperf.in"
      {"thistle2", 238, 210, 238},
      {""}, {""},
#line 4 "colour-names.gperf.in"
      {"AliceBlue", 240, 248, 255},
#line 592 "colour-names.gperf.in"
      {"PaleGoldenrod", 238, 232, 170},
#line 345 "colour-names.gperf.in"
      {"grey52", 133, 133, 133},
#line 246 "colour-names.gperf.in"
      {"gray61", 156, 156, 156},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 488 "colour-names.gperf.in"
      {"LightSkyBlue1", 176, 226, 255},
      {""}, {""}, {""}, {""},
#line 415 "colour-names.gperf.in"
      {"ivory1", 255, 255, 240},
      {""}, {""},
#line 719 "colour-names.gperf.in"
      {"thistle1", 255, 225, 255},
      {""}, {""}, {""}, {""},
#line 344 "colour-names.gperf.in"
      {"grey51", 130, 130, 130},
      {""}, {""}, {""},
#line 598 "colour-names.gperf.in"
      {"PaleTurquoise", 175, 238, 238},
#line 602 "colour-names.gperf.in"
      {"PaleTurquoise4", 102, 139, 139},
#line 238 "colour-names.gperf.in"
      {"gray54", 138, 138, 138},
      {""}, {""},
#line 601 "colour-names.gperf.in"
      {"PaleTurquoise3", 150, 205, 205},
#line 237 "colour-names.gperf.in"
      {"gray53", 135, 135, 135},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 600 "colour-names.gperf.in"
      {"PaleTurquoise2", 174, 238, 238},
#line 236 "colour-names.gperf.in"
      {"gray52", 133, 133, 133},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 728 "colour-names.gperf.in"
      {"turquoise", 64, 224, 208},
#line 732 "colour-names.gperf.in"
      {"turquoise4", 0, 134, 139},
      {""},
#line 731 "colour-names.gperf.in"
      {"turquoise3", 0, 197, 205},
#line 81 "colour-names.gperf.in"
      {"dark cyan", 0, 139, 139},
      {""}, {""}, {""},
#line 730 "colour-names.gperf.in"
      {"turquoise2", 0, 229, 238},
      {""},
#line 548 "colour-names.gperf.in"
      {"misty rose", 255, 228, 225},
      {""},
#line 599 "colour-names.gperf.in"
      {"PaleTurquoise1", 187, 255, 255},
#line 235 "colour-names.gperf.in"
      {"gray51", 130, 130, 130},
      {""}, {""}, {""}, {""},
#line 157 "colour-names.gperf.in"
      {"DimGrey", 105, 105, 105},
#line 729 "colour-names.gperf.in"
      {"turquoise1", 0, 245, 255},
#line 155 "colour-names.gperf.in"
      {"dim grey", 105, 105, 105},
      {""},
#line 431 "colour-names.gperf.in"
      {"lawn green", 124, 252, 0},
      {""}, {""}, {""}, {""},
#line 440 "colour-names.gperf.in"
      {"light coral", 240, 128, 128},
      {""}, {""}, {""},
#line 639 "colour-names.gperf.in"
      {"rosy brown", 188, 143, 143},
      {""}, {""}, {""}, {""}, {""},
#line 408 "colour-names.gperf.in"
      {"indian red", 205, 92, 92},
      {""},
#line 387 "colour-names.gperf.in"
      {"grey90", 229, 229, 229},
      {""},
#line 423 "colour-names.gperf.in"
      {"khaki4", 139, 134, 78},
#line 494 "colour-names.gperf.in"
      {"LightSlateGrey", 119, 136, 153},
      {""}, {""},
#line 422 "colour-names.gperf.in"
      {"khaki3", 205, 198, 115},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 566 "colour-names.gperf.in"
      {"olive drab", 107, 142, 35},
      {""}, {""},
#line 421 "colour-names.gperf.in"
      {"khaki2", 238, 230, 133},
      {""}, {""}, {""}, {""},
#line 156 "colour-names.gperf.in"
      {"DimGray", 105, 105, 105},
      {""},
#line 154 "colour-names.gperf.in"
      {"dim gray", 105, 105, 105},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 659 "colour-names.gperf.in"
      {"SandyBrown", 244, 164, 96},
      {""}, {""}, {""}, {""},
#line 420 "colour-names.gperf.in"
      {"khaki1", 255, 246, 143},
      {""}, {""}, {""},
#line 278 "colour-names.gperf.in"
      {"gray90", 229, 229, 229},
      {""}, {""},
#line 493 "colour-names.gperf.in"
      {"LightSlateGray", 119, 136, 153},
      {""}, {""}, {""}, {""},
#line 47 "colour-names.gperf.in"
      {"cadet blue", 95, 158, 160},
      {""}, {""}, {""}, {""},
#line 587 "colour-names.gperf.in"
      {"orchid4", 139, 71, 137},
      {""}, {""}, {""},
#line 586 "colour-names.gperf.in"
      {"orchid3", 205, 105, 201},
      {""}, {""}, {""},
#line 753 "colour-names.gperf.in"
      {"yellow4", 139, 139, 0},
      {""}, {""}, {""},
#line 752 "colour-names.gperf.in"
      {"yellow3", 205, 205, 0},
      {""},
#line 585 "colour-names.gperf.in"
      {"orchid2", 238, 122, 233},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 751 "colour-names.gperf.in"
      {"yellow2", 238, 238, 0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
#line 584 "colour-names.gperf.in"
      {"orchid1", 255, 131, 250},
#line 96 "colour-names.gperf.in"
      {"dark slate grey", 47, 79, 79},
#line 453 "colour-names.gperf.in"
      {"light slate grey", 119, 136, 153},
      {""}, {""}, {""},
#line 441 "colour-names.gperf.in"
      {"light cyan", 224, 255, 255},
      {""},
#line 750 "colour-names.gperf.in"
      {"yellow1", 255, 255, 0},
#line 376 "colour-names.gperf.in"
      {"grey80", 204, 204, 204},
      {""}, {""}, {""}, {""}, {""},
#line 454 "colour-names.gperf.in"
      {"light steel blue", 176, 196, 222},
      {""}, {""}, {""}, {""},
#line 754 "colour-names.gperf.in"
      {"YellowGreen", 154, 205, 50},
      {""}, {""}, {""}, {""},
#line 424 "colour-names.gperf.in"
      {"lavender", 230, 230, 250},
      {""}, {""}, {""}, {""},
#line 294 "colour-names.gperf.in"
      {"GreenYellow", 173, 255, 47},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 583 "colour-names.gperf.in"
      {"orchid", 218, 112, 214},
      {""}, {""}, {""}, {""}, {""},
#line 95 "colour-names.gperf.in"
      {"dark slate gray", 47, 79, 79},
#line 452 "colour-names.gperf.in"
      {"light slate gray", 119, 136, 153},
      {""}, {""},
#line 29 "colour-names.gperf.in"
      {"BlanchedAlmond", 255, 235, 205},
      {""}, {""}, {""},
#line 267 "colour-names.gperf.in"
      {"gray80", 204, 204, 204},
      {""}, {""},
#line 451 "colour-names.gperf.in"
      {"light slate blue", 132, 112, 255},
      {""}, {""}, {""}, {""},
#line 46 "colour-names.gperf.in"
      {"burlywood4", 139, 115, 85},
      {""},
#line 45 "colour-names.gperf.in"
      {"burlywood3", 205, 170, 125},
#line 3 "colour-names.gperf.in"
      {"alice blue", 240, 248, 255},
      {""}, {""}, {""},
#line 44 "colour-names.gperf.in"
      {"burlywood2", 238, 197, 145},
#line 561 "colour-names.gperf.in"
      {"navy", 0, 0, 128},
      {""}, {""},
#line 539 "colour-names.gperf.in"
      {"MediumSeaGreen", 60, 179, 113},
      {""},
#line 414 "colour-names.gperf.in"
      {"ivory", 255, 255, 240},
      {""}, {""}, {""}, {""},
#line 43 "colour-names.gperf.in"
      {"burlywood1", 255, 211, 155},
      {""}, {""},
#line 588 "colour-names.gperf.in"
      {"pale goldenrod", 238, 232, 170},
      {""},
#line 739 "colour-names.gperf.in"
      {"VioletRed4", 139, 34, 82},
      {""},
#line 738 "colour-names.gperf.in"
      {"VioletRed3", 205, 50, 120},
      {""}, {""}, {""}, {""},
#line 737 "colour-names.gperf.in"
      {"VioletRed2", 238, 58, 140},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 736 "colour-names.gperf.in"
      {"VioletRed1", 255, 62, 150},
      {""}, {""}, {""}, {""},
#line 745 "colour-names.gperf.in"
      {"white", 255, 255, 255},
#line 386 "colour-names.gperf.in"
      {"grey9", 23, 23, 23},
#line 341 "colour-names.gperf.in"
      {"grey49", 125, 125, 125},
      {""},
#line 330 "colour-names.gperf.in"
      {"grey39", 99, 99, 99},
      {""}, {""},
#line 733 "colour-names.gperf.in"
      {"violet", 238, 130, 238},
      {""},
#line 319 "colour-names.gperf.in"
      {"grey29", 74, 74, 74},
      {""}, {""}, {""},
#line 90 "colour-names.gperf.in"
      {"dark orchid", 153, 50, 204},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 308 "colour-names.gperf.in"
      {"grey19", 48, 48, 48},
      {""},
#line 97 "colour-names.gperf.in"
      {"dark turquoise", 0, 206, 209},
      {""}, {""}, {""}, {""},
#line 42 "colour-names.gperf.in"
      {"burlywood", 222, 184, 135},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 563 "colour-names.gperf.in"
      {"NavyBlue", 0, 0, 128},
      {""}, {""},
#line 365 "colour-names.gperf.in"
      {"grey70", 179, 179, 179},
      {""}, {""}, {""},
#line 277 "colour-names.gperf.in"
      {"gray9", 23, 23, 23},
#line 232 "colour-names.gperf.in"
      {"gray49", 125, 125, 125},
      {""},
#line 221 "colour-names.gperf.in"
      {"gray39", 99, 99, 99},
      {""}, {""}, {""},
#line 735 "colour-names.gperf.in"
      {"VioletRed", 208, 32, 144},
#line 210 "colour-names.gperf.in"
      {"gray29", 74, 74, 74},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 199 "colour-names.gperf.in"
      {"gray19", 48, 48, 48},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 256 "colour-names.gperf.in"
      {"gray70", 179, 179, 179},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 354 "colour-names.gperf.in"
      {"grey60", 153, 153, 153},
      {""}, {""},
#line 628 "colour-names.gperf.in"
      {"PowderBlue", 176, 224, 230},
      {""}, {""}, {""}, {""}, {""},
#line 11 "colour-names.gperf.in"
      {"aquamarine", 127, 255, 212},
#line 15 "colour-names.gperf.in"
      {"aquamarine4", 69, 139, 116},
      {""},
#line 14 "colour-names.gperf.in"
      {"aquamarine3", 102, 205, 170},
      {""}, {""}, {""}, {""},
#line 13 "colour-names.gperf.in"
      {"aquamarine2", 118, 238, 198},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 12 "colour-names.gperf.in"
      {"aquamarine1", 127, 255, 212},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 5 "colour-names.gperf.in"
      {"antique white", 250, 235, 215},
      {""}, {""},
#line 245 "colour-names.gperf.in"
      {"gray60", 153, 153, 153},
      {""},
#line 627 "colour-names.gperf.in"
      {"powder blue", 176, 224, 230},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 500 "colour-names.gperf.in"
      {"LightYellow", 255, 255, 224},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 343 "colour-names.gperf.in"
      {"grey50", 127, 127, 127},
      {""}, {""}, {""}, {""},
#line 522 "colour-names.gperf.in"
      {"medium sea green", 60, 179, 113},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 401 "colour-names.gperf.in"
      {"honeydew4", 131, 139, 131},
      {""}, {""}, {""},
#line 400 "colour-names.gperf.in"
      {"honeydew3", 193, 205, 193},
      {""}, {""},
#line 143 "colour-names.gperf.in"
      {"deep sky blue", 0, 191, 255},
#line 749 "colour-names.gperf.in"
      {"yellow green", 154, 205, 50},
      {""},
#line 6 "colour-names.gperf.in"
      {"AntiqueWhite", 250, 235, 215},
#line 10 "colour-names.gperf.in"
      {"AntiqueWhite4", 139, 131, 120},
      {""}, {""},
#line 399 "colour-names.gperf.in"
      {"honeydew2", 224, 238, 224},
#line 9 "colour-names.gperf.in"
      {"AntiqueWhite3", 205, 192, 176},
      {""}, {""}, {""}, {""},
#line 234 "colour-names.gperf.in"
      {"gray50", 127, 127, 127},
      {""}, {""},
#line 375 "colour-names.gperf.in"
      {"grey8", 20, 20, 20},
#line 340 "colour-names.gperf.in"
      {"grey48", 122, 122, 122},
#line 8 "colour-names.gperf.in"
      {"AntiqueWhite2", 238, 223, 204},
#line 329 "colour-names.gperf.in"
      {"grey38", 97, 97, 97},
      {""}, {""}, {""}, {""},
#line 318 "colour-names.gperf.in"
      {"grey28", 71, 71, 71},
      {""}, {""}, {""},
#line 528 "colour-names.gperf.in"
      {"MediumBlue", 0, 0, 205},
#line 398 "colour-names.gperf.in"
      {"honeydew1", 240, 255, 240},
      {""}, {""}, {""}, {""}, {""},
#line 307 "colour-names.gperf.in"
      {"grey18", 46, 46, 46},
#line 734 "colour-names.gperf.in"
      {"violet red", 208, 32, 144},
      {""},
#line 148 "colour-names.gperf.in"
      {"DeepPink4", 139, 10, 80},
      {""},
#line 7 "colour-names.gperf.in"
      {"AntiqueWhite1", 255, 239, 219},
      {""},
#line 147 "colour-names.gperf.in"
      {"DeepPink3", 205, 16, 118},
      {""}, {""}, {""},
#line 554 "colour-names.gperf.in"
      {"moccasin", 255, 228, 181},
      {""}, {""}, {""}, {""}, {""},
#line 146 "colour-names.gperf.in"
      {"DeepPink2", 238, 18, 137},
#line 450 "colour-names.gperf.in"
      {"light sky blue", 135, 206, 250},
      {""}, {""}, {""}, {""},
#line 266 "colour-names.gperf.in"
      {"gray8", 20, 20, 20},
#line 231 "colour-names.gperf.in"
      {"gray48", 122, 122, 122},
      {""},
#line 220 "colour-names.gperf.in"
      {"gray38", 97, 97, 97},
      {""}, {""}, {""}, {""},
#line 209 "colour-names.gperf.in"
      {"gray28", 71, 71, 71},
      {""}, {""},
#line 519 "colour-names.gperf.in"
      {"medium blue", 0, 0, 205},
      {""}, {""}, {""}, {""},
#line 145 "colour-names.gperf.in"
      {"DeepPink1", 255, 20, 147},
      {""}, {""},
#line 198 "colour-names.gperf.in"
      {"gray18", 46, 46, 46},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 74 "colour-names.gperf.in"
      {"cornsilk4", 139, 136, 120},
      {""}, {""}, {""},
#line 73 "colour-names.gperf.in"
      {"cornsilk3", 205, 200, 177},
      {""}, {""},
#line 562 "colour-names.gperf.in"
      {"navy blue", 0, 0, 128},
      {""}, {""}, {""}, {""},
#line 546 "colour-names.gperf.in"
      {"mint cream", 245, 255, 250},
      {""},
#line 72 "colour-names.gperf.in"
      {"cornsilk2", 238, 232, 205},
      {""}, {""}, {""},
#line 541 "colour-names.gperf.in"
      {"MediumSpringGreen", 0, 250, 154},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 71 "colour-names.gperf.in"
      {"cornsilk1", 255, 248, 220},
#line 443 "colour-names.gperf.in"
      {"light goldenrod yellow", 250, 250, 210},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 125 "colour-names.gperf.in"
      {"DarkOrchid4", 104, 34, 139},
      {""},
#line 124 "colour-names.gperf.in"
      {"DarkOrchid3", 154, 50, 205},
      {""}, {""}, {""}, {""},
#line 123 "colour-names.gperf.in"
      {"DarkOrchid2", 178, 58, 238},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 122 "colour-names.gperf.in"
      {"DarkOrchid1", 191, 62, 255},
      {""},
#line 407 "colour-names.gperf.in"
      {"HotPink4", 139, 58, 98},
      {""}, {""}, {""},
#line 406 "colour-names.gperf.in"
      {"HotPink3", 205, 96, 144},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 430 "colour-names.gperf.in"
      {"LavenderBlush4", 139, 131, 134},
      {""},
#line 405 "colour-names.gperf.in"
      {"HotPink2", 238, 106, 167},
      {""},
#line 429 "colour-names.gperf.in"
      {"LavenderBlush3", 205, 193, 197},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 428 "colour-names.gperf.in"
      {"LavenderBlush2", 238, 224, 229},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 404 "colour-names.gperf.in"
      {"HotPink1", 255, 110, 180},
      {""}, {""},
#line 540 "colour-names.gperf.in"
      {"MediumSlateBlue", 123, 104, 238},
      {""}, {""}, {""},
#line 168 "colour-names.gperf.in"
      {"firebrick4", 139, 26, 26},
      {""},
#line 167 "colour-names.gperf.in"
      {"firebrick3", 205, 38, 38},
#line 472 "colour-names.gperf.in"
      {"LightGoldenrodYellow", 250, 250, 210},
      {""},
#line 427 "colour-names.gperf.in"
      {"LavenderBlush1", 255, 240, 245},
      {""},
#line 166 "colour-names.gperf.in"
      {"firebrick2", 238, 44, 44},
      {""}, {""},
#line 121 "colour-names.gperf.in"
      {"DarkOrchid", 153, 50, 204},
      {""},
#line 480 "colour-names.gperf.in"
      {"LightPink4", 139, 95, 101},
      {""},
#line 479 "colour-names.gperf.in"
      {"LightPink3", 205, 140, 149},
      {""}, {""}, {""},
#line 165 "colour-names.gperf.in"
      {"firebrick1", 255, 48, 48},
#line 478 "colour-names.gperf.in"
      {"LightPink2", 238, 162, 173},
      {""}, {""}, {""},
#line 590 "colour-names.gperf.in"
      {"pale turquoise", 175, 238, 238},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 477 "colour-names.gperf.in"
      {"LightPink1", 255, 174, 185},
      {""}, {""},
#line 545 "colour-names.gperf.in"
      {"MidnightBlue", 25, 25, 112},
      {""}, {""}, {""}, {""},
#line 533 "colour-names.gperf.in"
      {"MediumOrchid4", 122, 55, 139},
      {""}, {""}, {""},
#line 532 "colour-names.gperf.in"
      {"MediumOrchid3", 180, 82, 205},
      {""}, {""}, {""}, {""},
#line 419 "colour-names.gperf.in"
      {"khaki", 240, 230, 140},
      {""}, {""}, {""}, {""},
#line 531 "colour-names.gperf.in"
      {"MediumOrchid2", 209, 95, 238},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 534 "colour-names.gperf.in"
      {"MediumPurple", 147, 112, 219},
#line 538 "colour-names.gperf.in"
      {"MediumPurple4", 93, 71, 139},
      {""}, {""}, {""},
#line 537 "colour-names.gperf.in"
      {"MediumPurple3", 137, 104, 205},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 530 "colour-names.gperf.in"
      {"MediumOrchid1", 224, 102, 255},
#line 536 "colour-names.gperf.in"
      {"MediumPurple2", 159, 121, 238},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 364 "colour-names.gperf.in"
      {"grey7", 18, 18, 18},
#line 339 "colour-names.gperf.in"
      {"grey47", 120, 120, 120},
#line 615 "colour-names.gperf.in"
      {"PeachPuff4", 139, 119, 101},
#line 328 "colour-names.gperf.in"
      {"grey37", 94, 94, 94},
#line 614 "colour-names.gperf.in"
      {"PeachPuff3", 205, 175, 149},
      {""}, {""}, {""},
#line 317 "colour-names.gperf.in"
      {"grey27", 69, 69, 69},
#line 613 "colour-names.gperf.in"
      {"PeachPuff2", 238, 203, 173},
      {""}, {""},
#line 535 "colour-names.gperf.in"
      {"MediumPurple1", 171, 130, 255},
      {""}, {""}, {""},
#line 547 "colour-names.gperf.in"
      {"MintCream", 245, 255, 250},
      {""}, {""},
#line 306 "colour-names.gperf.in"
      {"grey17", 43, 43, 43},
#line 612 "colour-names.gperf.in"
      {"PeachPuff1", 255, 218, 185},
      {""}, {""}, {""}, {""}, {""},
#line 529 "colour-names.gperf.in"
      {"MediumOrchid", 186, 85, 211},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 169 "colour-names.gperf.in"
      {"floral white", 255, 250, 240},
#line 69 "colour-names.gperf.in"
      {"CornflowerBlue", 100, 149, 237},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 255 "colour-names.gperf.in"
      {"gray7", 18, 18, 18},
#line 230 "colour-names.gperf.in"
      {"gray47", 120, 120, 120},
      {""},
#line 219 "colour-names.gperf.in"
      {"gray37", 94, 94, 94},
      {""}, {""}, {""}, {""},
#line 208 "colour-names.gperf.in"
      {"gray27", 69, 69, 69},
      {""}, {""}, {""}, {""},
#line 68 "colour-names.gperf.in"
      {"cornflower blue", 100, 149, 237},
      {""}, {""}, {""}, {""}, {""},
#line 197 "colour-names.gperf.in"
      {"gray17", 43, 43, 43},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
#line 748 "colour-names.gperf.in"
      {"yellow", 255, 255, 0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
#line 353 "colour-names.gperf.in"
      {"grey6", 15, 15, 15},
#line 338 "colour-names.gperf.in"
      {"grey46", 117, 117, 117},
      {""},
#line 327 "colour-names.gperf.in"
      {"grey36", 92, 92, 92},
      {""}, {""}, {""}, {""},
#line 316 "colour-names.gperf.in"
      {"grey26", 66, 66, 66},
      {""}, {""}, {""},
#line 28 "colour-names.gperf.in"
      {"blanched almond", 255, 235, 205},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 305 "colour-names.gperf.in"
      {"grey16", 41, 41, 41},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
#line 244 "colour-names.gperf.in"
      {"gray6", 15, 15, 15},
#line 229 "colour-names.gperf.in"
      {"gray46", 117, 117, 117},
      {""},
#line 218 "colour-names.gperf.in"
      {"gray36", 92, 92, 92},
      {""},
#line 523 "colour-names.gperf.in"
      {"medium slate blue", 123, 104, 238},
      {""}, {""},
#line 207 "colour-names.gperf.in"
      {"gray26", 66, 66, 66},
      {""}, {""}, {""},
#line 115 "colour-names.gperf.in"
      {"DarkOliveGreen4", 110, 139, 61},
      {""}, {""}, {""},
#line 114 "colour-names.gperf.in"
      {"DarkOliveGreen3", 162, 205, 90},
      {""}, {""},
#line 196 "colour-names.gperf.in"
      {"gray16", 41, 41, 41},
      {""},
#line 447 "colour-names.gperf.in"
      {"light pink", 255, 182, 193},
      {""}, {""}, {""}, {""},
#line 113 "colour-names.gperf.in"
      {"DarkOliveGreen2", 188, 238, 104},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 396 "colour-names.gperf.in"
      {"grey99", 252, 252, 252},
      {""},
#line 111 "colour-names.gperf.in"
      {"DarkOliveGreen", 85, 107, 47},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 144 "colour-names.gperf.in"
      {"DeepPink", 255, 20, 147},
      {""}, {""},
#line 611 "colour-names.gperf.in"
      {"PeachPuff", 255, 218, 185},
#line 112 "colour-names.gperf.in"
      {"DarkOliveGreen1", 202, 255, 112},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 287 "colour-names.gperf.in"
      {"gray99", 252, 252, 252},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 342 "colour-names.gperf.in"
      {"grey5", 13, 13, 13},
#line 337 "colour-names.gperf.in"
      {"grey45", 115, 115, 115},
      {""},
#line 326 "colour-names.gperf.in"
      {"grey35", 89, 89, 89},
      {""}, {""}, {""}, {""},
#line 315 "colour-names.gperf.in"
      {"grey25", 64, 64, 64},
      {""}, {""},
#line 70 "colour-names.gperf.in"
      {"cornsilk", 255, 248, 220},
#line 610 "colour-names.gperf.in"
      {"peach puff", 255, 218, 185},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 304 "colour-names.gperf.in"
      {"grey15", 38, 38, 38},
      {""}, {""},
#line 141 "colour-names.gperf.in"
      {"DarkViolet", 148, 0, 211},
#line 289 "colour-names.gperf.in"
      {"green yellow", 173, 255, 47},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 233 "colour-names.gperf.in"
      {"gray5", 13, 13, 13},
#line 228 "colour-names.gperf.in"
      {"gray45", 115, 115, 115},
      {""},
#line 217 "colour-names.gperf.in"
      {"gray35", 89, 89, 89},
      {""}, {""}, {""}, {""},
#line 206 "colour-names.gperf.in"
      {"gray25", 64, 64, 64},
#line 385 "colour-names.gperf.in"
      {"grey89", 227, 227, 227},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 195 "colour-names.gperf.in"
      {"gray15", 38, 38, 38},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 524 "colour-names.gperf.in"
      {"medium spring green", 0, 250, 154},
      {""}, {""},
#line 175 "colour-names.gperf.in"
      {"GhostWhite", 248, 248, 255},
      {""}, {""},
#line 426 "colour-names.gperf.in"
      {"LavenderBlush", 255, 240, 245},
      {""}, {""}, {""}, {""},
#line 403 "colour-names.gperf.in"
      {"HotPink", 255, 105, 180},
      {""}, {""}, {""}, {""}, {""},
#line 746 "colour-names.gperf.in"
      {"white smoke", 245, 245, 245},
      {""}, {""}, {""}, {""},
#line 521 "colour-names.gperf.in"
      {"medium purple", 147, 112, 219},
      {""},
#line 276 "colour-names.gperf.in"
      {"gray89", 227, 227, 227},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 164 "colour-names.gperf.in"
      {"firebrick", 178, 34, 34},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 476 "colour-names.gperf.in"
      {"LightPink", 255, 182, 193},
      {""}, {""}, {""},
#line 527 "colour-names.gperf.in"
      {"MediumAquamarine", 102, 205, 170},
      {""}, {""}, {""},
#line 544 "colour-names.gperf.in"
      {"midnight blue", 25, 25, 112},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 395 "colour-names.gperf.in"
      {"grey98", 250, 250, 250},
#line 397 "colour-names.gperf.in"
      {"honeydew", 240, 255, 240},
      {""}, {""},
#line 36 "colour-names.gperf.in"
      {"BlueViolet", 138, 43, 226},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 170 "colour-names.gperf.in"
      {"FloralWhite", 255, 250, 240},
      {""}, {""}, {""},
#line 374 "colour-names.gperf.in"
      {"grey79", 201, 201, 201},
      {""}, {""}, {""}, {""},
#line 88 "colour-names.gperf.in"
      {"dark olive green", 85, 107, 47},
#line 174 "colour-names.gperf.in"
      {"ghost white", 248, 248, 255},
#line 286 "colour-names.gperf.in"
      {"gray98", 250, 250, 250},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 142 "colour-names.gperf.in"
      {"deep pink", 255, 20, 147},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
#line 265 "colour-names.gperf.in"
      {"gray79", 201, 201, 201},
      {""}, {""}, {""}, {""},
#line 438 "colour-names.gperf.in"
      {"LemonChiffon4", 139, 137, 112},
      {""}, {""},
#line 542 "colour-names.gperf.in"
      {"MediumTurquoise", 72, 209, 204},
#line 437 "colour-names.gperf.in"
      {"LemonChiffon3", 205, 201, 165},
      {""}, {""},
#line 363 "colour-names.gperf.in"
      {"grey69", 176, 176, 176},
      {""}, {""}, {""},
#line 455 "colour-names.gperf.in"
      {"light yellow", 255, 255, 224},
      {""}, {""},
#line 436 "colour-names.gperf.in"
      {"LemonChiffon2", 238, 233, 191},
      {""}, {""}, {""},
#line 384 "colour-names.gperf.in"
      {"grey88", 224, 224, 224},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 434 "colour-names.gperf.in"
      {"LemonChiffon", 255, 250, 205},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 98 "colour-names.gperf.in"
      {"dark violet", 148, 0, 211},
      {""},
#line 435 "colour-names.gperf.in"
      {"LemonChiffon1", 255, 250, 205},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
#line 254 "colour-names.gperf.in"
      {"gray69", 176, 176, 176},
      {""},
#line 747 "colour-names.gperf.in"
      {"WhiteSmoke", 245, 245, 245},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 275 "colour-names.gperf.in"
      {"gray88", 224, 224, 224},
      {""}, {""}, {""},
#line 520 "colour-names.gperf.in"
      {"medium orchid", 186, 85, 211},
      {""}, {""}, {""}, {""}, {""},
#line 352 "colour-names.gperf.in"
      {"grey59", 150, 150, 150},
      {""}, {""}, {""}, {""}, {""},
#line 86 "colour-names.gperf.in"
      {"dark khaki", 189, 183, 107},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 243 "colour-names.gperf.in"
      {"gray59", 150, 150, 150},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 607 "colour-names.gperf.in"
      {"PaleVioletRed4", 139, 71, 93},
      {""}, {""}, {""},
#line 606 "colour-names.gperf.in"
      {"PaleVioletRed3", 205, 104, 137},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 605 "colour-names.gperf.in"
      {"PaleVioletRed2", 238, 121, 159},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 373 "colour-names.gperf.in"
      {"grey78", 199, 199, 199},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 604 "colour-names.gperf.in"
      {"PaleVioletRed1", 255, 130, 171},
      {""},
#line 31 "colour-names.gperf.in"
      {"blue violet", 138, 43, 226},
      {""},
#line 433 "colour-names.gperf.in"
      {"lemon chiffon", 255, 250, 205},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 394 "colour-names.gperf.in"
      {"grey97", 247, 247, 247},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 264 "colour-names.gperf.in"
      {"gray78", 199, 199, 199},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 603 "colour-names.gperf.in"
      {"PaleVioletRed", 219, 112, 147},
#line 555 "colour-names.gperf.in"
      {"navajo white", 255, 222, 173},
#line 402 "colour-names.gperf.in"
      {"hot pink", 255, 105, 180},
      {""}, {""},
#line 362 "colour-names.gperf.in"
      {"grey68", 173, 173, 173},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
#line 285 "colour-names.gperf.in"
      {"gray97", 247, 247, 247},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 253 "colour-names.gperf.in"
      {"gray68", 173, 173, 173},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 351 "colour-names.gperf.in"
      {"grey58", 148, 148, 148},
#line 425 "colour-names.gperf.in"
      {"lavender blush", 255, 240, 245},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 383 "colour-names.gperf.in"
      {"grey87", 222, 222, 222},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 393 "colour-names.gperf.in"
      {"grey96", 245, 245, 245},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 242 "colour-names.gperf.in"
      {"gray58", 148, 148, 148},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 274 "colour-names.gperf.in"
      {"gray87", 222, 222, 222},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 284 "colour-names.gperf.in"
      {"gray96", 245, 245, 245},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 518 "colour-names.gperf.in"
      {"medium aquamarine", 102, 205, 170},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 382 "colour-names.gperf.in"
      {"grey86", 219, 219, 219},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 392 "colour-names.gperf.in"
      {"grey95", 242, 242, 242},
#line 372 "colour-names.gperf.in"
      {"grey77", 196, 196, 196},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 525 "colour-names.gperf.in"
      {"medium turquoise", 72, 209, 204},
      {""}, {""},
#line 273 "colour-names.gperf.in"
      {"gray86", 219, 219, 219},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 109 "colour-names.gperf.in"
      {"DarkKhaki", 189, 183, 107},
      {""}, {""}, {""}, {""},
#line 283 "colour-names.gperf.in"
      {"gray95", 242, 242, 242},
#line 263 "colour-names.gperf.in"
      {"gray77", 196, 196, 196},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 361 "colour-names.gperf.in"
      {"grey67", 171, 171, 171},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 608 "colour-names.gperf.in"
      {"papaya whip", 255, 239, 213},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
#line 252 "colour-names.gperf.in"
      {"gray67", 171, 171, 171},
      {""}, {""},
#line 381 "colour-names.gperf.in"
      {"grey85", 217, 217, 217},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 371 "colour-names.gperf.in"
      {"grey76", 194, 194, 194},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 350 "colour-names.gperf.in"
      {"grey57", 145, 145, 145},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
#line 272 "colour-names.gperf.in"
      {"gray85", 217, 217, 217},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 262 "colour-names.gperf.in"
      {"gray76", 194, 194, 194},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 241 "colour-names.gperf.in"
      {"gray57", 145, 145, 145},
      {""}, {""},
#line 360 "colour-names.gperf.in"
      {"grey66", 168, 168, 168},
#line 556 "colour-names.gperf.in"
      {"NavajoWhite", 255, 222, 173},
#line 560 "colour-names.gperf.in"
      {"NavajoWhite4", 139, 121, 94},
      {""}, {""}, {""},
#line 559 "colour-names.gperf.in"
      {"NavajoWhite3", 205, 179, 139},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 558 "colour-names.gperf.in"
      {"NavajoWhite2", 238, 207, 161},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
#line 557 "colour-names.gperf.in"
      {"NavajoWhite1", 255, 222, 173},
      {""}, {""}, {""},
#line 251 "colour-names.gperf.in"
      {"gray66", 168, 168, 168},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 349 "colour-names.gperf.in"
      {"grey56", 143, 143, 143},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 370 "colour-names.gperf.in"
      {"grey75", 191, 191, 191},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
#line 240 "colour-names.gperf.in"
      {"gray56", 143, 143, 143},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 261 "colour-names.gperf.in"
      {"gray75", 191, 191, 191},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 591 "colour-names.gperf.in"
      {"pale violet red", 219, 112, 147},
      {""},
#line 359 "colour-names.gperf.in"
      {"grey65", 166, 166, 166},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
#line 250 "colour-names.gperf.in"
      {"gray65", 166, 166, 166},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 348 "colour-names.gperf.in"
      {"grey55", 140, 140, 140},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
#line 239 "colour-names.gperf.in"
      {"gray55", 140, 140, 140},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 609 "colour-names.gperf.in"
      {"PapayaWhip", 255, 239, 213},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 543 "colour-names.gperf.in"
      {"MediumVioletRed", 199, 21, 133},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
#line 526 "colour-names.gperf.in"
      {"medium violet red", 199, 21, 133}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          const char *s = wordlist[key].name;

          if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_strcmp (str, s))
            return &wordlist[key];
        }
    }
  return 0;
}
