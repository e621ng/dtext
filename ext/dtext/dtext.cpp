
#line 1 "ext/dtext/dtext.cpp.rl"
#include "dtext.h"

#include <string.h>
#include <algorithm>
#include <tuple>

#ifdef DEBUG
#undef g_debug
#define STRINGIFY(x) XSTRINGIFY(x)
#define XSTRINGIFY(x) #x
#define g_debug(fmt, ...) fprintf(stderr, "\x1B[1;32mDEBUG\x1B[0m %-28.28s %-24.24s " fmt "\n", __FILE__ ":" STRINGIFY(__LINE__), __func__, ##__VA_ARGS__)
#else
#undef g_debug
#define g_debug(...)
#endif

static const size_t MAX_STACK_DEPTH = 512;

// Characters that mark the end of a link.
//
// http://www.fileformat.info/info/unicode/category/Pe/list.htm
// http://www.fileformat.info/info/unicode/block/cjk_symbols_and_punctuation/list.htm
static char32_t boundary_characters[] = {
  0x0021, // '!' U+0021 EXCLAMATION MARK
  0x0029, // ')' U+0029 RIGHT PARENTHESIS
  0x002C, // ',' U+002C COMMA
  0x002E, // '.' U+002E FULL STOP
  0x003A, // ':' U+003A COLON
  0x003B, // ';' U+003B SEMICOLON
  0x003C, // '<' U+003C LESS-THAN SIGN
  0x003E, // '>' U+003E GREATER-THAN SIGN
  0x003F, // '?' U+003F QUESTION MARK
  0x005D, // ']' U+005D RIGHT SQUARE BRACKET
  0x007D, // '}' U+007D RIGHT CURLY BRACKET
  0x276D, // '❭' U+276D MEDIUM RIGHT-POINTING ANGLE BRACKET ORNAMENT
  0x3000, // '　' U+3000 IDEOGRAPHIC SPACE (U+3000)
  0x3001, // '、' U+3001 IDEOGRAPHIC COMMA (U+3001)
  0x3002, // '。' U+3002 IDEOGRAPHIC FULL STOP (U+3002)
  0x3008, // '〈' U+3008 LEFT ANGLE BRACKET (U+3008)
  0x3009, // '〉' U+3009 RIGHT ANGLE BRACKET (U+3009)
  0x300A, // '《' U+300A LEFT DOUBLE ANGLE BRACKET (U+300A)
  0x300B, // '》' U+300B RIGHT DOUBLE ANGLE BRACKET (U+300B)
  0x300C, // '「' U+300C LEFT CORNER BRACKET (U+300C)
  0x300D, // '」' U+300D RIGHT CORNER BRACKET (U+300D)
  0x300E, // '『' U+300E LEFT WHITE CORNER BRACKET (U+300E)
  0x300F, // '』' U+300F RIGHT WHITE CORNER BRACKET (U+300F)
  0x3010, // '【' U+3010 LEFT BLACK LENTICULAR BRACKET (U+3010)
  0x3011, // '】' U+3011 RIGHT BLACK LENTICULAR BRACKET (U+3011)
  0x3014, // '〔' U+3014 LEFT TORTOISE SHELL BRACKET (U+3014)
  0x3015, // '〕' U+3015 RIGHT TORTOISE SHELL BRACKET (U+3015)
  0x3016, // '〖' U+3016 LEFT WHITE LENTICULAR BRACKET (U+3016)
  0x3017, // '〗' U+3017 RIGHT WHITE LENTICULAR BRACKET (U+3017)
  0x3018, // '〘' U+3018 LEFT WHITE TORTOISE SHELL BRACKET (U+3018)
  0x3019, // '〙' U+3019 RIGHT WHITE TORTOISE SHELL BRACKET (U+3019)
  0x301A, // '〚' U+301A LEFT WHITE SQUARE BRACKET (U+301A)
  0x301B, // '〛' U+301B RIGHT WHITE SQUARE BRACKET (U+301B)
  0x301C, // '〜' U+301C WAVE DASH (U+301C)
  0xFF09, // '）' U+FF09 FULLWIDTH RIGHT PARENTHESIS
  0xFF3D, // '］' U+FF3D FULLWIDTH RIGHT SQUARE BRACKET
  0xFF5D, // '｝' U+FF5D FULLWIDTH RIGHT CURLY BRACKET
  0xFF60, // '｠' U+FF60 FULLWIDTH RIGHT WHITE PARENTHESIS
  0xFF63, // '｣' U+FF63 HALFWIDTH RIGHT CORNER BRACKET
};


#line 754 "ext/dtext/dtext.cpp.rl"



#line 68 "ext/dtext/dtext.cpp"
static const int dtext_start = 735;
static const int dtext_first_final = 735;
static const int dtext_error = -1;

static const int dtext_en_basic_inline = 751;
static const int dtext_en_inline = 753;
static const int dtext_en_inline_code = 807;
static const int dtext_en_code = 809;
static const int dtext_en_table = 811;
static const int dtext_en_main = 735;


#line 757 "ext/dtext/dtext.cpp.rl"

void StateMachine::dstack_push(element_t element) {
  dstack.push_back(element);
}

element_t StateMachine::dstack_pop() {
  if (dstack.empty()) {
    g_debug("dstack pop empty stack");
    return DSTACK_EMPTY;
  } else {
    auto element = dstack.back();
    dstack.pop_back();
    return element;
  }
}

element_t StateMachine::dstack_peek() {
  return dstack.empty() ? DSTACK_EMPTY : dstack.back();
}

bool StateMachine::dstack_check(element_t expected_element) {
  return dstack_peek() == expected_element;
}

// Return true if the given tag is currently open.
bool StateMachine::dstack_is_open(element_t element) {
  return std::find(dstack.begin(), dstack.end(), element) != dstack.end();
}

int StateMachine::dstack_count(element_t element) {
  return std::count(dstack.begin(), dstack.end(), element);
}

void StateMachine::append(const std::string_view c) {
  output += c;
}

void StateMachine::append(const char c) {
  output += c;
}

void StateMachine::append_block(const std::string_view s) {
  if (!options.f_inline) {
    append(s);
  }
}

void StateMachine::append_block(const char s) {
  if (!options.f_inline) {
    append(s);
  }
}

void StateMachine::append_html_escaped(char s) {
  switch (s) {
    case '<': append("&lt;"); break;
    case '>': append("&gt;"); break;
    case '&': append("&amp;"); break;
    case '"': append("&quot;"); break;
    default:  append(s);
  }
}

void StateMachine::append_html_escaped(const std::string_view input) {
  for (const unsigned char c : input) {
    append_html_escaped(c);
  }
}

void StateMachine::append_uri_escaped(const std::string_view uri_part, const char whitelist) {
  static const char hex[] = "0123456789ABCDEF";

  for (const unsigned char c : uri_part) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == whitelist) {
      append(c);
    } else {
      append('%');
      append(hex[c >> 4]);
      append(hex[c & 0x0F]);
    }
  }
}

void StateMachine::append_url(const char* url) {
  if ((url[0] == '/' || url[0] == '#') && !options.base_url.empty()) {
    append(options.base_url);
  }

  append(url);
}

void StateMachine::append_id_link(const char * title, const char * id_name, const char * url) {
  append("<a class=\"dtext-link dtext-id-link dtext-");
  append(id_name);
  append("-id-link\" href=\"");
  append_url(url);
  append_uri_escaped({ a1, a2 });
  append("\">");
  append(title);
  append(" #");
  append_html_escaped({ a1, a2 });
  append("</a>");
}

void StateMachine::append_unnamed_url(const std::string_view url) {
  append("<a rel=\"nofollow\" class=\"dtext-link\" href=\"");
  append_html_escaped(url);
  append("\">");
  append_html_escaped(url);
  append("</a>");
}

void StateMachine::append_named_url(const std::string_view url, const std::string_view title) {
  auto parsed_title = parse_basic_inline(title);

  if (url[0] == '/' || url[0] == '#') {
    append("<a rel=\"nofollow\" class=\"dtext-link\" href=\"");
    if (!options.base_url.empty()) {
      append(options.base_url);
    }
  } else {
    append("<a rel=\"nofollow\" class=\"dtext-link dtext-external-link\" href=\"");
  }

  append_html_escaped(url);
  append("\">");
  append(parsed_title);
  append("</a>");
}

void StateMachine::append_wiki_link(const std::string_view tag, const std::string_view title) {
  std::string normalized_tag = std::string(tag);
  std::transform(normalized_tag.begin(), normalized_tag.end(), normalized_tag.begin(), [](unsigned char c) { return c == ' ' ? '_' : std::tolower(c); });

  // FIXME: Take the anchor as an argument here
  if (tag[0] == '#') {
    append("<a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"#");
    append_uri_escaped(normalized_tag.substr(1, normalized_tag.size() - 1));
    append("\">");
  } else {
    append("<a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"");
    append_url("/wiki_pages/show_or_new?title=");
    append_uri_escaped(normalized_tag, '#');
    append("\">");
  }
  append_html_escaped(title);
  append("</a>");
}

void StateMachine::append_post_search_link(const std::string_view tag, const std::string_view title) {
  std::string normalized_tag = std::string(tag);
  std::transform(normalized_tag.begin(), normalized_tag.end(), normalized_tag.begin(), [](unsigned char c) { return std::tolower(c); });

  append("<a rel=\"nofollow\" class=\"dtext-link dtext-post-search-link\" href=\"");
  append_url("/posts?tags=");
  append_uri_escaped(normalized_tag);
  append("\">");
  append_html_escaped(title);
  append("</a>");
}

void StateMachine::append_section(const std::string_view summary, bool initially_open) {
  dstack_close_leaf_blocks();
  dstack_open_block(BLOCK_SECTION, "<details");
  if (initially_open) {
     append_block(" open");
  }
  append_block(">");
  append_block("<summary>");
  if (!summary.empty()) {
    append_html_escaped(summary);
  }
  append_block("</summary><div>");
}

void StateMachine::append_closing_p() {
  if (output.size() > 4 && output.ends_with("<br>")) {
    output.resize(output.size() - 4);
  }

  if (output.size() > 3 && output.ends_with("<p>")) {
    output.resize(output.size() - 3);
    return;
  }

  append_block("</p>");
}

void StateMachine::dstack_open_inline(element_t type, const char * html) {
  g_debug("push inline element [%d]: %s", type, html);

  dstack_push(type);
  append(html);
}

void StateMachine::dstack_open_block(element_t type, const char * html) {
  g_debug("push block element [%d]: %s", type, html);

  dstack_push(type);
  append_block(html);
}

void StateMachine::dstack_close_inline(element_t type, const char * close_html) {
  if (dstack_check(type)) {
    g_debug("pop inline element [%d]: %s", type, close_html);

    dstack_pop();
    append(close_html);
  } else {
    g_debug("ignored out-of-order closing inline tag [%d]", type);

    append({ ts, te });
  }
}

bool StateMachine::dstack_close_block(element_t type, const char * close_html) {
  if (dstack_check(type)) {
    g_debug("pop block element [%d]: %s", type, close_html);

    dstack_pop();
    append_block(close_html);
    return true;
  } else {
    g_debug("ignored out-of-order closing block tag [%d]", type);

    append_block({ ts, te });
    return false;
  }
}

// Close the last open tag.
void StateMachine::dstack_rewind() {
  element_t element = dstack_pop();

  switch(element) {
    case BLOCK_P: append_closing_p(); break;
    case INLINE_SPOILER: append("</span>"); break;
    case BLOCK_SPOILER: append_block("</div>"); break;
    case BLOCK_QUOTE: append_block("</blockquote>"); break;
    case BLOCK_SECTION: append_block("</div></details>"); break;
    case BLOCK_CODE: append_block("</pre>"); break;
    case BLOCK_TD: append_block("</td>"); break;
    case BLOCK_TH: append_block("</th>"); break;

    case INLINE_B: append("</strong>"); break;
    case INLINE_I: append("</em>"); break;
    case INLINE_U: append("</u>"); break;
    case INLINE_S: append("</s>"); break;
    case INLINE_SUB: append("</sub>"); break;
    case INLINE_SUP: append("</sup>"); break;
    case INLINE_COLOR: append("</span>"); break;
    case INLINE_CODE: append("</span>"); break;

    case BLOCK_TABLE: append_block("</table>"); break;
    case BLOCK_THEAD: append_block("</thead>"); break;
    case BLOCK_TBODY: append_block("</tbody>"); break;
    case BLOCK_TR: append_block("</tr>"); header_mode = false; break;
    case BLOCK_UL: append_block("</ul>"); header_mode = false; break;
    case BLOCK_LI: append_block("</li>"); header_mode = false; break;
    case BLOCK_H6: append_block("</h6>"); header_mode = false; break;
    case BLOCK_H5: append_block("</h5>"); header_mode = false; break;
    case BLOCK_H4: append_block("</h4>"); header_mode = false; break;
    case BLOCK_H3: append_block("</h3>"); header_mode = false; break;
    case BLOCK_H2: append_block("</h2>"); header_mode = false; break;
    case BLOCK_H1: append_block("</h1>"); header_mode = false; break;

    case DSTACK_EMPTY: break;
  }
}

// Close the last open paragraph or list, if there is one.
void StateMachine::dstack_close_before_block() {
  while (dstack_check(BLOCK_P) || dstack_check(BLOCK_LI) || dstack_check(BLOCK_UL)) {
    dstack_rewind();
  }
}

// Close all remaining open tags.
void StateMachine::dstack_close_all() {
  while (!dstack.empty()) {
    dstack_rewind();
  }
}

// container blocks: [quote], [spoiler], [section]
// leaf blocks: [code], [table], [td]?, [th]?, <h1>, <p>, <li>, <ul>
void StateMachine::dstack_close_leaf_blocks() {
  g_debug("dstack close leaf blocks");

  while (!dstack.empty() && !dstack_check(BLOCK_QUOTE) && !dstack_check(BLOCK_SPOILER) && !dstack_check(BLOCK_SECTION)) {
    dstack_rewind();
  }
}

// Close all open tags up to and including the given tag.
void StateMachine::dstack_close_until(element_t element) {
  while (!dstack.empty() && !dstack_check(element)) {
    dstack_rewind();
  }

  dstack_rewind();
}

void StateMachine::dstack_open_list(int depth) {
  g_debug("open list");

  if (dstack_is_open(BLOCK_LI)) {
    dstack_close_until(BLOCK_LI);
  } else {
    dstack_close_leaf_blocks();
  }

  while (dstack_count(BLOCK_UL) < depth) {
    dstack_open_block(BLOCK_UL, "<ul>");
  }

  while (dstack_count(BLOCK_UL) > depth) {
    dstack_close_until( BLOCK_UL);
  }

  dstack_open_block(BLOCK_LI, "<li>");
}

void StateMachine::dstack_close_list() {
  while (dstack_is_open(BLOCK_UL)) {
    dstack_close_until(BLOCK_UL);
  }
}

static inline std::tuple<char32_t, int> get_utf8_char(const char* c) {
  const unsigned char* p = reinterpret_cast<const unsigned char*>(c);

  // 0x10xxxxxx is a continuation byte; back up to the leading byte.
  while ((p[0] >> 6) == 0b10) {
    p--;
  }

  if (p[0] >> 7 == 0) {
    // 0x0xxxxxxx
    return { p[0], 1 };
  } else if ((p[0] >> 5) == 0b110) {
    // 0x110xxxxx, 0x10xxxxxx
    return { ((p[0] & 0b00011111) << 6) | (p[1] & 0b00111111), 2 };
  } else if ((p[0] >> 4) == 0b1110) {
    // 0x1110xxxx, 0x10xxxxxx, 0x10xxxxxx
    return { ((p[0] & 0b00001111) << 12) | (p[1] & 0b00111111) << 6 | (p[2] & 0b00111111), 3 };
  } else if ((p[0] >> 3) == 0b11110) {
    // 0x11110xxx, 0x10xxxxxx, 0x10xxxxxx, 0x10xxxxxx
    return { ((p[0] & 0b00000111) << 18) | (p[1] & 0b00111111) << 12 | (p[2] & 0b00111111) << 6 | (p[3] & 0b00111111), 4 };
  } else {
    return { 0, 0 };
  }
}

// Returns the preceding non-boundary character if `c` is a boundary character.
// Otherwise, returns `c` if `c` is not a boundary character. Boundary characters
// are trailing punctuation characters that should not be part of the matched text.
static inline const char* find_boundary_c(const char* c) {
  auto [ch, len] = get_utf8_char(c);

  if (std::binary_search(std::begin(boundary_characters), std::end(boundary_characters), ch)) {
    return c - len;
  } else {
    return c;
  }
}

StateMachine::StateMachine(const std::string_view dtext, int initial_state, const DTextOptions options) : options(options) {
  output.reserve(dtext.size() * 1.5);
  stack.reserve(16);
  dstack.reserve(16);
  posts.reserve(10);

  p = dtext.data();
  pb = p;
  pe = p + dtext.size();
  eof = pe;
  cs = initial_state;
}

std::string StateMachine::parse_basic_inline(const std::string_view dtext) {
    DTextOptions options = {};
    options.f_inline = true;
    options.allow_color = false;
    options.max_thumbs = 0;

    StateMachine sm(dtext, dtext_en_basic_inline, options);

    return sm.parse().dtext;
}

DTextResult StateMachine::parse_dtext(const std::string_view dtext, DTextOptions options) {
  StateMachine sm(dtext, dtext_en_main, options);
  return sm.parse();
}

DTextResult StateMachine::parse() {
  StateMachine* sm = this;
  g_debug("start\n");

  
#line 479 "ext/dtext/dtext.cpp"
	{
	( sm->top) = 0;
	( sm->ts) = 0;
	( sm->te) = 0;
	( sm->act) = 0;
	}

#line 1158 "ext/dtext/dtext.cpp.rl"
  
#line 485 "ext/dtext/dtext.cpp"
	{
	short _widec;
	if ( ( sm->p) == ( sm->pe) )
		goto _test_eof;
	goto _resume;

_again:
	switch (  sm->cs ) {
		case 735: goto st735;
		case 736: goto st736;
		case 0: goto st0;
		case 737: goto st737;
		case 738: goto st738;
		case 1: goto st1;
		case 739: goto st739;
		case 740: goto st740;
		case 2: goto st2;
		case 741: goto st741;
		case 3: goto st3;
		case 742: goto st742;
		case 743: goto st743;
		case 4: goto st4;
		case 5: goto st5;
		case 6: goto st6;
		case 7: goto st7;
		case 8: goto st8;
		case 9: goto st9;
		case 10: goto st10;
		case 11: goto st11;
		case 12: goto st12;
		case 13: goto st13;
		case 14: goto st14;
		case 15: goto st15;
		case 16: goto st16;
		case 744: goto st744;
		case 17: goto st17;
		case 18: goto st18;
		case 19: goto st19;
		case 20: goto st20;
		case 21: goto st21;
		case 22: goto st22;
		case 23: goto st23;
		case 24: goto st24;
		case 25: goto st25;
		case 26: goto st26;
		case 27: goto st27;
		case 28: goto st28;
		case 29: goto st29;
		case 30: goto st30;
		case 31: goto st31;
		case 32: goto st32;
		case 33: goto st33;
		case 34: goto st34;
		case 35: goto st35;
		case 36: goto st36;
		case 37: goto st37;
		case 38: goto st38;
		case 39: goto st39;
		case 40: goto st40;
		case 41: goto st41;
		case 42: goto st42;
		case 43: goto st43;
		case 44: goto st44;
		case 45: goto st45;
		case 46: goto st46;
		case 47: goto st47;
		case 48: goto st48;
		case 49: goto st49;
		case 50: goto st50;
		case 51: goto st51;
		case 52: goto st52;
		case 53: goto st53;
		case 54: goto st54;
		case 55: goto st55;
		case 56: goto st56;
		case 57: goto st57;
		case 58: goto st58;
		case 59: goto st59;
		case 60: goto st60;
		case 61: goto st61;
		case 62: goto st62;
		case 63: goto st63;
		case 64: goto st64;
		case 65: goto st65;
		case 66: goto st66;
		case 67: goto st67;
		case 68: goto st68;
		case 69: goto st69;
		case 70: goto st70;
		case 71: goto st71;
		case 72: goto st72;
		case 73: goto st73;
		case 74: goto st74;
		case 75: goto st75;
		case 76: goto st76;
		case 77: goto st77;
		case 78: goto st78;
		case 79: goto st79;
		case 80: goto st80;
		case 81: goto st81;
		case 82: goto st82;
		case 83: goto st83;
		case 84: goto st84;
		case 85: goto st85;
		case 86: goto st86;
		case 87: goto st87;
		case 88: goto st88;
		case 89: goto st89;
		case 90: goto st90;
		case 91: goto st91;
		case 92: goto st92;
		case 93: goto st93;
		case 94: goto st94;
		case 95: goto st95;
		case 96: goto st96;
		case 97: goto st97;
		case 98: goto st98;
		case 99: goto st99;
		case 100: goto st100;
		case 101: goto st101;
		case 102: goto st102;
		case 103: goto st103;
		case 104: goto st104;
		case 105: goto st105;
		case 106: goto st106;
		case 107: goto st107;
		case 108: goto st108;
		case 109: goto st109;
		case 110: goto st110;
		case 111: goto st111;
		case 112: goto st112;
		case 113: goto st113;
		case 114: goto st114;
		case 115: goto st115;
		case 116: goto st116;
		case 117: goto st117;
		case 118: goto st118;
		case 119: goto st119;
		case 120: goto st120;
		case 121: goto st121;
		case 122: goto st122;
		case 123: goto st123;
		case 124: goto st124;
		case 125: goto st125;
		case 126: goto st126;
		case 127: goto st127;
		case 128: goto st128;
		case 129: goto st129;
		case 130: goto st130;
		case 131: goto st131;
		case 132: goto st132;
		case 745: goto st745;
		case 133: goto st133;
		case 134: goto st134;
		case 135: goto st135;
		case 136: goto st136;
		case 137: goto st137;
		case 138: goto st138;
		case 139: goto st139;
		case 140: goto st140;
		case 141: goto st141;
		case 142: goto st142;
		case 143: goto st143;
		case 144: goto st144;
		case 145: goto st145;
		case 146: goto st146;
		case 147: goto st147;
		case 148: goto st148;
		case 149: goto st149;
		case 150: goto st150;
		case 746: goto st746;
		case 747: goto st747;
		case 151: goto st151;
		case 152: goto st152;
		case 748: goto st748;
		case 749: goto st749;
		case 153: goto st153;
		case 154: goto st154;
		case 155: goto st155;
		case 156: goto st156;
		case 157: goto st157;
		case 158: goto st158;
		case 159: goto st159;
		case 750: goto st750;
		case 160: goto st160;
		case 161: goto st161;
		case 162: goto st162;
		case 163: goto st163;
		case 164: goto st164;
		case 751: goto st751;
		case 752: goto st752;
		case 165: goto st165;
		case 166: goto st166;
		case 167: goto st167;
		case 168: goto st168;
		case 169: goto st169;
		case 170: goto st170;
		case 171: goto st171;
		case 172: goto st172;
		case 173: goto st173;
		case 174: goto st174;
		case 175: goto st175;
		case 176: goto st176;
		case 177: goto st177;
		case 178: goto st178;
		case 179: goto st179;
		case 753: goto st753;
		case 754: goto st754;
		case 755: goto st755;
		case 180: goto st180;
		case 181: goto st181;
		case 182: goto st182;
		case 183: goto st183;
		case 184: goto st184;
		case 185: goto st185;
		case 186: goto st186;
		case 187: goto st187;
		case 188: goto st188;
		case 189: goto st189;
		case 190: goto st190;
		case 191: goto st191;
		case 192: goto st192;
		case 193: goto st193;
		case 194: goto st194;
		case 756: goto st756;
		case 757: goto st757;
		case 195: goto st195;
		case 196: goto st196;
		case 758: goto st758;
		case 197: goto st197;
		case 198: goto st198;
		case 199: goto st199;
		case 200: goto st200;
		case 201: goto st201;
		case 202: goto st202;
		case 203: goto st203;
		case 759: goto st759;
		case 204: goto st204;
		case 205: goto st205;
		case 206: goto st206;
		case 207: goto st207;
		case 208: goto st208;
		case 209: goto st209;
		case 210: goto st210;
		case 760: goto st760;
		case 761: goto st761;
		case 762: goto st762;
		case 211: goto st211;
		case 212: goto st212;
		case 213: goto st213;
		case 214: goto st214;
		case 763: goto st763;
		case 215: goto st215;
		case 216: goto st216;
		case 217: goto st217;
		case 218: goto st218;
		case 219: goto st219;
		case 220: goto st220;
		case 221: goto st221;
		case 222: goto st222;
		case 223: goto st223;
		case 224: goto st224;
		case 225: goto st225;
		case 226: goto st226;
		case 227: goto st227;
		case 228: goto st228;
		case 229: goto st229;
		case 230: goto st230;
		case 231: goto st231;
		case 764: goto st764;
		case 232: goto st232;
		case 233: goto st233;
		case 234: goto st234;
		case 235: goto st235;
		case 236: goto st236;
		case 237: goto st237;
		case 238: goto st238;
		case 239: goto st239;
		case 240: goto st240;
		case 765: goto st765;
		case 241: goto st241;
		case 242: goto st242;
		case 243: goto st243;
		case 244: goto st244;
		case 245: goto st245;
		case 246: goto st246;
		case 766: goto st766;
		case 247: goto st247;
		case 248: goto st248;
		case 249: goto st249;
		case 250: goto st250;
		case 251: goto st251;
		case 252: goto st252;
		case 253: goto st253;
		case 767: goto st767;
		case 768: goto st768;
		case 254: goto st254;
		case 255: goto st255;
		case 256: goto st256;
		case 257: goto st257;
		case 769: goto st769;
		case 258: goto st258;
		case 259: goto st259;
		case 260: goto st260;
		case 261: goto st261;
		case 262: goto st262;
		case 770: goto st770;
		case 263: goto st263;
		case 264: goto st264;
		case 265: goto st265;
		case 266: goto st266;
		case 771: goto st771;
		case 772: goto st772;
		case 267: goto st267;
		case 268: goto st268;
		case 269: goto st269;
		case 270: goto st270;
		case 271: goto st271;
		case 272: goto st272;
		case 273: goto st273;
		case 274: goto st274;
		case 773: goto st773;
		case 774: goto st774;
		case 275: goto st275;
		case 276: goto st276;
		case 277: goto st277;
		case 278: goto st278;
		case 279: goto st279;
		case 775: goto st775;
		case 280: goto st280;
		case 281: goto st281;
		case 282: goto st282;
		case 283: goto st283;
		case 284: goto st284;
		case 285: goto st285;
		case 776: goto st776;
		case 777: goto st777;
		case 286: goto st286;
		case 287: goto st287;
		case 288: goto st288;
		case 289: goto st289;
		case 290: goto st290;
		case 291: goto st291;
		case 778: goto st778;
		case 292: goto st292;
		case 779: goto st779;
		case 293: goto st293;
		case 294: goto st294;
		case 295: goto st295;
		case 296: goto st296;
		case 297: goto st297;
		case 298: goto st298;
		case 299: goto st299;
		case 300: goto st300;
		case 301: goto st301;
		case 302: goto st302;
		case 303: goto st303;
		case 304: goto st304;
		case 780: goto st780;
		case 781: goto st781;
		case 305: goto st305;
		case 306: goto st306;
		case 307: goto st307;
		case 308: goto st308;
		case 309: goto st309;
		case 310: goto st310;
		case 311: goto st311;
		case 312: goto st312;
		case 313: goto st313;
		case 314: goto st314;
		case 315: goto st315;
		case 782: goto st782;
		case 783: goto st783;
		case 316: goto st316;
		case 317: goto st317;
		case 318: goto st318;
		case 319: goto st319;
		case 320: goto st320;
		case 784: goto st784;
		case 785: goto st785;
		case 321: goto st321;
		case 322: goto st322;
		case 323: goto st323;
		case 324: goto st324;
		case 325: goto st325;
		case 786: goto st786;
		case 326: goto st326;
		case 327: goto st327;
		case 328: goto st328;
		case 329: goto st329;
		case 787: goto st787;
		case 330: goto st330;
		case 331: goto st331;
		case 332: goto st332;
		case 333: goto st333;
		case 334: goto st334;
		case 335: goto st335;
		case 336: goto st336;
		case 337: goto st337;
		case 338: goto st338;
		case 788: goto st788;
		case 789: goto st789;
		case 339: goto st339;
		case 340: goto st340;
		case 341: goto st341;
		case 342: goto st342;
		case 343: goto st343;
		case 344: goto st344;
		case 345: goto st345;
		case 790: goto st790;
		case 791: goto st791;
		case 346: goto st346;
		case 347: goto st347;
		case 348: goto st348;
		case 349: goto st349;
		case 792: goto st792;
		case 793: goto st793;
		case 350: goto st350;
		case 351: goto st351;
		case 352: goto st352;
		case 353: goto st353;
		case 354: goto st354;
		case 355: goto st355;
		case 356: goto st356;
		case 357: goto st357;
		case 358: goto st358;
		case 359: goto st359;
		case 794: goto st794;
		case 360: goto st360;
		case 361: goto st361;
		case 362: goto st362;
		case 363: goto st363;
		case 364: goto st364;
		case 365: goto st365;
		case 366: goto st366;
		case 367: goto st367;
		case 368: goto st368;
		case 369: goto st369;
		case 370: goto st370;
		case 371: goto st371;
		case 372: goto st372;
		case 373: goto st373;
		case 795: goto st795;
		case 374: goto st374;
		case 375: goto st375;
		case 376: goto st376;
		case 377: goto st377;
		case 378: goto st378;
		case 379: goto st379;
		case 380: goto st380;
		case 796: goto st796;
		case 381: goto st381;
		case 382: goto st382;
		case 383: goto st383;
		case 384: goto st384;
		case 385: goto st385;
		case 386: goto st386;
		case 797: goto st797;
		case 798: goto st798;
		case 387: goto st387;
		case 388: goto st388;
		case 389: goto st389;
		case 390: goto st390;
		case 391: goto st391;
		case 799: goto st799;
		case 800: goto st800;
		case 392: goto st392;
		case 393: goto st393;
		case 394: goto st394;
		case 395: goto st395;
		case 396: goto st396;
		case 801: goto st801;
		case 802: goto st802;
		case 397: goto st397;
		case 398: goto st398;
		case 399: goto st399;
		case 400: goto st400;
		case 401: goto st401;
		case 402: goto st402;
		case 403: goto st403;
		case 404: goto st404;
		case 803: goto st803;
		case 405: goto st405;
		case 406: goto st406;
		case 407: goto st407;
		case 408: goto st408;
		case 409: goto st409;
		case 410: goto st410;
		case 411: goto st411;
		case 412: goto st412;
		case 413: goto st413;
		case 414: goto st414;
		case 415: goto st415;
		case 416: goto st416;
		case 417: goto st417;
		case 804: goto st804;
		case 418: goto st418;
		case 419: goto st419;
		case 420: goto st420;
		case 421: goto st421;
		case 422: goto st422;
		case 423: goto st423;
		case 424: goto st424;
		case 425: goto st425;
		case 426: goto st426;
		case 427: goto st427;
		case 428: goto st428;
		case 429: goto st429;
		case 430: goto st430;
		case 431: goto st431;
		case 432: goto st432;
		case 433: goto st433;
		case 434: goto st434;
		case 435: goto st435;
		case 436: goto st436;
		case 437: goto st437;
		case 438: goto st438;
		case 439: goto st439;
		case 440: goto st440;
		case 441: goto st441;
		case 442: goto st442;
		case 443: goto st443;
		case 444: goto st444;
		case 445: goto st445;
		case 446: goto st446;
		case 447: goto st447;
		case 448: goto st448;
		case 449: goto st449;
		case 450: goto st450;
		case 451: goto st451;
		case 452: goto st452;
		case 453: goto st453;
		case 454: goto st454;
		case 455: goto st455;
		case 456: goto st456;
		case 457: goto st457;
		case 458: goto st458;
		case 459: goto st459;
		case 460: goto st460;
		case 461: goto st461;
		case 462: goto st462;
		case 463: goto st463;
		case 464: goto st464;
		case 465: goto st465;
		case 466: goto st466;
		case 467: goto st467;
		case 468: goto st468;
		case 469: goto st469;
		case 470: goto st470;
		case 471: goto st471;
		case 472: goto st472;
		case 473: goto st473;
		case 474: goto st474;
		case 475: goto st475;
		case 476: goto st476;
		case 477: goto st477;
		case 478: goto st478;
		case 479: goto st479;
		case 480: goto st480;
		case 481: goto st481;
		case 482: goto st482;
		case 483: goto st483;
		case 484: goto st484;
		case 485: goto st485;
		case 486: goto st486;
		case 487: goto st487;
		case 488: goto st488;
		case 489: goto st489;
		case 490: goto st490;
		case 491: goto st491;
		case 492: goto st492;
		case 493: goto st493;
		case 494: goto st494;
		case 495: goto st495;
		case 496: goto st496;
		case 497: goto st497;
		case 498: goto st498;
		case 499: goto st499;
		case 500: goto st500;
		case 501: goto st501;
		case 502: goto st502;
		case 503: goto st503;
		case 504: goto st504;
		case 505: goto st505;
		case 506: goto st506;
		case 507: goto st507;
		case 508: goto st508;
		case 509: goto st509;
		case 510: goto st510;
		case 511: goto st511;
		case 512: goto st512;
		case 513: goto st513;
		case 514: goto st514;
		case 515: goto st515;
		case 516: goto st516;
		case 517: goto st517;
		case 518: goto st518;
		case 519: goto st519;
		case 520: goto st520;
		case 521: goto st521;
		case 522: goto st522;
		case 523: goto st523;
		case 524: goto st524;
		case 525: goto st525;
		case 526: goto st526;
		case 527: goto st527;
		case 528: goto st528;
		case 529: goto st529;
		case 530: goto st530;
		case 531: goto st531;
		case 532: goto st532;
		case 533: goto st533;
		case 534: goto st534;
		case 535: goto st535;
		case 536: goto st536;
		case 537: goto st537;
		case 538: goto st538;
		case 539: goto st539;
		case 540: goto st540;
		case 541: goto st541;
		case 542: goto st542;
		case 543: goto st543;
		case 544: goto st544;
		case 545: goto st545;
		case 546: goto st546;
		case 547: goto st547;
		case 548: goto st548;
		case 549: goto st549;
		case 550: goto st550;
		case 551: goto st551;
		case 552: goto st552;
		case 553: goto st553;
		case 554: goto st554;
		case 555: goto st555;
		case 556: goto st556;
		case 557: goto st557;
		case 558: goto st558;
		case 559: goto st559;
		case 560: goto st560;
		case 561: goto st561;
		case 562: goto st562;
		case 563: goto st563;
		case 564: goto st564;
		case 565: goto st565;
		case 566: goto st566;
		case 567: goto st567;
		case 568: goto st568;
		case 569: goto st569;
		case 570: goto st570;
		case 571: goto st571;
		case 572: goto st572;
		case 573: goto st573;
		case 574: goto st574;
		case 575: goto st575;
		case 576: goto st576;
		case 577: goto st577;
		case 578: goto st578;
		case 579: goto st579;
		case 580: goto st580;
		case 581: goto st581;
		case 582: goto st582;
		case 583: goto st583;
		case 584: goto st584;
		case 585: goto st585;
		case 586: goto st586;
		case 587: goto st587;
		case 588: goto st588;
		case 589: goto st589;
		case 590: goto st590;
		case 591: goto st591;
		case 592: goto st592;
		case 593: goto st593;
		case 594: goto st594;
		case 595: goto st595;
		case 596: goto st596;
		case 597: goto st597;
		case 598: goto st598;
		case 599: goto st599;
		case 600: goto st600;
		case 601: goto st601;
		case 602: goto st602;
		case 603: goto st603;
		case 604: goto st604;
		case 605: goto st605;
		case 606: goto st606;
		case 607: goto st607;
		case 608: goto st608;
		case 609: goto st609;
		case 610: goto st610;
		case 611: goto st611;
		case 612: goto st612;
		case 613: goto st613;
		case 614: goto st614;
		case 615: goto st615;
		case 616: goto st616;
		case 617: goto st617;
		case 618: goto st618;
		case 619: goto st619;
		case 620: goto st620;
		case 621: goto st621;
		case 622: goto st622;
		case 623: goto st623;
		case 624: goto st624;
		case 625: goto st625;
		case 626: goto st626;
		case 627: goto st627;
		case 628: goto st628;
		case 629: goto st629;
		case 630: goto st630;
		case 631: goto st631;
		case 632: goto st632;
		case 633: goto st633;
		case 634: goto st634;
		case 635: goto st635;
		case 636: goto st636;
		case 637: goto st637;
		case 638: goto st638;
		case 639: goto st639;
		case 640: goto st640;
		case 641: goto st641;
		case 642: goto st642;
		case 643: goto st643;
		case 644: goto st644;
		case 645: goto st645;
		case 646: goto st646;
		case 647: goto st647;
		case 648: goto st648;
		case 649: goto st649;
		case 650: goto st650;
		case 651: goto st651;
		case 652: goto st652;
		case 653: goto st653;
		case 654: goto st654;
		case 655: goto st655;
		case 656: goto st656;
		case 657: goto st657;
		case 658: goto st658;
		case 659: goto st659;
		case 660: goto st660;
		case 661: goto st661;
		case 662: goto st662;
		case 663: goto st663;
		case 664: goto st664;
		case 665: goto st665;
		case 666: goto st666;
		case 667: goto st667;
		case 668: goto st668;
		case 669: goto st669;
		case 670: goto st670;
		case 671: goto st671;
		case 672: goto st672;
		case 673: goto st673;
		case 674: goto st674;
		case 675: goto st675;
		case 676: goto st676;
		case 677: goto st677;
		case 678: goto st678;
		case 679: goto st679;
		case 680: goto st680;
		case 681: goto st681;
		case 682: goto st682;
		case 683: goto st683;
		case 684: goto st684;
		case 685: goto st685;
		case 686: goto st686;
		case 687: goto st687;
		case 688: goto st688;
		case 689: goto st689;
		case 690: goto st690;
		case 691: goto st691;
		case 692: goto st692;
		case 693: goto st693;
		case 694: goto st694;
		case 695: goto st695;
		case 696: goto st696;
		case 805: goto st805;
		case 806: goto st806;
		case 697: goto st697;
		case 698: goto st698;
		case 699: goto st699;
		case 700: goto st700;
		case 701: goto st701;
		case 702: goto st702;
		case 703: goto st703;
		case 807: goto st807;
		case 808: goto st808;
		case 809: goto st809;
		case 810: goto st810;
		case 704: goto st704;
		case 705: goto st705;
		case 706: goto st706;
		case 707: goto st707;
		case 708: goto st708;
		case 811: goto st811;
		case 812: goto st812;
		case 709: goto st709;
		case 710: goto st710;
		case 711: goto st711;
		case 712: goto st712;
		case 713: goto st713;
		case 714: goto st714;
		case 715: goto st715;
		case 716: goto st716;
		case 717: goto st717;
		case 718: goto st718;
		case 719: goto st719;
		case 720: goto st720;
		case 721: goto st721;
		case 722: goto st722;
		case 723: goto st723;
		case 724: goto st724;
		case 725: goto st725;
		case 726: goto st726;
		case 727: goto st727;
		case 728: goto st728;
		case 729: goto st729;
		case 730: goto st730;
		case 731: goto st731;
		case 732: goto st732;
		case 733: goto st733;
		case 734: goto st734;
	default: break;
	}

	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof;
_resume:
	switch (  sm->cs )
	{
tr0:
#line 1 "NONE"
	{	switch( ( sm->act) ) {
	case 110:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("block newline2");

    if (header_mode) {
      dstack_close_leaf_blocks();
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_until(BLOCK_UL);
    } else {
      dstack_close_before_block();
    }
  }
	break;
	case 111:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("block newline");
  }
	break;
	}
	}
	goto st735;
tr2:
#line 742 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("block char: %c", (*( sm->p)));
    ( sm->p)--;

    if (dstack.empty() || dstack_check(BLOCK_QUOTE) || dstack_check(BLOCK_SPOILER) || dstack_check(BLOCK_SECTION)) {
      dstack_open_block(BLOCK_P, "<p>");
    }

    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 735;goto st753;}}
  }}
	goto st735;
tr16:
#line 680 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("block [/spoiler]");
    dstack_close_before_block();
    if (dstack_check( BLOCK_SPOILER)) {
      g_debug("  rewind");
      dstack_rewind();
    }
  }}
	goto st735;
tr47:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 663 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote class=\"dtext-quote-color\" style=\"border-left-color:");
    if(a1[0] == '#') {
      append("#");
      append_uri_escaped({ a1 + 1, a2 });
    } else {
      append_uri_escaped({ a1, a2 });
    }
    append("\">");
  }}
	goto st735;
tr53:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 656 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote class=\"dtext-sidebar-colored-");
    append_uri_escaped({ a1, a2 });
    append("\">");
  }}
	goto st735;
tr177:
#line 713 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_TABLE, "<table class=\"striped\">");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 735;goto st811;}}
  }}
	goto st735;
tr809:
#line 742 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("block char: %c", (*( sm->p)));
    ( sm->p)--;

    if (dstack.empty() || dstack_check(BLOCK_QUOTE) || dstack_check(BLOCK_SPOILER) || dstack_check(BLOCK_SECTION)) {
      dstack_open_block(BLOCK_P, "<p>");
    }

    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 735;goto st753;}}
  }}
	goto st735;
tr816:
#line 742 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("block char: %c", (*( sm->p)));
    ( sm->p)--;

    if (dstack.empty() || dstack_check(BLOCK_QUOTE) || dstack_check(BLOCK_SPOILER) || dstack_check(BLOCK_SECTION)) {
      dstack_open_block(BLOCK_P, "<p>");
    }

    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 735;goto st753;}}
  }}
	goto st735;
tr817:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 719 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("block list");
    dstack_open_list(a2 - a1);
    {( sm->p) = (( b1))-1;}
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 735;goto st753;}}
  }}
	goto st735;
tr820:
#line 638 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    static element_t blocks[] = { BLOCK_H1, BLOCK_H2, BLOCK_H3, BLOCK_H4, BLOCK_H5, BLOCK_H6 };
    char header = *a1;
    element_t block = blocks[header - '1'];

    dstack_open_block(block, "<h");
    append_block(header);
    append_block(">");

    header_mode = true;
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 735;goto st753;}}
  }}
	goto st735;
tr827:
#line 689 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_CODE, "<pre>");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 735;goto st809;}}
  }}
	goto st735;
tr828:
#line 651 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote>");
  }}
	goto st735;
tr829:
#line 708 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("block expanded [section=]");
    append_section({ a1, a2 }, true);
  }}
	goto st735;
tr831:
#line 699 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_section({}, true);
  }}
	goto st735;
tr832:
#line 703 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("block [section=]");
    append_section({ a1, a2 }, false);
  }}
	goto st735;
tr834:
#line 695 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_section({}, false);
  }}
	goto st735;
tr835:
#line 675 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_SPOILER, "<div class=\"spoiler\">");
  }}
	goto st735;
st735:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof735;
case 735:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 1567 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr810;
		case 13: goto st737;
		case 42: goto tr812;
		case 72: goto tr813;
		case 91: goto tr814;
		case 104: goto tr813;
	}
	goto tr809;
tr1:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 726 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 110;}
	goto st736;
tr810:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 738 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 111;}
	goto st736;
st736:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof736;
case 736:
#line 1588 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr1;
		case 13: goto st0;
	}
	goto tr0;
st0:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof0;
case 0:
	if ( (*( sm->p)) == 10 )
		goto tr1;
	goto tr0;
st737:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof737;
case 737:
	if ( (*( sm->p)) == 10 )
		goto tr810;
	goto tr816;
tr812:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st738;
st738:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof738;
case 738:
#line 1615 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr5;
		case 32: goto tr5;
		case 42: goto st2;
	}
	goto tr816;
tr5:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1;
st1:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1;
case 1:
#line 1628 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr4;
		case 10: goto tr2;
		case 13: goto tr2;
		case 32: goto tr4;
	}
	goto tr3;
tr3:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st739;
st739:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof739;
case 739:
#line 1642 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr817;
		case 13: goto tr817;
	}
	goto st739;
tr4:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st740;
st740:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof740;
case 740:
#line 1654 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr4;
		case 10: goto tr817;
		case 13: goto tr817;
		case 32: goto tr4;
	}
	goto tr3;
st2:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof2;
case 2:
	switch( (*( sm->p)) ) {
		case 9: goto tr5;
		case 32: goto tr5;
		case 42: goto st2;
	}
	goto tr2;
tr813:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st741;
st741:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof741;
case 741:
#line 1678 "ext/dtext/dtext.cpp"
	if ( 49 <= (*( sm->p)) && (*( sm->p)) <= 54 )
		goto tr819;
	goto tr816;
tr819:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st3;
st3:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof3;
case 3:
#line 1688 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 46 )
		goto tr7;
	goto tr2;
tr7:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st742;
st742:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof742;
case 742:
#line 1698 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto st742;
		case 32: goto st742;
	}
	goto tr820;
tr814:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st743;
st743:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof743;
case 743:
#line 1710 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 47: goto st4;
		case 67: goto st13;
		case 81: goto st17;
		case 83: goto st133;
		case 84: goto st160;
		case 99: goto st13;
		case 113: goto st17;
		case 115: goto st133;
		case 116: goto st160;
	}
	goto tr816;
st4:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof4;
case 4:
	switch( (*( sm->p)) ) {
		case 83: goto st5;
		case 115: goto st5;
	}
	goto tr2;
st5:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof5;
case 5:
	switch( (*( sm->p)) ) {
		case 80: goto st6;
		case 112: goto st6;
	}
	goto tr2;
st6:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof6;
case 6:
	switch( (*( sm->p)) ) {
		case 79: goto st7;
		case 111: goto st7;
	}
	goto tr2;
st7:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof7;
case 7:
	switch( (*( sm->p)) ) {
		case 73: goto st8;
		case 105: goto st8;
	}
	goto tr2;
st8:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof8;
case 8:
	switch( (*( sm->p)) ) {
		case 76: goto st9;
		case 108: goto st9;
	}
	goto tr2;
st9:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof9;
case 9:
	switch( (*( sm->p)) ) {
		case 69: goto st10;
		case 101: goto st10;
	}
	goto tr2;
st10:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof10;
case 10:
	switch( (*( sm->p)) ) {
		case 82: goto st11;
		case 114: goto st11;
	}
	goto tr2;
st11:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof11;
case 11:
	switch( (*( sm->p)) ) {
		case 83: goto st12;
		case 93: goto tr16;
		case 115: goto st12;
	}
	goto tr2;
st12:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof12;
case 12:
	if ( (*( sm->p)) == 93 )
		goto tr16;
	goto tr2;
st13:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof13;
case 13:
	switch( (*( sm->p)) ) {
		case 79: goto st14;
		case 111: goto st14;
	}
	goto tr2;
st14:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof14;
case 14:
	switch( (*( sm->p)) ) {
		case 68: goto st15;
		case 100: goto st15;
	}
	goto tr2;
st15:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof15;
case 15:
	switch( (*( sm->p)) ) {
		case 69: goto st16;
		case 101: goto st16;
	}
	goto tr2;
st16:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof16;
case 16:
	if ( (*( sm->p)) == 93 )
		goto st744;
	goto tr2;
st744:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof744;
case 744:
	if ( (*( sm->p)) == 32 )
		goto st744;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st744;
	goto tr827;
st17:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof17;
case 17:
	switch( (*( sm->p)) ) {
		case 85: goto st18;
		case 117: goto st18;
	}
	goto tr2;
st18:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof18;
case 18:
	switch( (*( sm->p)) ) {
		case 79: goto st19;
		case 111: goto st19;
	}
	goto tr2;
st19:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof19;
case 19:
	switch( (*( sm->p)) ) {
		case 84: goto st20;
		case 116: goto st20;
	}
	goto tr2;
st20:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof20;
case 20:
	switch( (*( sm->p)) ) {
		case 69: goto st21;
		case 101: goto st21;
	}
	goto tr2;
st21:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof21;
case 21:
	switch( (*( sm->p)) ) {
		case 61: goto st22;
		case 93: goto st745;
	}
	goto tr2;
st22:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof22;
case 22:
	switch( (*( sm->p)) ) {
		case 35: goto tr27;
		case 65: goto tr28;
		case 67: goto tr29;
		case 71: goto tr30;
		case 73: goto tr31;
		case 76: goto tr32;
		case 77: goto tr33;
		case 83: goto tr34;
		case 97: goto tr35;
		case 99: goto tr37;
		case 103: goto tr38;
		case 105: goto tr39;
		case 108: goto tr40;
		case 109: goto tr41;
		case 115: goto tr42;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto tr36;
	goto tr2;
tr27:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st23;
st23:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof23;
case 23:
#line 1921 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st24;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st24;
	} else
		goto st24;
	goto tr2;
st24:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof24;
case 24:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st25;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st25;
	} else
		goto st25;
	goto tr2;
st25:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof25;
case 25:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st26;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st26;
	} else
		goto st26;
	goto tr2;
st26:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof26;
case 26:
	if ( (*( sm->p)) == 93 )
		goto tr47;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st27;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st27;
	} else
		goto st27;
	goto tr2;
st27:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof27;
case 27:
	if ( (*( sm->p)) == 93 )
		goto tr47;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st28;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st28;
	} else
		goto st28;
	goto tr2;
st28:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof28;
case 28:
	if ( (*( sm->p)) == 93 )
		goto tr47;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st29;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st29;
	} else
		goto st29;
	goto tr2;
st29:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof29;
case 29:
	if ( (*( sm->p)) == 93 )
		goto tr47;
	goto tr2;
tr28:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st30;
st30:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof30;
case 30:
#line 2015 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st31;
		case 114: goto st31;
	}
	goto tr2;
st31:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof31;
case 31:
	switch( (*( sm->p)) ) {
		case 84: goto st32;
		case 116: goto st32;
	}
	goto tr2;
st32:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof32;
case 32:
	switch( (*( sm->p)) ) {
		case 73: goto st33;
		case 93: goto tr53;
		case 105: goto st33;
	}
	goto tr2;
st33:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof33;
case 33:
	switch( (*( sm->p)) ) {
		case 83: goto st34;
		case 115: goto st34;
	}
	goto tr2;
st34:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof34;
case 34:
	switch( (*( sm->p)) ) {
		case 84: goto st35;
		case 116: goto st35;
	}
	goto tr2;
st35:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof35;
case 35:
	if ( (*( sm->p)) == 93 )
		goto tr53;
	goto tr2;
tr29:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st36;
st36:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof36;
case 36:
#line 2071 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st37;
		case 79: goto st44;
		case 104: goto st37;
		case 111: goto st44;
	}
	goto tr2;
st37:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof37;
case 37:
	switch( (*( sm->p)) ) {
		case 65: goto st38;
		case 97: goto st38;
	}
	goto tr2;
st38:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof38;
case 38:
	switch( (*( sm->p)) ) {
		case 82: goto st39;
		case 114: goto st39;
	}
	goto tr2;
st39:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof39;
case 39:
	switch( (*( sm->p)) ) {
		case 65: goto st40;
		case 93: goto tr53;
		case 97: goto st40;
	}
	goto tr2;
st40:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof40;
case 40:
	switch( (*( sm->p)) ) {
		case 67: goto st41;
		case 99: goto st41;
	}
	goto tr2;
st41:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof41;
case 41:
	switch( (*( sm->p)) ) {
		case 84: goto st42;
		case 116: goto st42;
	}
	goto tr2;
st42:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof42;
case 42:
	switch( (*( sm->p)) ) {
		case 69: goto st43;
		case 101: goto st43;
	}
	goto tr2;
st43:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof43;
case 43:
	switch( (*( sm->p)) ) {
		case 82: goto st35;
		case 114: goto st35;
	}
	goto tr2;
st44:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof44;
case 44:
	switch( (*( sm->p)) ) {
		case 78: goto st45;
		case 80: goto st52;
		case 110: goto st45;
		case 112: goto st52;
	}
	goto tr2;
st45:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof45;
case 45:
	switch( (*( sm->p)) ) {
		case 84: goto st46;
		case 116: goto st46;
	}
	goto tr2;
st46:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof46;
case 46:
	switch( (*( sm->p)) ) {
		case 82: goto st47;
		case 93: goto tr53;
		case 114: goto st47;
	}
	goto tr2;
st47:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof47;
case 47:
	switch( (*( sm->p)) ) {
		case 73: goto st48;
		case 105: goto st48;
	}
	goto tr2;
st48:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof48;
case 48:
	switch( (*( sm->p)) ) {
		case 66: goto st49;
		case 98: goto st49;
	}
	goto tr2;
st49:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof49;
case 49:
	switch( (*( sm->p)) ) {
		case 85: goto st50;
		case 117: goto st50;
	}
	goto tr2;
st50:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof50;
case 50:
	switch( (*( sm->p)) ) {
		case 84: goto st51;
		case 116: goto st51;
	}
	goto tr2;
st51:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof51;
case 51:
	switch( (*( sm->p)) ) {
		case 79: goto st43;
		case 111: goto st43;
	}
	goto tr2;
st52:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof52;
case 52:
	switch( (*( sm->p)) ) {
		case 89: goto st53;
		case 121: goto st53;
	}
	goto tr2;
st53:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof53;
case 53:
	switch( (*( sm->p)) ) {
		case 82: goto st54;
		case 93: goto tr53;
		case 114: goto st54;
	}
	goto tr2;
st54:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof54;
case 54:
	switch( (*( sm->p)) ) {
		case 73: goto st55;
		case 105: goto st55;
	}
	goto tr2;
st55:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof55;
case 55:
	switch( (*( sm->p)) ) {
		case 71: goto st56;
		case 103: goto st56;
	}
	goto tr2;
st56:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof56;
case 56:
	switch( (*( sm->p)) ) {
		case 72: goto st34;
		case 104: goto st34;
	}
	goto tr2;
tr30:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st57;
st57:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof57;
case 57:
#line 2270 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st58;
		case 101: goto st58;
	}
	goto tr2;
st58:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof58;
case 58:
	switch( (*( sm->p)) ) {
		case 78: goto st59;
		case 110: goto st59;
	}
	goto tr2;
st59:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof59;
case 59:
	switch( (*( sm->p)) ) {
		case 69: goto st60;
		case 93: goto tr53;
		case 101: goto st60;
	}
	goto tr2;
st60:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof60;
case 60:
	switch( (*( sm->p)) ) {
		case 82: goto st61;
		case 114: goto st61;
	}
	goto tr2;
st61:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof61;
case 61:
	switch( (*( sm->p)) ) {
		case 65: goto st62;
		case 97: goto st62;
	}
	goto tr2;
st62:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof62;
case 62:
	switch( (*( sm->p)) ) {
		case 76: goto st35;
		case 108: goto st35;
	}
	goto tr2;
tr31:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st63;
st63:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof63;
case 63:
#line 2328 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st64;
		case 110: goto st64;
	}
	goto tr2;
st64:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof64;
case 64:
	switch( (*( sm->p)) ) {
		case 86: goto st65;
		case 118: goto st65;
	}
	goto tr2;
st65:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof65;
case 65:
	switch( (*( sm->p)) ) {
		case 65: goto st66;
		case 93: goto tr53;
		case 97: goto st66;
	}
	goto tr2;
st66:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof66;
case 66:
	switch( (*( sm->p)) ) {
		case 76: goto st67;
		case 108: goto st67;
	}
	goto tr2;
st67:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof67;
case 67:
	switch( (*( sm->p)) ) {
		case 73: goto st68;
		case 105: goto st68;
	}
	goto tr2;
st68:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof68;
case 68:
	switch( (*( sm->p)) ) {
		case 68: goto st35;
		case 100: goto st35;
	}
	goto tr2;
tr32:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st69;
st69:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof69;
case 69:
#line 2386 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st70;
		case 111: goto st70;
	}
	goto tr2;
st70:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof70;
case 70:
	switch( (*( sm->p)) ) {
		case 82: goto st71;
		case 114: goto st71;
	}
	goto tr2;
st71:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof71;
case 71:
	switch( (*( sm->p)) ) {
		case 69: goto st35;
		case 93: goto tr53;
		case 101: goto st35;
	}
	goto tr2;
tr33:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st72;
st72:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof72;
case 72:
#line 2417 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st73;
		case 101: goto st73;
	}
	goto tr2;
st73:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof73;
case 73:
	switch( (*( sm->p)) ) {
		case 84: goto st74;
		case 116: goto st74;
	}
	goto tr2;
st74:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof74;
case 74:
	switch( (*( sm->p)) ) {
		case 65: goto st35;
		case 97: goto st35;
	}
	goto tr2;
tr34:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st75;
st75:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof75;
case 75:
#line 2447 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st76;
		case 112: goto st76;
	}
	goto tr2;
st76:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof76;
case 76:
	switch( (*( sm->p)) ) {
		case 69: goto st77;
		case 101: goto st77;
	}
	goto tr2;
st77:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof77;
case 77:
	switch( (*( sm->p)) ) {
		case 67: goto st78;
		case 99: goto st78;
	}
	goto tr2;
st78:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof78;
case 78:
	switch( (*( sm->p)) ) {
		case 73: goto st79;
		case 93: goto tr53;
		case 105: goto st79;
	}
	goto tr2;
st79:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof79;
case 79:
	switch( (*( sm->p)) ) {
		case 69: goto st80;
		case 101: goto st80;
	}
	goto tr2;
st80:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof80;
case 80:
	switch( (*( sm->p)) ) {
		case 83: goto st35;
		case 115: goto st35;
	}
	goto tr2;
tr35:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st81;
st81:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof81;
case 81:
#line 2505 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st31;
		case 93: goto tr47;
		case 114: goto st83;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr36:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st82;
st82:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof82;
case 82:
#line 2520 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr47;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st83:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof83;
case 83:
	switch( (*( sm->p)) ) {
		case 84: goto st32;
		case 93: goto tr47;
		case 116: goto st84;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st84:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof84;
case 84:
	switch( (*( sm->p)) ) {
		case 73: goto st33;
		case 93: goto tr53;
		case 105: goto st85;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st85:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof85;
case 85:
	switch( (*( sm->p)) ) {
		case 83: goto st34;
		case 93: goto tr47;
		case 115: goto st86;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st86:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof86;
case 86:
	switch( (*( sm->p)) ) {
		case 84: goto st35;
		case 93: goto tr47;
		case 116: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st87:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof87;
case 87:
	if ( (*( sm->p)) == 93 )
		goto tr53;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr37:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st88;
st88:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof88;
case 88:
#line 2589 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st37;
		case 79: goto st44;
		case 93: goto tr47;
		case 104: goto st89;
		case 111: goto st96;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st89:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof89;
case 89:
	switch( (*( sm->p)) ) {
		case 65: goto st38;
		case 93: goto tr47;
		case 97: goto st90;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st90:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof90;
case 90:
	switch( (*( sm->p)) ) {
		case 82: goto st39;
		case 93: goto tr47;
		case 114: goto st91;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st91:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof91;
case 91:
	switch( (*( sm->p)) ) {
		case 65: goto st40;
		case 93: goto tr53;
		case 97: goto st92;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st92:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof92;
case 92:
	switch( (*( sm->p)) ) {
		case 67: goto st41;
		case 93: goto tr47;
		case 99: goto st93;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st93:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof93;
case 93:
	switch( (*( sm->p)) ) {
		case 84: goto st42;
		case 93: goto tr47;
		case 116: goto st94;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st94:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof94;
case 94:
	switch( (*( sm->p)) ) {
		case 69: goto st43;
		case 93: goto tr47;
		case 101: goto st95;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st95:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof95;
case 95:
	switch( (*( sm->p)) ) {
		case 82: goto st35;
		case 93: goto tr47;
		case 114: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st96:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof96;
case 96:
	switch( (*( sm->p)) ) {
		case 78: goto st45;
		case 80: goto st52;
		case 93: goto tr47;
		case 110: goto st97;
		case 112: goto st104;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st97:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof97;
case 97:
	switch( (*( sm->p)) ) {
		case 84: goto st46;
		case 93: goto tr47;
		case 116: goto st98;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st98:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof98;
case 98:
	switch( (*( sm->p)) ) {
		case 82: goto st47;
		case 93: goto tr53;
		case 114: goto st99;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st99:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof99;
case 99:
	switch( (*( sm->p)) ) {
		case 73: goto st48;
		case 93: goto tr47;
		case 105: goto st100;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st100:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof100;
case 100:
	switch( (*( sm->p)) ) {
		case 66: goto st49;
		case 93: goto tr47;
		case 98: goto st101;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st101:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof101;
case 101:
	switch( (*( sm->p)) ) {
		case 85: goto st50;
		case 93: goto tr47;
		case 117: goto st102;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st102:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof102;
case 102:
	switch( (*( sm->p)) ) {
		case 84: goto st51;
		case 93: goto tr47;
		case 116: goto st103;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st103:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof103;
case 103:
	switch( (*( sm->p)) ) {
		case 79: goto st43;
		case 93: goto tr47;
		case 111: goto st95;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st104:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof104;
case 104:
	switch( (*( sm->p)) ) {
		case 89: goto st53;
		case 93: goto tr47;
		case 121: goto st105;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st105:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof105;
case 105:
	switch( (*( sm->p)) ) {
		case 82: goto st54;
		case 93: goto tr53;
		case 114: goto st106;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st106:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof106;
case 106:
	switch( (*( sm->p)) ) {
		case 73: goto st55;
		case 93: goto tr47;
		case 105: goto st107;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st107:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof107;
case 107:
	switch( (*( sm->p)) ) {
		case 71: goto st56;
		case 93: goto tr47;
		case 103: goto st108;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st108:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof108;
case 108:
	switch( (*( sm->p)) ) {
		case 72: goto st34;
		case 93: goto tr47;
		case 104: goto st86;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr38:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st109;
st109:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof109;
case 109:
#line 2848 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st58;
		case 93: goto tr47;
		case 101: goto st110;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st110:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof110;
case 110:
	switch( (*( sm->p)) ) {
		case 78: goto st59;
		case 93: goto tr47;
		case 110: goto st111;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st111:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof111;
case 111:
	switch( (*( sm->p)) ) {
		case 69: goto st60;
		case 93: goto tr53;
		case 101: goto st112;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st112:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof112;
case 112:
	switch( (*( sm->p)) ) {
		case 82: goto st61;
		case 93: goto tr47;
		case 114: goto st113;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st113:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof113;
case 113:
	switch( (*( sm->p)) ) {
		case 65: goto st62;
		case 93: goto tr47;
		case 97: goto st114;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st114:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof114;
case 114:
	switch( (*( sm->p)) ) {
		case 76: goto st35;
		case 93: goto tr47;
		case 108: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr39:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st115;
st115:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof115;
case 115:
#line 2923 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st64;
		case 93: goto tr47;
		case 110: goto st116;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st116:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof116;
case 116:
	switch( (*( sm->p)) ) {
		case 86: goto st65;
		case 93: goto tr47;
		case 118: goto st117;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st117:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof117;
case 117:
	switch( (*( sm->p)) ) {
		case 65: goto st66;
		case 93: goto tr53;
		case 97: goto st118;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st118:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof118;
case 118:
	switch( (*( sm->p)) ) {
		case 76: goto st67;
		case 93: goto tr47;
		case 108: goto st119;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st119:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof119;
case 119:
	switch( (*( sm->p)) ) {
		case 73: goto st68;
		case 93: goto tr47;
		case 105: goto st120;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st120:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof120;
case 120:
	switch( (*( sm->p)) ) {
		case 68: goto st35;
		case 93: goto tr47;
		case 100: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr40:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st121;
st121:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof121;
case 121:
#line 2998 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st70;
		case 93: goto tr47;
		case 111: goto st122;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st122:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof122;
case 122:
	switch( (*( sm->p)) ) {
		case 82: goto st71;
		case 93: goto tr47;
		case 114: goto st123;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st123:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof123;
case 123:
	switch( (*( sm->p)) ) {
		case 69: goto st35;
		case 93: goto tr53;
		case 101: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr41:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st124;
st124:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof124;
case 124:
#line 3037 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st73;
		case 93: goto tr47;
		case 101: goto st125;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st125:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof125;
case 125:
	switch( (*( sm->p)) ) {
		case 84: goto st74;
		case 93: goto tr47;
		case 116: goto st126;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st126:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof126;
case 126:
	switch( (*( sm->p)) ) {
		case 65: goto st35;
		case 93: goto tr47;
		case 97: goto st87;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr42:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st127;
st127:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof127;
case 127:
#line 3076 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st76;
		case 93: goto tr47;
		case 112: goto st128;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st128:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof128;
case 128:
	switch( (*( sm->p)) ) {
		case 69: goto st77;
		case 93: goto tr47;
		case 101: goto st129;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st129:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof129;
case 129:
	switch( (*( sm->p)) ) {
		case 67: goto st78;
		case 93: goto tr47;
		case 99: goto st130;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st130:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof130;
case 130:
	switch( (*( sm->p)) ) {
		case 73: goto st79;
		case 93: goto tr53;
		case 105: goto st131;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st131:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof131;
case 131:
	switch( (*( sm->p)) ) {
		case 69: goto st80;
		case 93: goto tr47;
		case 101: goto st132;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st132:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof132;
case 132:
	switch( (*( sm->p)) ) {
		case 83: goto st35;
		case 93: goto tr47;
		case 115: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st745:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof745;
case 745:
	if ( (*( sm->p)) == 32 )
		goto st745;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st745;
	goto tr828;
st133:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof133;
case 133:
	switch( (*( sm->p)) ) {
		case 69: goto st134;
		case 80: goto st153;
		case 101: goto st134;
		case 112: goto st153;
	}
	goto tr2;
st134:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof134;
case 134:
	switch( (*( sm->p)) ) {
		case 67: goto st135;
		case 99: goto st135;
	}
	goto tr2;
st135:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof135;
case 135:
	switch( (*( sm->p)) ) {
		case 84: goto st136;
		case 116: goto st136;
	}
	goto tr2;
st136:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof136;
case 136:
	switch( (*( sm->p)) ) {
		case 73: goto st137;
		case 105: goto st137;
	}
	goto tr2;
st137:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof137;
case 137:
	switch( (*( sm->p)) ) {
		case 79: goto st138;
		case 111: goto st138;
	}
	goto tr2;
st138:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof138;
case 138:
	switch( (*( sm->p)) ) {
		case 78: goto st139;
		case 110: goto st139;
	}
	goto tr2;
st139:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof139;
case 139:
	switch( (*( sm->p)) ) {
		case 44: goto st140;
		case 61: goto st151;
		case 93: goto st749;
	}
	goto tr2;
st140:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof140;
case 140:
	switch( (*( sm->p)) ) {
		case 69: goto st141;
		case 101: goto st141;
	}
	goto tr2;
st141:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof141;
case 141:
	switch( (*( sm->p)) ) {
		case 88: goto st142;
		case 120: goto st142;
	}
	goto tr2;
st142:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof142;
case 142:
	switch( (*( sm->p)) ) {
		case 80: goto st143;
		case 112: goto st143;
	}
	goto tr2;
st143:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof143;
case 143:
	switch( (*( sm->p)) ) {
		case 65: goto st144;
		case 97: goto st144;
	}
	goto tr2;
st144:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof144;
case 144:
	switch( (*( sm->p)) ) {
		case 78: goto st145;
		case 110: goto st145;
	}
	goto tr2;
st145:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof145;
case 145:
	switch( (*( sm->p)) ) {
		case 68: goto st146;
		case 100: goto st146;
	}
	goto tr2;
st146:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof146;
case 146:
	switch( (*( sm->p)) ) {
		case 69: goto st147;
		case 101: goto st147;
	}
	goto tr2;
st147:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof147;
case 147:
	switch( (*( sm->p)) ) {
		case 68: goto st148;
		case 100: goto st148;
	}
	goto tr2;
st148:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof148;
case 148:
	switch( (*( sm->p)) ) {
		case 61: goto st149;
		case 93: goto st747;
	}
	goto tr2;
st149:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof149;
case 149:
	if ( (*( sm->p)) == 93 )
		goto tr2;
	goto tr160;
tr160:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st150;
st150:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof150;
case 150:
#line 3314 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr162;
	goto st150;
tr162:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st746;
st746:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof746;
case 746:
#line 3324 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto st746;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st746;
	goto tr829;
st747:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof747;
case 747:
	if ( (*( sm->p)) == 32 )
		goto st747;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st747;
	goto tr831;
st151:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof151;
case 151:
	if ( (*( sm->p)) == 93 )
		goto tr2;
	goto tr163;
tr163:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st152;
st152:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof152;
case 152:
#line 3352 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr165;
	goto st152;
tr165:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st748;
st748:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof748;
case 748:
#line 3362 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto st748;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st748;
	goto tr832;
st749:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof749;
case 749:
	if ( (*( sm->p)) == 32 )
		goto st749;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st749;
	goto tr834;
st153:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof153;
case 153:
	switch( (*( sm->p)) ) {
		case 79: goto st154;
		case 111: goto st154;
	}
	goto tr2;
st154:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof154;
case 154:
	switch( (*( sm->p)) ) {
		case 73: goto st155;
		case 105: goto st155;
	}
	goto tr2;
st155:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof155;
case 155:
	switch( (*( sm->p)) ) {
		case 76: goto st156;
		case 108: goto st156;
	}
	goto tr2;
st156:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof156;
case 156:
	switch( (*( sm->p)) ) {
		case 69: goto st157;
		case 101: goto st157;
	}
	goto tr2;
st157:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof157;
case 157:
	switch( (*( sm->p)) ) {
		case 82: goto st158;
		case 114: goto st158;
	}
	goto tr2;
st158:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof158;
case 158:
	switch( (*( sm->p)) ) {
		case 83: goto st159;
		case 93: goto st750;
		case 115: goto st159;
	}
	goto tr2;
st159:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof159;
case 159:
	if ( (*( sm->p)) == 93 )
		goto st750;
	goto tr2;
st750:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof750;
case 750:
	if ( (*( sm->p)) == 32 )
		goto st750;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st750;
	goto tr835;
st160:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof160;
case 160:
	switch( (*( sm->p)) ) {
		case 65: goto st161;
		case 97: goto st161;
	}
	goto tr2;
st161:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof161;
case 161:
	switch( (*( sm->p)) ) {
		case 66: goto st162;
		case 98: goto st162;
	}
	goto tr2;
st162:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof162;
case 162:
	switch( (*( sm->p)) ) {
		case 76: goto st163;
		case 108: goto st163;
	}
	goto tr2;
st163:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof163;
case 163:
	switch( (*( sm->p)) ) {
		case 69: goto st164;
		case 101: goto st164;
	}
	goto tr2;
st164:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof164;
case 164:
	if ( (*( sm->p)) == 93 )
		goto tr177;
	goto tr2;
tr178:
#line 225 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{ append_html_escaped((*( sm->p))); }}
	goto st751;
tr183:
#line 190 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_B, "</strong>"); }}
	goto st751;
tr184:
#line 192 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_I, "</em>"); }}
	goto st751;
tr186:
#line 194 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_S, "</s>"); }}
	goto st751;
tr189:
#line 218 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (ignored_sup_sub_tags > 0) {
      ignored_sup_sub_tags--;
    } else {
      dstack_close_inline(INLINE_SUB, "</sub>");
    }
  }}
	goto st751;
tr190:
#line 204 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (ignored_sup_sub_tags > 0) {
      ignored_sup_sub_tags--;
    } else {
      dstack_close_inline(INLINE_SUP, "</sup>");
    }
  }}
	goto st751;
tr191:
#line 196 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_U, "</u>"); }}
	goto st751;
tr192:
#line 189 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_B, "<strong>"); }}
	goto st751;
tr193:
#line 191 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_I, "<em>"); }}
	goto st751;
tr195:
#line 193 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_S, "<s>"); }}
	goto st751;
tr198:
#line 211 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_count(INLINE_SUP) + dstack_count(INLINE_SUB) < 3) {
      dstack_open_inline(INLINE_SUB, "<sub>");
    } else {
      ignored_sup_sub_tags++;
    }
  }}
	goto st751;
tr199:
#line 197 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_count(INLINE_SUP) + dstack_count(INLINE_SUB) < 3) {
      dstack_open_inline(INLINE_SUP, "<sup>");
    } else {
      ignored_sup_sub_tags++;
    }
  }}
	goto st751;
tr200:
#line 195 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_U, "<u>"); }}
	goto st751;
tr836:
#line 225 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ append_html_escaped((*( sm->p))); }}
	goto st751;
tr838:
#line 225 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_html_escaped((*( sm->p))); }}
	goto st751;
st751:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof751;
case 751:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 3565 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 91 )
		goto tr837;
	goto tr836;
tr837:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st752;
st752:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof752;
case 752:
#line 3575 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 47: goto st165;
		case 66: goto st173;
		case 73: goto st174;
		case 83: goto st175;
		case 85: goto st179;
		case 98: goto st173;
		case 105: goto st174;
		case 115: goto st175;
		case 117: goto st179;
	}
	goto tr838;
st165:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof165;
case 165:
	switch( (*( sm->p)) ) {
		case 66: goto st166;
		case 73: goto st167;
		case 83: goto st168;
		case 85: goto st172;
		case 98: goto st166;
		case 105: goto st167;
		case 115: goto st168;
		case 117: goto st172;
	}
	goto tr178;
st166:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof166;
case 166:
	if ( (*( sm->p)) == 93 )
		goto tr183;
	goto tr178;
st167:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof167;
case 167:
	if ( (*( sm->p)) == 93 )
		goto tr184;
	goto tr178;
st168:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof168;
case 168:
	switch( (*( sm->p)) ) {
		case 85: goto st169;
		case 93: goto tr186;
		case 117: goto st169;
	}
	goto tr178;
st169:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof169;
case 169:
	switch( (*( sm->p)) ) {
		case 66: goto st170;
		case 80: goto st171;
		case 98: goto st170;
		case 112: goto st171;
	}
	goto tr178;
st170:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof170;
case 170:
	if ( (*( sm->p)) == 93 )
		goto tr189;
	goto tr178;
st171:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof171;
case 171:
	if ( (*( sm->p)) == 93 )
		goto tr190;
	goto tr178;
st172:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof172;
case 172:
	if ( (*( sm->p)) == 93 )
		goto tr191;
	goto tr178;
st173:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof173;
case 173:
	if ( (*( sm->p)) == 93 )
		goto tr192;
	goto tr178;
st174:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof174;
case 174:
	if ( (*( sm->p)) == 93 )
		goto tr193;
	goto tr178;
st175:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof175;
case 175:
	switch( (*( sm->p)) ) {
		case 85: goto st176;
		case 93: goto tr195;
		case 117: goto st176;
	}
	goto tr178;
st176:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof176;
case 176:
	switch( (*( sm->p)) ) {
		case 66: goto st177;
		case 80: goto st178;
		case 98: goto st177;
		case 112: goto st178;
	}
	goto tr178;
st177:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof177;
case 177:
	if ( (*( sm->p)) == 93 )
		goto tr198;
	goto tr178;
st178:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof178;
case 178:
	if ( (*( sm->p)) == 93 )
		goto tr199;
	goto tr178;
st179:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof179;
case 179:
	if ( (*( sm->p)) == 93 )
		goto tr200;
	goto tr178;
tr201:
#line 1 "NONE"
	{	switch( ( sm->act) ) {
	case 78:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }
	break;
	case 79:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("inline newline");

    if (header_mode) {
      dstack_close_leaf_blocks();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_list();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append("<br>");
    }
  }
	break;
	case 81:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }
	break;
	}
	}
	goto st753;
tr203:
#line 529 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr214:
#line 413 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [/spoiler]");
    dstack_close_before_block();

    if (dstack_check(INLINE_SPOILER)) {
      dstack_close_inline(INLINE_SPOILER, "</span>");
    } else if (dstack_close_block(BLOCK_SPOILER, "</div>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st753;
tr216:
#line 523 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_close_block(BLOCK_TD, "</td>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st753;
tr217:
#line 539 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("inline newline");

    if (header_mode) {
      dstack_close_leaf_blocks();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_list();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append("<br>");
    }
  }}
	goto st753;
tr239:
#line 557 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }}
	goto st753;
tr257:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 314 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_named_url({ b1, b2 }, { a1, a2 });
  }}
	goto st753;
tr273:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 330 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_unnamed_url({ a1, a2 });
  }}
	goto st753;
tr434:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 238 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("<a id=\"");
    std::string lowercased_tag = std::string(a1, a2 - a1);
    std::transform(lowercased_tag.begin(), lowercased_tag.end(), lowercased_tag.begin(), [](unsigned char c) { return std::tolower(c); });
    append_uri_escaped(lowercased_tag);
    append("\"></a>");
  }}
	goto st753;
tr441:
#line 341 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_B, "</strong>"); }}
	goto st753;
tr449:
#line 402 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if(options.allow_color) {
      dstack_close_inline(INLINE_COLOR, "</span>");
    }
    {goto st753;}
  }}
	goto st753;
tr450:
#line 343 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_I, "</em>"); }}
	goto st753;
tr452:
#line 345 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_S, "</s>"); }}
	goto st753;
tr455:
#line 369 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (ignored_sup_sub_tags > 0) {
      ignored_sup_sub_tags--;
    } else {
      dstack_close_inline(INLINE_SUB, "</sub>");
    }
  }}
	goto st753;
tr456:
#line 355 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (ignored_sup_sub_tags > 0) {
      ignored_sup_sub_tags--;
    } else {
      dstack_close_inline(INLINE_SUP, "</sup>");
    }
  }}
	goto st753;
tr463:
#line 517 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_close_block(BLOCK_TH, "</th>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st753;
tr464:
#line 347 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_U, "</u>"); }}
	goto st753;
tr465:
#line 340 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_B, "<strong>"); }}
	goto st753;
tr470:
#line 455 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_before_block();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr494:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 387 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if(options.allow_color) {
      dstack_push(INLINE_COLOR);
      append("<span class=\"dtext-color\" style=\"color:");
      if(a1[0] == '#') {
        append("#");
        append_uri_escaped({ a1 + 1, a2 });
      } else {
        append_uri_escaped({ a1, a2 });
      }
      append("\">");
    }
    {goto st753;}
  }}
	goto st753;
tr500:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 377 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if(options.allow_color) {
      dstack_push(INLINE_COLOR);
      append("<span class=\"dtext-color-");
      append_uri_escaped({ a1, a2 });
      append("\">");
    }
    {goto st753;}
  }}
	goto st753;
tr587:
#line 342 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_I, "<em>"); }}
	goto st753;
tr593:
#line 477 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [quote]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr614:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 484 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [quote=color]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr620:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 491 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [quote=type]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr710:
#line 344 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_S, "<s>"); }}
	goto st753;
tr718:
#line 504 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [section]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr729:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 504 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [section]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr736:
#line 409 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_inline(INLINE_SPOILER, "<span class=\"spoiler\">");
  }}
	goto st753;
tr739:
#line 362 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_count(INLINE_SUP) + dstack_count(INLINE_SUB) < 3) {
      dstack_open_inline(INLINE_SUB, "<sub>");
    } else {
      ignored_sup_sub_tags++;
    }
  }}
	goto st753;
tr740:
#line 348 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_count(INLINE_SUP) + dstack_count(INLINE_SUB) < 3) {
      dstack_open_inline(INLINE_SUP, "<sup>");
    } else {
      ignored_sup_sub_tags++;
    }
  }}
	goto st753;
tr745:
#line 433 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_before_block();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr746:
#line 346 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_U, "<u>"); }}
	goto st753;
tr752:
#line 294 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_wiki_link({ a1, a2 }, { a1, a2 });
  }}
	goto st753;
tr756:
#line 298 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_wiki_link({ a1, a2 }, { b1, b2 });
  }}
	goto st753;
tr766:
#line 290 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_post_search_link({ a1, a2 }, { b1, b2 });
  }}
	goto st753;
tr767:
#line 286 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_post_search_link({ a1, a2 }, { a1, a2 });
  }}
	goto st753;
tr844:
#line 557 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }}
	goto st753;
tr865:
#line 233 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_inline(INLINE_CODE, "<span class=\"inline-code\">");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 753;goto st807;}}
  }}
	goto st753;
tr867:
#line 539 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline newline");

    if (header_mode) {
      dstack_close_leaf_blocks();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_list();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append("<br>");
    }
  }}
	goto st753;
tr872:
#line 529 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr874:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 334 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline list");
    {( sm->p) = (( ts + 1))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr876:
#line 427 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr878:
#line 498 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/quote]");
    dstack_close_until(BLOCK_QUOTE);
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr879:
#line 511 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/expand]");
    dstack_close_until(BLOCK_SECTION);
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st753;
tr880:
#line 553 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append(' ');
  }}
	goto st753;
tr881:
#line 557 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }}
	goto st753;
tr883:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 302 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    const char* match_end = b2;
    const char* url_start = b1;
    const char* url_end = find_boundary_c(match_end - 1) + 1;

    append_named_url({ url_start, url_end }, { a1, a2 });

    if (url_end < match_end) {
      append_html_escaped({ url_end, match_end });
    }
  }}
	goto st753;
tr887:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 276 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("alias", "tag-alias", "/tag_aliases/"); }}
	goto st753;
tr889:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 273 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("artist", "artist", "/artists/"); }}
	goto st753;
tr894:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 274 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("ban", "ban", "/bans/"); }}
	goto st753;
tr896:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 282 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("blip", "blip", "/blips/"); }}
	goto st753;
tr898:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 275 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("BUR", "bulk-update-request", "/bulk_update_requests/"); }}
	goto st753;
tr901:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 270 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("comment", "comment", "/comments/"); }}
	goto st753;
tr905:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 266 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("flag", "post-flag", "/post_flags/"); }}
	goto st753;
tr907:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 268 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("forum", "forum-post", "/forum_posts/"); }}
	goto st753;
tr910:
#line 318 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    const char* match_end = te;
    const char* url_start = ts;
    const char* url_end = find_boundary_c(match_end - 1) + 1;

    append_unnamed_url({ url_start, url_end });

    if (url_end < match_end) {
      append_html_escaped({ url_end, match_end });
    }
  }}
	goto st753;
tr912:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 277 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("implication", "tag-implication", "/tag_implications/"); }}
	goto st753;
tr915:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 278 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("mod action", "mod-action", "/mod_actions/"); }}
	goto st753;
tr918:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 267 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("note", "note", "/notes/"); }}
	goto st753;
tr921:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 271 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("pool", "pool", "/pools/"); }}
	goto st753;
tr923:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 264 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("post", "post", "/posts/"); }}
	goto st753;
tr925:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 265 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("post changes", "post-changes-for", "/post_versions?search[post_id]="); }}
	goto st753;
tr928:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 279 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("record", "user-feedback", "/user_feedbacks/"); }}
	goto st753;
tr931:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 281 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("set", "set", "/post_sets/"); }}
	goto st753;
tr937:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 284 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("takedown", "takedown", "/takedowns/"); }}
	goto st753;
tr939:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 246 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    if(posts.size() < options.max_thumbs) {
      long post_id = strtol(a1, (char**)&a2, 10);
      posts.push_back(post_id);
      append("<a class=\"dtext-link dtext-id-link dtext-post-id-link thumb-placeholder-link\" data-id=\"");
      append_html_escaped({ a1, a2 });
      append("\" href=\"");
      append_url("/posts/");
      append_uri_escaped({ a1, a2 });
      append("\">");
      append("post #");
      append_html_escaped({ a1, a2 });
      append("</a>");
    } else {
      append_id_link("post", "post", "/posts/");
    }
  }}
	goto st753;
tr941:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 283 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("ticket", "ticket", "/tickets/"); }}
	goto st753;
tr943:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 269 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("topic", "forum-topic", "/forum_topics/"); }}
	goto st753;
tr946:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 272 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("user", "user", "/users/"); }}
	goto st753;
tr949:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 280 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("wiki", "wiki-page", "/wiki_pages/"); }}
	goto st753;
tr961:
#line 461 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/code]");
    dstack_close_before_block();

    if (dstack_check(BLOCK_LI)) {
      dstack_close_list();
    }

    if (dstack_check(BLOCK_CODE)) {
      dstack_rewind();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append_block("[/code]");
    }
  }}
	goto st753;
tr962:
#line 439 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/table]");
    dstack_close_before_block();

    if (dstack_check(BLOCK_LI)) {
      dstack_close_list();
    }

    if (dstack_check(BLOCK_TABLE)) {
      dstack_rewind();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append_block("[/table]");
    }
  }}
	goto st753;
tr963:
#line 229 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("`");
  }}
	goto st753;
st753:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof753;
case 753:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 4276 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr845;
		case 13: goto st761;
		case 34: goto tr847;
		case 60: goto tr848;
		case 65: goto tr849;
		case 66: goto tr850;
		case 67: goto tr851;
		case 70: goto tr852;
		case 72: goto tr853;
		case 73: goto tr854;
		case 77: goto tr855;
		case 78: goto tr856;
		case 80: goto tr857;
		case 82: goto tr858;
		case 83: goto tr859;
		case 84: goto tr860;
		case 85: goto tr861;
		case 87: goto tr862;
		case 91: goto tr863;
		case 92: goto st805;
		case 96: goto tr865;
		case 97: goto tr849;
		case 98: goto tr850;
		case 99: goto tr851;
		case 102: goto tr852;
		case 104: goto tr853;
		case 105: goto tr854;
		case 109: goto tr855;
		case 110: goto tr856;
		case 112: goto tr857;
		case 114: goto tr858;
		case 115: goto tr859;
		case 116: goto tr860;
		case 117: goto tr861;
		case 119: goto tr862;
		case 123: goto tr866;
	}
	goto tr844;
tr845:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 539 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 79;}
	goto st754;
st754:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof754;
case 754:
#line 4323 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr202;
		case 13: goto st180;
		case 42: goto tr869;
		case 72: goto st195;
		case 91: goto st197;
		case 104: goto st195;
	}
	goto tr867;
tr202:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 529 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 78;}
	goto st755;
st755:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof755;
case 755:
#line 4340 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr202;
		case 13: goto st180;
		case 91: goto st181;
	}
	goto tr872;
st180:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof180;
case 180:
	if ( (*( sm->p)) == 10 )
		goto tr202;
	goto tr201;
st181:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof181;
case 181:
	if ( (*( sm->p)) == 47 )
		goto st182;
	goto tr203;
st182:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof182;
case 182:
	switch( (*( sm->p)) ) {
		case 83: goto st183;
		case 84: goto st191;
		case 115: goto st183;
		case 116: goto st191;
	}
	goto tr203;
st183:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof183;
case 183:
	switch( (*( sm->p)) ) {
		case 80: goto st184;
		case 112: goto st184;
	}
	goto tr203;
st184:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof184;
case 184:
	switch( (*( sm->p)) ) {
		case 79: goto st185;
		case 111: goto st185;
	}
	goto tr201;
st185:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof185;
case 185:
	switch( (*( sm->p)) ) {
		case 73: goto st186;
		case 105: goto st186;
	}
	goto tr201;
st186:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof186;
case 186:
	switch( (*( sm->p)) ) {
		case 76: goto st187;
		case 108: goto st187;
	}
	goto tr201;
st187:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof187;
case 187:
	switch( (*( sm->p)) ) {
		case 69: goto st188;
		case 101: goto st188;
	}
	goto tr201;
st188:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof188;
case 188:
	switch( (*( sm->p)) ) {
		case 82: goto st189;
		case 114: goto st189;
	}
	goto tr201;
st189:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof189;
case 189:
	switch( (*( sm->p)) ) {
		case 83: goto st190;
		case 93: goto tr214;
		case 115: goto st190;
	}
	goto tr201;
st190:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof190;
case 190:
	if ( (*( sm->p)) == 93 )
		goto tr214;
	goto tr201;
st191:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof191;
case 191:
	switch( (*( sm->p)) ) {
		case 68: goto st192;
		case 100: goto st192;
	}
	goto tr201;
st192:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof192;
case 192:
	if ( (*( sm->p)) == 93 )
		goto tr216;
	goto tr201;
tr869:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st193;
st193:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof193;
case 193:
#line 4465 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr218;
		case 32: goto tr218;
		case 42: goto st193;
	}
	goto tr217;
tr218:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st194;
st194:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof194;
case 194:
#line 4478 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr221;
		case 10: goto tr217;
		case 13: goto tr217;
		case 32: goto tr221;
	}
	goto tr220;
tr220:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st756;
st756:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof756;
case 756:
#line 4492 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr874;
		case 13: goto tr874;
	}
	goto st756;
tr221:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st757;
st757:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof757;
case 757:
#line 4504 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr221;
		case 10: goto tr874;
		case 13: goto tr874;
		case 32: goto tr221;
	}
	goto tr220;
st195:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof195;
case 195:
	if ( 49 <= (*( sm->p)) && (*( sm->p)) <= 54 )
		goto tr222;
	goto tr217;
tr222:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st196;
st196:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof196;
case 196:
#line 4525 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 46 )
		goto tr223;
	goto tr217;
tr223:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st758;
st758:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof758;
case 758:
#line 4535 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto st758;
		case 32: goto st758;
	}
	goto tr876;
st197:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof197;
case 197:
	if ( (*( sm->p)) == 47 )
		goto st198;
	goto tr217;
st198:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof198;
case 198:
	switch( (*( sm->p)) ) {
		case 81: goto st199;
		case 83: goto st204;
		case 84: goto st191;
		case 113: goto st199;
		case 115: goto st204;
		case 116: goto st191;
	}
	goto tr217;
st199:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof199;
case 199:
	switch( (*( sm->p)) ) {
		case 85: goto st200;
		case 117: goto st200;
	}
	goto tr201;
st200:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof200;
case 200:
	switch( (*( sm->p)) ) {
		case 79: goto st201;
		case 111: goto st201;
	}
	goto tr201;
st201:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof201;
case 201:
	switch( (*( sm->p)) ) {
		case 84: goto st202;
		case 116: goto st202;
	}
	goto tr201;
st202:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof202;
case 202:
	switch( (*( sm->p)) ) {
		case 69: goto st203;
		case 101: goto st203;
	}
	goto tr201;
st203:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof203;
case 203:
	_widec = (*( sm->p));
	if ( 93 <= (*( sm->p)) && (*( sm->p)) <= 93 ) {
		_widec = (short)(128 + ((*( sm->p)) - -128));
		if ( 
#line 97 "ext/dtext/dtext.cpp.rl"
 dstack_is_open(BLOCK_QUOTE)  ) _widec += 256;
	}
	if ( _widec == 605 )
		goto st759;
	goto tr201;
st759:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof759;
case 759:
	switch( (*( sm->p)) ) {
		case 9: goto st759;
		case 32: goto st759;
	}
	goto tr878;
st204:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof204;
case 204:
	switch( (*( sm->p)) ) {
		case 69: goto st205;
		case 80: goto st184;
		case 101: goto st205;
		case 112: goto st184;
	}
	goto tr217;
st205:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof205;
case 205:
	switch( (*( sm->p)) ) {
		case 67: goto st206;
		case 99: goto st206;
	}
	goto tr201;
st206:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof206;
case 206:
	switch( (*( sm->p)) ) {
		case 84: goto st207;
		case 116: goto st207;
	}
	goto tr201;
st207:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof207;
case 207:
	switch( (*( sm->p)) ) {
		case 73: goto st208;
		case 105: goto st208;
	}
	goto tr201;
st208:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof208;
case 208:
	switch( (*( sm->p)) ) {
		case 79: goto st209;
		case 111: goto st209;
	}
	goto tr201;
st209:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof209;
case 209:
	switch( (*( sm->p)) ) {
		case 78: goto st210;
		case 110: goto st210;
	}
	goto tr201;
st210:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof210;
case 210:
	_widec = (*( sm->p));
	if ( 93 <= (*( sm->p)) && (*( sm->p)) <= 93 ) {
		_widec = (short)(640 + ((*( sm->p)) - -128));
		if ( 
#line 98 "ext/dtext/dtext.cpp.rl"
 dstack_is_open(BLOCK_SECTION)  ) _widec += 256;
	}
	if ( _widec == 1117 )
		goto st760;
	goto tr201;
st760:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof760;
case 760:
	switch( (*( sm->p)) ) {
		case 9: goto st760;
		case 32: goto st760;
	}
	goto tr879;
st761:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof761;
case 761:
	if ( (*( sm->p)) == 10 )
		goto tr845;
	goto tr880;
tr847:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st762;
st762:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof762;
case 762:
#line 4710 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 34 )
		goto tr881;
	goto tr882;
tr882:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st211;
st211:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof211;
case 211:
#line 4720 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 34 )
		goto tr241;
	goto st211;
tr241:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st212;
st212:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof212;
case 212:
#line 4730 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 58 )
		goto st213;
	goto tr239;
st213:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof213;
case 213:
	switch( (*( sm->p)) ) {
		case 35: goto tr243;
		case 47: goto tr243;
		case 72: goto tr244;
		case 91: goto st222;
		case 104: goto tr244;
	}
	goto tr239;
tr243:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st214;
st214:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof214;
case 214:
#line 4752 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto tr239;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr239;
	goto st763;
st763:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof763;
case 763:
	if ( (*( sm->p)) == 32 )
		goto tr883;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr883;
	goto st763;
tr244:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st215;
st215:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof215;
case 215:
#line 4773 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st216;
		case 116: goto st216;
	}
	goto tr239;
st216:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof216;
case 216:
	switch( (*( sm->p)) ) {
		case 84: goto st217;
		case 116: goto st217;
	}
	goto tr239;
st217:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof217;
case 217:
	switch( (*( sm->p)) ) {
		case 80: goto st218;
		case 112: goto st218;
	}
	goto tr239;
st218:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof218;
case 218:
	switch( (*( sm->p)) ) {
		case 58: goto st219;
		case 83: goto st221;
		case 115: goto st221;
	}
	goto tr239;
st219:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof219;
case 219:
	if ( (*( sm->p)) == 47 )
		goto st220;
	goto tr239;
st220:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof220;
case 220:
	if ( (*( sm->p)) == 47 )
		goto st214;
	goto tr239;
st221:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof221;
case 221:
	if ( (*( sm->p)) == 58 )
		goto st219;
	goto tr239;
st222:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof222;
case 222:
	switch( (*( sm->p)) ) {
		case 35: goto tr254;
		case 47: goto tr254;
		case 72: goto tr255;
		case 104: goto tr255;
	}
	goto tr239;
tr254:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st223;
st223:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof223;
case 223:
#line 4845 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto tr239;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr239;
	goto st224;
st224:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof224;
case 224:
	switch( (*( sm->p)) ) {
		case 32: goto tr239;
		case 93: goto tr257;
	}
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr239;
	goto st224;
tr255:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st225;
st225:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof225;
case 225:
#line 4868 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st226;
		case 116: goto st226;
	}
	goto tr239;
st226:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof226;
case 226:
	switch( (*( sm->p)) ) {
		case 84: goto st227;
		case 116: goto st227;
	}
	goto tr239;
st227:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof227;
case 227:
	switch( (*( sm->p)) ) {
		case 80: goto st228;
		case 112: goto st228;
	}
	goto tr239;
st228:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof228;
case 228:
	switch( (*( sm->p)) ) {
		case 58: goto st229;
		case 83: goto st231;
		case 115: goto st231;
	}
	goto tr239;
st229:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof229;
case 229:
	if ( (*( sm->p)) == 47 )
		goto st230;
	goto tr239;
st230:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof230;
case 230:
	if ( (*( sm->p)) == 47 )
		goto st223;
	goto tr239;
st231:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof231;
case 231:
	if ( (*( sm->p)) == 58 )
		goto st229;
	goto tr239;
tr848:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st764;
st764:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof764;
case 764:
#line 4929 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto tr884;
		case 104: goto tr884;
	}
	goto tr881;
tr884:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st232;
st232:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof232;
case 232:
#line 4941 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st233;
		case 116: goto st233;
	}
	goto tr239;
st233:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof233;
case 233:
	switch( (*( sm->p)) ) {
		case 84: goto st234;
		case 116: goto st234;
	}
	goto tr239;
st234:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof234;
case 234:
	switch( (*( sm->p)) ) {
		case 80: goto st235;
		case 112: goto st235;
	}
	goto tr239;
st235:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof235;
case 235:
	switch( (*( sm->p)) ) {
		case 58: goto st236;
		case 83: goto st240;
		case 115: goto st240;
	}
	goto tr239;
st236:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof236;
case 236:
	if ( (*( sm->p)) == 47 )
		goto st237;
	goto tr239;
st237:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof237;
case 237:
	if ( (*( sm->p)) == 47 )
		goto st238;
	goto tr239;
st238:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof238;
case 238:
	if ( (*( sm->p)) == 32 )
		goto tr239;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr239;
	goto st239;
st239:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof239;
case 239:
	switch( (*( sm->p)) ) {
		case 32: goto tr239;
		case 62: goto tr273;
	}
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr239;
	goto st239;
st240:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof240;
case 240:
	if ( (*( sm->p)) == 58 )
		goto st236;
	goto tr239;
tr849:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st765;
st765:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof765;
case 765:
#line 5022 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st241;
		case 82: goto st247;
		case 108: goto st241;
		case 114: goto st247;
	}
	goto tr881;
st241:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof241;
case 241:
	switch( (*( sm->p)) ) {
		case 73: goto st242;
		case 105: goto st242;
	}
	goto tr239;
st242:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof242;
case 242:
	switch( (*( sm->p)) ) {
		case 65: goto st243;
		case 97: goto st243;
	}
	goto tr239;
st243:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof243;
case 243:
	switch( (*( sm->p)) ) {
		case 83: goto st244;
		case 115: goto st244;
	}
	goto tr239;
st244:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof244;
case 244:
	if ( (*( sm->p)) == 32 )
		goto st245;
	goto tr239;
st245:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof245;
case 245:
	if ( (*( sm->p)) == 35 )
		goto st246;
	goto tr239;
st246:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof246;
case 246:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr279;
	goto tr239;
tr279:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st766;
st766:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof766;
case 766:
#line 5084 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st766;
	goto tr887;
st247:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof247;
case 247:
	switch( (*( sm->p)) ) {
		case 84: goto st248;
		case 116: goto st248;
	}
	goto tr239;
st248:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof248;
case 248:
	switch( (*( sm->p)) ) {
		case 73: goto st249;
		case 105: goto st249;
	}
	goto tr239;
st249:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof249;
case 249:
	switch( (*( sm->p)) ) {
		case 83: goto st250;
		case 115: goto st250;
	}
	goto tr239;
st250:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof250;
case 250:
	switch( (*( sm->p)) ) {
		case 84: goto st251;
		case 116: goto st251;
	}
	goto tr239;
st251:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof251;
case 251:
	if ( (*( sm->p)) == 32 )
		goto st252;
	goto tr239;
st252:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof252;
case 252:
	if ( (*( sm->p)) == 35 )
		goto st253;
	goto tr239;
st253:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof253;
case 253:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr286;
	goto tr239;
tr286:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st767;
st767:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof767;
case 767:
#line 5151 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st767;
	goto tr889;
tr850:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st768;
st768:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof768;
case 768:
#line 5161 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st254;
		case 76: goto st258;
		case 85: goto st263;
		case 97: goto st254;
		case 108: goto st258;
		case 117: goto st263;
	}
	goto tr881;
st254:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof254;
case 254:
	switch( (*( sm->p)) ) {
		case 78: goto st255;
		case 110: goto st255;
	}
	goto tr239;
st255:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof255;
case 255:
	if ( (*( sm->p)) == 32 )
		goto st256;
	goto tr239;
st256:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof256;
case 256:
	if ( (*( sm->p)) == 35 )
		goto st257;
	goto tr239;
st257:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof257;
case 257:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr290;
	goto tr239;
tr290:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st769;
st769:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof769;
case 769:
#line 5207 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st769;
	goto tr894;
st258:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof258;
case 258:
	switch( (*( sm->p)) ) {
		case 73: goto st259;
		case 105: goto st259;
	}
	goto tr239;
st259:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof259;
case 259:
	switch( (*( sm->p)) ) {
		case 80: goto st260;
		case 112: goto st260;
	}
	goto tr239;
st260:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof260;
case 260:
	if ( (*( sm->p)) == 32 )
		goto st261;
	goto tr239;
st261:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof261;
case 261:
	if ( (*( sm->p)) == 35 )
		goto st262;
	goto tr239;
st262:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof262;
case 262:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr295;
	goto tr239;
tr295:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st770;
st770:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof770;
case 770:
#line 5256 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st770;
	goto tr896;
st263:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof263;
case 263:
	switch( (*( sm->p)) ) {
		case 82: goto st264;
		case 114: goto st264;
	}
	goto tr239;
st264:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof264;
case 264:
	if ( (*( sm->p)) == 32 )
		goto st265;
	goto tr239;
st265:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof265;
case 265:
	if ( (*( sm->p)) == 35 )
		goto st266;
	goto tr239;
st266:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof266;
case 266:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr299;
	goto tr239;
tr299:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st771;
st771:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof771;
case 771:
#line 5296 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st771;
	goto tr898;
tr851:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st772;
st772:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof772;
case 772:
#line 5306 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st267;
		case 111: goto st267;
	}
	goto tr881;
st267:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof267;
case 267:
	switch( (*( sm->p)) ) {
		case 77: goto st268;
		case 109: goto st268;
	}
	goto tr239;
st268:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof268;
case 268:
	switch( (*( sm->p)) ) {
		case 77: goto st269;
		case 109: goto st269;
	}
	goto tr239;
st269:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof269;
case 269:
	switch( (*( sm->p)) ) {
		case 69: goto st270;
		case 101: goto st270;
	}
	goto tr239;
st270:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof270;
case 270:
	switch( (*( sm->p)) ) {
		case 78: goto st271;
		case 110: goto st271;
	}
	goto tr239;
st271:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof271;
case 271:
	switch( (*( sm->p)) ) {
		case 84: goto st272;
		case 116: goto st272;
	}
	goto tr239;
st272:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof272;
case 272:
	if ( (*( sm->p)) == 32 )
		goto st273;
	goto tr239;
st273:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof273;
case 273:
	if ( (*( sm->p)) == 35 )
		goto st274;
	goto tr239;
st274:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof274;
case 274:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr307;
	goto tr239;
tr307:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st773;
st773:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof773;
case 773:
#line 5384 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st773;
	goto tr901;
tr852:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st774;
st774:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof774;
case 774:
#line 5394 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st275;
		case 79: goto st280;
		case 108: goto st275;
		case 111: goto st280;
	}
	goto tr881;
st275:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof275;
case 275:
	switch( (*( sm->p)) ) {
		case 65: goto st276;
		case 97: goto st276;
	}
	goto tr239;
st276:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof276;
case 276:
	switch( (*( sm->p)) ) {
		case 71: goto st277;
		case 103: goto st277;
	}
	goto tr239;
st277:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof277;
case 277:
	if ( (*( sm->p)) == 32 )
		goto st278;
	goto tr239;
st278:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof278;
case 278:
	if ( (*( sm->p)) == 35 )
		goto st279;
	goto tr239;
st279:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof279;
case 279:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr312;
	goto tr239;
tr312:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st775;
st775:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof775;
case 775:
#line 5447 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st775;
	goto tr905;
st280:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof280;
case 280:
	switch( (*( sm->p)) ) {
		case 82: goto st281;
		case 114: goto st281;
	}
	goto tr239;
st281:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof281;
case 281:
	switch( (*( sm->p)) ) {
		case 85: goto st282;
		case 117: goto st282;
	}
	goto tr239;
st282:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof282;
case 282:
	switch( (*( sm->p)) ) {
		case 77: goto st283;
		case 109: goto st283;
	}
	goto tr239;
st283:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof283;
case 283:
	if ( (*( sm->p)) == 32 )
		goto st284;
	goto tr239;
st284:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof284;
case 284:
	if ( (*( sm->p)) == 35 )
		goto st285;
	goto tr239;
st285:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof285;
case 285:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr318;
	goto tr239;
tr318:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st776;
st776:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof776;
case 776:
#line 5505 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st776;
	goto tr907;
tr853:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st777;
st777:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof777;
case 777:
#line 5515 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st286;
		case 116: goto st286;
	}
	goto tr881;
st286:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof286;
case 286:
	switch( (*( sm->p)) ) {
		case 84: goto st287;
		case 116: goto st287;
	}
	goto tr239;
st287:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof287;
case 287:
	switch( (*( sm->p)) ) {
		case 80: goto st288;
		case 112: goto st288;
	}
	goto tr239;
st288:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof288;
case 288:
	switch( (*( sm->p)) ) {
		case 58: goto st289;
		case 83: goto st292;
		case 115: goto st292;
	}
	goto tr239;
st289:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof289;
case 289:
	if ( (*( sm->p)) == 47 )
		goto st290;
	goto tr239;
st290:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof290;
case 290:
	if ( (*( sm->p)) == 47 )
		goto st291;
	goto tr239;
st291:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof291;
case 291:
	if ( (*( sm->p)) == 32 )
		goto tr239;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr239;
	goto st778;
st778:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof778;
case 778:
	if ( (*( sm->p)) == 32 )
		goto tr910;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr910;
	goto st778;
st292:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof292;
case 292:
	if ( (*( sm->p)) == 58 )
		goto st289;
	goto tr239;
tr854:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st779;
st779:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof779;
case 779:
#line 5594 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 77: goto st293;
		case 109: goto st293;
	}
	goto tr881;
st293:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof293;
case 293:
	switch( (*( sm->p)) ) {
		case 80: goto st294;
		case 112: goto st294;
	}
	goto tr239;
st294:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof294;
case 294:
	switch( (*( sm->p)) ) {
		case 76: goto st295;
		case 108: goto st295;
	}
	goto tr239;
st295:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof295;
case 295:
	switch( (*( sm->p)) ) {
		case 73: goto st296;
		case 105: goto st296;
	}
	goto tr239;
st296:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof296;
case 296:
	switch( (*( sm->p)) ) {
		case 67: goto st297;
		case 99: goto st297;
	}
	goto tr239;
st297:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof297;
case 297:
	switch( (*( sm->p)) ) {
		case 65: goto st298;
		case 97: goto st298;
	}
	goto tr239;
st298:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof298;
case 298:
	switch( (*( sm->p)) ) {
		case 84: goto st299;
		case 116: goto st299;
	}
	goto tr239;
st299:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof299;
case 299:
	switch( (*( sm->p)) ) {
		case 73: goto st300;
		case 105: goto st300;
	}
	goto tr239;
st300:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof300;
case 300:
	switch( (*( sm->p)) ) {
		case 79: goto st301;
		case 111: goto st301;
	}
	goto tr239;
st301:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof301;
case 301:
	switch( (*( sm->p)) ) {
		case 78: goto st302;
		case 110: goto st302;
	}
	goto tr239;
st302:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof302;
case 302:
	if ( (*( sm->p)) == 32 )
		goto st303;
	goto tr239;
st303:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof303;
case 303:
	if ( (*( sm->p)) == 35 )
		goto st304;
	goto tr239;
st304:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof304;
case 304:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr337;
	goto tr239;
tr337:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st780;
st780:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof780;
case 780:
#line 5708 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st780;
	goto tr912;
tr855:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st781;
st781:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof781;
case 781:
#line 5718 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st305;
		case 111: goto st305;
	}
	goto tr881;
st305:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof305;
case 305:
	switch( (*( sm->p)) ) {
		case 68: goto st306;
		case 100: goto st306;
	}
	goto tr239;
st306:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof306;
case 306:
	if ( (*( sm->p)) == 32 )
		goto st307;
	goto tr239;
st307:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof307;
case 307:
	switch( (*( sm->p)) ) {
		case 65: goto st308;
		case 97: goto st308;
	}
	goto tr239;
st308:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof308;
case 308:
	switch( (*( sm->p)) ) {
		case 67: goto st309;
		case 99: goto st309;
	}
	goto tr239;
st309:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof309;
case 309:
	switch( (*( sm->p)) ) {
		case 84: goto st310;
		case 116: goto st310;
	}
	goto tr239;
st310:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof310;
case 310:
	switch( (*( sm->p)) ) {
		case 73: goto st311;
		case 105: goto st311;
	}
	goto tr239;
st311:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof311;
case 311:
	switch( (*( sm->p)) ) {
		case 79: goto st312;
		case 111: goto st312;
	}
	goto tr239;
st312:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof312;
case 312:
	switch( (*( sm->p)) ) {
		case 78: goto st313;
		case 110: goto st313;
	}
	goto tr239;
st313:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof313;
case 313:
	if ( (*( sm->p)) == 32 )
		goto st314;
	goto tr239;
st314:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof314;
case 314:
	if ( (*( sm->p)) == 35 )
		goto st315;
	goto tr239;
st315:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof315;
case 315:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr348;
	goto tr239;
tr348:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st782;
st782:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof782;
case 782:
#line 5821 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st782;
	goto tr915;
tr856:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st783;
st783:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof783;
case 783:
#line 5831 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st316;
		case 111: goto st316;
	}
	goto tr881;
st316:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof316;
case 316:
	switch( (*( sm->p)) ) {
		case 84: goto st317;
		case 116: goto st317;
	}
	goto tr239;
st317:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof317;
case 317:
	switch( (*( sm->p)) ) {
		case 69: goto st318;
		case 101: goto st318;
	}
	goto tr239;
st318:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof318;
case 318:
	if ( (*( sm->p)) == 32 )
		goto st319;
	goto tr239;
st319:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof319;
case 319:
	if ( (*( sm->p)) == 35 )
		goto st320;
	goto tr239;
st320:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof320;
case 320:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr353;
	goto tr239;
tr353:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st784;
st784:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof784;
case 784:
#line 5882 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st784;
	goto tr918;
tr857:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st785;
st785:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof785;
case 785:
#line 5892 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st321;
		case 111: goto st321;
	}
	goto tr881;
st321:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof321;
case 321:
	switch( (*( sm->p)) ) {
		case 79: goto st322;
		case 83: goto st326;
		case 111: goto st322;
		case 115: goto st326;
	}
	goto tr239;
st322:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof322;
case 322:
	switch( (*( sm->p)) ) {
		case 76: goto st323;
		case 108: goto st323;
	}
	goto tr239;
st323:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof323;
case 323:
	if ( (*( sm->p)) == 32 )
		goto st324;
	goto tr239;
st324:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof324;
case 324:
	if ( (*( sm->p)) == 35 )
		goto st325;
	goto tr239;
st325:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof325;
case 325:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr359;
	goto tr239;
tr359:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st786;
st786:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof786;
case 786:
#line 5945 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st786;
	goto tr921;
st326:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof326;
case 326:
	switch( (*( sm->p)) ) {
		case 84: goto st327;
		case 116: goto st327;
	}
	goto tr239;
st327:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof327;
case 327:
	if ( (*( sm->p)) == 32 )
		goto st328;
	goto tr239;
st328:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof328;
case 328:
	switch( (*( sm->p)) ) {
		case 35: goto st329;
		case 67: goto st330;
		case 99: goto st330;
	}
	goto tr239;
st329:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof329;
case 329:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr364;
	goto tr239;
tr364:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st787;
st787:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof787;
case 787:
#line 5988 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st787;
	goto tr923;
st330:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof330;
case 330:
	switch( (*( sm->p)) ) {
		case 72: goto st331;
		case 104: goto st331;
	}
	goto tr239;
st331:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof331;
case 331:
	switch( (*( sm->p)) ) {
		case 65: goto st332;
		case 97: goto st332;
	}
	goto tr239;
st332:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof332;
case 332:
	switch( (*( sm->p)) ) {
		case 78: goto st333;
		case 110: goto st333;
	}
	goto tr239;
st333:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof333;
case 333:
	switch( (*( sm->p)) ) {
		case 71: goto st334;
		case 103: goto st334;
	}
	goto tr239;
st334:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof334;
case 334:
	switch( (*( sm->p)) ) {
		case 69: goto st335;
		case 101: goto st335;
	}
	goto tr239;
st335:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof335;
case 335:
	switch( (*( sm->p)) ) {
		case 83: goto st336;
		case 115: goto st336;
	}
	goto tr239;
st336:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof336;
case 336:
	if ( (*( sm->p)) == 32 )
		goto st337;
	goto tr239;
st337:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof337;
case 337:
	if ( (*( sm->p)) == 35 )
		goto st338;
	goto tr239;
st338:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof338;
case 338:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr373;
	goto tr239;
tr373:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st788;
st788:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof788;
case 788:
#line 6073 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st788;
	goto tr925;
tr858:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st789;
st789:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof789;
case 789:
#line 6083 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st339;
		case 101: goto st339;
	}
	goto tr881;
st339:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof339;
case 339:
	switch( (*( sm->p)) ) {
		case 67: goto st340;
		case 99: goto st340;
	}
	goto tr239;
st340:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof340;
case 340:
	switch( (*( sm->p)) ) {
		case 79: goto st341;
		case 111: goto st341;
	}
	goto tr239;
st341:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof341;
case 341:
	switch( (*( sm->p)) ) {
		case 82: goto st342;
		case 114: goto st342;
	}
	goto tr239;
st342:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof342;
case 342:
	switch( (*( sm->p)) ) {
		case 68: goto st343;
		case 100: goto st343;
	}
	goto tr239;
st343:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof343;
case 343:
	if ( (*( sm->p)) == 32 )
		goto st344;
	goto tr239;
st344:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof344;
case 344:
	if ( (*( sm->p)) == 35 )
		goto st345;
	goto tr239;
st345:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof345;
case 345:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr380;
	goto tr239;
tr380:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st790;
st790:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof790;
case 790:
#line 6152 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st790;
	goto tr928;
tr859:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st791;
st791:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof791;
case 791:
#line 6162 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st346;
		case 101: goto st346;
	}
	goto tr881;
st346:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof346;
case 346:
	switch( (*( sm->p)) ) {
		case 84: goto st347;
		case 116: goto st347;
	}
	goto tr239;
st347:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof347;
case 347:
	if ( (*( sm->p)) == 32 )
		goto st348;
	goto tr239;
st348:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof348;
case 348:
	if ( (*( sm->p)) == 35 )
		goto st349;
	goto tr239;
st349:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof349;
case 349:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr384;
	goto tr239;
tr384:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st792;
st792:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof792;
case 792:
#line 6204 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st792;
	goto tr931;
tr860:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st793;
st793:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof793;
case 793:
#line 6214 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st350;
		case 72: goto st368;
		case 73: goto st374;
		case 79: goto st381;
		case 97: goto st350;
		case 104: goto st368;
		case 105: goto st374;
		case 111: goto st381;
	}
	goto tr881;
st350:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof350;
case 350:
	switch( (*( sm->p)) ) {
		case 75: goto st351;
		case 107: goto st351;
	}
	goto tr239;
st351:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof351;
case 351:
	switch( (*( sm->p)) ) {
		case 69: goto st352;
		case 101: goto st352;
	}
	goto tr239;
st352:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof352;
case 352:
	switch( (*( sm->p)) ) {
		case 32: goto st353;
		case 68: goto st354;
		case 100: goto st354;
	}
	goto tr239;
st353:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof353;
case 353:
	switch( (*( sm->p)) ) {
		case 68: goto st354;
		case 100: goto st354;
	}
	goto tr239;
st354:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof354;
case 354:
	switch( (*( sm->p)) ) {
		case 79: goto st355;
		case 111: goto st355;
	}
	goto tr239;
st355:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof355;
case 355:
	switch( (*( sm->p)) ) {
		case 87: goto st356;
		case 119: goto st356;
	}
	goto tr239;
st356:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof356;
case 356:
	switch( (*( sm->p)) ) {
		case 78: goto st357;
		case 110: goto st357;
	}
	goto tr239;
st357:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof357;
case 357:
	if ( (*( sm->p)) == 32 )
		goto st358;
	goto tr239;
st358:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof358;
case 358:
	switch( (*( sm->p)) ) {
		case 35: goto st359;
		case 82: goto st360;
		case 114: goto st360;
	}
	goto tr239;
st359:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof359;
case 359:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr395;
	goto tr239;
tr395:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st794;
st794:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof794;
case 794:
#line 6320 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st794;
	goto tr937;
st360:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof360;
case 360:
	switch( (*( sm->p)) ) {
		case 69: goto st361;
		case 101: goto st361;
	}
	goto tr239;
st361:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof361;
case 361:
	switch( (*( sm->p)) ) {
		case 81: goto st362;
		case 113: goto st362;
	}
	goto tr239;
st362:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof362;
case 362:
	switch( (*( sm->p)) ) {
		case 85: goto st363;
		case 117: goto st363;
	}
	goto tr239;
st363:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof363;
case 363:
	switch( (*( sm->p)) ) {
		case 69: goto st364;
		case 101: goto st364;
	}
	goto tr239;
st364:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof364;
case 364:
	switch( (*( sm->p)) ) {
		case 83: goto st365;
		case 115: goto st365;
	}
	goto tr239;
st365:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof365;
case 365:
	switch( (*( sm->p)) ) {
		case 84: goto st366;
		case 116: goto st366;
	}
	goto tr239;
st366:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof366;
case 366:
	if ( (*( sm->p)) == 32 )
		goto st367;
	goto tr239;
st367:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof367;
case 367:
	if ( (*( sm->p)) == 35 )
		goto st359;
	goto tr239;
st368:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof368;
case 368:
	switch( (*( sm->p)) ) {
		case 85: goto st369;
		case 117: goto st369;
	}
	goto tr239;
st369:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof369;
case 369:
	switch( (*( sm->p)) ) {
		case 77: goto st370;
		case 109: goto st370;
	}
	goto tr239;
st370:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof370;
case 370:
	switch( (*( sm->p)) ) {
		case 66: goto st371;
		case 98: goto st371;
	}
	goto tr239;
st371:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof371;
case 371:
	if ( (*( sm->p)) == 32 )
		goto st372;
	goto tr239;
st372:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof372;
case 372:
	if ( (*( sm->p)) == 35 )
		goto st373;
	goto tr239;
st373:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof373;
case 373:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr408;
	goto tr239;
tr408:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st795;
st795:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof795;
case 795:
#line 6446 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st795;
	goto tr939;
st374:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof374;
case 374:
	switch( (*( sm->p)) ) {
		case 67: goto st375;
		case 99: goto st375;
	}
	goto tr239;
st375:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof375;
case 375:
	switch( (*( sm->p)) ) {
		case 75: goto st376;
		case 107: goto st376;
	}
	goto tr239;
st376:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof376;
case 376:
	switch( (*( sm->p)) ) {
		case 69: goto st377;
		case 101: goto st377;
	}
	goto tr239;
st377:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof377;
case 377:
	switch( (*( sm->p)) ) {
		case 84: goto st378;
		case 116: goto st378;
	}
	goto tr239;
st378:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof378;
case 378:
	if ( (*( sm->p)) == 32 )
		goto st379;
	goto tr239;
st379:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof379;
case 379:
	if ( (*( sm->p)) == 35 )
		goto st380;
	goto tr239;
st380:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof380;
case 380:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr415;
	goto tr239;
tr415:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st796;
st796:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof796;
case 796:
#line 6513 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st796;
	goto tr941;
st381:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof381;
case 381:
	switch( (*( sm->p)) ) {
		case 80: goto st382;
		case 112: goto st382;
	}
	goto tr239;
st382:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof382;
case 382:
	switch( (*( sm->p)) ) {
		case 73: goto st383;
		case 105: goto st383;
	}
	goto tr239;
st383:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof383;
case 383:
	switch( (*( sm->p)) ) {
		case 67: goto st384;
		case 99: goto st384;
	}
	goto tr239;
st384:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof384;
case 384:
	if ( (*( sm->p)) == 32 )
		goto st385;
	goto tr239;
st385:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof385;
case 385:
	if ( (*( sm->p)) == 35 )
		goto st386;
	goto tr239;
st386:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof386;
case 386:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr421;
	goto tr239;
tr421:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st797;
st797:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof797;
case 797:
#line 6571 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st797;
	goto tr943;
tr861:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st798;
st798:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof798;
case 798:
#line 6581 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 83: goto st387;
		case 115: goto st387;
	}
	goto tr881;
st387:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof387;
case 387:
	switch( (*( sm->p)) ) {
		case 69: goto st388;
		case 101: goto st388;
	}
	goto tr239;
st388:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof388;
case 388:
	switch( (*( sm->p)) ) {
		case 82: goto st389;
		case 114: goto st389;
	}
	goto tr239;
st389:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof389;
case 389:
	if ( (*( sm->p)) == 32 )
		goto st390;
	goto tr239;
st390:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof390;
case 390:
	if ( (*( sm->p)) == 35 )
		goto st391;
	goto tr239;
st391:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof391;
case 391:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr426;
	goto tr239;
tr426:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st799;
st799:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof799;
case 799:
#line 6632 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st799;
	goto tr946;
tr862:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st800;
st800:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof800;
case 800:
#line 6642 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 73: goto st392;
		case 105: goto st392;
	}
	goto tr881;
st392:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof392;
case 392:
	switch( (*( sm->p)) ) {
		case 75: goto st393;
		case 107: goto st393;
	}
	goto tr239;
st393:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof393;
case 393:
	switch( (*( sm->p)) ) {
		case 73: goto st394;
		case 105: goto st394;
	}
	goto tr239;
st394:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof394;
case 394:
	if ( (*( sm->p)) == 32 )
		goto st395;
	goto tr239;
st395:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof395;
case 395:
	if ( (*( sm->p)) == 35 )
		goto st396;
	goto tr239;
st396:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof396;
case 396:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr431;
	goto tr239;
tr431:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st801;
st801:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof801;
case 801:
#line 6693 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st801;
	goto tr949;
tr863:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 557 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 81;}
	goto st802;
st802:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof802;
case 802:
#line 6704 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 35: goto st397;
		case 47: goto st399;
		case 66: goto st420;
		case 67: goto st421;
		case 73: goto st539;
		case 81: goto st540;
		case 83: goto st656;
		case 84: goto st684;
		case 85: goto st689;
		case 91: goto st690;
		case 98: goto st420;
		case 99: goto st421;
		case 105: goto st539;
		case 113: goto st540;
		case 115: goto st656;
		case 116: goto st684;
		case 117: goto st689;
	}
	goto tr881;
st397:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof397;
case 397:
	switch( (*( sm->p)) ) {
		case 45: goto tr432;
		case 95: goto tr432;
	}
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto tr432;
	} else if ( (*( sm->p)) > 90 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
			goto tr432;
	} else
		goto tr432;
	goto tr239;
tr432:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st398;
st398:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof398;
case 398:
#line 6748 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 45: goto st398;
		case 93: goto tr434;
		case 95: goto st398;
	}
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st398;
	} else if ( (*( sm->p)) > 90 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
			goto st398;
	} else
		goto st398;
	goto tr239;
st399:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof399;
case 399:
	switch( (*( sm->p)) ) {
		case 66: goto st400;
		case 67: goto st401;
		case 73: goto st408;
		case 81: goto st199;
		case 83: goto st409;
		case 84: goto st413;
		case 85: goto st419;
		case 98: goto st400;
		case 99: goto st401;
		case 105: goto st408;
		case 113: goto st199;
		case 115: goto st409;
		case 116: goto st413;
		case 117: goto st419;
	}
	goto tr239;
st400:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof400;
case 400:
	if ( (*( sm->p)) == 93 )
		goto tr441;
	goto tr239;
st401:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof401;
case 401:
	switch( (*( sm->p)) ) {
		case 79: goto st402;
		case 111: goto st402;
	}
	goto tr239;
st402:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof402;
case 402:
	switch( (*( sm->p)) ) {
		case 68: goto st403;
		case 76: goto st405;
		case 100: goto st403;
		case 108: goto st405;
	}
	goto tr239;
st403:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof403;
case 403:
	switch( (*( sm->p)) ) {
		case 69: goto st404;
		case 101: goto st404;
	}
	goto tr239;
st404:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof404;
case 404:
	if ( (*( sm->p)) == 93 )
		goto st803;
	goto tr239;
st803:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof803;
case 803:
	if ( (*( sm->p)) == 32 )
		goto st803;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st803;
	goto tr961;
st405:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof405;
case 405:
	switch( (*( sm->p)) ) {
		case 79: goto st406;
		case 111: goto st406;
	}
	goto tr239;
st406:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof406;
case 406:
	switch( (*( sm->p)) ) {
		case 82: goto st407;
		case 114: goto st407;
	}
	goto tr239;
st407:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof407;
case 407:
	if ( (*( sm->p)) == 93 )
		goto tr449;
	goto tr239;
st408:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof408;
case 408:
	if ( (*( sm->p)) == 93 )
		goto tr450;
	goto tr239;
st409:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof409;
case 409:
	switch( (*( sm->p)) ) {
		case 69: goto st205;
		case 80: goto st184;
		case 85: goto st410;
		case 93: goto tr452;
		case 101: goto st205;
		case 112: goto st184;
		case 117: goto st410;
	}
	goto tr239;
st410:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof410;
case 410:
	switch( (*( sm->p)) ) {
		case 66: goto st411;
		case 80: goto st412;
		case 98: goto st411;
		case 112: goto st412;
	}
	goto tr239;
st411:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof411;
case 411:
	if ( (*( sm->p)) == 93 )
		goto tr455;
	goto tr239;
st412:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof412;
case 412:
	if ( (*( sm->p)) == 93 )
		goto tr456;
	goto tr239;
st413:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof413;
case 413:
	switch( (*( sm->p)) ) {
		case 65: goto st414;
		case 68: goto st192;
		case 72: goto st418;
		case 97: goto st414;
		case 100: goto st192;
		case 104: goto st418;
	}
	goto tr239;
st414:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof414;
case 414:
	switch( (*( sm->p)) ) {
		case 66: goto st415;
		case 98: goto st415;
	}
	goto tr239;
st415:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof415;
case 415:
	switch( (*( sm->p)) ) {
		case 76: goto st416;
		case 108: goto st416;
	}
	goto tr239;
st416:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof416;
case 416:
	switch( (*( sm->p)) ) {
		case 69: goto st417;
		case 101: goto st417;
	}
	goto tr239;
st417:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof417;
case 417:
	if ( (*( sm->p)) == 93 )
		goto st804;
	goto tr239;
st804:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof804;
case 804:
	if ( (*( sm->p)) == 32 )
		goto st804;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st804;
	goto tr962;
st418:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof418;
case 418:
	if ( (*( sm->p)) == 93 )
		goto tr463;
	goto tr239;
st419:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof419;
case 419:
	if ( (*( sm->p)) == 93 )
		goto tr464;
	goto tr239;
st420:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof420;
case 420:
	if ( (*( sm->p)) == 93 )
		goto tr465;
	goto tr239;
st421:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof421;
case 421:
	switch( (*( sm->p)) ) {
		case 79: goto st422;
		case 111: goto st422;
	}
	goto tr239;
st422:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof422;
case 422:
	switch( (*( sm->p)) ) {
		case 68: goto st423;
		case 76: goto st425;
		case 100: goto st423;
		case 108: goto st425;
	}
	goto tr239;
st423:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof423;
case 423:
	switch( (*( sm->p)) ) {
		case 69: goto st424;
		case 101: goto st424;
	}
	goto tr239;
st424:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof424;
case 424:
	if ( (*( sm->p)) == 93 )
		goto tr470;
	goto tr239;
st425:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof425;
case 425:
	switch( (*( sm->p)) ) {
		case 79: goto st426;
		case 111: goto st426;
	}
	goto tr239;
st426:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof426;
case 426:
	switch( (*( sm->p)) ) {
		case 82: goto st427;
		case 114: goto st427;
	}
	goto tr239;
st427:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof427;
case 427:
	if ( (*( sm->p)) == 61 )
		goto st428;
	goto tr239;
st428:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof428;
case 428:
	switch( (*( sm->p)) ) {
		case 35: goto tr474;
		case 65: goto tr475;
		case 67: goto tr476;
		case 71: goto tr477;
		case 73: goto tr478;
		case 76: goto tr479;
		case 77: goto tr480;
		case 83: goto tr481;
		case 97: goto tr482;
		case 99: goto tr484;
		case 103: goto tr485;
		case 105: goto tr486;
		case 108: goto tr487;
		case 109: goto tr488;
		case 115: goto tr489;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto tr483;
	goto tr239;
tr474:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st429;
st429:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof429;
case 429:
#line 7075 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st430;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st430;
	} else
		goto st430;
	goto tr239;
st430:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof430;
case 430:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st431;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st431;
	} else
		goto st431;
	goto tr239;
st431:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof431;
case 431:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st432;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st432;
	} else
		goto st432;
	goto tr239;
st432:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof432;
case 432:
	if ( (*( sm->p)) == 93 )
		goto tr494;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st433;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st433;
	} else
		goto st433;
	goto tr239;
st433:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof433;
case 433:
	if ( (*( sm->p)) == 93 )
		goto tr494;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st434;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st434;
	} else
		goto st434;
	goto tr239;
st434:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof434;
case 434:
	if ( (*( sm->p)) == 93 )
		goto tr494;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st435;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st435;
	} else
		goto st435;
	goto tr239;
st435:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof435;
case 435:
	if ( (*( sm->p)) == 93 )
		goto tr494;
	goto tr239;
tr475:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st436;
st436:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof436;
case 436:
#line 7169 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st437;
		case 114: goto st437;
	}
	goto tr239;
st437:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof437;
case 437:
	switch( (*( sm->p)) ) {
		case 84: goto st438;
		case 116: goto st438;
	}
	goto tr239;
st438:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof438;
case 438:
	switch( (*( sm->p)) ) {
		case 73: goto st439;
		case 93: goto tr500;
		case 105: goto st439;
	}
	goto tr239;
st439:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof439;
case 439:
	switch( (*( sm->p)) ) {
		case 83: goto st440;
		case 115: goto st440;
	}
	goto tr239;
st440:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof440;
case 440:
	switch( (*( sm->p)) ) {
		case 84: goto st441;
		case 116: goto st441;
	}
	goto tr239;
st441:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof441;
case 441:
	if ( (*( sm->p)) == 93 )
		goto tr500;
	goto tr239;
tr476:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st442;
st442:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof442;
case 442:
#line 7225 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st443;
		case 79: goto st450;
		case 104: goto st443;
		case 111: goto st450;
	}
	goto tr239;
st443:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof443;
case 443:
	switch( (*( sm->p)) ) {
		case 65: goto st444;
		case 97: goto st444;
	}
	goto tr239;
st444:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof444;
case 444:
	switch( (*( sm->p)) ) {
		case 82: goto st445;
		case 114: goto st445;
	}
	goto tr239;
st445:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof445;
case 445:
	switch( (*( sm->p)) ) {
		case 65: goto st446;
		case 93: goto tr500;
		case 97: goto st446;
	}
	goto tr239;
st446:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof446;
case 446:
	switch( (*( sm->p)) ) {
		case 67: goto st447;
		case 99: goto st447;
	}
	goto tr239;
st447:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof447;
case 447:
	switch( (*( sm->p)) ) {
		case 84: goto st448;
		case 116: goto st448;
	}
	goto tr239;
st448:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof448;
case 448:
	switch( (*( sm->p)) ) {
		case 69: goto st449;
		case 101: goto st449;
	}
	goto tr239;
st449:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof449;
case 449:
	switch( (*( sm->p)) ) {
		case 82: goto st441;
		case 114: goto st441;
	}
	goto tr239;
st450:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof450;
case 450:
	switch( (*( sm->p)) ) {
		case 78: goto st451;
		case 80: goto st458;
		case 110: goto st451;
		case 112: goto st458;
	}
	goto tr239;
st451:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof451;
case 451:
	switch( (*( sm->p)) ) {
		case 84: goto st452;
		case 116: goto st452;
	}
	goto tr239;
st452:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof452;
case 452:
	switch( (*( sm->p)) ) {
		case 82: goto st453;
		case 93: goto tr500;
		case 114: goto st453;
	}
	goto tr239;
st453:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof453;
case 453:
	switch( (*( sm->p)) ) {
		case 73: goto st454;
		case 105: goto st454;
	}
	goto tr239;
st454:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof454;
case 454:
	switch( (*( sm->p)) ) {
		case 66: goto st455;
		case 98: goto st455;
	}
	goto tr239;
st455:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof455;
case 455:
	switch( (*( sm->p)) ) {
		case 85: goto st456;
		case 117: goto st456;
	}
	goto tr239;
st456:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof456;
case 456:
	switch( (*( sm->p)) ) {
		case 84: goto st457;
		case 116: goto st457;
	}
	goto tr239;
st457:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof457;
case 457:
	switch( (*( sm->p)) ) {
		case 79: goto st449;
		case 111: goto st449;
	}
	goto tr239;
st458:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof458;
case 458:
	switch( (*( sm->p)) ) {
		case 89: goto st459;
		case 121: goto st459;
	}
	goto tr239;
st459:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof459;
case 459:
	switch( (*( sm->p)) ) {
		case 82: goto st460;
		case 93: goto tr500;
		case 114: goto st460;
	}
	goto tr239;
st460:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof460;
case 460:
	switch( (*( sm->p)) ) {
		case 73: goto st461;
		case 105: goto st461;
	}
	goto tr239;
st461:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof461;
case 461:
	switch( (*( sm->p)) ) {
		case 71: goto st462;
		case 103: goto st462;
	}
	goto tr239;
st462:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof462;
case 462:
	switch( (*( sm->p)) ) {
		case 72: goto st440;
		case 104: goto st440;
	}
	goto tr239;
tr477:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st463;
st463:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof463;
case 463:
#line 7424 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st464;
		case 101: goto st464;
	}
	goto tr239;
st464:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof464;
case 464:
	switch( (*( sm->p)) ) {
		case 78: goto st465;
		case 110: goto st465;
	}
	goto tr239;
st465:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof465;
case 465:
	switch( (*( sm->p)) ) {
		case 69: goto st466;
		case 93: goto tr500;
		case 101: goto st466;
	}
	goto tr239;
st466:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof466;
case 466:
	switch( (*( sm->p)) ) {
		case 82: goto st467;
		case 114: goto st467;
	}
	goto tr239;
st467:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof467;
case 467:
	switch( (*( sm->p)) ) {
		case 65: goto st468;
		case 97: goto st468;
	}
	goto tr239;
st468:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof468;
case 468:
	switch( (*( sm->p)) ) {
		case 76: goto st441;
		case 108: goto st441;
	}
	goto tr239;
tr478:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st469;
st469:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof469;
case 469:
#line 7482 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st470;
		case 110: goto st470;
	}
	goto tr239;
st470:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof470;
case 470:
	switch( (*( sm->p)) ) {
		case 86: goto st471;
		case 118: goto st471;
	}
	goto tr239;
st471:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof471;
case 471:
	switch( (*( sm->p)) ) {
		case 65: goto st472;
		case 93: goto tr500;
		case 97: goto st472;
	}
	goto tr239;
st472:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof472;
case 472:
	switch( (*( sm->p)) ) {
		case 76: goto st473;
		case 108: goto st473;
	}
	goto tr239;
st473:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof473;
case 473:
	switch( (*( sm->p)) ) {
		case 73: goto st474;
		case 105: goto st474;
	}
	goto tr239;
st474:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof474;
case 474:
	switch( (*( sm->p)) ) {
		case 68: goto st441;
		case 100: goto st441;
	}
	goto tr239;
tr479:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st475;
st475:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof475;
case 475:
#line 7540 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st476;
		case 111: goto st476;
	}
	goto tr239;
st476:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof476;
case 476:
	switch( (*( sm->p)) ) {
		case 82: goto st477;
		case 114: goto st477;
	}
	goto tr239;
st477:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof477;
case 477:
	switch( (*( sm->p)) ) {
		case 69: goto st441;
		case 93: goto tr500;
		case 101: goto st441;
	}
	goto tr239;
tr480:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st478;
st478:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof478;
case 478:
#line 7571 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st479;
		case 101: goto st479;
	}
	goto tr239;
st479:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof479;
case 479:
	switch( (*( sm->p)) ) {
		case 84: goto st480;
		case 116: goto st480;
	}
	goto tr239;
st480:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof480;
case 480:
	switch( (*( sm->p)) ) {
		case 65: goto st441;
		case 97: goto st441;
	}
	goto tr239;
tr481:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st481;
st481:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof481;
case 481:
#line 7601 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st482;
		case 112: goto st482;
	}
	goto tr239;
st482:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof482;
case 482:
	switch( (*( sm->p)) ) {
		case 69: goto st483;
		case 101: goto st483;
	}
	goto tr239;
st483:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof483;
case 483:
	switch( (*( sm->p)) ) {
		case 67: goto st484;
		case 99: goto st484;
	}
	goto tr239;
st484:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof484;
case 484:
	switch( (*( sm->p)) ) {
		case 73: goto st485;
		case 93: goto tr500;
		case 105: goto st485;
	}
	goto tr239;
st485:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof485;
case 485:
	switch( (*( sm->p)) ) {
		case 69: goto st486;
		case 101: goto st486;
	}
	goto tr239;
st486:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof486;
case 486:
	switch( (*( sm->p)) ) {
		case 83: goto st441;
		case 115: goto st441;
	}
	goto tr239;
tr482:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st487;
st487:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof487;
case 487:
#line 7659 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st437;
		case 93: goto tr494;
		case 114: goto st489;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
tr483:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st488;
st488:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof488;
case 488:
#line 7674 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr494;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st489:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof489;
case 489:
	switch( (*( sm->p)) ) {
		case 84: goto st438;
		case 93: goto tr494;
		case 116: goto st490;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st490:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof490;
case 490:
	switch( (*( sm->p)) ) {
		case 73: goto st439;
		case 93: goto tr500;
		case 105: goto st491;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st491:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof491;
case 491:
	switch( (*( sm->p)) ) {
		case 83: goto st440;
		case 93: goto tr494;
		case 115: goto st492;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st492:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof492;
case 492:
	switch( (*( sm->p)) ) {
		case 84: goto st441;
		case 93: goto tr494;
		case 116: goto st493;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st493:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof493;
case 493:
	if ( (*( sm->p)) == 93 )
		goto tr500;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
tr484:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st494;
st494:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof494;
case 494:
#line 7743 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st443;
		case 79: goto st450;
		case 93: goto tr494;
		case 104: goto st495;
		case 111: goto st502;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st495:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof495;
case 495:
	switch( (*( sm->p)) ) {
		case 65: goto st444;
		case 93: goto tr494;
		case 97: goto st496;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st496:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof496;
case 496:
	switch( (*( sm->p)) ) {
		case 82: goto st445;
		case 93: goto tr494;
		case 114: goto st497;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st497:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof497;
case 497:
	switch( (*( sm->p)) ) {
		case 65: goto st446;
		case 93: goto tr500;
		case 97: goto st498;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st498:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof498;
case 498:
	switch( (*( sm->p)) ) {
		case 67: goto st447;
		case 93: goto tr494;
		case 99: goto st499;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st499:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof499;
case 499:
	switch( (*( sm->p)) ) {
		case 84: goto st448;
		case 93: goto tr494;
		case 116: goto st500;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st500:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof500;
case 500:
	switch( (*( sm->p)) ) {
		case 69: goto st449;
		case 93: goto tr494;
		case 101: goto st501;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st501:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof501;
case 501:
	switch( (*( sm->p)) ) {
		case 82: goto st441;
		case 93: goto tr494;
		case 114: goto st493;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st502:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof502;
case 502:
	switch( (*( sm->p)) ) {
		case 78: goto st451;
		case 80: goto st458;
		case 93: goto tr494;
		case 110: goto st503;
		case 112: goto st510;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st503:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof503;
case 503:
	switch( (*( sm->p)) ) {
		case 84: goto st452;
		case 93: goto tr494;
		case 116: goto st504;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st504:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof504;
case 504:
	switch( (*( sm->p)) ) {
		case 82: goto st453;
		case 93: goto tr500;
		case 114: goto st505;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st505:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof505;
case 505:
	switch( (*( sm->p)) ) {
		case 73: goto st454;
		case 93: goto tr494;
		case 105: goto st506;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st506:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof506;
case 506:
	switch( (*( sm->p)) ) {
		case 66: goto st455;
		case 93: goto tr494;
		case 98: goto st507;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st507:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof507;
case 507:
	switch( (*( sm->p)) ) {
		case 85: goto st456;
		case 93: goto tr494;
		case 117: goto st508;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st508:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof508;
case 508:
	switch( (*( sm->p)) ) {
		case 84: goto st457;
		case 93: goto tr494;
		case 116: goto st509;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st509:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof509;
case 509:
	switch( (*( sm->p)) ) {
		case 79: goto st449;
		case 93: goto tr494;
		case 111: goto st501;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st510:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof510;
case 510:
	switch( (*( sm->p)) ) {
		case 89: goto st459;
		case 93: goto tr494;
		case 121: goto st511;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st511:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof511;
case 511:
	switch( (*( sm->p)) ) {
		case 82: goto st460;
		case 93: goto tr500;
		case 114: goto st512;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st512:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof512;
case 512:
	switch( (*( sm->p)) ) {
		case 73: goto st461;
		case 93: goto tr494;
		case 105: goto st513;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st513:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof513;
case 513:
	switch( (*( sm->p)) ) {
		case 71: goto st462;
		case 93: goto tr494;
		case 103: goto st514;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st514:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof514;
case 514:
	switch( (*( sm->p)) ) {
		case 72: goto st440;
		case 93: goto tr494;
		case 104: goto st492;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
tr485:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st515;
st515:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof515;
case 515:
#line 8002 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st464;
		case 93: goto tr494;
		case 101: goto st516;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st516:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof516;
case 516:
	switch( (*( sm->p)) ) {
		case 78: goto st465;
		case 93: goto tr494;
		case 110: goto st517;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st517:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof517;
case 517:
	switch( (*( sm->p)) ) {
		case 69: goto st466;
		case 93: goto tr500;
		case 101: goto st518;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st518:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof518;
case 518:
	switch( (*( sm->p)) ) {
		case 82: goto st467;
		case 93: goto tr494;
		case 114: goto st519;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st519:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof519;
case 519:
	switch( (*( sm->p)) ) {
		case 65: goto st468;
		case 93: goto tr494;
		case 97: goto st520;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st520:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof520;
case 520:
	switch( (*( sm->p)) ) {
		case 76: goto st441;
		case 93: goto tr494;
		case 108: goto st493;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
tr486:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st521;
st521:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof521;
case 521:
#line 8077 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st470;
		case 93: goto tr494;
		case 110: goto st522;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st522:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof522;
case 522:
	switch( (*( sm->p)) ) {
		case 86: goto st471;
		case 93: goto tr494;
		case 118: goto st523;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st523:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof523;
case 523:
	switch( (*( sm->p)) ) {
		case 65: goto st472;
		case 93: goto tr500;
		case 97: goto st524;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st524:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof524;
case 524:
	switch( (*( sm->p)) ) {
		case 76: goto st473;
		case 93: goto tr494;
		case 108: goto st525;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st525:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof525;
case 525:
	switch( (*( sm->p)) ) {
		case 73: goto st474;
		case 93: goto tr494;
		case 105: goto st526;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st526:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof526;
case 526:
	switch( (*( sm->p)) ) {
		case 68: goto st441;
		case 93: goto tr494;
		case 100: goto st493;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
tr487:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st527;
st527:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof527;
case 527:
#line 8152 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st476;
		case 93: goto tr494;
		case 111: goto st528;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st528:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof528;
case 528:
	switch( (*( sm->p)) ) {
		case 82: goto st477;
		case 93: goto tr494;
		case 114: goto st529;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st529:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof529;
case 529:
	switch( (*( sm->p)) ) {
		case 69: goto st441;
		case 93: goto tr500;
		case 101: goto st493;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
tr488:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st530;
st530:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof530;
case 530:
#line 8191 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st479;
		case 93: goto tr494;
		case 101: goto st531;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st531:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof531;
case 531:
	switch( (*( sm->p)) ) {
		case 84: goto st480;
		case 93: goto tr494;
		case 116: goto st532;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st532:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof532;
case 532:
	switch( (*( sm->p)) ) {
		case 65: goto st441;
		case 93: goto tr494;
		case 97: goto st493;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
tr489:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st533;
st533:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof533;
case 533:
#line 8230 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st482;
		case 93: goto tr494;
		case 112: goto st534;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st534:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof534;
case 534:
	switch( (*( sm->p)) ) {
		case 69: goto st483;
		case 93: goto tr494;
		case 101: goto st535;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st535:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof535;
case 535:
	switch( (*( sm->p)) ) {
		case 67: goto st484;
		case 93: goto tr494;
		case 99: goto st536;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st536:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof536;
case 536:
	switch( (*( sm->p)) ) {
		case 73: goto st485;
		case 93: goto tr500;
		case 105: goto st537;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st537:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof537;
case 537:
	switch( (*( sm->p)) ) {
		case 69: goto st486;
		case 93: goto tr494;
		case 101: goto st538;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st538:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof538;
case 538:
	switch( (*( sm->p)) ) {
		case 83: goto st441;
		case 93: goto tr494;
		case 115: goto st493;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st488;
	goto tr239;
st539:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof539;
case 539:
	if ( (*( sm->p)) == 93 )
		goto tr587;
	goto tr239;
st540:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof540;
case 540:
	switch( (*( sm->p)) ) {
		case 85: goto st541;
		case 117: goto st541;
	}
	goto tr239;
st541:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof541;
case 541:
	switch( (*( sm->p)) ) {
		case 79: goto st542;
		case 111: goto st542;
	}
	goto tr239;
st542:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof542;
case 542:
	switch( (*( sm->p)) ) {
		case 84: goto st543;
		case 116: goto st543;
	}
	goto tr239;
st543:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof543;
case 543:
	switch( (*( sm->p)) ) {
		case 69: goto st544;
		case 101: goto st544;
	}
	goto tr239;
st544:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof544;
case 544:
	switch( (*( sm->p)) ) {
		case 61: goto st545;
		case 93: goto tr593;
	}
	goto tr239;
st545:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof545;
case 545:
	switch( (*( sm->p)) ) {
		case 35: goto tr594;
		case 65: goto tr595;
		case 67: goto tr596;
		case 71: goto tr597;
		case 73: goto tr598;
		case 76: goto tr599;
		case 77: goto tr600;
		case 83: goto tr601;
		case 97: goto tr602;
		case 99: goto tr604;
		case 103: goto tr605;
		case 105: goto tr606;
		case 108: goto tr607;
		case 109: goto tr608;
		case 115: goto tr609;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto tr603;
	goto tr239;
tr594:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st546;
st546:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof546;
case 546:
#line 8381 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st547;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st547;
	} else
		goto st547;
	goto tr239;
st547:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof547;
case 547:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st548;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st548;
	} else
		goto st548;
	goto tr239;
st548:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof548;
case 548:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st549;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st549;
	} else
		goto st549;
	goto tr239;
st549:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof549;
case 549:
	if ( (*( sm->p)) == 93 )
		goto tr614;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st550;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st550;
	} else
		goto st550;
	goto tr239;
st550:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof550;
case 550:
	if ( (*( sm->p)) == 93 )
		goto tr614;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st551;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st551;
	} else
		goto st551;
	goto tr239;
st551:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof551;
case 551:
	if ( (*( sm->p)) == 93 )
		goto tr614;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st552;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st552;
	} else
		goto st552;
	goto tr239;
st552:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof552;
case 552:
	if ( (*( sm->p)) == 93 )
		goto tr614;
	goto tr239;
tr595:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st553;
st553:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof553;
case 553:
#line 8475 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st554;
		case 114: goto st554;
	}
	goto tr239;
st554:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof554;
case 554:
	switch( (*( sm->p)) ) {
		case 84: goto st555;
		case 116: goto st555;
	}
	goto tr239;
st555:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof555;
case 555:
	switch( (*( sm->p)) ) {
		case 73: goto st556;
		case 93: goto tr620;
		case 105: goto st556;
	}
	goto tr239;
st556:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof556;
case 556:
	switch( (*( sm->p)) ) {
		case 83: goto st557;
		case 115: goto st557;
	}
	goto tr239;
st557:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof557;
case 557:
	switch( (*( sm->p)) ) {
		case 84: goto st558;
		case 116: goto st558;
	}
	goto tr239;
st558:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof558;
case 558:
	if ( (*( sm->p)) == 93 )
		goto tr620;
	goto tr239;
tr596:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st559;
st559:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof559;
case 559:
#line 8531 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st560;
		case 79: goto st567;
		case 104: goto st560;
		case 111: goto st567;
	}
	goto tr239;
st560:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof560;
case 560:
	switch( (*( sm->p)) ) {
		case 65: goto st561;
		case 97: goto st561;
	}
	goto tr239;
st561:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof561;
case 561:
	switch( (*( sm->p)) ) {
		case 82: goto st562;
		case 114: goto st562;
	}
	goto tr239;
st562:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof562;
case 562:
	switch( (*( sm->p)) ) {
		case 65: goto st563;
		case 93: goto tr620;
		case 97: goto st563;
	}
	goto tr239;
st563:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof563;
case 563:
	switch( (*( sm->p)) ) {
		case 67: goto st564;
		case 99: goto st564;
	}
	goto tr239;
st564:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof564;
case 564:
	switch( (*( sm->p)) ) {
		case 84: goto st565;
		case 116: goto st565;
	}
	goto tr239;
st565:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof565;
case 565:
	switch( (*( sm->p)) ) {
		case 69: goto st566;
		case 101: goto st566;
	}
	goto tr239;
st566:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof566;
case 566:
	switch( (*( sm->p)) ) {
		case 82: goto st558;
		case 114: goto st558;
	}
	goto tr239;
st567:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof567;
case 567:
	switch( (*( sm->p)) ) {
		case 78: goto st568;
		case 80: goto st575;
		case 110: goto st568;
		case 112: goto st575;
	}
	goto tr239;
st568:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof568;
case 568:
	switch( (*( sm->p)) ) {
		case 84: goto st569;
		case 116: goto st569;
	}
	goto tr239;
st569:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof569;
case 569:
	switch( (*( sm->p)) ) {
		case 82: goto st570;
		case 93: goto tr620;
		case 114: goto st570;
	}
	goto tr239;
st570:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof570;
case 570:
	switch( (*( sm->p)) ) {
		case 73: goto st571;
		case 105: goto st571;
	}
	goto tr239;
st571:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof571;
case 571:
	switch( (*( sm->p)) ) {
		case 66: goto st572;
		case 98: goto st572;
	}
	goto tr239;
st572:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof572;
case 572:
	switch( (*( sm->p)) ) {
		case 85: goto st573;
		case 117: goto st573;
	}
	goto tr239;
st573:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof573;
case 573:
	switch( (*( sm->p)) ) {
		case 84: goto st574;
		case 116: goto st574;
	}
	goto tr239;
st574:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof574;
case 574:
	switch( (*( sm->p)) ) {
		case 79: goto st566;
		case 111: goto st566;
	}
	goto tr239;
st575:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof575;
case 575:
	switch( (*( sm->p)) ) {
		case 89: goto st576;
		case 121: goto st576;
	}
	goto tr239;
st576:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof576;
case 576:
	switch( (*( sm->p)) ) {
		case 82: goto st577;
		case 93: goto tr620;
		case 114: goto st577;
	}
	goto tr239;
st577:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof577;
case 577:
	switch( (*( sm->p)) ) {
		case 73: goto st578;
		case 105: goto st578;
	}
	goto tr239;
st578:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof578;
case 578:
	switch( (*( sm->p)) ) {
		case 71: goto st579;
		case 103: goto st579;
	}
	goto tr239;
st579:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof579;
case 579:
	switch( (*( sm->p)) ) {
		case 72: goto st557;
		case 104: goto st557;
	}
	goto tr239;
tr597:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st580;
st580:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof580;
case 580:
#line 8730 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st581;
		case 101: goto st581;
	}
	goto tr239;
st581:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof581;
case 581:
	switch( (*( sm->p)) ) {
		case 78: goto st582;
		case 110: goto st582;
	}
	goto tr239;
st582:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof582;
case 582:
	switch( (*( sm->p)) ) {
		case 69: goto st583;
		case 93: goto tr620;
		case 101: goto st583;
	}
	goto tr239;
st583:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof583;
case 583:
	switch( (*( sm->p)) ) {
		case 82: goto st584;
		case 114: goto st584;
	}
	goto tr239;
st584:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof584;
case 584:
	switch( (*( sm->p)) ) {
		case 65: goto st585;
		case 97: goto st585;
	}
	goto tr239;
st585:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof585;
case 585:
	switch( (*( sm->p)) ) {
		case 76: goto st558;
		case 108: goto st558;
	}
	goto tr239;
tr598:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st586;
st586:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof586;
case 586:
#line 8788 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st587;
		case 110: goto st587;
	}
	goto tr239;
st587:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof587;
case 587:
	switch( (*( sm->p)) ) {
		case 86: goto st588;
		case 118: goto st588;
	}
	goto tr239;
st588:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof588;
case 588:
	switch( (*( sm->p)) ) {
		case 65: goto st589;
		case 93: goto tr620;
		case 97: goto st589;
	}
	goto tr239;
st589:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof589;
case 589:
	switch( (*( sm->p)) ) {
		case 76: goto st590;
		case 108: goto st590;
	}
	goto tr239;
st590:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof590;
case 590:
	switch( (*( sm->p)) ) {
		case 73: goto st591;
		case 105: goto st591;
	}
	goto tr239;
st591:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof591;
case 591:
	switch( (*( sm->p)) ) {
		case 68: goto st558;
		case 100: goto st558;
	}
	goto tr239;
tr599:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st592;
st592:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof592;
case 592:
#line 8846 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st593;
		case 111: goto st593;
	}
	goto tr239;
st593:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof593;
case 593:
	switch( (*( sm->p)) ) {
		case 82: goto st594;
		case 114: goto st594;
	}
	goto tr239;
st594:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof594;
case 594:
	switch( (*( sm->p)) ) {
		case 69: goto st558;
		case 93: goto tr620;
		case 101: goto st558;
	}
	goto tr239;
tr600:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st595;
st595:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof595;
case 595:
#line 8877 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st596;
		case 101: goto st596;
	}
	goto tr239;
st596:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof596;
case 596:
	switch( (*( sm->p)) ) {
		case 84: goto st597;
		case 116: goto st597;
	}
	goto tr239;
st597:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof597;
case 597:
	switch( (*( sm->p)) ) {
		case 65: goto st558;
		case 97: goto st558;
	}
	goto tr239;
tr601:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st598;
st598:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof598;
case 598:
#line 8907 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st599;
		case 112: goto st599;
	}
	goto tr239;
st599:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof599;
case 599:
	switch( (*( sm->p)) ) {
		case 69: goto st600;
		case 101: goto st600;
	}
	goto tr239;
st600:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof600;
case 600:
	switch( (*( sm->p)) ) {
		case 67: goto st601;
		case 99: goto st601;
	}
	goto tr239;
st601:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof601;
case 601:
	switch( (*( sm->p)) ) {
		case 73: goto st602;
		case 93: goto tr620;
		case 105: goto st602;
	}
	goto tr239;
st602:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof602;
case 602:
	switch( (*( sm->p)) ) {
		case 69: goto st603;
		case 101: goto st603;
	}
	goto tr239;
st603:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof603;
case 603:
	switch( (*( sm->p)) ) {
		case 83: goto st558;
		case 115: goto st558;
	}
	goto tr239;
tr602:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st604;
st604:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof604;
case 604:
#line 8965 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st554;
		case 93: goto tr614;
		case 114: goto st606;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
tr603:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st605;
st605:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof605;
case 605:
#line 8980 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr614;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st606:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof606;
case 606:
	switch( (*( sm->p)) ) {
		case 84: goto st555;
		case 93: goto tr614;
		case 116: goto st607;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st607:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof607;
case 607:
	switch( (*( sm->p)) ) {
		case 73: goto st556;
		case 93: goto tr620;
		case 105: goto st608;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st608:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof608;
case 608:
	switch( (*( sm->p)) ) {
		case 83: goto st557;
		case 93: goto tr614;
		case 115: goto st609;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st609:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof609;
case 609:
	switch( (*( sm->p)) ) {
		case 84: goto st558;
		case 93: goto tr614;
		case 116: goto st610;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st610:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof610;
case 610:
	if ( (*( sm->p)) == 93 )
		goto tr620;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
tr604:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st611;
st611:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof611;
case 611:
#line 9049 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st560;
		case 79: goto st567;
		case 93: goto tr614;
		case 104: goto st612;
		case 111: goto st619;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st612:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof612;
case 612:
	switch( (*( sm->p)) ) {
		case 65: goto st561;
		case 93: goto tr614;
		case 97: goto st613;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st613:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof613;
case 613:
	switch( (*( sm->p)) ) {
		case 82: goto st562;
		case 93: goto tr614;
		case 114: goto st614;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st614:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof614;
case 614:
	switch( (*( sm->p)) ) {
		case 65: goto st563;
		case 93: goto tr620;
		case 97: goto st615;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st615:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof615;
case 615:
	switch( (*( sm->p)) ) {
		case 67: goto st564;
		case 93: goto tr614;
		case 99: goto st616;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st616:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof616;
case 616:
	switch( (*( sm->p)) ) {
		case 84: goto st565;
		case 93: goto tr614;
		case 116: goto st617;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st617:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof617;
case 617:
	switch( (*( sm->p)) ) {
		case 69: goto st566;
		case 93: goto tr614;
		case 101: goto st618;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st618:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof618;
case 618:
	switch( (*( sm->p)) ) {
		case 82: goto st558;
		case 93: goto tr614;
		case 114: goto st610;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st619:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof619;
case 619:
	switch( (*( sm->p)) ) {
		case 78: goto st568;
		case 80: goto st575;
		case 93: goto tr614;
		case 110: goto st620;
		case 112: goto st627;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st620:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof620;
case 620:
	switch( (*( sm->p)) ) {
		case 84: goto st569;
		case 93: goto tr614;
		case 116: goto st621;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st621:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof621;
case 621:
	switch( (*( sm->p)) ) {
		case 82: goto st570;
		case 93: goto tr620;
		case 114: goto st622;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st622:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof622;
case 622:
	switch( (*( sm->p)) ) {
		case 73: goto st571;
		case 93: goto tr614;
		case 105: goto st623;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st623:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof623;
case 623:
	switch( (*( sm->p)) ) {
		case 66: goto st572;
		case 93: goto tr614;
		case 98: goto st624;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st624:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof624;
case 624:
	switch( (*( sm->p)) ) {
		case 85: goto st573;
		case 93: goto tr614;
		case 117: goto st625;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st625:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof625;
case 625:
	switch( (*( sm->p)) ) {
		case 84: goto st574;
		case 93: goto tr614;
		case 116: goto st626;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st626:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof626;
case 626:
	switch( (*( sm->p)) ) {
		case 79: goto st566;
		case 93: goto tr614;
		case 111: goto st618;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st627:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof627;
case 627:
	switch( (*( sm->p)) ) {
		case 89: goto st576;
		case 93: goto tr614;
		case 121: goto st628;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st628:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof628;
case 628:
	switch( (*( sm->p)) ) {
		case 82: goto st577;
		case 93: goto tr620;
		case 114: goto st629;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st629:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof629;
case 629:
	switch( (*( sm->p)) ) {
		case 73: goto st578;
		case 93: goto tr614;
		case 105: goto st630;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st630:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof630;
case 630:
	switch( (*( sm->p)) ) {
		case 71: goto st579;
		case 93: goto tr614;
		case 103: goto st631;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st631:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof631;
case 631:
	switch( (*( sm->p)) ) {
		case 72: goto st557;
		case 93: goto tr614;
		case 104: goto st609;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
tr605:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st632;
st632:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof632;
case 632:
#line 9308 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st581;
		case 93: goto tr614;
		case 101: goto st633;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st633:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof633;
case 633:
	switch( (*( sm->p)) ) {
		case 78: goto st582;
		case 93: goto tr614;
		case 110: goto st634;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st634:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof634;
case 634:
	switch( (*( sm->p)) ) {
		case 69: goto st583;
		case 93: goto tr620;
		case 101: goto st635;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st635:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof635;
case 635:
	switch( (*( sm->p)) ) {
		case 82: goto st584;
		case 93: goto tr614;
		case 114: goto st636;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st636:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof636;
case 636:
	switch( (*( sm->p)) ) {
		case 65: goto st585;
		case 93: goto tr614;
		case 97: goto st637;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st637:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof637;
case 637:
	switch( (*( sm->p)) ) {
		case 76: goto st558;
		case 93: goto tr614;
		case 108: goto st610;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
tr606:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st638;
st638:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof638;
case 638:
#line 9383 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st587;
		case 93: goto tr614;
		case 110: goto st639;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st639:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof639;
case 639:
	switch( (*( sm->p)) ) {
		case 86: goto st588;
		case 93: goto tr614;
		case 118: goto st640;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st640:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof640;
case 640:
	switch( (*( sm->p)) ) {
		case 65: goto st589;
		case 93: goto tr620;
		case 97: goto st641;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st641:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof641;
case 641:
	switch( (*( sm->p)) ) {
		case 76: goto st590;
		case 93: goto tr614;
		case 108: goto st642;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st642:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof642;
case 642:
	switch( (*( sm->p)) ) {
		case 73: goto st591;
		case 93: goto tr614;
		case 105: goto st643;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st643:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof643;
case 643:
	switch( (*( sm->p)) ) {
		case 68: goto st558;
		case 93: goto tr614;
		case 100: goto st610;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
tr607:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st644;
st644:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof644;
case 644:
#line 9458 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st593;
		case 93: goto tr614;
		case 111: goto st645;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st645:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof645;
case 645:
	switch( (*( sm->p)) ) {
		case 82: goto st594;
		case 93: goto tr614;
		case 114: goto st646;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st646:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof646;
case 646:
	switch( (*( sm->p)) ) {
		case 69: goto st558;
		case 93: goto tr620;
		case 101: goto st610;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
tr608:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st647;
st647:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof647;
case 647:
#line 9497 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st596;
		case 93: goto tr614;
		case 101: goto st648;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st648:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof648;
case 648:
	switch( (*( sm->p)) ) {
		case 84: goto st597;
		case 93: goto tr614;
		case 116: goto st649;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st649:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof649;
case 649:
	switch( (*( sm->p)) ) {
		case 65: goto st558;
		case 93: goto tr614;
		case 97: goto st610;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
tr609:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st650;
st650:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof650;
case 650:
#line 9536 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st599;
		case 93: goto tr614;
		case 112: goto st651;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st651:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof651;
case 651:
	switch( (*( sm->p)) ) {
		case 69: goto st600;
		case 93: goto tr614;
		case 101: goto st652;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st652:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof652;
case 652:
	switch( (*( sm->p)) ) {
		case 67: goto st601;
		case 93: goto tr614;
		case 99: goto st653;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st653:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof653;
case 653:
	switch( (*( sm->p)) ) {
		case 73: goto st602;
		case 93: goto tr620;
		case 105: goto st654;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st654:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof654;
case 654:
	switch( (*( sm->p)) ) {
		case 69: goto st603;
		case 93: goto tr614;
		case 101: goto st655;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st655:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof655;
case 655:
	switch( (*( sm->p)) ) {
		case 83: goto st558;
		case 93: goto tr614;
		case 115: goto st610;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st605;
	goto tr239;
st656:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof656;
case 656:
	switch( (*( sm->p)) ) {
		case 69: goto st657;
		case 80: goto st674;
		case 85: goto st681;
		case 93: goto tr710;
		case 101: goto st657;
		case 112: goto st674;
		case 117: goto st681;
	}
	goto tr239;
st657:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof657;
case 657:
	switch( (*( sm->p)) ) {
		case 67: goto st658;
		case 99: goto st658;
	}
	goto tr239;
st658:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof658;
case 658:
	switch( (*( sm->p)) ) {
		case 84: goto st659;
		case 116: goto st659;
	}
	goto tr239;
st659:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof659;
case 659:
	switch( (*( sm->p)) ) {
		case 73: goto st660;
		case 105: goto st660;
	}
	goto tr239;
st660:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof660;
case 660:
	switch( (*( sm->p)) ) {
		case 79: goto st661;
		case 111: goto st661;
	}
	goto tr239;
st661:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof661;
case 661:
	switch( (*( sm->p)) ) {
		case 78: goto st662;
		case 110: goto st662;
	}
	goto tr239;
st662:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof662;
case 662:
	switch( (*( sm->p)) ) {
		case 44: goto st663;
		case 61: goto st672;
		case 93: goto tr718;
	}
	goto tr239;
st663:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof663;
case 663:
	switch( (*( sm->p)) ) {
		case 69: goto st664;
		case 101: goto st664;
	}
	goto tr239;
st664:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof664;
case 664:
	switch( (*( sm->p)) ) {
		case 88: goto st665;
		case 120: goto st665;
	}
	goto tr239;
st665:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof665;
case 665:
	switch( (*( sm->p)) ) {
		case 80: goto st666;
		case 112: goto st666;
	}
	goto tr239;
st666:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof666;
case 666:
	switch( (*( sm->p)) ) {
		case 65: goto st667;
		case 97: goto st667;
	}
	goto tr239;
st667:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof667;
case 667:
	switch( (*( sm->p)) ) {
		case 78: goto st668;
		case 110: goto st668;
	}
	goto tr239;
st668:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof668;
case 668:
	switch( (*( sm->p)) ) {
		case 68: goto st669;
		case 100: goto st669;
	}
	goto tr239;
st669:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof669;
case 669:
	switch( (*( sm->p)) ) {
		case 69: goto st670;
		case 101: goto st670;
	}
	goto tr239;
st670:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof670;
case 670:
	switch( (*( sm->p)) ) {
		case 68: goto st671;
		case 100: goto st671;
	}
	goto tr239;
st671:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof671;
case 671:
	switch( (*( sm->p)) ) {
		case 61: goto st672;
		case 93: goto tr718;
	}
	goto tr239;
st672:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof672;
case 672:
	if ( (*( sm->p)) == 93 )
		goto tr239;
	goto tr727;
tr727:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st673;
st673:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof673;
case 673:
#line 9768 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr729;
	goto st673;
st674:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof674;
case 674:
	switch( (*( sm->p)) ) {
		case 79: goto st675;
		case 111: goto st675;
	}
	goto tr239;
st675:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof675;
case 675:
	switch( (*( sm->p)) ) {
		case 73: goto st676;
		case 105: goto st676;
	}
	goto tr239;
st676:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof676;
case 676:
	switch( (*( sm->p)) ) {
		case 76: goto st677;
		case 108: goto st677;
	}
	goto tr239;
st677:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof677;
case 677:
	switch( (*( sm->p)) ) {
		case 69: goto st678;
		case 101: goto st678;
	}
	goto tr239;
st678:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof678;
case 678:
	switch( (*( sm->p)) ) {
		case 82: goto st679;
		case 114: goto st679;
	}
	goto tr239;
st679:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof679;
case 679:
	switch( (*( sm->p)) ) {
		case 83: goto st680;
		case 93: goto tr736;
		case 115: goto st680;
	}
	goto tr239;
st680:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof680;
case 680:
	if ( (*( sm->p)) == 93 )
		goto tr736;
	goto tr239;
st681:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof681;
case 681:
	switch( (*( sm->p)) ) {
		case 66: goto st682;
		case 80: goto st683;
		case 98: goto st682;
		case 112: goto st683;
	}
	goto tr239;
st682:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof682;
case 682:
	if ( (*( sm->p)) == 93 )
		goto tr739;
	goto tr239;
st683:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof683;
case 683:
	if ( (*( sm->p)) == 93 )
		goto tr740;
	goto tr239;
st684:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof684;
case 684:
	switch( (*( sm->p)) ) {
		case 65: goto st685;
		case 97: goto st685;
	}
	goto tr239;
st685:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof685;
case 685:
	switch( (*( sm->p)) ) {
		case 66: goto st686;
		case 98: goto st686;
	}
	goto tr239;
st686:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof686;
case 686:
	switch( (*( sm->p)) ) {
		case 76: goto st687;
		case 108: goto st687;
	}
	goto tr239;
st687:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof687;
case 687:
	switch( (*( sm->p)) ) {
		case 69: goto st688;
		case 101: goto st688;
	}
	goto tr239;
st688:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof688;
case 688:
	if ( (*( sm->p)) == 93 )
		goto tr745;
	goto tr239;
st689:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof689;
case 689:
	if ( (*( sm->p)) == 93 )
		goto tr746;
	goto tr239;
st690:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof690;
case 690:
	switch( (*( sm->p)) ) {
		case 93: goto tr239;
		case 124: goto tr748;
	}
	goto tr747;
tr747:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st691;
st691:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof691;
case 691:
#line 9924 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 93: goto tr750;
		case 124: goto tr751;
	}
	goto st691;
tr750:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st692;
st692:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof692;
case 692:
#line 9936 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr752;
	goto tr239;
tr751:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st693;
st693:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof693;
case 693:
#line 9946 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 93: goto tr239;
		case 124: goto tr239;
	}
	goto tr753;
tr753:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st694;
st694:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof694;
case 694:
#line 9958 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 93: goto tr755;
		case 124: goto tr239;
	}
	goto st694;
tr755:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
	goto st695;
st695:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof695;
case 695:
#line 9970 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr756;
	goto tr239;
tr748:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st696;
st696:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof696;
case 696:
#line 9980 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 93: goto tr750;
		case 124: goto tr239;
	}
	goto st696;
st805:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof805;
case 805:
	if ( (*( sm->p)) == 96 )
		goto tr963;
	goto tr881;
tr866:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st806;
st806:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof806;
case 806:
#line 9999 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 123 )
		goto st697;
	goto tr881;
st697:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof697;
case 697:
	switch( (*( sm->p)) ) {
		case 124: goto tr759;
		case 125: goto tr239;
	}
	goto tr758;
tr758:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st698;
st698:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof698;
case 698:
#line 10018 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 124: goto tr761;
		case 125: goto tr762;
	}
	goto st698;
tr761:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st699;
st699:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof699;
case 699:
#line 10030 "ext/dtext/dtext.cpp"
	if ( 124 <= (*( sm->p)) && (*( sm->p)) <= 125 )
		goto tr239;
	goto tr763;
tr763:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st700;
st700:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof700;
case 700:
#line 10040 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 124: goto tr239;
		case 125: goto tr765;
	}
	goto st700;
tr765:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
	goto st701;
st701:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof701;
case 701:
#line 10052 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 125 )
		goto tr766;
	goto tr239;
tr762:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st702;
st702:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof702;
case 702:
#line 10062 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 125 )
		goto tr767;
	goto tr239;
tr759:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st703;
st703:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof703;
case 703:
#line 10072 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 124: goto tr239;
		case 125: goto tr762;
	}
	goto st703;
tr965:
#line 573 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_html_escaped((*( sm->p)));
  }}
	goto st807;
tr967:
#line 568 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_inline(INLINE_CODE, "</span>");
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st807;
tr968:
#line 573 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_html_escaped((*( sm->p)));
  }}
	goto st807;
tr969:
#line 564 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("`");
  }}
	goto st807;
st807:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof807;
case 807:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 10104 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 92: goto st808;
		case 96: goto tr967;
	}
	goto tr965;
st808:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof808;
case 808:
	if ( (*( sm->p)) == 96 )
		goto tr969;
	goto tr968;
tr769:
#line 588 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    append_html_escaped((*( sm->p)));
  }}
	goto st809;
tr774:
#line 579 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_check(BLOCK_CODE)) {
      dstack_rewind();
    } else {
      append("[/code]");
    }
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st809;
tr970:
#line 588 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_html_escaped((*( sm->p)));
  }}
	goto st809;
tr972:
#line 588 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_html_escaped((*( sm->p)));
  }}
	goto st809;
st809:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof809;
case 809:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 10147 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 91 )
		goto tr971;
	goto tr970;
tr971:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st810;
st810:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof810;
case 810:
#line 10157 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 47 )
		goto st704;
	goto tr972;
st704:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof704;
case 704:
	switch( (*( sm->p)) ) {
		case 67: goto st705;
		case 99: goto st705;
	}
	goto tr769;
st705:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof705;
case 705:
	switch( (*( sm->p)) ) {
		case 79: goto st706;
		case 111: goto st706;
	}
	goto tr769;
st706:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof706;
case 706:
	switch( (*( sm->p)) ) {
		case 68: goto st707;
		case 100: goto st707;
	}
	goto tr769;
st707:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof707;
case 707:
	switch( (*( sm->p)) ) {
		case 69: goto st708;
		case 101: goto st708;
	}
	goto tr769;
st708:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof708;
case 708:
	if ( (*( sm->p)) == 93 )
		goto tr774;
	goto tr769;
tr775:
#line 634 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}}
	goto st811;
tr784:
#line 628 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_close_block(BLOCK_TABLE, "</table>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st811;
tr788:
#line 606 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_block(BLOCK_TBODY, "</tbody>");
  }}
	goto st811;
tr792:
#line 598 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_block(BLOCK_THEAD, "</thead>");
  }}
	goto st811;
tr793:
#line 619 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_block(BLOCK_TR, "</tr>");
  }}
	goto st811;
tr801:
#line 602 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TBODY, "<tbody>");
  }}
	goto st811;
tr802:
#line 623 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TD, "<td>");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 811;goto st753;}}
  }}
	goto st811;
tr804:
#line 610 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TH, "<th>");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 811;goto st753;}}
  }}
	goto st811;
tr807:
#line 594 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_THEAD, "<thead>");
  }}
	goto st811;
tr808:
#line 615 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TR, "<tr>");
  }}
	goto st811;
tr974:
#line 634 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;}
	goto st811;
tr976:
#line 634 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;}
	goto st811;
st811:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof811;
case 811:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 10291 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 91 )
		goto tr975;
	goto tr974;
tr975:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st812;
st812:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof812;
case 812:
#line 10301 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 47: goto st709;
		case 84: goto st724;
		case 116: goto st724;
	}
	goto tr976;
st709:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof709;
case 709:
	switch( (*( sm->p)) ) {
		case 84: goto st710;
		case 116: goto st710;
	}
	goto tr775;
st710:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof710;
case 710:
	switch( (*( sm->p)) ) {
		case 65: goto st711;
		case 66: goto st715;
		case 72: goto st719;
		case 82: goto st723;
		case 97: goto st711;
		case 98: goto st715;
		case 104: goto st719;
		case 114: goto st723;
	}
	goto tr775;
st711:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof711;
case 711:
	switch( (*( sm->p)) ) {
		case 66: goto st712;
		case 98: goto st712;
	}
	goto tr775;
st712:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof712;
case 712:
	switch( (*( sm->p)) ) {
		case 76: goto st713;
		case 108: goto st713;
	}
	goto tr775;
st713:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof713;
case 713:
	switch( (*( sm->p)) ) {
		case 69: goto st714;
		case 101: goto st714;
	}
	goto tr775;
st714:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof714;
case 714:
	if ( (*( sm->p)) == 93 )
		goto tr784;
	goto tr775;
st715:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof715;
case 715:
	switch( (*( sm->p)) ) {
		case 79: goto st716;
		case 111: goto st716;
	}
	goto tr775;
st716:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof716;
case 716:
	switch( (*( sm->p)) ) {
		case 68: goto st717;
		case 100: goto st717;
	}
	goto tr775;
st717:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof717;
case 717:
	switch( (*( sm->p)) ) {
		case 89: goto st718;
		case 121: goto st718;
	}
	goto tr775;
st718:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof718;
case 718:
	if ( (*( sm->p)) == 93 )
		goto tr788;
	goto tr775;
st719:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof719;
case 719:
	switch( (*( sm->p)) ) {
		case 69: goto st720;
		case 101: goto st720;
	}
	goto tr775;
st720:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof720;
case 720:
	switch( (*( sm->p)) ) {
		case 65: goto st721;
		case 97: goto st721;
	}
	goto tr775;
st721:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof721;
case 721:
	switch( (*( sm->p)) ) {
		case 68: goto st722;
		case 100: goto st722;
	}
	goto tr775;
st722:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof722;
case 722:
	if ( (*( sm->p)) == 93 )
		goto tr792;
	goto tr775;
st723:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof723;
case 723:
	if ( (*( sm->p)) == 93 )
		goto tr793;
	goto tr775;
st724:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof724;
case 724:
	switch( (*( sm->p)) ) {
		case 66: goto st725;
		case 68: goto st729;
		case 72: goto st730;
		case 82: goto st734;
		case 98: goto st725;
		case 100: goto st729;
		case 104: goto st730;
		case 114: goto st734;
	}
	goto tr775;
st725:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof725;
case 725:
	switch( (*( sm->p)) ) {
		case 79: goto st726;
		case 111: goto st726;
	}
	goto tr775;
st726:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof726;
case 726:
	switch( (*( sm->p)) ) {
		case 68: goto st727;
		case 100: goto st727;
	}
	goto tr775;
st727:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof727;
case 727:
	switch( (*( sm->p)) ) {
		case 89: goto st728;
		case 121: goto st728;
	}
	goto tr775;
st728:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof728;
case 728:
	if ( (*( sm->p)) == 93 )
		goto tr801;
	goto tr775;
st729:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof729;
case 729:
	if ( (*( sm->p)) == 93 )
		goto tr802;
	goto tr775;
st730:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof730;
case 730:
	switch( (*( sm->p)) ) {
		case 69: goto st731;
		case 93: goto tr804;
		case 101: goto st731;
	}
	goto tr775;
st731:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof731;
case 731:
	switch( (*( sm->p)) ) {
		case 65: goto st732;
		case 97: goto st732;
	}
	goto tr775;
st732:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof732;
case 732:
	switch( (*( sm->p)) ) {
		case 68: goto st733;
		case 100: goto st733;
	}
	goto tr775;
st733:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof733;
case 733:
	if ( (*( sm->p)) == 93 )
		goto tr807;
	goto tr775;
st734:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof734;
case 734:
	if ( (*( sm->p)) == 93 )
		goto tr808;
	goto tr775;
	}
	_test_eof735:  sm->cs = 735; goto _test_eof; 
	_test_eof736:  sm->cs = 736; goto _test_eof; 
	_test_eof0:  sm->cs = 0; goto _test_eof; 
	_test_eof737:  sm->cs = 737; goto _test_eof; 
	_test_eof738:  sm->cs = 738; goto _test_eof; 
	_test_eof1:  sm->cs = 1; goto _test_eof; 
	_test_eof739:  sm->cs = 739; goto _test_eof; 
	_test_eof740:  sm->cs = 740; goto _test_eof; 
	_test_eof2:  sm->cs = 2; goto _test_eof; 
	_test_eof741:  sm->cs = 741; goto _test_eof; 
	_test_eof3:  sm->cs = 3; goto _test_eof; 
	_test_eof742:  sm->cs = 742; goto _test_eof; 
	_test_eof743:  sm->cs = 743; goto _test_eof; 
	_test_eof4:  sm->cs = 4; goto _test_eof; 
	_test_eof5:  sm->cs = 5; goto _test_eof; 
	_test_eof6:  sm->cs = 6; goto _test_eof; 
	_test_eof7:  sm->cs = 7; goto _test_eof; 
	_test_eof8:  sm->cs = 8; goto _test_eof; 
	_test_eof9:  sm->cs = 9; goto _test_eof; 
	_test_eof10:  sm->cs = 10; goto _test_eof; 
	_test_eof11:  sm->cs = 11; goto _test_eof; 
	_test_eof12:  sm->cs = 12; goto _test_eof; 
	_test_eof13:  sm->cs = 13; goto _test_eof; 
	_test_eof14:  sm->cs = 14; goto _test_eof; 
	_test_eof15:  sm->cs = 15; goto _test_eof; 
	_test_eof16:  sm->cs = 16; goto _test_eof; 
	_test_eof744:  sm->cs = 744; goto _test_eof; 
	_test_eof17:  sm->cs = 17; goto _test_eof; 
	_test_eof18:  sm->cs = 18; goto _test_eof; 
	_test_eof19:  sm->cs = 19; goto _test_eof; 
	_test_eof20:  sm->cs = 20; goto _test_eof; 
	_test_eof21:  sm->cs = 21; goto _test_eof; 
	_test_eof22:  sm->cs = 22; goto _test_eof; 
	_test_eof23:  sm->cs = 23; goto _test_eof; 
	_test_eof24:  sm->cs = 24; goto _test_eof; 
	_test_eof25:  sm->cs = 25; goto _test_eof; 
	_test_eof26:  sm->cs = 26; goto _test_eof; 
	_test_eof27:  sm->cs = 27; goto _test_eof; 
	_test_eof28:  sm->cs = 28; goto _test_eof; 
	_test_eof29:  sm->cs = 29; goto _test_eof; 
	_test_eof30:  sm->cs = 30; goto _test_eof; 
	_test_eof31:  sm->cs = 31; goto _test_eof; 
	_test_eof32:  sm->cs = 32; goto _test_eof; 
	_test_eof33:  sm->cs = 33; goto _test_eof; 
	_test_eof34:  sm->cs = 34; goto _test_eof; 
	_test_eof35:  sm->cs = 35; goto _test_eof; 
	_test_eof36:  sm->cs = 36; goto _test_eof; 
	_test_eof37:  sm->cs = 37; goto _test_eof; 
	_test_eof38:  sm->cs = 38; goto _test_eof; 
	_test_eof39:  sm->cs = 39; goto _test_eof; 
	_test_eof40:  sm->cs = 40; goto _test_eof; 
	_test_eof41:  sm->cs = 41; goto _test_eof; 
	_test_eof42:  sm->cs = 42; goto _test_eof; 
	_test_eof43:  sm->cs = 43; goto _test_eof; 
	_test_eof44:  sm->cs = 44; goto _test_eof; 
	_test_eof45:  sm->cs = 45; goto _test_eof; 
	_test_eof46:  sm->cs = 46; goto _test_eof; 
	_test_eof47:  sm->cs = 47; goto _test_eof; 
	_test_eof48:  sm->cs = 48; goto _test_eof; 
	_test_eof49:  sm->cs = 49; goto _test_eof; 
	_test_eof50:  sm->cs = 50; goto _test_eof; 
	_test_eof51:  sm->cs = 51; goto _test_eof; 
	_test_eof52:  sm->cs = 52; goto _test_eof; 
	_test_eof53:  sm->cs = 53; goto _test_eof; 
	_test_eof54:  sm->cs = 54; goto _test_eof; 
	_test_eof55:  sm->cs = 55; goto _test_eof; 
	_test_eof56:  sm->cs = 56; goto _test_eof; 
	_test_eof57:  sm->cs = 57; goto _test_eof; 
	_test_eof58:  sm->cs = 58; goto _test_eof; 
	_test_eof59:  sm->cs = 59; goto _test_eof; 
	_test_eof60:  sm->cs = 60; goto _test_eof; 
	_test_eof61:  sm->cs = 61; goto _test_eof; 
	_test_eof62:  sm->cs = 62; goto _test_eof; 
	_test_eof63:  sm->cs = 63; goto _test_eof; 
	_test_eof64:  sm->cs = 64; goto _test_eof; 
	_test_eof65:  sm->cs = 65; goto _test_eof; 
	_test_eof66:  sm->cs = 66; goto _test_eof; 
	_test_eof67:  sm->cs = 67; goto _test_eof; 
	_test_eof68:  sm->cs = 68; goto _test_eof; 
	_test_eof69:  sm->cs = 69; goto _test_eof; 
	_test_eof70:  sm->cs = 70; goto _test_eof; 
	_test_eof71:  sm->cs = 71; goto _test_eof; 
	_test_eof72:  sm->cs = 72; goto _test_eof; 
	_test_eof73:  sm->cs = 73; goto _test_eof; 
	_test_eof74:  sm->cs = 74; goto _test_eof; 
	_test_eof75:  sm->cs = 75; goto _test_eof; 
	_test_eof76:  sm->cs = 76; goto _test_eof; 
	_test_eof77:  sm->cs = 77; goto _test_eof; 
	_test_eof78:  sm->cs = 78; goto _test_eof; 
	_test_eof79:  sm->cs = 79; goto _test_eof; 
	_test_eof80:  sm->cs = 80; goto _test_eof; 
	_test_eof81:  sm->cs = 81; goto _test_eof; 
	_test_eof82:  sm->cs = 82; goto _test_eof; 
	_test_eof83:  sm->cs = 83; goto _test_eof; 
	_test_eof84:  sm->cs = 84; goto _test_eof; 
	_test_eof85:  sm->cs = 85; goto _test_eof; 
	_test_eof86:  sm->cs = 86; goto _test_eof; 
	_test_eof87:  sm->cs = 87; goto _test_eof; 
	_test_eof88:  sm->cs = 88; goto _test_eof; 
	_test_eof89:  sm->cs = 89; goto _test_eof; 
	_test_eof90:  sm->cs = 90; goto _test_eof; 
	_test_eof91:  sm->cs = 91; goto _test_eof; 
	_test_eof92:  sm->cs = 92; goto _test_eof; 
	_test_eof93:  sm->cs = 93; goto _test_eof; 
	_test_eof94:  sm->cs = 94; goto _test_eof; 
	_test_eof95:  sm->cs = 95; goto _test_eof; 
	_test_eof96:  sm->cs = 96; goto _test_eof; 
	_test_eof97:  sm->cs = 97; goto _test_eof; 
	_test_eof98:  sm->cs = 98; goto _test_eof; 
	_test_eof99:  sm->cs = 99; goto _test_eof; 
	_test_eof100:  sm->cs = 100; goto _test_eof; 
	_test_eof101:  sm->cs = 101; goto _test_eof; 
	_test_eof102:  sm->cs = 102; goto _test_eof; 
	_test_eof103:  sm->cs = 103; goto _test_eof; 
	_test_eof104:  sm->cs = 104; goto _test_eof; 
	_test_eof105:  sm->cs = 105; goto _test_eof; 
	_test_eof106:  sm->cs = 106; goto _test_eof; 
	_test_eof107:  sm->cs = 107; goto _test_eof; 
	_test_eof108:  sm->cs = 108; goto _test_eof; 
	_test_eof109:  sm->cs = 109; goto _test_eof; 
	_test_eof110:  sm->cs = 110; goto _test_eof; 
	_test_eof111:  sm->cs = 111; goto _test_eof; 
	_test_eof112:  sm->cs = 112; goto _test_eof; 
	_test_eof113:  sm->cs = 113; goto _test_eof; 
	_test_eof114:  sm->cs = 114; goto _test_eof; 
	_test_eof115:  sm->cs = 115; goto _test_eof; 
	_test_eof116:  sm->cs = 116; goto _test_eof; 
	_test_eof117:  sm->cs = 117; goto _test_eof; 
	_test_eof118:  sm->cs = 118; goto _test_eof; 
	_test_eof119:  sm->cs = 119; goto _test_eof; 
	_test_eof120:  sm->cs = 120; goto _test_eof; 
	_test_eof121:  sm->cs = 121; goto _test_eof; 
	_test_eof122:  sm->cs = 122; goto _test_eof; 
	_test_eof123:  sm->cs = 123; goto _test_eof; 
	_test_eof124:  sm->cs = 124; goto _test_eof; 
	_test_eof125:  sm->cs = 125; goto _test_eof; 
	_test_eof126:  sm->cs = 126; goto _test_eof; 
	_test_eof127:  sm->cs = 127; goto _test_eof; 
	_test_eof128:  sm->cs = 128; goto _test_eof; 
	_test_eof129:  sm->cs = 129; goto _test_eof; 
	_test_eof130:  sm->cs = 130; goto _test_eof; 
	_test_eof131:  sm->cs = 131; goto _test_eof; 
	_test_eof132:  sm->cs = 132; goto _test_eof; 
	_test_eof745:  sm->cs = 745; goto _test_eof; 
	_test_eof133:  sm->cs = 133; goto _test_eof; 
	_test_eof134:  sm->cs = 134; goto _test_eof; 
	_test_eof135:  sm->cs = 135; goto _test_eof; 
	_test_eof136:  sm->cs = 136; goto _test_eof; 
	_test_eof137:  sm->cs = 137; goto _test_eof; 
	_test_eof138:  sm->cs = 138; goto _test_eof; 
	_test_eof139:  sm->cs = 139; goto _test_eof; 
	_test_eof140:  sm->cs = 140; goto _test_eof; 
	_test_eof141:  sm->cs = 141; goto _test_eof; 
	_test_eof142:  sm->cs = 142; goto _test_eof; 
	_test_eof143:  sm->cs = 143; goto _test_eof; 
	_test_eof144:  sm->cs = 144; goto _test_eof; 
	_test_eof145:  sm->cs = 145; goto _test_eof; 
	_test_eof146:  sm->cs = 146; goto _test_eof; 
	_test_eof147:  sm->cs = 147; goto _test_eof; 
	_test_eof148:  sm->cs = 148; goto _test_eof; 
	_test_eof149:  sm->cs = 149; goto _test_eof; 
	_test_eof150:  sm->cs = 150; goto _test_eof; 
	_test_eof746:  sm->cs = 746; goto _test_eof; 
	_test_eof747:  sm->cs = 747; goto _test_eof; 
	_test_eof151:  sm->cs = 151; goto _test_eof; 
	_test_eof152:  sm->cs = 152; goto _test_eof; 
	_test_eof748:  sm->cs = 748; goto _test_eof; 
	_test_eof749:  sm->cs = 749; goto _test_eof; 
	_test_eof153:  sm->cs = 153; goto _test_eof; 
	_test_eof154:  sm->cs = 154; goto _test_eof; 
	_test_eof155:  sm->cs = 155; goto _test_eof; 
	_test_eof156:  sm->cs = 156; goto _test_eof; 
	_test_eof157:  sm->cs = 157; goto _test_eof; 
	_test_eof158:  sm->cs = 158; goto _test_eof; 
	_test_eof159:  sm->cs = 159; goto _test_eof; 
	_test_eof750:  sm->cs = 750; goto _test_eof; 
	_test_eof160:  sm->cs = 160; goto _test_eof; 
	_test_eof161:  sm->cs = 161; goto _test_eof; 
	_test_eof162:  sm->cs = 162; goto _test_eof; 
	_test_eof163:  sm->cs = 163; goto _test_eof; 
	_test_eof164:  sm->cs = 164; goto _test_eof; 
	_test_eof751:  sm->cs = 751; goto _test_eof; 
	_test_eof752:  sm->cs = 752; goto _test_eof; 
	_test_eof165:  sm->cs = 165; goto _test_eof; 
	_test_eof166:  sm->cs = 166; goto _test_eof; 
	_test_eof167:  sm->cs = 167; goto _test_eof; 
	_test_eof168:  sm->cs = 168; goto _test_eof; 
	_test_eof169:  sm->cs = 169; goto _test_eof; 
	_test_eof170:  sm->cs = 170; goto _test_eof; 
	_test_eof171:  sm->cs = 171; goto _test_eof; 
	_test_eof172:  sm->cs = 172; goto _test_eof; 
	_test_eof173:  sm->cs = 173; goto _test_eof; 
	_test_eof174:  sm->cs = 174; goto _test_eof; 
	_test_eof175:  sm->cs = 175; goto _test_eof; 
	_test_eof176:  sm->cs = 176; goto _test_eof; 
	_test_eof177:  sm->cs = 177; goto _test_eof; 
	_test_eof178:  sm->cs = 178; goto _test_eof; 
	_test_eof179:  sm->cs = 179; goto _test_eof; 
	_test_eof753:  sm->cs = 753; goto _test_eof; 
	_test_eof754:  sm->cs = 754; goto _test_eof; 
	_test_eof755:  sm->cs = 755; goto _test_eof; 
	_test_eof180:  sm->cs = 180; goto _test_eof; 
	_test_eof181:  sm->cs = 181; goto _test_eof; 
	_test_eof182:  sm->cs = 182; goto _test_eof; 
	_test_eof183:  sm->cs = 183; goto _test_eof; 
	_test_eof184:  sm->cs = 184; goto _test_eof; 
	_test_eof185:  sm->cs = 185; goto _test_eof; 
	_test_eof186:  sm->cs = 186; goto _test_eof; 
	_test_eof187:  sm->cs = 187; goto _test_eof; 
	_test_eof188:  sm->cs = 188; goto _test_eof; 
	_test_eof189:  sm->cs = 189; goto _test_eof; 
	_test_eof190:  sm->cs = 190; goto _test_eof; 
	_test_eof191:  sm->cs = 191; goto _test_eof; 
	_test_eof192:  sm->cs = 192; goto _test_eof; 
	_test_eof193:  sm->cs = 193; goto _test_eof; 
	_test_eof194:  sm->cs = 194; goto _test_eof; 
	_test_eof756:  sm->cs = 756; goto _test_eof; 
	_test_eof757:  sm->cs = 757; goto _test_eof; 
	_test_eof195:  sm->cs = 195; goto _test_eof; 
	_test_eof196:  sm->cs = 196; goto _test_eof; 
	_test_eof758:  sm->cs = 758; goto _test_eof; 
	_test_eof197:  sm->cs = 197; goto _test_eof; 
	_test_eof198:  sm->cs = 198; goto _test_eof; 
	_test_eof199:  sm->cs = 199; goto _test_eof; 
	_test_eof200:  sm->cs = 200; goto _test_eof; 
	_test_eof201:  sm->cs = 201; goto _test_eof; 
	_test_eof202:  sm->cs = 202; goto _test_eof; 
	_test_eof203:  sm->cs = 203; goto _test_eof; 
	_test_eof759:  sm->cs = 759; goto _test_eof; 
	_test_eof204:  sm->cs = 204; goto _test_eof; 
	_test_eof205:  sm->cs = 205; goto _test_eof; 
	_test_eof206:  sm->cs = 206; goto _test_eof; 
	_test_eof207:  sm->cs = 207; goto _test_eof; 
	_test_eof208:  sm->cs = 208; goto _test_eof; 
	_test_eof209:  sm->cs = 209; goto _test_eof; 
	_test_eof210:  sm->cs = 210; goto _test_eof; 
	_test_eof760:  sm->cs = 760; goto _test_eof; 
	_test_eof761:  sm->cs = 761; goto _test_eof; 
	_test_eof762:  sm->cs = 762; goto _test_eof; 
	_test_eof211:  sm->cs = 211; goto _test_eof; 
	_test_eof212:  sm->cs = 212; goto _test_eof; 
	_test_eof213:  sm->cs = 213; goto _test_eof; 
	_test_eof214:  sm->cs = 214; goto _test_eof; 
	_test_eof763:  sm->cs = 763; goto _test_eof; 
	_test_eof215:  sm->cs = 215; goto _test_eof; 
	_test_eof216:  sm->cs = 216; goto _test_eof; 
	_test_eof217:  sm->cs = 217; goto _test_eof; 
	_test_eof218:  sm->cs = 218; goto _test_eof; 
	_test_eof219:  sm->cs = 219; goto _test_eof; 
	_test_eof220:  sm->cs = 220; goto _test_eof; 
	_test_eof221:  sm->cs = 221; goto _test_eof; 
	_test_eof222:  sm->cs = 222; goto _test_eof; 
	_test_eof223:  sm->cs = 223; goto _test_eof; 
	_test_eof224:  sm->cs = 224; goto _test_eof; 
	_test_eof225:  sm->cs = 225; goto _test_eof; 
	_test_eof226:  sm->cs = 226; goto _test_eof; 
	_test_eof227:  sm->cs = 227; goto _test_eof; 
	_test_eof228:  sm->cs = 228; goto _test_eof; 
	_test_eof229:  sm->cs = 229; goto _test_eof; 
	_test_eof230:  sm->cs = 230; goto _test_eof; 
	_test_eof231:  sm->cs = 231; goto _test_eof; 
	_test_eof764:  sm->cs = 764; goto _test_eof; 
	_test_eof232:  sm->cs = 232; goto _test_eof; 
	_test_eof233:  sm->cs = 233; goto _test_eof; 
	_test_eof234:  sm->cs = 234; goto _test_eof; 
	_test_eof235:  sm->cs = 235; goto _test_eof; 
	_test_eof236:  sm->cs = 236; goto _test_eof; 
	_test_eof237:  sm->cs = 237; goto _test_eof; 
	_test_eof238:  sm->cs = 238; goto _test_eof; 
	_test_eof239:  sm->cs = 239; goto _test_eof; 
	_test_eof240:  sm->cs = 240; goto _test_eof; 
	_test_eof765:  sm->cs = 765; goto _test_eof; 
	_test_eof241:  sm->cs = 241; goto _test_eof; 
	_test_eof242:  sm->cs = 242; goto _test_eof; 
	_test_eof243:  sm->cs = 243; goto _test_eof; 
	_test_eof244:  sm->cs = 244; goto _test_eof; 
	_test_eof245:  sm->cs = 245; goto _test_eof; 
	_test_eof246:  sm->cs = 246; goto _test_eof; 
	_test_eof766:  sm->cs = 766; goto _test_eof; 
	_test_eof247:  sm->cs = 247; goto _test_eof; 
	_test_eof248:  sm->cs = 248; goto _test_eof; 
	_test_eof249:  sm->cs = 249; goto _test_eof; 
	_test_eof250:  sm->cs = 250; goto _test_eof; 
	_test_eof251:  sm->cs = 251; goto _test_eof; 
	_test_eof252:  sm->cs = 252; goto _test_eof; 
	_test_eof253:  sm->cs = 253; goto _test_eof; 
	_test_eof767:  sm->cs = 767; goto _test_eof; 
	_test_eof768:  sm->cs = 768; goto _test_eof; 
	_test_eof254:  sm->cs = 254; goto _test_eof; 
	_test_eof255:  sm->cs = 255; goto _test_eof; 
	_test_eof256:  sm->cs = 256; goto _test_eof; 
	_test_eof257:  sm->cs = 257; goto _test_eof; 
	_test_eof769:  sm->cs = 769; goto _test_eof; 
	_test_eof258:  sm->cs = 258; goto _test_eof; 
	_test_eof259:  sm->cs = 259; goto _test_eof; 
	_test_eof260:  sm->cs = 260; goto _test_eof; 
	_test_eof261:  sm->cs = 261; goto _test_eof; 
	_test_eof262:  sm->cs = 262; goto _test_eof; 
	_test_eof770:  sm->cs = 770; goto _test_eof; 
	_test_eof263:  sm->cs = 263; goto _test_eof; 
	_test_eof264:  sm->cs = 264; goto _test_eof; 
	_test_eof265:  sm->cs = 265; goto _test_eof; 
	_test_eof266:  sm->cs = 266; goto _test_eof; 
	_test_eof771:  sm->cs = 771; goto _test_eof; 
	_test_eof772:  sm->cs = 772; goto _test_eof; 
	_test_eof267:  sm->cs = 267; goto _test_eof; 
	_test_eof268:  sm->cs = 268; goto _test_eof; 
	_test_eof269:  sm->cs = 269; goto _test_eof; 
	_test_eof270:  sm->cs = 270; goto _test_eof; 
	_test_eof271:  sm->cs = 271; goto _test_eof; 
	_test_eof272:  sm->cs = 272; goto _test_eof; 
	_test_eof273:  sm->cs = 273; goto _test_eof; 
	_test_eof274:  sm->cs = 274; goto _test_eof; 
	_test_eof773:  sm->cs = 773; goto _test_eof; 
	_test_eof774:  sm->cs = 774; goto _test_eof; 
	_test_eof275:  sm->cs = 275; goto _test_eof; 
	_test_eof276:  sm->cs = 276; goto _test_eof; 
	_test_eof277:  sm->cs = 277; goto _test_eof; 
	_test_eof278:  sm->cs = 278; goto _test_eof; 
	_test_eof279:  sm->cs = 279; goto _test_eof; 
	_test_eof775:  sm->cs = 775; goto _test_eof; 
	_test_eof280:  sm->cs = 280; goto _test_eof; 
	_test_eof281:  sm->cs = 281; goto _test_eof; 
	_test_eof282:  sm->cs = 282; goto _test_eof; 
	_test_eof283:  sm->cs = 283; goto _test_eof; 
	_test_eof284:  sm->cs = 284; goto _test_eof; 
	_test_eof285:  sm->cs = 285; goto _test_eof; 
	_test_eof776:  sm->cs = 776; goto _test_eof; 
	_test_eof777:  sm->cs = 777; goto _test_eof; 
	_test_eof286:  sm->cs = 286; goto _test_eof; 
	_test_eof287:  sm->cs = 287; goto _test_eof; 
	_test_eof288:  sm->cs = 288; goto _test_eof; 
	_test_eof289:  sm->cs = 289; goto _test_eof; 
	_test_eof290:  sm->cs = 290; goto _test_eof; 
	_test_eof291:  sm->cs = 291; goto _test_eof; 
	_test_eof778:  sm->cs = 778; goto _test_eof; 
	_test_eof292:  sm->cs = 292; goto _test_eof; 
	_test_eof779:  sm->cs = 779; goto _test_eof; 
	_test_eof293:  sm->cs = 293; goto _test_eof; 
	_test_eof294:  sm->cs = 294; goto _test_eof; 
	_test_eof295:  sm->cs = 295; goto _test_eof; 
	_test_eof296:  sm->cs = 296; goto _test_eof; 
	_test_eof297:  sm->cs = 297; goto _test_eof; 
	_test_eof298:  sm->cs = 298; goto _test_eof; 
	_test_eof299:  sm->cs = 299; goto _test_eof; 
	_test_eof300:  sm->cs = 300; goto _test_eof; 
	_test_eof301:  sm->cs = 301; goto _test_eof; 
	_test_eof302:  sm->cs = 302; goto _test_eof; 
	_test_eof303:  sm->cs = 303; goto _test_eof; 
	_test_eof304:  sm->cs = 304; goto _test_eof; 
	_test_eof780:  sm->cs = 780; goto _test_eof; 
	_test_eof781:  sm->cs = 781; goto _test_eof; 
	_test_eof305:  sm->cs = 305; goto _test_eof; 
	_test_eof306:  sm->cs = 306; goto _test_eof; 
	_test_eof307:  sm->cs = 307; goto _test_eof; 
	_test_eof308:  sm->cs = 308; goto _test_eof; 
	_test_eof309:  sm->cs = 309; goto _test_eof; 
	_test_eof310:  sm->cs = 310; goto _test_eof; 
	_test_eof311:  sm->cs = 311; goto _test_eof; 
	_test_eof312:  sm->cs = 312; goto _test_eof; 
	_test_eof313:  sm->cs = 313; goto _test_eof; 
	_test_eof314:  sm->cs = 314; goto _test_eof; 
	_test_eof315:  sm->cs = 315; goto _test_eof; 
	_test_eof782:  sm->cs = 782; goto _test_eof; 
	_test_eof783:  sm->cs = 783; goto _test_eof; 
	_test_eof316:  sm->cs = 316; goto _test_eof; 
	_test_eof317:  sm->cs = 317; goto _test_eof; 
	_test_eof318:  sm->cs = 318; goto _test_eof; 
	_test_eof319:  sm->cs = 319; goto _test_eof; 
	_test_eof320:  sm->cs = 320; goto _test_eof; 
	_test_eof784:  sm->cs = 784; goto _test_eof; 
	_test_eof785:  sm->cs = 785; goto _test_eof; 
	_test_eof321:  sm->cs = 321; goto _test_eof; 
	_test_eof322:  sm->cs = 322; goto _test_eof; 
	_test_eof323:  sm->cs = 323; goto _test_eof; 
	_test_eof324:  sm->cs = 324; goto _test_eof; 
	_test_eof325:  sm->cs = 325; goto _test_eof; 
	_test_eof786:  sm->cs = 786; goto _test_eof; 
	_test_eof326:  sm->cs = 326; goto _test_eof; 
	_test_eof327:  sm->cs = 327; goto _test_eof; 
	_test_eof328:  sm->cs = 328; goto _test_eof; 
	_test_eof329:  sm->cs = 329; goto _test_eof; 
	_test_eof787:  sm->cs = 787; goto _test_eof; 
	_test_eof330:  sm->cs = 330; goto _test_eof; 
	_test_eof331:  sm->cs = 331; goto _test_eof; 
	_test_eof332:  sm->cs = 332; goto _test_eof; 
	_test_eof333:  sm->cs = 333; goto _test_eof; 
	_test_eof334:  sm->cs = 334; goto _test_eof; 
	_test_eof335:  sm->cs = 335; goto _test_eof; 
	_test_eof336:  sm->cs = 336; goto _test_eof; 
	_test_eof337:  sm->cs = 337; goto _test_eof; 
	_test_eof338:  sm->cs = 338; goto _test_eof; 
	_test_eof788:  sm->cs = 788; goto _test_eof; 
	_test_eof789:  sm->cs = 789; goto _test_eof; 
	_test_eof339:  sm->cs = 339; goto _test_eof; 
	_test_eof340:  sm->cs = 340; goto _test_eof; 
	_test_eof341:  sm->cs = 341; goto _test_eof; 
	_test_eof342:  sm->cs = 342; goto _test_eof; 
	_test_eof343:  sm->cs = 343; goto _test_eof; 
	_test_eof344:  sm->cs = 344; goto _test_eof; 
	_test_eof345:  sm->cs = 345; goto _test_eof; 
	_test_eof790:  sm->cs = 790; goto _test_eof; 
	_test_eof791:  sm->cs = 791; goto _test_eof; 
	_test_eof346:  sm->cs = 346; goto _test_eof; 
	_test_eof347:  sm->cs = 347; goto _test_eof; 
	_test_eof348:  sm->cs = 348; goto _test_eof; 
	_test_eof349:  sm->cs = 349; goto _test_eof; 
	_test_eof792:  sm->cs = 792; goto _test_eof; 
	_test_eof793:  sm->cs = 793; goto _test_eof; 
	_test_eof350:  sm->cs = 350; goto _test_eof; 
	_test_eof351:  sm->cs = 351; goto _test_eof; 
	_test_eof352:  sm->cs = 352; goto _test_eof; 
	_test_eof353:  sm->cs = 353; goto _test_eof; 
	_test_eof354:  sm->cs = 354; goto _test_eof; 
	_test_eof355:  sm->cs = 355; goto _test_eof; 
	_test_eof356:  sm->cs = 356; goto _test_eof; 
	_test_eof357:  sm->cs = 357; goto _test_eof; 
	_test_eof358:  sm->cs = 358; goto _test_eof; 
	_test_eof359:  sm->cs = 359; goto _test_eof; 
	_test_eof794:  sm->cs = 794; goto _test_eof; 
	_test_eof360:  sm->cs = 360; goto _test_eof; 
	_test_eof361:  sm->cs = 361; goto _test_eof; 
	_test_eof362:  sm->cs = 362; goto _test_eof; 
	_test_eof363:  sm->cs = 363; goto _test_eof; 
	_test_eof364:  sm->cs = 364; goto _test_eof; 
	_test_eof365:  sm->cs = 365; goto _test_eof; 
	_test_eof366:  sm->cs = 366; goto _test_eof; 
	_test_eof367:  sm->cs = 367; goto _test_eof; 
	_test_eof368:  sm->cs = 368; goto _test_eof; 
	_test_eof369:  sm->cs = 369; goto _test_eof; 
	_test_eof370:  sm->cs = 370; goto _test_eof; 
	_test_eof371:  sm->cs = 371; goto _test_eof; 
	_test_eof372:  sm->cs = 372; goto _test_eof; 
	_test_eof373:  sm->cs = 373; goto _test_eof; 
	_test_eof795:  sm->cs = 795; goto _test_eof; 
	_test_eof374:  sm->cs = 374; goto _test_eof; 
	_test_eof375:  sm->cs = 375; goto _test_eof; 
	_test_eof376:  sm->cs = 376; goto _test_eof; 
	_test_eof377:  sm->cs = 377; goto _test_eof; 
	_test_eof378:  sm->cs = 378; goto _test_eof; 
	_test_eof379:  sm->cs = 379; goto _test_eof; 
	_test_eof380:  sm->cs = 380; goto _test_eof; 
	_test_eof796:  sm->cs = 796; goto _test_eof; 
	_test_eof381:  sm->cs = 381; goto _test_eof; 
	_test_eof382:  sm->cs = 382; goto _test_eof; 
	_test_eof383:  sm->cs = 383; goto _test_eof; 
	_test_eof384:  sm->cs = 384; goto _test_eof; 
	_test_eof385:  sm->cs = 385; goto _test_eof; 
	_test_eof386:  sm->cs = 386; goto _test_eof; 
	_test_eof797:  sm->cs = 797; goto _test_eof; 
	_test_eof798:  sm->cs = 798; goto _test_eof; 
	_test_eof387:  sm->cs = 387; goto _test_eof; 
	_test_eof388:  sm->cs = 388; goto _test_eof; 
	_test_eof389:  sm->cs = 389; goto _test_eof; 
	_test_eof390:  sm->cs = 390; goto _test_eof; 
	_test_eof391:  sm->cs = 391; goto _test_eof; 
	_test_eof799:  sm->cs = 799; goto _test_eof; 
	_test_eof800:  sm->cs = 800; goto _test_eof; 
	_test_eof392:  sm->cs = 392; goto _test_eof; 
	_test_eof393:  sm->cs = 393; goto _test_eof; 
	_test_eof394:  sm->cs = 394; goto _test_eof; 
	_test_eof395:  sm->cs = 395; goto _test_eof; 
	_test_eof396:  sm->cs = 396; goto _test_eof; 
	_test_eof801:  sm->cs = 801; goto _test_eof; 
	_test_eof802:  sm->cs = 802; goto _test_eof; 
	_test_eof397:  sm->cs = 397; goto _test_eof; 
	_test_eof398:  sm->cs = 398; goto _test_eof; 
	_test_eof399:  sm->cs = 399; goto _test_eof; 
	_test_eof400:  sm->cs = 400; goto _test_eof; 
	_test_eof401:  sm->cs = 401; goto _test_eof; 
	_test_eof402:  sm->cs = 402; goto _test_eof; 
	_test_eof403:  sm->cs = 403; goto _test_eof; 
	_test_eof404:  sm->cs = 404; goto _test_eof; 
	_test_eof803:  sm->cs = 803; goto _test_eof; 
	_test_eof405:  sm->cs = 405; goto _test_eof; 
	_test_eof406:  sm->cs = 406; goto _test_eof; 
	_test_eof407:  sm->cs = 407; goto _test_eof; 
	_test_eof408:  sm->cs = 408; goto _test_eof; 
	_test_eof409:  sm->cs = 409; goto _test_eof; 
	_test_eof410:  sm->cs = 410; goto _test_eof; 
	_test_eof411:  sm->cs = 411; goto _test_eof; 
	_test_eof412:  sm->cs = 412; goto _test_eof; 
	_test_eof413:  sm->cs = 413; goto _test_eof; 
	_test_eof414:  sm->cs = 414; goto _test_eof; 
	_test_eof415:  sm->cs = 415; goto _test_eof; 
	_test_eof416:  sm->cs = 416; goto _test_eof; 
	_test_eof417:  sm->cs = 417; goto _test_eof; 
	_test_eof804:  sm->cs = 804; goto _test_eof; 
	_test_eof418:  sm->cs = 418; goto _test_eof; 
	_test_eof419:  sm->cs = 419; goto _test_eof; 
	_test_eof420:  sm->cs = 420; goto _test_eof; 
	_test_eof421:  sm->cs = 421; goto _test_eof; 
	_test_eof422:  sm->cs = 422; goto _test_eof; 
	_test_eof423:  sm->cs = 423; goto _test_eof; 
	_test_eof424:  sm->cs = 424; goto _test_eof; 
	_test_eof425:  sm->cs = 425; goto _test_eof; 
	_test_eof426:  sm->cs = 426; goto _test_eof; 
	_test_eof427:  sm->cs = 427; goto _test_eof; 
	_test_eof428:  sm->cs = 428; goto _test_eof; 
	_test_eof429:  sm->cs = 429; goto _test_eof; 
	_test_eof430:  sm->cs = 430; goto _test_eof; 
	_test_eof431:  sm->cs = 431; goto _test_eof; 
	_test_eof432:  sm->cs = 432; goto _test_eof; 
	_test_eof433:  sm->cs = 433; goto _test_eof; 
	_test_eof434:  sm->cs = 434; goto _test_eof; 
	_test_eof435:  sm->cs = 435; goto _test_eof; 
	_test_eof436:  sm->cs = 436; goto _test_eof; 
	_test_eof437:  sm->cs = 437; goto _test_eof; 
	_test_eof438:  sm->cs = 438; goto _test_eof; 
	_test_eof439:  sm->cs = 439; goto _test_eof; 
	_test_eof440:  sm->cs = 440; goto _test_eof; 
	_test_eof441:  sm->cs = 441; goto _test_eof; 
	_test_eof442:  sm->cs = 442; goto _test_eof; 
	_test_eof443:  sm->cs = 443; goto _test_eof; 
	_test_eof444:  sm->cs = 444; goto _test_eof; 
	_test_eof445:  sm->cs = 445; goto _test_eof; 
	_test_eof446:  sm->cs = 446; goto _test_eof; 
	_test_eof447:  sm->cs = 447; goto _test_eof; 
	_test_eof448:  sm->cs = 448; goto _test_eof; 
	_test_eof449:  sm->cs = 449; goto _test_eof; 
	_test_eof450:  sm->cs = 450; goto _test_eof; 
	_test_eof451:  sm->cs = 451; goto _test_eof; 
	_test_eof452:  sm->cs = 452; goto _test_eof; 
	_test_eof453:  sm->cs = 453; goto _test_eof; 
	_test_eof454:  sm->cs = 454; goto _test_eof; 
	_test_eof455:  sm->cs = 455; goto _test_eof; 
	_test_eof456:  sm->cs = 456; goto _test_eof; 
	_test_eof457:  sm->cs = 457; goto _test_eof; 
	_test_eof458:  sm->cs = 458; goto _test_eof; 
	_test_eof459:  sm->cs = 459; goto _test_eof; 
	_test_eof460:  sm->cs = 460; goto _test_eof; 
	_test_eof461:  sm->cs = 461; goto _test_eof; 
	_test_eof462:  sm->cs = 462; goto _test_eof; 
	_test_eof463:  sm->cs = 463; goto _test_eof; 
	_test_eof464:  sm->cs = 464; goto _test_eof; 
	_test_eof465:  sm->cs = 465; goto _test_eof; 
	_test_eof466:  sm->cs = 466; goto _test_eof; 
	_test_eof467:  sm->cs = 467; goto _test_eof; 
	_test_eof468:  sm->cs = 468; goto _test_eof; 
	_test_eof469:  sm->cs = 469; goto _test_eof; 
	_test_eof470:  sm->cs = 470; goto _test_eof; 
	_test_eof471:  sm->cs = 471; goto _test_eof; 
	_test_eof472:  sm->cs = 472; goto _test_eof; 
	_test_eof473:  sm->cs = 473; goto _test_eof; 
	_test_eof474:  sm->cs = 474; goto _test_eof; 
	_test_eof475:  sm->cs = 475; goto _test_eof; 
	_test_eof476:  sm->cs = 476; goto _test_eof; 
	_test_eof477:  sm->cs = 477; goto _test_eof; 
	_test_eof478:  sm->cs = 478; goto _test_eof; 
	_test_eof479:  sm->cs = 479; goto _test_eof; 
	_test_eof480:  sm->cs = 480; goto _test_eof; 
	_test_eof481:  sm->cs = 481; goto _test_eof; 
	_test_eof482:  sm->cs = 482; goto _test_eof; 
	_test_eof483:  sm->cs = 483; goto _test_eof; 
	_test_eof484:  sm->cs = 484; goto _test_eof; 
	_test_eof485:  sm->cs = 485; goto _test_eof; 
	_test_eof486:  sm->cs = 486; goto _test_eof; 
	_test_eof487:  sm->cs = 487; goto _test_eof; 
	_test_eof488:  sm->cs = 488; goto _test_eof; 
	_test_eof489:  sm->cs = 489; goto _test_eof; 
	_test_eof490:  sm->cs = 490; goto _test_eof; 
	_test_eof491:  sm->cs = 491; goto _test_eof; 
	_test_eof492:  sm->cs = 492; goto _test_eof; 
	_test_eof493:  sm->cs = 493; goto _test_eof; 
	_test_eof494:  sm->cs = 494; goto _test_eof; 
	_test_eof495:  sm->cs = 495; goto _test_eof; 
	_test_eof496:  sm->cs = 496; goto _test_eof; 
	_test_eof497:  sm->cs = 497; goto _test_eof; 
	_test_eof498:  sm->cs = 498; goto _test_eof; 
	_test_eof499:  sm->cs = 499; goto _test_eof; 
	_test_eof500:  sm->cs = 500; goto _test_eof; 
	_test_eof501:  sm->cs = 501; goto _test_eof; 
	_test_eof502:  sm->cs = 502; goto _test_eof; 
	_test_eof503:  sm->cs = 503; goto _test_eof; 
	_test_eof504:  sm->cs = 504; goto _test_eof; 
	_test_eof505:  sm->cs = 505; goto _test_eof; 
	_test_eof506:  sm->cs = 506; goto _test_eof; 
	_test_eof507:  sm->cs = 507; goto _test_eof; 
	_test_eof508:  sm->cs = 508; goto _test_eof; 
	_test_eof509:  sm->cs = 509; goto _test_eof; 
	_test_eof510:  sm->cs = 510; goto _test_eof; 
	_test_eof511:  sm->cs = 511; goto _test_eof; 
	_test_eof512:  sm->cs = 512; goto _test_eof; 
	_test_eof513:  sm->cs = 513; goto _test_eof; 
	_test_eof514:  sm->cs = 514; goto _test_eof; 
	_test_eof515:  sm->cs = 515; goto _test_eof; 
	_test_eof516:  sm->cs = 516; goto _test_eof; 
	_test_eof517:  sm->cs = 517; goto _test_eof; 
	_test_eof518:  sm->cs = 518; goto _test_eof; 
	_test_eof519:  sm->cs = 519; goto _test_eof; 
	_test_eof520:  sm->cs = 520; goto _test_eof; 
	_test_eof521:  sm->cs = 521; goto _test_eof; 
	_test_eof522:  sm->cs = 522; goto _test_eof; 
	_test_eof523:  sm->cs = 523; goto _test_eof; 
	_test_eof524:  sm->cs = 524; goto _test_eof; 
	_test_eof525:  sm->cs = 525; goto _test_eof; 
	_test_eof526:  sm->cs = 526; goto _test_eof; 
	_test_eof527:  sm->cs = 527; goto _test_eof; 
	_test_eof528:  sm->cs = 528; goto _test_eof; 
	_test_eof529:  sm->cs = 529; goto _test_eof; 
	_test_eof530:  sm->cs = 530; goto _test_eof; 
	_test_eof531:  sm->cs = 531; goto _test_eof; 
	_test_eof532:  sm->cs = 532; goto _test_eof; 
	_test_eof533:  sm->cs = 533; goto _test_eof; 
	_test_eof534:  sm->cs = 534; goto _test_eof; 
	_test_eof535:  sm->cs = 535; goto _test_eof; 
	_test_eof536:  sm->cs = 536; goto _test_eof; 
	_test_eof537:  sm->cs = 537; goto _test_eof; 
	_test_eof538:  sm->cs = 538; goto _test_eof; 
	_test_eof539:  sm->cs = 539; goto _test_eof; 
	_test_eof540:  sm->cs = 540; goto _test_eof; 
	_test_eof541:  sm->cs = 541; goto _test_eof; 
	_test_eof542:  sm->cs = 542; goto _test_eof; 
	_test_eof543:  sm->cs = 543; goto _test_eof; 
	_test_eof544:  sm->cs = 544; goto _test_eof; 
	_test_eof545:  sm->cs = 545; goto _test_eof; 
	_test_eof546:  sm->cs = 546; goto _test_eof; 
	_test_eof547:  sm->cs = 547; goto _test_eof; 
	_test_eof548:  sm->cs = 548; goto _test_eof; 
	_test_eof549:  sm->cs = 549; goto _test_eof; 
	_test_eof550:  sm->cs = 550; goto _test_eof; 
	_test_eof551:  sm->cs = 551; goto _test_eof; 
	_test_eof552:  sm->cs = 552; goto _test_eof; 
	_test_eof553:  sm->cs = 553; goto _test_eof; 
	_test_eof554:  sm->cs = 554; goto _test_eof; 
	_test_eof555:  sm->cs = 555; goto _test_eof; 
	_test_eof556:  sm->cs = 556; goto _test_eof; 
	_test_eof557:  sm->cs = 557; goto _test_eof; 
	_test_eof558:  sm->cs = 558; goto _test_eof; 
	_test_eof559:  sm->cs = 559; goto _test_eof; 
	_test_eof560:  sm->cs = 560; goto _test_eof; 
	_test_eof561:  sm->cs = 561; goto _test_eof; 
	_test_eof562:  sm->cs = 562; goto _test_eof; 
	_test_eof563:  sm->cs = 563; goto _test_eof; 
	_test_eof564:  sm->cs = 564; goto _test_eof; 
	_test_eof565:  sm->cs = 565; goto _test_eof; 
	_test_eof566:  sm->cs = 566; goto _test_eof; 
	_test_eof567:  sm->cs = 567; goto _test_eof; 
	_test_eof568:  sm->cs = 568; goto _test_eof; 
	_test_eof569:  sm->cs = 569; goto _test_eof; 
	_test_eof570:  sm->cs = 570; goto _test_eof; 
	_test_eof571:  sm->cs = 571; goto _test_eof; 
	_test_eof572:  sm->cs = 572; goto _test_eof; 
	_test_eof573:  sm->cs = 573; goto _test_eof; 
	_test_eof574:  sm->cs = 574; goto _test_eof; 
	_test_eof575:  sm->cs = 575; goto _test_eof; 
	_test_eof576:  sm->cs = 576; goto _test_eof; 
	_test_eof577:  sm->cs = 577; goto _test_eof; 
	_test_eof578:  sm->cs = 578; goto _test_eof; 
	_test_eof579:  sm->cs = 579; goto _test_eof; 
	_test_eof580:  sm->cs = 580; goto _test_eof; 
	_test_eof581:  sm->cs = 581; goto _test_eof; 
	_test_eof582:  sm->cs = 582; goto _test_eof; 
	_test_eof583:  sm->cs = 583; goto _test_eof; 
	_test_eof584:  sm->cs = 584; goto _test_eof; 
	_test_eof585:  sm->cs = 585; goto _test_eof; 
	_test_eof586:  sm->cs = 586; goto _test_eof; 
	_test_eof587:  sm->cs = 587; goto _test_eof; 
	_test_eof588:  sm->cs = 588; goto _test_eof; 
	_test_eof589:  sm->cs = 589; goto _test_eof; 
	_test_eof590:  sm->cs = 590; goto _test_eof; 
	_test_eof591:  sm->cs = 591; goto _test_eof; 
	_test_eof592:  sm->cs = 592; goto _test_eof; 
	_test_eof593:  sm->cs = 593; goto _test_eof; 
	_test_eof594:  sm->cs = 594; goto _test_eof; 
	_test_eof595:  sm->cs = 595; goto _test_eof; 
	_test_eof596:  sm->cs = 596; goto _test_eof; 
	_test_eof597:  sm->cs = 597; goto _test_eof; 
	_test_eof598:  sm->cs = 598; goto _test_eof; 
	_test_eof599:  sm->cs = 599; goto _test_eof; 
	_test_eof600:  sm->cs = 600; goto _test_eof; 
	_test_eof601:  sm->cs = 601; goto _test_eof; 
	_test_eof602:  sm->cs = 602; goto _test_eof; 
	_test_eof603:  sm->cs = 603; goto _test_eof; 
	_test_eof604:  sm->cs = 604; goto _test_eof; 
	_test_eof605:  sm->cs = 605; goto _test_eof; 
	_test_eof606:  sm->cs = 606; goto _test_eof; 
	_test_eof607:  sm->cs = 607; goto _test_eof; 
	_test_eof608:  sm->cs = 608; goto _test_eof; 
	_test_eof609:  sm->cs = 609; goto _test_eof; 
	_test_eof610:  sm->cs = 610; goto _test_eof; 
	_test_eof611:  sm->cs = 611; goto _test_eof; 
	_test_eof612:  sm->cs = 612; goto _test_eof; 
	_test_eof613:  sm->cs = 613; goto _test_eof; 
	_test_eof614:  sm->cs = 614; goto _test_eof; 
	_test_eof615:  sm->cs = 615; goto _test_eof; 
	_test_eof616:  sm->cs = 616; goto _test_eof; 
	_test_eof617:  sm->cs = 617; goto _test_eof; 
	_test_eof618:  sm->cs = 618; goto _test_eof; 
	_test_eof619:  sm->cs = 619; goto _test_eof; 
	_test_eof620:  sm->cs = 620; goto _test_eof; 
	_test_eof621:  sm->cs = 621; goto _test_eof; 
	_test_eof622:  sm->cs = 622; goto _test_eof; 
	_test_eof623:  sm->cs = 623; goto _test_eof; 
	_test_eof624:  sm->cs = 624; goto _test_eof; 
	_test_eof625:  sm->cs = 625; goto _test_eof; 
	_test_eof626:  sm->cs = 626; goto _test_eof; 
	_test_eof627:  sm->cs = 627; goto _test_eof; 
	_test_eof628:  sm->cs = 628; goto _test_eof; 
	_test_eof629:  sm->cs = 629; goto _test_eof; 
	_test_eof630:  sm->cs = 630; goto _test_eof; 
	_test_eof631:  sm->cs = 631; goto _test_eof; 
	_test_eof632:  sm->cs = 632; goto _test_eof; 
	_test_eof633:  sm->cs = 633; goto _test_eof; 
	_test_eof634:  sm->cs = 634; goto _test_eof; 
	_test_eof635:  sm->cs = 635; goto _test_eof; 
	_test_eof636:  sm->cs = 636; goto _test_eof; 
	_test_eof637:  sm->cs = 637; goto _test_eof; 
	_test_eof638:  sm->cs = 638; goto _test_eof; 
	_test_eof639:  sm->cs = 639; goto _test_eof; 
	_test_eof640:  sm->cs = 640; goto _test_eof; 
	_test_eof641:  sm->cs = 641; goto _test_eof; 
	_test_eof642:  sm->cs = 642; goto _test_eof; 
	_test_eof643:  sm->cs = 643; goto _test_eof; 
	_test_eof644:  sm->cs = 644; goto _test_eof; 
	_test_eof645:  sm->cs = 645; goto _test_eof; 
	_test_eof646:  sm->cs = 646; goto _test_eof; 
	_test_eof647:  sm->cs = 647; goto _test_eof; 
	_test_eof648:  sm->cs = 648; goto _test_eof; 
	_test_eof649:  sm->cs = 649; goto _test_eof; 
	_test_eof650:  sm->cs = 650; goto _test_eof; 
	_test_eof651:  sm->cs = 651; goto _test_eof; 
	_test_eof652:  sm->cs = 652; goto _test_eof; 
	_test_eof653:  sm->cs = 653; goto _test_eof; 
	_test_eof654:  sm->cs = 654; goto _test_eof; 
	_test_eof655:  sm->cs = 655; goto _test_eof; 
	_test_eof656:  sm->cs = 656; goto _test_eof; 
	_test_eof657:  sm->cs = 657; goto _test_eof; 
	_test_eof658:  sm->cs = 658; goto _test_eof; 
	_test_eof659:  sm->cs = 659; goto _test_eof; 
	_test_eof660:  sm->cs = 660; goto _test_eof; 
	_test_eof661:  sm->cs = 661; goto _test_eof; 
	_test_eof662:  sm->cs = 662; goto _test_eof; 
	_test_eof663:  sm->cs = 663; goto _test_eof; 
	_test_eof664:  sm->cs = 664; goto _test_eof; 
	_test_eof665:  sm->cs = 665; goto _test_eof; 
	_test_eof666:  sm->cs = 666; goto _test_eof; 
	_test_eof667:  sm->cs = 667; goto _test_eof; 
	_test_eof668:  sm->cs = 668; goto _test_eof; 
	_test_eof669:  sm->cs = 669; goto _test_eof; 
	_test_eof670:  sm->cs = 670; goto _test_eof; 
	_test_eof671:  sm->cs = 671; goto _test_eof; 
	_test_eof672:  sm->cs = 672; goto _test_eof; 
	_test_eof673:  sm->cs = 673; goto _test_eof; 
	_test_eof674:  sm->cs = 674; goto _test_eof; 
	_test_eof675:  sm->cs = 675; goto _test_eof; 
	_test_eof676:  sm->cs = 676; goto _test_eof; 
	_test_eof677:  sm->cs = 677; goto _test_eof; 
	_test_eof678:  sm->cs = 678; goto _test_eof; 
	_test_eof679:  sm->cs = 679; goto _test_eof; 
	_test_eof680:  sm->cs = 680; goto _test_eof; 
	_test_eof681:  sm->cs = 681; goto _test_eof; 
	_test_eof682:  sm->cs = 682; goto _test_eof; 
	_test_eof683:  sm->cs = 683; goto _test_eof; 
	_test_eof684:  sm->cs = 684; goto _test_eof; 
	_test_eof685:  sm->cs = 685; goto _test_eof; 
	_test_eof686:  sm->cs = 686; goto _test_eof; 
	_test_eof687:  sm->cs = 687; goto _test_eof; 
	_test_eof688:  sm->cs = 688; goto _test_eof; 
	_test_eof689:  sm->cs = 689; goto _test_eof; 
	_test_eof690:  sm->cs = 690; goto _test_eof; 
	_test_eof691:  sm->cs = 691; goto _test_eof; 
	_test_eof692:  sm->cs = 692; goto _test_eof; 
	_test_eof693:  sm->cs = 693; goto _test_eof; 
	_test_eof694:  sm->cs = 694; goto _test_eof; 
	_test_eof695:  sm->cs = 695; goto _test_eof; 
	_test_eof696:  sm->cs = 696; goto _test_eof; 
	_test_eof805:  sm->cs = 805; goto _test_eof; 
	_test_eof806:  sm->cs = 806; goto _test_eof; 
	_test_eof697:  sm->cs = 697; goto _test_eof; 
	_test_eof698:  sm->cs = 698; goto _test_eof; 
	_test_eof699:  sm->cs = 699; goto _test_eof; 
	_test_eof700:  sm->cs = 700; goto _test_eof; 
	_test_eof701:  sm->cs = 701; goto _test_eof; 
	_test_eof702:  sm->cs = 702; goto _test_eof; 
	_test_eof703:  sm->cs = 703; goto _test_eof; 
	_test_eof807:  sm->cs = 807; goto _test_eof; 
	_test_eof808:  sm->cs = 808; goto _test_eof; 
	_test_eof809:  sm->cs = 809; goto _test_eof; 
	_test_eof810:  sm->cs = 810; goto _test_eof; 
	_test_eof704:  sm->cs = 704; goto _test_eof; 
	_test_eof705:  sm->cs = 705; goto _test_eof; 
	_test_eof706:  sm->cs = 706; goto _test_eof; 
	_test_eof707:  sm->cs = 707; goto _test_eof; 
	_test_eof708:  sm->cs = 708; goto _test_eof; 
	_test_eof811:  sm->cs = 811; goto _test_eof; 
	_test_eof812:  sm->cs = 812; goto _test_eof; 
	_test_eof709:  sm->cs = 709; goto _test_eof; 
	_test_eof710:  sm->cs = 710; goto _test_eof; 
	_test_eof711:  sm->cs = 711; goto _test_eof; 
	_test_eof712:  sm->cs = 712; goto _test_eof; 
	_test_eof713:  sm->cs = 713; goto _test_eof; 
	_test_eof714:  sm->cs = 714; goto _test_eof; 
	_test_eof715:  sm->cs = 715; goto _test_eof; 
	_test_eof716:  sm->cs = 716; goto _test_eof; 
	_test_eof717:  sm->cs = 717; goto _test_eof; 
	_test_eof718:  sm->cs = 718; goto _test_eof; 
	_test_eof719:  sm->cs = 719; goto _test_eof; 
	_test_eof720:  sm->cs = 720; goto _test_eof; 
	_test_eof721:  sm->cs = 721; goto _test_eof; 
	_test_eof722:  sm->cs = 722; goto _test_eof; 
	_test_eof723:  sm->cs = 723; goto _test_eof; 
	_test_eof724:  sm->cs = 724; goto _test_eof; 
	_test_eof725:  sm->cs = 725; goto _test_eof; 
	_test_eof726:  sm->cs = 726; goto _test_eof; 
	_test_eof727:  sm->cs = 727; goto _test_eof; 
	_test_eof728:  sm->cs = 728; goto _test_eof; 
	_test_eof729:  sm->cs = 729; goto _test_eof; 
	_test_eof730:  sm->cs = 730; goto _test_eof; 
	_test_eof731:  sm->cs = 731; goto _test_eof; 
	_test_eof732:  sm->cs = 732; goto _test_eof; 
	_test_eof733:  sm->cs = 733; goto _test_eof; 
	_test_eof734:  sm->cs = 734; goto _test_eof; 

	_test_eof: {}
	if ( ( sm->p) == ( sm->eof) )
	{
	switch (  sm->cs ) {
	case 736: goto tr0;
	case 0: goto tr0;
	case 737: goto tr816;
	case 738: goto tr816;
	case 1: goto tr2;
	case 739: goto tr817;
	case 740: goto tr817;
	case 2: goto tr2;
	case 741: goto tr816;
	case 3: goto tr2;
	case 742: goto tr820;
	case 743: goto tr816;
	case 4: goto tr2;
	case 5: goto tr2;
	case 6: goto tr2;
	case 7: goto tr2;
	case 8: goto tr2;
	case 9: goto tr2;
	case 10: goto tr2;
	case 11: goto tr2;
	case 12: goto tr2;
	case 13: goto tr2;
	case 14: goto tr2;
	case 15: goto tr2;
	case 16: goto tr2;
	case 744: goto tr827;
	case 17: goto tr2;
	case 18: goto tr2;
	case 19: goto tr2;
	case 20: goto tr2;
	case 21: goto tr2;
	case 22: goto tr2;
	case 23: goto tr2;
	case 24: goto tr2;
	case 25: goto tr2;
	case 26: goto tr2;
	case 27: goto tr2;
	case 28: goto tr2;
	case 29: goto tr2;
	case 30: goto tr2;
	case 31: goto tr2;
	case 32: goto tr2;
	case 33: goto tr2;
	case 34: goto tr2;
	case 35: goto tr2;
	case 36: goto tr2;
	case 37: goto tr2;
	case 38: goto tr2;
	case 39: goto tr2;
	case 40: goto tr2;
	case 41: goto tr2;
	case 42: goto tr2;
	case 43: goto tr2;
	case 44: goto tr2;
	case 45: goto tr2;
	case 46: goto tr2;
	case 47: goto tr2;
	case 48: goto tr2;
	case 49: goto tr2;
	case 50: goto tr2;
	case 51: goto tr2;
	case 52: goto tr2;
	case 53: goto tr2;
	case 54: goto tr2;
	case 55: goto tr2;
	case 56: goto tr2;
	case 57: goto tr2;
	case 58: goto tr2;
	case 59: goto tr2;
	case 60: goto tr2;
	case 61: goto tr2;
	case 62: goto tr2;
	case 63: goto tr2;
	case 64: goto tr2;
	case 65: goto tr2;
	case 66: goto tr2;
	case 67: goto tr2;
	case 68: goto tr2;
	case 69: goto tr2;
	case 70: goto tr2;
	case 71: goto tr2;
	case 72: goto tr2;
	case 73: goto tr2;
	case 74: goto tr2;
	case 75: goto tr2;
	case 76: goto tr2;
	case 77: goto tr2;
	case 78: goto tr2;
	case 79: goto tr2;
	case 80: goto tr2;
	case 81: goto tr2;
	case 82: goto tr2;
	case 83: goto tr2;
	case 84: goto tr2;
	case 85: goto tr2;
	case 86: goto tr2;
	case 87: goto tr2;
	case 88: goto tr2;
	case 89: goto tr2;
	case 90: goto tr2;
	case 91: goto tr2;
	case 92: goto tr2;
	case 93: goto tr2;
	case 94: goto tr2;
	case 95: goto tr2;
	case 96: goto tr2;
	case 97: goto tr2;
	case 98: goto tr2;
	case 99: goto tr2;
	case 100: goto tr2;
	case 101: goto tr2;
	case 102: goto tr2;
	case 103: goto tr2;
	case 104: goto tr2;
	case 105: goto tr2;
	case 106: goto tr2;
	case 107: goto tr2;
	case 108: goto tr2;
	case 109: goto tr2;
	case 110: goto tr2;
	case 111: goto tr2;
	case 112: goto tr2;
	case 113: goto tr2;
	case 114: goto tr2;
	case 115: goto tr2;
	case 116: goto tr2;
	case 117: goto tr2;
	case 118: goto tr2;
	case 119: goto tr2;
	case 120: goto tr2;
	case 121: goto tr2;
	case 122: goto tr2;
	case 123: goto tr2;
	case 124: goto tr2;
	case 125: goto tr2;
	case 126: goto tr2;
	case 127: goto tr2;
	case 128: goto tr2;
	case 129: goto tr2;
	case 130: goto tr2;
	case 131: goto tr2;
	case 132: goto tr2;
	case 745: goto tr828;
	case 133: goto tr2;
	case 134: goto tr2;
	case 135: goto tr2;
	case 136: goto tr2;
	case 137: goto tr2;
	case 138: goto tr2;
	case 139: goto tr2;
	case 140: goto tr2;
	case 141: goto tr2;
	case 142: goto tr2;
	case 143: goto tr2;
	case 144: goto tr2;
	case 145: goto tr2;
	case 146: goto tr2;
	case 147: goto tr2;
	case 148: goto tr2;
	case 149: goto tr2;
	case 150: goto tr2;
	case 746: goto tr829;
	case 747: goto tr831;
	case 151: goto tr2;
	case 152: goto tr2;
	case 748: goto tr832;
	case 749: goto tr834;
	case 153: goto tr2;
	case 154: goto tr2;
	case 155: goto tr2;
	case 156: goto tr2;
	case 157: goto tr2;
	case 158: goto tr2;
	case 159: goto tr2;
	case 750: goto tr835;
	case 160: goto tr2;
	case 161: goto tr2;
	case 162: goto tr2;
	case 163: goto tr2;
	case 164: goto tr2;
	case 752: goto tr838;
	case 165: goto tr178;
	case 166: goto tr178;
	case 167: goto tr178;
	case 168: goto tr178;
	case 169: goto tr178;
	case 170: goto tr178;
	case 171: goto tr178;
	case 172: goto tr178;
	case 173: goto tr178;
	case 174: goto tr178;
	case 175: goto tr178;
	case 176: goto tr178;
	case 177: goto tr178;
	case 178: goto tr178;
	case 179: goto tr178;
	case 754: goto tr867;
	case 755: goto tr872;
	case 180: goto tr201;
	case 181: goto tr203;
	case 182: goto tr203;
	case 183: goto tr203;
	case 184: goto tr201;
	case 185: goto tr201;
	case 186: goto tr201;
	case 187: goto tr201;
	case 188: goto tr201;
	case 189: goto tr201;
	case 190: goto tr201;
	case 191: goto tr201;
	case 192: goto tr201;
	case 193: goto tr217;
	case 194: goto tr217;
	case 756: goto tr874;
	case 757: goto tr874;
	case 195: goto tr217;
	case 196: goto tr217;
	case 758: goto tr876;
	case 197: goto tr217;
	case 198: goto tr217;
	case 199: goto tr201;
	case 200: goto tr201;
	case 201: goto tr201;
	case 202: goto tr201;
	case 203: goto tr201;
	case 759: goto tr878;
	case 204: goto tr217;
	case 205: goto tr201;
	case 206: goto tr201;
	case 207: goto tr201;
	case 208: goto tr201;
	case 209: goto tr201;
	case 210: goto tr201;
	case 760: goto tr879;
	case 761: goto tr880;
	case 762: goto tr881;
	case 211: goto tr239;
	case 212: goto tr239;
	case 213: goto tr239;
	case 214: goto tr239;
	case 763: goto tr883;
	case 215: goto tr239;
	case 216: goto tr239;
	case 217: goto tr239;
	case 218: goto tr239;
	case 219: goto tr239;
	case 220: goto tr239;
	case 221: goto tr239;
	case 222: goto tr239;
	case 223: goto tr239;
	case 224: goto tr239;
	case 225: goto tr239;
	case 226: goto tr239;
	case 227: goto tr239;
	case 228: goto tr239;
	case 229: goto tr239;
	case 230: goto tr239;
	case 231: goto tr239;
	case 764: goto tr881;
	case 232: goto tr239;
	case 233: goto tr239;
	case 234: goto tr239;
	case 235: goto tr239;
	case 236: goto tr239;
	case 237: goto tr239;
	case 238: goto tr239;
	case 239: goto tr239;
	case 240: goto tr239;
	case 765: goto tr881;
	case 241: goto tr239;
	case 242: goto tr239;
	case 243: goto tr239;
	case 244: goto tr239;
	case 245: goto tr239;
	case 246: goto tr239;
	case 766: goto tr887;
	case 247: goto tr239;
	case 248: goto tr239;
	case 249: goto tr239;
	case 250: goto tr239;
	case 251: goto tr239;
	case 252: goto tr239;
	case 253: goto tr239;
	case 767: goto tr889;
	case 768: goto tr881;
	case 254: goto tr239;
	case 255: goto tr239;
	case 256: goto tr239;
	case 257: goto tr239;
	case 769: goto tr894;
	case 258: goto tr239;
	case 259: goto tr239;
	case 260: goto tr239;
	case 261: goto tr239;
	case 262: goto tr239;
	case 770: goto tr896;
	case 263: goto tr239;
	case 264: goto tr239;
	case 265: goto tr239;
	case 266: goto tr239;
	case 771: goto tr898;
	case 772: goto tr881;
	case 267: goto tr239;
	case 268: goto tr239;
	case 269: goto tr239;
	case 270: goto tr239;
	case 271: goto tr239;
	case 272: goto tr239;
	case 273: goto tr239;
	case 274: goto tr239;
	case 773: goto tr901;
	case 774: goto tr881;
	case 275: goto tr239;
	case 276: goto tr239;
	case 277: goto tr239;
	case 278: goto tr239;
	case 279: goto tr239;
	case 775: goto tr905;
	case 280: goto tr239;
	case 281: goto tr239;
	case 282: goto tr239;
	case 283: goto tr239;
	case 284: goto tr239;
	case 285: goto tr239;
	case 776: goto tr907;
	case 777: goto tr881;
	case 286: goto tr239;
	case 287: goto tr239;
	case 288: goto tr239;
	case 289: goto tr239;
	case 290: goto tr239;
	case 291: goto tr239;
	case 778: goto tr910;
	case 292: goto tr239;
	case 779: goto tr881;
	case 293: goto tr239;
	case 294: goto tr239;
	case 295: goto tr239;
	case 296: goto tr239;
	case 297: goto tr239;
	case 298: goto tr239;
	case 299: goto tr239;
	case 300: goto tr239;
	case 301: goto tr239;
	case 302: goto tr239;
	case 303: goto tr239;
	case 304: goto tr239;
	case 780: goto tr912;
	case 781: goto tr881;
	case 305: goto tr239;
	case 306: goto tr239;
	case 307: goto tr239;
	case 308: goto tr239;
	case 309: goto tr239;
	case 310: goto tr239;
	case 311: goto tr239;
	case 312: goto tr239;
	case 313: goto tr239;
	case 314: goto tr239;
	case 315: goto tr239;
	case 782: goto tr915;
	case 783: goto tr881;
	case 316: goto tr239;
	case 317: goto tr239;
	case 318: goto tr239;
	case 319: goto tr239;
	case 320: goto tr239;
	case 784: goto tr918;
	case 785: goto tr881;
	case 321: goto tr239;
	case 322: goto tr239;
	case 323: goto tr239;
	case 324: goto tr239;
	case 325: goto tr239;
	case 786: goto tr921;
	case 326: goto tr239;
	case 327: goto tr239;
	case 328: goto tr239;
	case 329: goto tr239;
	case 787: goto tr923;
	case 330: goto tr239;
	case 331: goto tr239;
	case 332: goto tr239;
	case 333: goto tr239;
	case 334: goto tr239;
	case 335: goto tr239;
	case 336: goto tr239;
	case 337: goto tr239;
	case 338: goto tr239;
	case 788: goto tr925;
	case 789: goto tr881;
	case 339: goto tr239;
	case 340: goto tr239;
	case 341: goto tr239;
	case 342: goto tr239;
	case 343: goto tr239;
	case 344: goto tr239;
	case 345: goto tr239;
	case 790: goto tr928;
	case 791: goto tr881;
	case 346: goto tr239;
	case 347: goto tr239;
	case 348: goto tr239;
	case 349: goto tr239;
	case 792: goto tr931;
	case 793: goto tr881;
	case 350: goto tr239;
	case 351: goto tr239;
	case 352: goto tr239;
	case 353: goto tr239;
	case 354: goto tr239;
	case 355: goto tr239;
	case 356: goto tr239;
	case 357: goto tr239;
	case 358: goto tr239;
	case 359: goto tr239;
	case 794: goto tr937;
	case 360: goto tr239;
	case 361: goto tr239;
	case 362: goto tr239;
	case 363: goto tr239;
	case 364: goto tr239;
	case 365: goto tr239;
	case 366: goto tr239;
	case 367: goto tr239;
	case 368: goto tr239;
	case 369: goto tr239;
	case 370: goto tr239;
	case 371: goto tr239;
	case 372: goto tr239;
	case 373: goto tr239;
	case 795: goto tr939;
	case 374: goto tr239;
	case 375: goto tr239;
	case 376: goto tr239;
	case 377: goto tr239;
	case 378: goto tr239;
	case 379: goto tr239;
	case 380: goto tr239;
	case 796: goto tr941;
	case 381: goto tr239;
	case 382: goto tr239;
	case 383: goto tr239;
	case 384: goto tr239;
	case 385: goto tr239;
	case 386: goto tr239;
	case 797: goto tr943;
	case 798: goto tr881;
	case 387: goto tr239;
	case 388: goto tr239;
	case 389: goto tr239;
	case 390: goto tr239;
	case 391: goto tr239;
	case 799: goto tr946;
	case 800: goto tr881;
	case 392: goto tr239;
	case 393: goto tr239;
	case 394: goto tr239;
	case 395: goto tr239;
	case 396: goto tr239;
	case 801: goto tr949;
	case 802: goto tr881;
	case 397: goto tr239;
	case 398: goto tr239;
	case 399: goto tr239;
	case 400: goto tr239;
	case 401: goto tr239;
	case 402: goto tr239;
	case 403: goto tr239;
	case 404: goto tr239;
	case 803: goto tr961;
	case 405: goto tr239;
	case 406: goto tr239;
	case 407: goto tr239;
	case 408: goto tr239;
	case 409: goto tr239;
	case 410: goto tr239;
	case 411: goto tr239;
	case 412: goto tr239;
	case 413: goto tr239;
	case 414: goto tr239;
	case 415: goto tr239;
	case 416: goto tr239;
	case 417: goto tr239;
	case 804: goto tr962;
	case 418: goto tr239;
	case 419: goto tr239;
	case 420: goto tr239;
	case 421: goto tr239;
	case 422: goto tr239;
	case 423: goto tr239;
	case 424: goto tr239;
	case 425: goto tr239;
	case 426: goto tr239;
	case 427: goto tr239;
	case 428: goto tr239;
	case 429: goto tr239;
	case 430: goto tr239;
	case 431: goto tr239;
	case 432: goto tr239;
	case 433: goto tr239;
	case 434: goto tr239;
	case 435: goto tr239;
	case 436: goto tr239;
	case 437: goto tr239;
	case 438: goto tr239;
	case 439: goto tr239;
	case 440: goto tr239;
	case 441: goto tr239;
	case 442: goto tr239;
	case 443: goto tr239;
	case 444: goto tr239;
	case 445: goto tr239;
	case 446: goto tr239;
	case 447: goto tr239;
	case 448: goto tr239;
	case 449: goto tr239;
	case 450: goto tr239;
	case 451: goto tr239;
	case 452: goto tr239;
	case 453: goto tr239;
	case 454: goto tr239;
	case 455: goto tr239;
	case 456: goto tr239;
	case 457: goto tr239;
	case 458: goto tr239;
	case 459: goto tr239;
	case 460: goto tr239;
	case 461: goto tr239;
	case 462: goto tr239;
	case 463: goto tr239;
	case 464: goto tr239;
	case 465: goto tr239;
	case 466: goto tr239;
	case 467: goto tr239;
	case 468: goto tr239;
	case 469: goto tr239;
	case 470: goto tr239;
	case 471: goto tr239;
	case 472: goto tr239;
	case 473: goto tr239;
	case 474: goto tr239;
	case 475: goto tr239;
	case 476: goto tr239;
	case 477: goto tr239;
	case 478: goto tr239;
	case 479: goto tr239;
	case 480: goto tr239;
	case 481: goto tr239;
	case 482: goto tr239;
	case 483: goto tr239;
	case 484: goto tr239;
	case 485: goto tr239;
	case 486: goto tr239;
	case 487: goto tr239;
	case 488: goto tr239;
	case 489: goto tr239;
	case 490: goto tr239;
	case 491: goto tr239;
	case 492: goto tr239;
	case 493: goto tr239;
	case 494: goto tr239;
	case 495: goto tr239;
	case 496: goto tr239;
	case 497: goto tr239;
	case 498: goto tr239;
	case 499: goto tr239;
	case 500: goto tr239;
	case 501: goto tr239;
	case 502: goto tr239;
	case 503: goto tr239;
	case 504: goto tr239;
	case 505: goto tr239;
	case 506: goto tr239;
	case 507: goto tr239;
	case 508: goto tr239;
	case 509: goto tr239;
	case 510: goto tr239;
	case 511: goto tr239;
	case 512: goto tr239;
	case 513: goto tr239;
	case 514: goto tr239;
	case 515: goto tr239;
	case 516: goto tr239;
	case 517: goto tr239;
	case 518: goto tr239;
	case 519: goto tr239;
	case 520: goto tr239;
	case 521: goto tr239;
	case 522: goto tr239;
	case 523: goto tr239;
	case 524: goto tr239;
	case 525: goto tr239;
	case 526: goto tr239;
	case 527: goto tr239;
	case 528: goto tr239;
	case 529: goto tr239;
	case 530: goto tr239;
	case 531: goto tr239;
	case 532: goto tr239;
	case 533: goto tr239;
	case 534: goto tr239;
	case 535: goto tr239;
	case 536: goto tr239;
	case 537: goto tr239;
	case 538: goto tr239;
	case 539: goto tr239;
	case 540: goto tr239;
	case 541: goto tr239;
	case 542: goto tr239;
	case 543: goto tr239;
	case 544: goto tr239;
	case 545: goto tr239;
	case 546: goto tr239;
	case 547: goto tr239;
	case 548: goto tr239;
	case 549: goto tr239;
	case 550: goto tr239;
	case 551: goto tr239;
	case 552: goto tr239;
	case 553: goto tr239;
	case 554: goto tr239;
	case 555: goto tr239;
	case 556: goto tr239;
	case 557: goto tr239;
	case 558: goto tr239;
	case 559: goto tr239;
	case 560: goto tr239;
	case 561: goto tr239;
	case 562: goto tr239;
	case 563: goto tr239;
	case 564: goto tr239;
	case 565: goto tr239;
	case 566: goto tr239;
	case 567: goto tr239;
	case 568: goto tr239;
	case 569: goto tr239;
	case 570: goto tr239;
	case 571: goto tr239;
	case 572: goto tr239;
	case 573: goto tr239;
	case 574: goto tr239;
	case 575: goto tr239;
	case 576: goto tr239;
	case 577: goto tr239;
	case 578: goto tr239;
	case 579: goto tr239;
	case 580: goto tr239;
	case 581: goto tr239;
	case 582: goto tr239;
	case 583: goto tr239;
	case 584: goto tr239;
	case 585: goto tr239;
	case 586: goto tr239;
	case 587: goto tr239;
	case 588: goto tr239;
	case 589: goto tr239;
	case 590: goto tr239;
	case 591: goto tr239;
	case 592: goto tr239;
	case 593: goto tr239;
	case 594: goto tr239;
	case 595: goto tr239;
	case 596: goto tr239;
	case 597: goto tr239;
	case 598: goto tr239;
	case 599: goto tr239;
	case 600: goto tr239;
	case 601: goto tr239;
	case 602: goto tr239;
	case 603: goto tr239;
	case 604: goto tr239;
	case 605: goto tr239;
	case 606: goto tr239;
	case 607: goto tr239;
	case 608: goto tr239;
	case 609: goto tr239;
	case 610: goto tr239;
	case 611: goto tr239;
	case 612: goto tr239;
	case 613: goto tr239;
	case 614: goto tr239;
	case 615: goto tr239;
	case 616: goto tr239;
	case 617: goto tr239;
	case 618: goto tr239;
	case 619: goto tr239;
	case 620: goto tr239;
	case 621: goto tr239;
	case 622: goto tr239;
	case 623: goto tr239;
	case 624: goto tr239;
	case 625: goto tr239;
	case 626: goto tr239;
	case 627: goto tr239;
	case 628: goto tr239;
	case 629: goto tr239;
	case 630: goto tr239;
	case 631: goto tr239;
	case 632: goto tr239;
	case 633: goto tr239;
	case 634: goto tr239;
	case 635: goto tr239;
	case 636: goto tr239;
	case 637: goto tr239;
	case 638: goto tr239;
	case 639: goto tr239;
	case 640: goto tr239;
	case 641: goto tr239;
	case 642: goto tr239;
	case 643: goto tr239;
	case 644: goto tr239;
	case 645: goto tr239;
	case 646: goto tr239;
	case 647: goto tr239;
	case 648: goto tr239;
	case 649: goto tr239;
	case 650: goto tr239;
	case 651: goto tr239;
	case 652: goto tr239;
	case 653: goto tr239;
	case 654: goto tr239;
	case 655: goto tr239;
	case 656: goto tr239;
	case 657: goto tr239;
	case 658: goto tr239;
	case 659: goto tr239;
	case 660: goto tr239;
	case 661: goto tr239;
	case 662: goto tr239;
	case 663: goto tr239;
	case 664: goto tr239;
	case 665: goto tr239;
	case 666: goto tr239;
	case 667: goto tr239;
	case 668: goto tr239;
	case 669: goto tr239;
	case 670: goto tr239;
	case 671: goto tr239;
	case 672: goto tr239;
	case 673: goto tr239;
	case 674: goto tr239;
	case 675: goto tr239;
	case 676: goto tr239;
	case 677: goto tr239;
	case 678: goto tr239;
	case 679: goto tr239;
	case 680: goto tr239;
	case 681: goto tr239;
	case 682: goto tr239;
	case 683: goto tr239;
	case 684: goto tr239;
	case 685: goto tr239;
	case 686: goto tr239;
	case 687: goto tr239;
	case 688: goto tr239;
	case 689: goto tr239;
	case 690: goto tr239;
	case 691: goto tr239;
	case 692: goto tr239;
	case 693: goto tr239;
	case 694: goto tr239;
	case 695: goto tr239;
	case 696: goto tr239;
	case 805: goto tr881;
	case 806: goto tr881;
	case 697: goto tr239;
	case 698: goto tr239;
	case 699: goto tr239;
	case 700: goto tr239;
	case 701: goto tr239;
	case 702: goto tr239;
	case 703: goto tr239;
	case 808: goto tr968;
	case 810: goto tr972;
	case 704: goto tr769;
	case 705: goto tr769;
	case 706: goto tr769;
	case 707: goto tr769;
	case 708: goto tr769;
	case 812: goto tr976;
	case 709: goto tr775;
	case 710: goto tr775;
	case 711: goto tr775;
	case 712: goto tr775;
	case 713: goto tr775;
	case 714: goto tr775;
	case 715: goto tr775;
	case 716: goto tr775;
	case 717: goto tr775;
	case 718: goto tr775;
	case 719: goto tr775;
	case 720: goto tr775;
	case 721: goto tr775;
	case 722: goto tr775;
	case 723: goto tr775;
	case 724: goto tr775;
	case 725: goto tr775;
	case 726: goto tr775;
	case 727: goto tr775;
	case 728: goto tr775;
	case 729: goto tr775;
	case 730: goto tr775;
	case 731: goto tr775;
	case 732: goto tr775;
	case 733: goto tr775;
	case 734: goto tr775;
	}
	}

	}

#line 1159 "ext/dtext/dtext.cpp.rl"

  sm->dstack_close_all();

  return DTextResult { sm->output, sm->posts };
}
