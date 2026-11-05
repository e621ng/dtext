
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


#line 780 "ext/dtext/dtext.cpp.rl"



#line 68 "ext/dtext/dtext.cpp"
static const int dtext_start = 1072;
static const int dtext_first_final = 1072;
static const int dtext_error = -1;

static const int dtext_en_basic_inline = 1088;
static const int dtext_en_inline = 1090;
static const int dtext_en_inline_code = 1145;
static const int dtext_en_code = 1147;
static const int dtext_en_table = 1149;
static const int dtext_en_main = 1072;


#line 783 "ext/dtext/dtext.cpp.rl"

void StateMachine::dstack_push(element_t element) {
  dstack.push_back(element);
  dstack_counts[element]++;
}

element_t StateMachine::dstack_pop() {
  if (dstack.empty()) {
    g_debug("dstack pop empty stack");
    return DSTACK_EMPTY;
  } else {
    auto element = dstack.back();
    dstack.pop_back();
    dstack_counts[element]--;
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
  return dstack_counts[element] > 0;
}

int StateMachine::dstack_count(element_t element) {
  return dstack_counts[element];
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

// Decodes the UTF-8 character containing `c`, reading only bytes inside [begin, end).
// Returns the code point and the address of the character's first byte. A sequence
// that is truncated, malformed, or does not end at `c` decodes to 0.
static inline std::tuple<char32_t, const char*> get_utf8_char(const char* c, const char* begin, const char* end) {
  const unsigned char* p = reinterpret_cast<const unsigned char*>(c);
  const unsigned char* b = reinterpret_cast<const unsigned char*>(begin);
  const unsigned char* e = reinterpret_cast<const unsigned char*>(end);

  // 0x10xxxxxx is a continuation byte; back up to the leading byte.
  while (p > b && (p[0] >> 6) == 0b10) {
    p--;
  }

  const char* lead = reinterpret_cast<const char*>(p);
  int len;

  if (p[0] >> 7 == 0) {
    // 0x0xxxxxxx
    len = 1;
  } else if ((p[0] >> 5) == 0b110) {
    // 0x110xxxxx, 0x10xxxxxx
    len = 2;
  } else if ((p[0] >> 4) == 0b1110) {
    // 0x1110xxxx, 0x10xxxxxx, 0x10xxxxxx
    len = 3;
  } else if ((p[0] >> 3) == 0b11110) {
    // 0x11110xxx, 0x10xxxxxx, 0x10xxxxxx, 0x10xxxxxx
    len = 4;
  } else {
    return { 0, lead };
  }

  const unsigned char* last = reinterpret_cast<const unsigned char*>(c);

  if (last >= e || p + len - 1 != last) {
    // Either the sequence is truncated by the end of the input (the missing bytes are
    // not ours to read), or `c` is a stray continuation byte that does not complete the
    // sequence started at `lead`. Neither is a well-formed character.
    return { 0, lead };
  }

  switch (len) {
    case 1:  return { p[0], lead };
    case 2:  return { ((p[0] & 0b00011111) << 6) | (p[1] & 0b00111111), lead };
    case 3:  return { ((p[0] & 0b00001111) << 12) | (p[1] & 0b00111111) << 6 | (p[2] & 0b00111111), lead };
    default: return { ((p[0] & 0b00000111) << 18) | (p[1] & 0b00111111) << 12 | (p[2] & 0b00111111) << 6 | (p[3] & 0b00111111), lead };
  }
}

// If the character containing `c` is a boundary character, returns the byte before
// that character (so that `+ 1` yields the trimmed end). Otherwise returns `c`.
// Boundary characters are trailing punctuation characters that should not be part
// of the matched text. Only bytes inside [begin, end) are read.
static inline const char* find_boundary_c(const char* c, const char* begin, const char* end) {
  auto [ch, lead] = get_utf8_char(c, begin, end);

  if (ch != 0 && std::binary_search(std::begin(boundary_characters), std::end(boundary_characters), ch)) {
    return lead - 1;
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

  
#line 506 "ext/dtext/dtext.cpp"
	{
	( sm->top) = 0;
	( sm->ts) = 0;
	( sm->te) = 0;
	( sm->act) = 0;
	}

#line 1211 "ext/dtext/dtext.cpp.rl"
  
#line 512 "ext/dtext/dtext.cpp"
	{
	short _widec;
	if ( ( sm->p) == ( sm->pe) )
		goto _test_eof;
	goto _resume;

_again:
	switch (  sm->cs ) {
		case 1072: goto st1072;
		case 1073: goto st1073;
		case 0: goto st0;
		case 1074: goto st1074;
		case 1075: goto st1075;
		case 1: goto st1;
		case 1076: goto st1076;
		case 1077: goto st1077;
		case 2: goto st2;
		case 1078: goto st1078;
		case 3: goto st3;
		case 1079: goto st1079;
		case 1080: goto st1080;
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
		case 1081: goto st1081;
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
		case 151: goto st151;
		case 152: goto st152;
		case 153: goto st153;
		case 154: goto st154;
		case 155: goto st155;
		case 156: goto st156;
		case 157: goto st157;
		case 158: goto st158;
		case 159: goto st159;
		case 160: goto st160;
		case 161: goto st161;
		case 162: goto st162;
		case 163: goto st163;
		case 164: goto st164;
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
		case 195: goto st195;
		case 196: goto st196;
		case 197: goto st197;
		case 198: goto st198;
		case 199: goto st199;
		case 200: goto st200;
		case 201: goto st201;
		case 202: goto st202;
		case 203: goto st203;
		case 204: goto st204;
		case 205: goto st205;
		case 206: goto st206;
		case 207: goto st207;
		case 208: goto st208;
		case 209: goto st209;
		case 210: goto st210;
		case 211: goto st211;
		case 212: goto st212;
		case 213: goto st213;
		case 214: goto st214;
		case 215: goto st215;
		case 216: goto st216;
		case 217: goto st217;
		case 218: goto st218;
		case 219: goto st219;
		case 220: goto st220;
		case 221: goto st221;
		case 222: goto st222;
		case 223: goto st223;
		case 1082: goto st1082;
		case 224: goto st224;
		case 225: goto st225;
		case 226: goto st226;
		case 227: goto st227;
		case 228: goto st228;
		case 229: goto st229;
		case 230: goto st230;
		case 231: goto st231;
		case 232: goto st232;
		case 233: goto st233;
		case 234: goto st234;
		case 235: goto st235;
		case 236: goto st236;
		case 237: goto st237;
		case 238: goto st238;
		case 239: goto st239;
		case 240: goto st240;
		case 241: goto st241;
		case 242: goto st242;
		case 243: goto st243;
		case 244: goto st244;
		case 245: goto st245;
		case 246: goto st246;
		case 247: goto st247;
		case 248: goto st248;
		case 249: goto st249;
		case 250: goto st250;
		case 251: goto st251;
		case 252: goto st252;
		case 253: goto st253;
		case 254: goto st254;
		case 255: goto st255;
		case 256: goto st256;
		case 257: goto st257;
		case 258: goto st258;
		case 1083: goto st1083;
		case 1084: goto st1084;
		case 259: goto st259;
		case 260: goto st260;
		case 261: goto st261;
		case 262: goto st262;
		case 263: goto st263;
		case 264: goto st264;
		case 265: goto st265;
		case 266: goto st266;
		case 267: goto st267;
		case 268: goto st268;
		case 269: goto st269;
		case 270: goto st270;
		case 271: goto st271;
		case 272: goto st272;
		case 273: goto st273;
		case 274: goto st274;
		case 275: goto st275;
		case 276: goto st276;
		case 277: goto st277;
		case 1085: goto st1085;
		case 1086: goto st1086;
		case 278: goto st278;
		case 279: goto st279;
		case 280: goto st280;
		case 281: goto st281;
		case 282: goto st282;
		case 283: goto st283;
		case 284: goto st284;
		case 1087: goto st1087;
		case 285: goto st285;
		case 286: goto st286;
		case 287: goto st287;
		case 288: goto st288;
		case 289: goto st289;
		case 1088: goto st1088;
		case 1089: goto st1089;
		case 290: goto st290;
		case 291: goto st291;
		case 292: goto st292;
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
		case 1090: goto st1090;
		case 1091: goto st1091;
		case 1092: goto st1092;
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
		case 316: goto st316;
		case 317: goto st317;
		case 318: goto st318;
		case 319: goto st319;
		case 1093: goto st1093;
		case 1094: goto st1094;
		case 320: goto st320;
		case 321: goto st321;
		case 1095: goto st1095;
		case 322: goto st322;
		case 323: goto st323;
		case 324: goto st324;
		case 325: goto st325;
		case 326: goto st326;
		case 327: goto st327;
		case 328: goto st328;
		case 1096: goto st1096;
		case 329: goto st329;
		case 330: goto st330;
		case 331: goto st331;
		case 332: goto st332;
		case 333: goto st333;
		case 334: goto st334;
		case 335: goto st335;
		case 1097: goto st1097;
		case 1098: goto st1098;
		case 1099: goto st1099;
		case 336: goto st336;
		case 337: goto st337;
		case 338: goto st338;
		case 339: goto st339;
		case 1100: goto st1100;
		case 340: goto st340;
		case 341: goto st341;
		case 342: goto st342;
		case 343: goto st343;
		case 344: goto st344;
		case 345: goto st345;
		case 346: goto st346;
		case 347: goto st347;
		case 348: goto st348;
		case 349: goto st349;
		case 350: goto st350;
		case 351: goto st351;
		case 352: goto st352;
		case 353: goto st353;
		case 354: goto st354;
		case 355: goto st355;
		case 356: goto st356;
		case 1101: goto st1101;
		case 357: goto st357;
		case 358: goto st358;
		case 359: goto st359;
		case 360: goto st360;
		case 361: goto st361;
		case 362: goto st362;
		case 363: goto st363;
		case 364: goto st364;
		case 365: goto st365;
		case 1102: goto st1102;
		case 366: goto st366;
		case 367: goto st367;
		case 368: goto st368;
		case 369: goto st369;
		case 370: goto st370;
		case 371: goto st371;
		case 1103: goto st1103;
		case 372: goto st372;
		case 373: goto st373;
		case 374: goto st374;
		case 375: goto st375;
		case 376: goto st376;
		case 377: goto st377;
		case 378: goto st378;
		case 1104: goto st1104;
		case 379: goto st379;
		case 380: goto st380;
		case 381: goto st381;
		case 382: goto st382;
		case 383: goto st383;
		case 384: goto st384;
		case 385: goto st385;
		case 1105: goto st1105;
		case 1106: goto st1106;
		case 386: goto st386;
		case 387: goto st387;
		case 388: goto st388;
		case 389: goto st389;
		case 1107: goto st1107;
		case 390: goto st390;
		case 391: goto st391;
		case 392: goto st392;
		case 393: goto st393;
		case 394: goto st394;
		case 1108: goto st1108;
		case 395: goto st395;
		case 396: goto st396;
		case 397: goto st397;
		case 398: goto st398;
		case 1109: goto st1109;
		case 1110: goto st1110;
		case 399: goto st399;
		case 400: goto st400;
		case 401: goto st401;
		case 402: goto st402;
		case 403: goto st403;
		case 404: goto st404;
		case 405: goto st405;
		case 406: goto st406;
		case 1111: goto st1111;
		case 1112: goto st1112;
		case 407: goto st407;
		case 408: goto st408;
		case 409: goto st409;
		case 410: goto st410;
		case 411: goto st411;
		case 1113: goto st1113;
		case 412: goto st412;
		case 413: goto st413;
		case 414: goto st414;
		case 415: goto st415;
		case 416: goto st416;
		case 417: goto st417;
		case 1114: goto st1114;
		case 1115: goto st1115;
		case 418: goto st418;
		case 419: goto st419;
		case 420: goto st420;
		case 421: goto st421;
		case 422: goto st422;
		case 423: goto st423;
		case 1116: goto st1116;
		case 424: goto st424;
		case 1117: goto st1117;
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
		case 1118: goto st1118;
		case 1119: goto st1119;
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
		case 1120: goto st1120;
		case 1121: goto st1121;
		case 448: goto st448;
		case 449: goto st449;
		case 450: goto st450;
		case 451: goto st451;
		case 452: goto st452;
		case 1122: goto st1122;
		case 1123: goto st1123;
		case 453: goto st453;
		case 454: goto st454;
		case 455: goto st455;
		case 456: goto st456;
		case 457: goto st457;
		case 1124: goto st1124;
		case 458: goto st458;
		case 459: goto st459;
		case 460: goto st460;
		case 461: goto st461;
		case 1125: goto st1125;
		case 462: goto st462;
		case 463: goto st463;
		case 464: goto st464;
		case 465: goto st465;
		case 466: goto st466;
		case 467: goto st467;
		case 468: goto st468;
		case 469: goto st469;
		case 470: goto st470;
		case 1126: goto st1126;
		case 1127: goto st1127;
		case 471: goto st471;
		case 472: goto st472;
		case 473: goto st473;
		case 474: goto st474;
		case 475: goto st475;
		case 476: goto st476;
		case 477: goto st477;
		case 1128: goto st1128;
		case 1129: goto st1129;
		case 478: goto st478;
		case 479: goto st479;
		case 480: goto st480;
		case 481: goto st481;
		case 1130: goto st1130;
		case 1131: goto st1131;
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
		case 1132: goto st1132;
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
		case 1133: goto st1133;
		case 506: goto st506;
		case 507: goto st507;
		case 508: goto st508;
		case 509: goto st509;
		case 510: goto st510;
		case 511: goto st511;
		case 512: goto st512;
		case 1134: goto st1134;
		case 513: goto st513;
		case 514: goto st514;
		case 515: goto st515;
		case 516: goto st516;
		case 517: goto st517;
		case 518: goto st518;
		case 1135: goto st1135;
		case 1136: goto st1136;
		case 519: goto st519;
		case 520: goto st520;
		case 521: goto st521;
		case 522: goto st522;
		case 523: goto st523;
		case 1137: goto st1137;
		case 1138: goto st1138;
		case 524: goto st524;
		case 525: goto st525;
		case 526: goto st526;
		case 527: goto st527;
		case 528: goto st528;
		case 1139: goto st1139;
		case 1140: goto st1140;
		case 529: goto st529;
		case 530: goto st530;
		case 531: goto st531;
		case 532: goto st532;
		case 533: goto st533;
		case 534: goto st534;
		case 535: goto st535;
		case 536: goto st536;
		case 1141: goto st1141;
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
		case 1142: goto st1142;
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
		case 697: goto st697;
		case 698: goto st698;
		case 699: goto st699;
		case 700: goto st700;
		case 701: goto st701;
		case 702: goto st702;
		case 703: goto st703;
		case 704: goto st704;
		case 705: goto st705;
		case 706: goto st706;
		case 707: goto st707;
		case 708: goto st708;
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
		case 735: goto st735;
		case 736: goto st736;
		case 737: goto st737;
		case 738: goto st738;
		case 739: goto st739;
		case 740: goto st740;
		case 741: goto st741;
		case 742: goto st742;
		case 743: goto st743;
		case 744: goto st744;
		case 745: goto st745;
		case 746: goto st746;
		case 747: goto st747;
		case 748: goto st748;
		case 749: goto st749;
		case 750: goto st750;
		case 751: goto st751;
		case 752: goto st752;
		case 753: goto st753;
		case 754: goto st754;
		case 755: goto st755;
		case 756: goto st756;
		case 757: goto st757;
		case 758: goto st758;
		case 759: goto st759;
		case 760: goto st760;
		case 761: goto st761;
		case 762: goto st762;
		case 763: goto st763;
		case 764: goto st764;
		case 765: goto st765;
		case 766: goto st766;
		case 767: goto st767;
		case 768: goto st768;
		case 769: goto st769;
		case 770: goto st770;
		case 771: goto st771;
		case 772: goto st772;
		case 773: goto st773;
		case 774: goto st774;
		case 775: goto st775;
		case 776: goto st776;
		case 777: goto st777;
		case 778: goto st778;
		case 779: goto st779;
		case 780: goto st780;
		case 781: goto st781;
		case 782: goto st782;
		case 783: goto st783;
		case 784: goto st784;
		case 785: goto st785;
		case 786: goto st786;
		case 787: goto st787;
		case 788: goto st788;
		case 789: goto st789;
		case 790: goto st790;
		case 791: goto st791;
		case 792: goto st792;
		case 793: goto st793;
		case 794: goto st794;
		case 795: goto st795;
		case 796: goto st796;
		case 797: goto st797;
		case 798: goto st798;
		case 799: goto st799;
		case 800: goto st800;
		case 801: goto st801;
		case 802: goto st802;
		case 803: goto st803;
		case 804: goto st804;
		case 805: goto st805;
		case 806: goto st806;
		case 807: goto st807;
		case 808: goto st808;
		case 809: goto st809;
		case 810: goto st810;
		case 811: goto st811;
		case 812: goto st812;
		case 813: goto st813;
		case 814: goto st814;
		case 815: goto st815;
		case 816: goto st816;
		case 817: goto st817;
		case 818: goto st818;
		case 819: goto st819;
		case 820: goto st820;
		case 821: goto st821;
		case 822: goto st822;
		case 823: goto st823;
		case 824: goto st824;
		case 825: goto st825;
		case 826: goto st826;
		case 827: goto st827;
		case 828: goto st828;
		case 829: goto st829;
		case 830: goto st830;
		case 831: goto st831;
		case 832: goto st832;
		case 833: goto st833;
		case 834: goto st834;
		case 835: goto st835;
		case 836: goto st836;
		case 837: goto st837;
		case 838: goto st838;
		case 839: goto st839;
		case 840: goto st840;
		case 841: goto st841;
		case 842: goto st842;
		case 843: goto st843;
		case 844: goto st844;
		case 845: goto st845;
		case 846: goto st846;
		case 847: goto st847;
		case 848: goto st848;
		case 849: goto st849;
		case 850: goto st850;
		case 851: goto st851;
		case 852: goto st852;
		case 853: goto st853;
		case 854: goto st854;
		case 855: goto st855;
		case 856: goto st856;
		case 857: goto st857;
		case 858: goto st858;
		case 859: goto st859;
		case 860: goto st860;
		case 861: goto st861;
		case 862: goto st862;
		case 863: goto st863;
		case 864: goto st864;
		case 865: goto st865;
		case 866: goto st866;
		case 867: goto st867;
		case 868: goto st868;
		case 869: goto st869;
		case 870: goto st870;
		case 871: goto st871;
		case 872: goto st872;
		case 873: goto st873;
		case 874: goto st874;
		case 875: goto st875;
		case 876: goto st876;
		case 877: goto st877;
		case 878: goto st878;
		case 879: goto st879;
		case 880: goto st880;
		case 881: goto st881;
		case 882: goto st882;
		case 883: goto st883;
		case 884: goto st884;
		case 885: goto st885;
		case 886: goto st886;
		case 887: goto st887;
		case 888: goto st888;
		case 889: goto st889;
		case 890: goto st890;
		case 891: goto st891;
		case 892: goto st892;
		case 893: goto st893;
		case 894: goto st894;
		case 895: goto st895;
		case 896: goto st896;
		case 897: goto st897;
		case 898: goto st898;
		case 899: goto st899;
		case 900: goto st900;
		case 901: goto st901;
		case 902: goto st902;
		case 903: goto st903;
		case 904: goto st904;
		case 905: goto st905;
		case 906: goto st906;
		case 907: goto st907;
		case 908: goto st908;
		case 909: goto st909;
		case 910: goto st910;
		case 911: goto st911;
		case 912: goto st912;
		case 913: goto st913;
		case 914: goto st914;
		case 915: goto st915;
		case 916: goto st916;
		case 917: goto st917;
		case 918: goto st918;
		case 919: goto st919;
		case 920: goto st920;
		case 921: goto st921;
		case 922: goto st922;
		case 923: goto st923;
		case 924: goto st924;
		case 925: goto st925;
		case 926: goto st926;
		case 927: goto st927;
		case 928: goto st928;
		case 929: goto st929;
		case 930: goto st930;
		case 931: goto st931;
		case 932: goto st932;
		case 933: goto st933;
		case 934: goto st934;
		case 935: goto st935;
		case 936: goto st936;
		case 937: goto st937;
		case 938: goto st938;
		case 939: goto st939;
		case 940: goto st940;
		case 941: goto st941;
		case 942: goto st942;
		case 943: goto st943;
		case 944: goto st944;
		case 945: goto st945;
		case 946: goto st946;
		case 947: goto st947;
		case 948: goto st948;
		case 949: goto st949;
		case 950: goto st950;
		case 951: goto st951;
		case 952: goto st952;
		case 953: goto st953;
		case 954: goto st954;
		case 955: goto st955;
		case 956: goto st956;
		case 957: goto st957;
		case 958: goto st958;
		case 959: goto st959;
		case 960: goto st960;
		case 961: goto st961;
		case 962: goto st962;
		case 963: goto st963;
		case 964: goto st964;
		case 965: goto st965;
		case 966: goto st966;
		case 967: goto st967;
		case 968: goto st968;
		case 969: goto st969;
		case 970: goto st970;
		case 971: goto st971;
		case 972: goto st972;
		case 973: goto st973;
		case 974: goto st974;
		case 975: goto st975;
		case 976: goto st976;
		case 977: goto st977;
		case 978: goto st978;
		case 979: goto st979;
		case 980: goto st980;
		case 981: goto st981;
		case 982: goto st982;
		case 983: goto st983;
		case 984: goto st984;
		case 985: goto st985;
		case 986: goto st986;
		case 987: goto st987;
		case 988: goto st988;
		case 989: goto st989;
		case 990: goto st990;
		case 991: goto st991;
		case 992: goto st992;
		case 993: goto st993;
		case 994: goto st994;
		case 995: goto st995;
		case 996: goto st996;
		case 997: goto st997;
		case 998: goto st998;
		case 999: goto st999;
		case 1000: goto st1000;
		case 1001: goto st1001;
		case 1002: goto st1002;
		case 1003: goto st1003;
		case 1004: goto st1004;
		case 1005: goto st1005;
		case 1006: goto st1006;
		case 1007: goto st1007;
		case 1008: goto st1008;
		case 1009: goto st1009;
		case 1010: goto st1010;
		case 1011: goto st1011;
		case 1012: goto st1012;
		case 1013: goto st1013;
		case 1014: goto st1014;
		case 1015: goto st1015;
		case 1016: goto st1016;
		case 1017: goto st1017;
		case 1018: goto st1018;
		case 1019: goto st1019;
		case 1020: goto st1020;
		case 1021: goto st1021;
		case 1022: goto st1022;
		case 1023: goto st1023;
		case 1024: goto st1024;
		case 1025: goto st1025;
		case 1026: goto st1026;
		case 1027: goto st1027;
		case 1028: goto st1028;
		case 1029: goto st1029;
		case 1030: goto st1030;
		case 1143: goto st1143;
		case 1144: goto st1144;
		case 1031: goto st1031;
		case 1032: goto st1032;
		case 1033: goto st1033;
		case 1034: goto st1034;
		case 1035: goto st1035;
		case 1036: goto st1036;
		case 1037: goto st1037;
		case 1038: goto st1038;
		case 1039: goto st1039;
		case 1040: goto st1040;
		case 1145: goto st1145;
		case 1146: goto st1146;
		case 1147: goto st1147;
		case 1148: goto st1148;
		case 1041: goto st1041;
		case 1042: goto st1042;
		case 1043: goto st1043;
		case 1044: goto st1044;
		case 1045: goto st1045;
		case 1149: goto st1149;
		case 1150: goto st1150;
		case 1046: goto st1046;
		case 1047: goto st1047;
		case 1048: goto st1048;
		case 1049: goto st1049;
		case 1050: goto st1050;
		case 1051: goto st1051;
		case 1052: goto st1052;
		case 1053: goto st1053;
		case 1054: goto st1054;
		case 1055: goto st1055;
		case 1056: goto st1056;
		case 1057: goto st1057;
		case 1058: goto st1058;
		case 1059: goto st1059;
		case 1060: goto st1060;
		case 1061: goto st1061;
		case 1062: goto st1062;
		case 1063: goto st1063;
		case 1064: goto st1064;
		case 1065: goto st1065;
		case 1066: goto st1066;
		case 1067: goto st1067;
		case 1068: goto st1068;
		case 1069: goto st1069;
		case 1070: goto st1070;
		case 1071: goto st1071;
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
	case 111:
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
	case 112:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("block newline");
  }
	break;
	}
	}
	goto st1072;
tr2:
#line 768 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1072;goto st1090;}}
  }}
	goto st1072;
tr16:
#line 706 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("block [/spoiler]");
    dstack_close_before_block();
    if (dstack_check( BLOCK_SPOILER)) {
      g_debug("  rewind");
      dstack_rewind();
    }
  }}
	goto st1072;
tr59:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 687 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote class=\"dtext-quote-color\" style=\"border-left-color:");
    if (!options.f_inline) {
      if(a1[0] == '#') {
        append("#");
        append_uri_escaped({ a1 + 1, a2 });
      } else {
        append_uri_escaped({ a1, a2 });
      }
      append("\">");
    }
  }}
	goto st1072;
tr67:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 678 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote class=\"dtext-sidebar-colored-");
    if (!options.f_inline) {
      append_uri_escaped({ a1, a2 });
      append("\">");
    }
  }}
	goto st1072;
tr304:
#line 739 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1072;goto st1149;}}
  }}
	goto st1072;
tr1153:
#line 768 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1072;goto st1090;}}
  }}
	goto st1072;
tr1160:
#line 768 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1072;goto st1090;}}
  }}
	goto st1072;
tr1161:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 745 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1072;goto st1090;}}
  }}
	goto st1072;
tr1164:
#line 660 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1072;goto st1090;}}
  }}
	goto st1072;
tr1171:
#line 715 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1072;goto st1147;}}
  }}
	goto st1072;
tr1172:
#line 673 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote>");
  }}
	goto st1072;
tr1173:
#line 734 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("block expanded [section=]");
    append_section({ a1, a2 }, true);
  }}
	goto st1072;
tr1175:
#line 725 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_section({}, true);
  }}
	goto st1072;
tr1176:
#line 729 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("block [section=]");
    append_section({ a1, a2 }, false);
  }}
	goto st1072;
tr1178:
#line 721 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_section({}, false);
  }}
	goto st1072;
tr1179:
#line 701 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_SPOILER, "<div class=\"spoiler\">");
  }}
	goto st1072;
st1072:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1072;
case 1072:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 1936 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr1154;
		case 13: goto st1074;
		case 42: goto tr1156;
		case 72: goto tr1157;
		case 91: goto tr1158;
		case 104: goto tr1157;
	}
	goto tr1153;
tr1:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 752 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 111;}
	goto st1073;
tr1154:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 764 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 112;}
	goto st1073;
st1073:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1073;
case 1073:
#line 1957 "ext/dtext/dtext.cpp"
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
st1074:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1074;
case 1074:
	if ( (*( sm->p)) == 10 )
		goto tr1154;
	goto tr1160;
tr1156:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1075;
st1075:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1075;
case 1075:
#line 1984 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr5;
		case 32: goto tr5;
		case 42: goto st2;
	}
	goto tr1160;
tr5:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1;
st1:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1;
case 1:
#line 1997 "ext/dtext/dtext.cpp"
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
	goto st1076;
st1076:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1076;
case 1076:
#line 2011 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr1161;
		case 13: goto tr1161;
	}
	goto st1076;
tr4:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st1077;
st1077:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1077;
case 1077:
#line 2023 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr4;
		case 10: goto tr1161;
		case 13: goto tr1161;
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
tr1157:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1078;
st1078:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1078;
case 1078:
#line 2047 "ext/dtext/dtext.cpp"
	if ( 49 <= (*( sm->p)) && (*( sm->p)) <= 54 )
		goto tr1163;
	goto tr1160;
tr1163:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st3;
st3:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof3;
case 3:
#line 2057 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 46 )
		goto tr7;
	goto tr2;
tr7:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1079;
st1079:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1079;
case 1079:
#line 2067 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto st1079;
		case 32: goto st1079;
	}
	goto tr1164;
tr1158:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1080;
st1080:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1080;
case 1080:
#line 2079 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 47: goto st4;
		case 67: goto st13;
		case 81: goto st17;
		case 83: goto st224;
		case 84: goto st285;
		case 99: goto st13;
		case 113: goto st17;
		case 115: goto st224;
		case 116: goto st285;
	}
	goto tr1160;
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
		goto st1081;
	goto tr2;
st1081:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1081;
case 1081:
	if ( (*( sm->p)) == 32 )
		goto st1081;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st1081;
	goto tr1171;
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
		case 93: goto st1082;
	}
	goto tr2;
st22:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof22;
case 22:
	switch( (*( sm->p)) ) {
		case 35: goto tr27;
		case 65: goto tr28;
		case 66: goto tr29;
		case 67: goto tr30;
		case 68: goto tr31;
		case 70: goto tr32;
		case 71: goto tr33;
		case 73: goto tr34;
		case 74: goto tr35;
		case 76: goto tr36;
		case 77: goto tr37;
		case 79: goto tr38;
		case 80: goto tr39;
		case 83: goto tr40;
		case 97: goto tr41;
		case 98: goto tr42;
		case 99: goto tr43;
		case 100: goto tr44;
		case 102: goto tr46;
		case 103: goto tr47;
		case 105: goto tr48;
		case 106: goto tr49;
		case 108: goto tr50;
		case 109: goto tr51;
		case 111: goto tr52;
		case 112: goto tr53;
		case 115: goto tr54;
	}
	if ( 101 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto tr45;
	goto tr2;
tr27:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st23;
st23:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof23;
case 23:
#line 2302 "ext/dtext/dtext.cpp"
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
		goto tr59;
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
		goto tr59;
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
		goto tr59;
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
		goto tr59;
	goto tr2;
tr28:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st30;
st30:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof30;
case 30:
#line 2396 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 68: goto st31;
		case 82: goto st35;
		case 100: goto st31;
		case 114: goto st35;
	}
	goto tr2;
st31:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof31;
case 31:
	switch( (*( sm->p)) ) {
		case 77: goto st32;
		case 109: goto st32;
	}
	goto tr2;
st32:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof32;
case 32:
	switch( (*( sm->p)) ) {
		case 73: goto st33;
		case 105: goto st33;
	}
	goto tr2;
st33:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof33;
case 33:
	switch( (*( sm->p)) ) {
		case 78: goto st34;
		case 110: goto st34;
	}
	goto tr2;
st34:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof34;
case 34:
	if ( (*( sm->p)) == 93 )
		goto tr67;
	goto tr2;
st35:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof35;
case 35:
	switch( (*( sm->p)) ) {
		case 84: goto st36;
		case 116: goto st36;
	}
	goto tr2;
st36:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof36;
case 36:
	switch( (*( sm->p)) ) {
		case 73: goto st37;
		case 93: goto tr67;
		case 105: goto st37;
	}
	goto tr2;
st37:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof37;
case 37:
	switch( (*( sm->p)) ) {
		case 83: goto st38;
		case 115: goto st38;
	}
	goto tr2;
st38:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof38;
case 38:
	switch( (*( sm->p)) ) {
		case 84: goto st34;
		case 116: goto st34;
	}
	goto tr2;
tr29:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st39;
st39:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof39;
case 39:
#line 2481 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st40;
		case 108: goto st40;
	}
	goto tr2;
st40:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof40;
case 40:
	switch( (*( sm->p)) ) {
		case 79: goto st41;
		case 111: goto st41;
	}
	goto tr2;
st41:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof41;
case 41:
	switch( (*( sm->p)) ) {
		case 67: goto st42;
		case 99: goto st42;
	}
	goto tr2;
st42:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof42;
case 42:
	switch( (*( sm->p)) ) {
		case 75: goto st43;
		case 107: goto st43;
	}
	goto tr2;
st43:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof43;
case 43:
	switch( (*( sm->p)) ) {
		case 69: goto st44;
		case 101: goto st44;
	}
	goto tr2;
st44:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof44;
case 44:
	switch( (*( sm->p)) ) {
		case 68: goto st34;
		case 100: goto st34;
	}
	goto tr2;
tr30:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st45;
st45:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof45;
case 45:
#line 2538 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st46;
		case 79: goto st53;
		case 104: goto st46;
		case 111: goto st53;
	}
	goto tr2;
st46:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof46;
case 46:
	switch( (*( sm->p)) ) {
		case 65: goto st47;
		case 93: goto tr67;
		case 97: goto st47;
	}
	goto tr2;
st47:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof47;
case 47:
	switch( (*( sm->p)) ) {
		case 82: goto st48;
		case 114: goto st48;
	}
	goto tr2;
st48:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof48;
case 48:
	switch( (*( sm->p)) ) {
		case 65: goto st49;
		case 93: goto tr67;
		case 97: goto st49;
	}
	goto tr2;
st49:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof49;
case 49:
	switch( (*( sm->p)) ) {
		case 67: goto st50;
		case 99: goto st50;
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
		case 69: goto st52;
		case 101: goto st52;
	}
	goto tr2;
st52:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof52;
case 52:
	switch( (*( sm->p)) ) {
		case 82: goto st34;
		case 114: goto st34;
	}
	goto tr2;
st53:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof53;
case 53:
	switch( (*( sm->p)) ) {
		case 78: goto st54;
		case 80: goto st61;
		case 110: goto st54;
		case 112: goto st61;
	}
	goto tr2;
st54:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof54;
case 54:
	switch( (*( sm->p)) ) {
		case 84: goto st55;
		case 116: goto st55;
	}
	goto tr2;
st55:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof55;
case 55:
	switch( (*( sm->p)) ) {
		case 82: goto st56;
		case 93: goto tr67;
		case 114: goto st56;
	}
	goto tr2;
st56:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof56;
case 56:
	switch( (*( sm->p)) ) {
		case 73: goto st57;
		case 105: goto st57;
	}
	goto tr2;
st57:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof57;
case 57:
	switch( (*( sm->p)) ) {
		case 66: goto st58;
		case 98: goto st58;
	}
	goto tr2;
st58:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof58;
case 58:
	switch( (*( sm->p)) ) {
		case 85: goto st59;
		case 117: goto st59;
	}
	goto tr2;
st59:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof59;
case 59:
	switch( (*( sm->p)) ) {
		case 84: goto st60;
		case 116: goto st60;
	}
	goto tr2;
st60:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof60;
case 60:
	switch( (*( sm->p)) ) {
		case 79: goto st52;
		case 111: goto st52;
	}
	goto tr2;
st61:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof61;
case 61:
	switch( (*( sm->p)) ) {
		case 89: goto st62;
		case 121: goto st62;
	}
	goto tr2;
st62:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof62;
case 62:
	switch( (*( sm->p)) ) {
		case 82: goto st63;
		case 93: goto tr67;
		case 114: goto st63;
	}
	goto tr2;
st63:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof63;
case 63:
	switch( (*( sm->p)) ) {
		case 73: goto st64;
		case 105: goto st64;
	}
	goto tr2;
st64:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof64;
case 64:
	switch( (*( sm->p)) ) {
		case 71: goto st65;
		case 103: goto st65;
	}
	goto tr2;
st65:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof65;
case 65:
	switch( (*( sm->p)) ) {
		case 72: goto st38;
		case 104: goto st38;
	}
	goto tr2;
tr31:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st66;
st66:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof66;
case 66:
#line 2738 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 73: goto st67;
		case 105: goto st67;
	}
	goto tr2;
st67:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof67;
case 67:
	switch( (*( sm->p)) ) {
		case 82: goto st68;
		case 114: goto st68;
	}
	goto tr2;
st68:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof68;
case 68:
	switch( (*( sm->p)) ) {
		case 69: goto st69;
		case 93: goto tr67;
		case 101: goto st69;
	}
	goto tr2;
st69:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof69;
case 69:
	switch( (*( sm->p)) ) {
		case 67: goto st70;
		case 99: goto st70;
	}
	goto tr2;
st70:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof70;
case 70:
	switch( (*( sm->p)) ) {
		case 84: goto st71;
		case 116: goto st71;
	}
	goto tr2;
st71:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof71;
case 71:
	switch( (*( sm->p)) ) {
		case 79: goto st52;
		case 93: goto tr67;
		case 111: goto st52;
	}
	goto tr2;
tr32:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st72;
st72:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof72;
case 72:
#line 2797 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st73;
		case 82: goto st83;
		case 111: goto st73;
		case 114: goto st83;
	}
	goto tr2;
st73:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof73;
case 73:
	switch( (*( sm->p)) ) {
		case 82: goto st74;
		case 114: goto st74;
	}
	goto tr2;
st74:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof74;
case 74:
	switch( (*( sm->p)) ) {
		case 77: goto st75;
		case 109: goto st75;
	}
	goto tr2;
st75:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof75;
case 75:
	switch( (*( sm->p)) ) {
		case 69: goto st76;
		case 101: goto st76;
	}
	goto tr2;
st76:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof76;
case 76:
	switch( (*( sm->p)) ) {
		case 82: goto st77;
		case 114: goto st77;
	}
	goto tr2;
st77:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof77;
case 77:
	switch( (*( sm->p)) ) {
		case 45: goto st78;
		case 93: goto tr67;
	}
	goto tr2;
st78:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof78;
case 78:
	switch( (*( sm->p)) ) {
		case 83: goto st79;
		case 115: goto st79;
	}
	goto tr2;
st79:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof79;
case 79:
	switch( (*( sm->p)) ) {
		case 84: goto st80;
		case 116: goto st80;
	}
	goto tr2;
st80:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof80;
case 80:
	switch( (*( sm->p)) ) {
		case 65: goto st81;
		case 97: goto st81;
	}
	goto tr2;
st81:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof81;
case 81:
	switch( (*( sm->p)) ) {
		case 70: goto st82;
		case 102: goto st82;
	}
	goto tr2;
st82:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof82;
case 82:
	switch( (*( sm->p)) ) {
		case 70: goto st34;
		case 102: goto st34;
	}
	goto tr2;
st83:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof83;
case 83:
	switch( (*( sm->p)) ) {
		case 65: goto st84;
		case 72: goto st87;
		case 93: goto tr67;
		case 97: goto st84;
		case 104: goto st87;
	}
	goto tr2;
st84:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof84;
case 84:
	switch( (*( sm->p)) ) {
		case 78: goto st85;
		case 110: goto st85;
	}
	goto tr2;
st85:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof85;
case 85:
	switch( (*( sm->p)) ) {
		case 67: goto st86;
		case 99: goto st86;
	}
	goto tr2;
st86:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof86;
case 86:
	switch( (*( sm->p)) ) {
		case 72: goto st87;
		case 93: goto tr67;
		case 104: goto st87;
	}
	goto tr2;
st87:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof87;
case 87:
	switch( (*( sm->p)) ) {
		case 73: goto st88;
		case 105: goto st88;
	}
	goto tr2;
st88:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof88;
case 88:
	switch( (*( sm->p)) ) {
		case 83: goto st89;
		case 115: goto st89;
	}
	goto tr2;
st89:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof89;
case 89:
	switch( (*( sm->p)) ) {
		case 69: goto st34;
		case 101: goto st34;
	}
	goto tr2;
tr33:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st90;
st90:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof90;
case 90:
#line 2968 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st91;
		case 101: goto st91;
	}
	goto tr2;
st91:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof91;
case 91:
	switch( (*( sm->p)) ) {
		case 78: goto st92;
		case 110: goto st92;
	}
	goto tr2;
st92:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof92;
case 92:
	switch( (*( sm->p)) ) {
		case 69: goto st93;
		case 93: goto tr67;
		case 101: goto st93;
	}
	goto tr2;
st93:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof93;
case 93:
	switch( (*( sm->p)) ) {
		case 82: goto st94;
		case 114: goto st94;
	}
	goto tr2;
st94:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof94;
case 94:
	switch( (*( sm->p)) ) {
		case 65: goto st95;
		case 97: goto st95;
	}
	goto tr2;
st95:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof95;
case 95:
	switch( (*( sm->p)) ) {
		case 76: goto st34;
		case 108: goto st34;
	}
	goto tr2;
tr34:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st96;
st96:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof96;
case 96:
#line 3026 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st97;
		case 110: goto st97;
	}
	goto tr2;
st97:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof97;
case 97:
	switch( (*( sm->p)) ) {
		case 86: goto st98;
		case 118: goto st98;
	}
	goto tr2;
st98:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof98;
case 98:
	switch( (*( sm->p)) ) {
		case 65: goto st99;
		case 93: goto tr67;
		case 97: goto st99;
	}
	goto tr2;
st99:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof99;
case 99:
	switch( (*( sm->p)) ) {
		case 76: goto st100;
		case 108: goto st100;
	}
	goto tr2;
st100:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof100;
case 100:
	switch( (*( sm->p)) ) {
		case 73: goto st44;
		case 105: goto st44;
	}
	goto tr2;
tr35:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st101;
st101:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof101;
case 101:
#line 3075 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st102;
		case 97: goto st102;
	}
	goto tr2;
st102:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof102;
case 102:
	switch( (*( sm->p)) ) {
		case 78: goto st103;
		case 110: goto st103;
	}
	goto tr2;
st103:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof103;
case 103:
	switch( (*( sm->p)) ) {
		case 73: goto st59;
		case 93: goto tr67;
		case 105: goto st59;
	}
	goto tr2;
tr36:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st104;
st104:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof104;
case 104:
#line 3106 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st105;
		case 111: goto st105;
	}
	goto tr2;
st105:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof105;
case 105:
	switch( (*( sm->p)) ) {
		case 82: goto st106;
		case 114: goto st106;
	}
	goto tr2;
st106:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof106;
case 106:
	switch( (*( sm->p)) ) {
		case 69: goto st34;
		case 93: goto tr67;
		case 101: goto st34;
	}
	goto tr2;
tr37:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st107;
st107:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof107;
case 107:
#line 3137 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st108;
		case 79: goto st111;
		case 101: goto st108;
		case 111: goto st111;
	}
	goto tr2;
st108:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof108;
case 108:
	switch( (*( sm->p)) ) {
		case 77: goto st109;
		case 84: goto st110;
		case 109: goto st109;
		case 116: goto st110;
	}
	goto tr2;
st109:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof109;
case 109:
	switch( (*( sm->p)) ) {
		case 66: goto st51;
		case 98: goto st51;
	}
	goto tr2;
st110:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof110;
case 110:
	switch( (*( sm->p)) ) {
		case 65: goto st34;
		case 97: goto st34;
	}
	goto tr2;
st111:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof111;
case 111:
	switch( (*( sm->p)) ) {
		case 68: goto st112;
		case 100: goto st112;
	}
	goto tr2;
st112:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof112;
case 112:
	switch( (*( sm->p)) ) {
		case 69: goto st113;
		case 93: goto tr67;
		case 101: goto st113;
	}
	goto tr2;
st113:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof113;
case 113:
	switch( (*( sm->p)) ) {
		case 82: goto st114;
		case 114: goto st114;
	}
	goto tr2;
st114:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof114;
case 114:
	switch( (*( sm->p)) ) {
		case 65: goto st59;
		case 97: goto st59;
	}
	goto tr2;
tr38:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st115;
st115:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof115;
case 115:
#line 3217 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 67: goto st34;
		case 99: goto st34;
	}
	goto tr2;
tr39:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st116;
st116:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof116;
case 116:
#line 3229 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st117;
		case 114: goto st117;
	}
	goto tr2;
st117:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof117;
case 117:
	switch( (*( sm->p)) ) {
		case 73: goto st118;
		case 105: goto st118;
	}
	goto tr2;
st118:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof118;
case 118:
	switch( (*( sm->p)) ) {
		case 86: goto st119;
		case 118: goto st119;
	}
	goto tr2;
st119:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof119;
case 119:
	switch( (*( sm->p)) ) {
		case 73: goto st120;
		case 93: goto tr67;
		case 105: goto st120;
	}
	goto tr2;
st120:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof120;
case 120:
	switch( (*( sm->p)) ) {
		case 76: goto st121;
		case 108: goto st121;
	}
	goto tr2;
st121:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof121;
case 121:
	switch( (*( sm->p)) ) {
		case 69: goto st122;
		case 101: goto st122;
	}
	goto tr2;
st122:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof122;
case 122:
	switch( (*( sm->p)) ) {
		case 71: goto st43;
		case 103: goto st43;
	}
	goto tr2;
tr40:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st123;
st123:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof123;
case 123:
#line 3296 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st124;
		case 112: goto st124;
	}
	goto tr2;
st124:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof124;
case 124:
	switch( (*( sm->p)) ) {
		case 69: goto st125;
		case 101: goto st125;
	}
	goto tr2;
st125:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof125;
case 125:
	switch( (*( sm->p)) ) {
		case 67: goto st126;
		case 99: goto st126;
	}
	goto tr2;
st126:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof126;
case 126:
	switch( (*( sm->p)) ) {
		case 73: goto st127;
		case 93: goto tr67;
		case 105: goto st127;
	}
	goto tr2;
st127:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof127;
case 127:
	switch( (*( sm->p)) ) {
		case 69: goto st128;
		case 101: goto st128;
	}
	goto tr2;
st128:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof128;
case 128:
	switch( (*( sm->p)) ) {
		case 83: goto st34;
		case 115: goto st34;
	}
	goto tr2;
tr41:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st129;
st129:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof129;
case 129:
#line 3354 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 68: goto st31;
		case 82: goto st35;
		case 93: goto tr59;
		case 100: goto st131;
		case 114: goto st135;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr45:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st130;
st130:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof130;
case 130:
#line 3371 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr59;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st131:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof131;
case 131:
	switch( (*( sm->p)) ) {
		case 77: goto st32;
		case 93: goto tr59;
		case 109: goto st132;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st132:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof132;
case 132:
	switch( (*( sm->p)) ) {
		case 73: goto st33;
		case 93: goto tr59;
		case 105: goto st133;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st133:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof133;
case 133:
	switch( (*( sm->p)) ) {
		case 78: goto st34;
		case 93: goto tr59;
		case 110: goto st134;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st134:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof134;
case 134:
	if ( (*( sm->p)) == 93 )
		goto tr67;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st135:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof135;
case 135:
	switch( (*( sm->p)) ) {
		case 84: goto st36;
		case 93: goto tr59;
		case 116: goto st136;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st136:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof136;
case 136:
	switch( (*( sm->p)) ) {
		case 73: goto st37;
		case 93: goto tr67;
		case 105: goto st137;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st137:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof137;
case 137:
	switch( (*( sm->p)) ) {
		case 83: goto st38;
		case 93: goto tr59;
		case 115: goto st138;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st138:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof138;
case 138:
	switch( (*( sm->p)) ) {
		case 84: goto st34;
		case 93: goto tr59;
		case 116: goto st134;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr42:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st139;
st139:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof139;
case 139:
#line 3476 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st40;
		case 93: goto tr59;
		case 108: goto st140;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st140:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof140;
case 140:
	switch( (*( sm->p)) ) {
		case 79: goto st41;
		case 93: goto tr59;
		case 111: goto st141;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st141:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof141;
case 141:
	switch( (*( sm->p)) ) {
		case 67: goto st42;
		case 93: goto tr59;
		case 99: goto st142;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st142:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof142;
case 142:
	switch( (*( sm->p)) ) {
		case 75: goto st43;
		case 93: goto tr59;
		case 107: goto st143;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st143:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof143;
case 143:
	switch( (*( sm->p)) ) {
		case 69: goto st44;
		case 93: goto tr59;
		case 101: goto st144;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st144:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof144;
case 144:
	switch( (*( sm->p)) ) {
		case 68: goto st34;
		case 93: goto tr59;
		case 100: goto st134;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr43:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st145;
st145:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof145;
case 145:
#line 3551 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st46;
		case 79: goto st53;
		case 93: goto tr59;
		case 104: goto st146;
		case 111: goto st153;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st146:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof146;
case 146:
	switch( (*( sm->p)) ) {
		case 65: goto st47;
		case 93: goto tr67;
		case 97: goto st147;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st147:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof147;
case 147:
	switch( (*( sm->p)) ) {
		case 82: goto st48;
		case 93: goto tr59;
		case 114: goto st148;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st148:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof148;
case 148:
	switch( (*( sm->p)) ) {
		case 65: goto st49;
		case 93: goto tr67;
		case 97: goto st149;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st149:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof149;
case 149:
	switch( (*( sm->p)) ) {
		case 67: goto st50;
		case 93: goto tr59;
		case 99: goto st150;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st150:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof150;
case 150:
	switch( (*( sm->p)) ) {
		case 84: goto st51;
		case 93: goto tr59;
		case 116: goto st151;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st151:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof151;
case 151:
	switch( (*( sm->p)) ) {
		case 69: goto st52;
		case 93: goto tr59;
		case 101: goto st152;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st152:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof152;
case 152:
	switch( (*( sm->p)) ) {
		case 82: goto st34;
		case 93: goto tr59;
		case 114: goto st134;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st153:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof153;
case 153:
	switch( (*( sm->p)) ) {
		case 78: goto st54;
		case 80: goto st61;
		case 93: goto tr59;
		case 110: goto st154;
		case 112: goto st161;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st154:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof154;
case 154:
	switch( (*( sm->p)) ) {
		case 84: goto st55;
		case 93: goto tr59;
		case 116: goto st155;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st155:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof155;
case 155:
	switch( (*( sm->p)) ) {
		case 82: goto st56;
		case 93: goto tr67;
		case 114: goto st156;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st156:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof156;
case 156:
	switch( (*( sm->p)) ) {
		case 73: goto st57;
		case 93: goto tr59;
		case 105: goto st157;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st157:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof157;
case 157:
	switch( (*( sm->p)) ) {
		case 66: goto st58;
		case 93: goto tr59;
		case 98: goto st158;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st158:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof158;
case 158:
	switch( (*( sm->p)) ) {
		case 85: goto st59;
		case 93: goto tr59;
		case 117: goto st159;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st159:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof159;
case 159:
	switch( (*( sm->p)) ) {
		case 84: goto st60;
		case 93: goto tr59;
		case 116: goto st160;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st160:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof160;
case 160:
	switch( (*( sm->p)) ) {
		case 79: goto st52;
		case 93: goto tr59;
		case 111: goto st152;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st161:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof161;
case 161:
	switch( (*( sm->p)) ) {
		case 89: goto st62;
		case 93: goto tr59;
		case 121: goto st162;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st162:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof162;
case 162:
	switch( (*( sm->p)) ) {
		case 82: goto st63;
		case 93: goto tr67;
		case 114: goto st163;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st163:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof163;
case 163:
	switch( (*( sm->p)) ) {
		case 73: goto st64;
		case 93: goto tr59;
		case 105: goto st164;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st164:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof164;
case 164:
	switch( (*( sm->p)) ) {
		case 71: goto st65;
		case 93: goto tr59;
		case 103: goto st165;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st165:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof165;
case 165:
	switch( (*( sm->p)) ) {
		case 72: goto st38;
		case 93: goto tr59;
		case 104: goto st138;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr44:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st166;
st166:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof166;
case 166:
#line 3810 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 73: goto st67;
		case 93: goto tr59;
		case 105: goto st167;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st167:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof167;
case 167:
	switch( (*( sm->p)) ) {
		case 82: goto st68;
		case 93: goto tr59;
		case 114: goto st168;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st168:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof168;
case 168:
	switch( (*( sm->p)) ) {
		case 69: goto st69;
		case 93: goto tr67;
		case 101: goto st169;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st169:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof169;
case 169:
	switch( (*( sm->p)) ) {
		case 67: goto st70;
		case 93: goto tr59;
		case 99: goto st170;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st170:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof170;
case 170:
	switch( (*( sm->p)) ) {
		case 84: goto st71;
		case 93: goto tr59;
		case 116: goto st171;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st171:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof171;
case 171:
	switch( (*( sm->p)) ) {
		case 79: goto st52;
		case 93: goto tr67;
		case 111: goto st152;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr46:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st172;
st172:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof172;
case 172:
#line 3885 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st73;
		case 82: goto st83;
		case 93: goto tr59;
		case 111: goto st173;
		case 114: goto st178;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st173:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof173;
case 173:
	switch( (*( sm->p)) ) {
		case 82: goto st74;
		case 93: goto tr59;
		case 114: goto st174;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st174:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof174;
case 174:
	switch( (*( sm->p)) ) {
		case 77: goto st75;
		case 93: goto tr59;
		case 109: goto st175;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st175:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof175;
case 175:
	switch( (*( sm->p)) ) {
		case 69: goto st76;
		case 93: goto tr59;
		case 101: goto st176;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st176:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof176;
case 176:
	switch( (*( sm->p)) ) {
		case 82: goto st77;
		case 93: goto tr59;
		case 114: goto st177;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st177:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof177;
case 177:
	switch( (*( sm->p)) ) {
		case 45: goto st78;
		case 93: goto tr67;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st178:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof178;
case 178:
	switch( (*( sm->p)) ) {
		case 65: goto st84;
		case 72: goto st87;
		case 93: goto tr67;
		case 97: goto st179;
		case 104: goto st182;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st179:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof179;
case 179:
	switch( (*( sm->p)) ) {
		case 78: goto st85;
		case 93: goto tr59;
		case 110: goto st180;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st180:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof180;
case 180:
	switch( (*( sm->p)) ) {
		case 67: goto st86;
		case 93: goto tr59;
		case 99: goto st181;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st181:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof181;
case 181:
	switch( (*( sm->p)) ) {
		case 72: goto st87;
		case 93: goto tr67;
		case 104: goto st182;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st182:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof182;
case 182:
	switch( (*( sm->p)) ) {
		case 73: goto st88;
		case 93: goto tr59;
		case 105: goto st183;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st183:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof183;
case 183:
	switch( (*( sm->p)) ) {
		case 83: goto st89;
		case 93: goto tr59;
		case 115: goto st184;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st184:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof184;
case 184:
	switch( (*( sm->p)) ) {
		case 69: goto st34;
		case 93: goto tr59;
		case 101: goto st134;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr47:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st185;
st185:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof185;
case 185:
#line 4047 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st91;
		case 93: goto tr59;
		case 101: goto st186;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st186:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof186;
case 186:
	switch( (*( sm->p)) ) {
		case 78: goto st92;
		case 93: goto tr59;
		case 110: goto st187;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st187:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof187;
case 187:
	switch( (*( sm->p)) ) {
		case 69: goto st93;
		case 93: goto tr67;
		case 101: goto st188;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st188:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof188;
case 188:
	switch( (*( sm->p)) ) {
		case 82: goto st94;
		case 93: goto tr59;
		case 114: goto st189;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st189:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof189;
case 189:
	switch( (*( sm->p)) ) {
		case 65: goto st95;
		case 93: goto tr59;
		case 97: goto st190;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st190:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof190;
case 190:
	switch( (*( sm->p)) ) {
		case 76: goto st34;
		case 93: goto tr59;
		case 108: goto st134;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr48:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st191;
st191:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof191;
case 191:
#line 4122 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st97;
		case 93: goto tr59;
		case 110: goto st192;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st192:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof192;
case 192:
	switch( (*( sm->p)) ) {
		case 86: goto st98;
		case 93: goto tr59;
		case 118: goto st193;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st193:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof193;
case 193:
	switch( (*( sm->p)) ) {
		case 65: goto st99;
		case 93: goto tr67;
		case 97: goto st194;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st194:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof194;
case 194:
	switch( (*( sm->p)) ) {
		case 76: goto st100;
		case 93: goto tr59;
		case 108: goto st195;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st195:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof195;
case 195:
	switch( (*( sm->p)) ) {
		case 73: goto st44;
		case 93: goto tr59;
		case 105: goto st144;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr49:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st196;
st196:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof196;
case 196:
#line 4185 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st102;
		case 93: goto tr59;
		case 97: goto st197;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st197:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof197;
case 197:
	switch( (*( sm->p)) ) {
		case 78: goto st103;
		case 93: goto tr59;
		case 110: goto st198;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st198:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof198;
case 198:
	switch( (*( sm->p)) ) {
		case 73: goto st59;
		case 93: goto tr67;
		case 105: goto st159;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr50:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st199;
st199:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof199;
case 199:
#line 4224 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st105;
		case 93: goto tr59;
		case 111: goto st200;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st200:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof200;
case 200:
	switch( (*( sm->p)) ) {
		case 82: goto st106;
		case 93: goto tr59;
		case 114: goto st201;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st201:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof201;
case 201:
	switch( (*( sm->p)) ) {
		case 69: goto st34;
		case 93: goto tr67;
		case 101: goto st134;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr51:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st202;
st202:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof202;
case 202:
#line 4263 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st108;
		case 79: goto st111;
		case 93: goto tr59;
		case 101: goto st203;
		case 111: goto st206;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st203:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof203;
case 203:
	switch( (*( sm->p)) ) {
		case 77: goto st109;
		case 84: goto st110;
		case 93: goto tr59;
		case 109: goto st204;
		case 116: goto st205;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st204:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof204;
case 204:
	switch( (*( sm->p)) ) {
		case 66: goto st51;
		case 93: goto tr59;
		case 98: goto st151;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st205:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof205;
case 205:
	switch( (*( sm->p)) ) {
		case 65: goto st34;
		case 93: goto tr59;
		case 97: goto st134;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st206:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof206;
case 206:
	switch( (*( sm->p)) ) {
		case 68: goto st112;
		case 93: goto tr59;
		case 100: goto st207;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st207:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof207;
case 207:
	switch( (*( sm->p)) ) {
		case 69: goto st113;
		case 93: goto tr67;
		case 101: goto st208;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st208:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof208;
case 208:
	switch( (*( sm->p)) ) {
		case 82: goto st114;
		case 93: goto tr59;
		case 114: goto st209;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st209:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof209;
case 209:
	switch( (*( sm->p)) ) {
		case 65: goto st59;
		case 93: goto tr59;
		case 97: goto st159;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr52:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st210;
st210:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof210;
case 210:
#line 4366 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 67: goto st34;
		case 93: goto tr59;
		case 99: goto st134;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr53:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st211;
st211:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof211;
case 211:
#line 4381 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st117;
		case 93: goto tr59;
		case 114: goto st212;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st212:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof212;
case 212:
	switch( (*( sm->p)) ) {
		case 73: goto st118;
		case 93: goto tr59;
		case 105: goto st213;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st213:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof213;
case 213:
	switch( (*( sm->p)) ) {
		case 86: goto st119;
		case 93: goto tr59;
		case 118: goto st214;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st214:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof214;
case 214:
	switch( (*( sm->p)) ) {
		case 73: goto st120;
		case 93: goto tr67;
		case 105: goto st215;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st215:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof215;
case 215:
	switch( (*( sm->p)) ) {
		case 76: goto st121;
		case 93: goto tr59;
		case 108: goto st216;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st216:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof216;
case 216:
	switch( (*( sm->p)) ) {
		case 69: goto st122;
		case 93: goto tr59;
		case 101: goto st217;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st217:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof217;
case 217:
	switch( (*( sm->p)) ) {
		case 71: goto st43;
		case 93: goto tr59;
		case 103: goto st143;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
tr54:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st218;
st218:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof218;
case 218:
#line 4468 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st124;
		case 93: goto tr59;
		case 112: goto st219;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st219:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof219;
case 219:
	switch( (*( sm->p)) ) {
		case 69: goto st125;
		case 93: goto tr59;
		case 101: goto st220;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st220:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof220;
case 220:
	switch( (*( sm->p)) ) {
		case 67: goto st126;
		case 93: goto tr59;
		case 99: goto st221;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st221:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof221;
case 221:
	switch( (*( sm->p)) ) {
		case 73: goto st127;
		case 93: goto tr67;
		case 105: goto st222;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st222:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof222;
case 222:
	switch( (*( sm->p)) ) {
		case 69: goto st128;
		case 93: goto tr59;
		case 101: goto st223;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st223:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof223;
case 223:
	switch( (*( sm->p)) ) {
		case 83: goto st34;
		case 93: goto tr59;
		case 115: goto st134;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st130;
	goto tr2;
st1082:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1082;
case 1082:
	if ( (*( sm->p)) == 32 )
		goto st1082;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st1082;
	goto tr1172;
st224:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof224;
case 224:
	switch( (*( sm->p)) ) {
		case 69: goto st225;
		case 80: goto st278;
		case 101: goto st225;
		case 112: goto st278;
	}
	goto tr2;
st225:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof225;
case 225:
	switch( (*( sm->p)) ) {
		case 67: goto st226;
		case 99: goto st226;
	}
	goto tr2;
st226:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof226;
case 226:
	switch( (*( sm->p)) ) {
		case 84: goto st227;
		case 116: goto st227;
	}
	goto tr2;
st227:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof227;
case 227:
	switch( (*( sm->p)) ) {
		case 73: goto st228;
		case 105: goto st228;
	}
	goto tr2;
st228:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof228;
case 228:
	switch( (*( sm->p)) ) {
		case 79: goto st229;
		case 111: goto st229;
	}
	goto tr2;
st229:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof229;
case 229:
	switch( (*( sm->p)) ) {
		case 78: goto st230;
		case 110: goto st230;
	}
	goto tr2;
st230:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof230;
case 230:
	switch( (*( sm->p)) ) {
		case 44: goto st231;
		case 61: goto st259;
		case 93: goto st1086;
	}
	goto tr2;
st231:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof231;
case 231:
	switch( (*( sm->p)) ) {
		case 69: goto st232;
		case 101: goto st232;
	}
	goto tr2;
st232:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof232;
case 232:
	switch( (*( sm->p)) ) {
		case 88: goto st233;
		case 120: goto st233;
	}
	goto tr2;
st233:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof233;
case 233:
	switch( (*( sm->p)) ) {
		case 80: goto st234;
		case 112: goto st234;
	}
	goto tr2;
st234:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof234;
case 234:
	switch( (*( sm->p)) ) {
		case 65: goto st235;
		case 97: goto st235;
	}
	goto tr2;
st235:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof235;
case 235:
	switch( (*( sm->p)) ) {
		case 78: goto st236;
		case 110: goto st236;
	}
	goto tr2;
st236:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof236;
case 236:
	switch( (*( sm->p)) ) {
		case 68: goto st237;
		case 100: goto st237;
	}
	goto tr2;
st237:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof237;
case 237:
	switch( (*( sm->p)) ) {
		case 69: goto st238;
		case 101: goto st238;
	}
	goto tr2;
st238:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof238;
case 238:
	switch( (*( sm->p)) ) {
		case 68: goto st239;
		case 100: goto st239;
	}
	goto tr2;
st239:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof239;
case 239:
	switch( (*( sm->p)) ) {
		case 61: goto st240;
		case 93: goto st1084;
	}
	goto tr2;
st240:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof240;
case 240:
	switch( (*( sm->p)) ) {
		case 91: goto tr252;
		case 93: goto tr2;
	}
	goto tr251;
tr251:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st241;
st241:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof241;
case 241:
#line 4708 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 91: goto st242;
		case 93: goto tr255;
	}
	goto st241;
tr252:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st242;
st242:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof242;
case 242:
#line 4720 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 83: goto st243;
		case 91: goto st242;
		case 93: goto tr255;
		case 115: goto st243;
	}
	goto st241;
st243:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof243;
case 243:
	switch( (*( sm->p)) ) {
		case 69: goto st244;
		case 91: goto st242;
		case 93: goto tr255;
		case 101: goto st244;
	}
	goto st241;
st244:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof244;
case 244:
	switch( (*( sm->p)) ) {
		case 67: goto st245;
		case 91: goto st242;
		case 93: goto tr255;
		case 99: goto st245;
	}
	goto st241;
st245:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof245;
case 245:
	switch( (*( sm->p)) ) {
		case 84: goto st246;
		case 91: goto st242;
		case 93: goto tr255;
		case 116: goto st246;
	}
	goto st241;
st246:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof246;
case 246:
	switch( (*( sm->p)) ) {
		case 73: goto st247;
		case 91: goto st242;
		case 93: goto tr255;
		case 105: goto st247;
	}
	goto st241;
st247:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof247;
case 247:
	switch( (*( sm->p)) ) {
		case 79: goto st248;
		case 91: goto st242;
		case 93: goto tr255;
		case 111: goto st248;
	}
	goto st241;
st248:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof248;
case 248:
	switch( (*( sm->p)) ) {
		case 78: goto st249;
		case 91: goto st242;
		case 93: goto tr255;
		case 110: goto st249;
	}
	goto st241;
st249:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof249;
case 249:
	switch( (*( sm->p)) ) {
		case 44: goto st250;
		case 61: goto tr2;
		case 91: goto st242;
		case 93: goto tr255;
	}
	goto st241;
st250:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof250;
case 250:
	switch( (*( sm->p)) ) {
		case 69: goto st251;
		case 91: goto st242;
		case 93: goto tr255;
		case 101: goto st251;
	}
	goto st241;
st251:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof251;
case 251:
	switch( (*( sm->p)) ) {
		case 88: goto st252;
		case 91: goto st242;
		case 93: goto tr255;
		case 120: goto st252;
	}
	goto st241;
st252:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof252;
case 252:
	switch( (*( sm->p)) ) {
		case 80: goto st253;
		case 91: goto st242;
		case 93: goto tr255;
		case 112: goto st253;
	}
	goto st241;
st253:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof253;
case 253:
	switch( (*( sm->p)) ) {
		case 65: goto st254;
		case 91: goto st242;
		case 93: goto tr255;
		case 97: goto st254;
	}
	goto st241;
st254:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof254;
case 254:
	switch( (*( sm->p)) ) {
		case 78: goto st255;
		case 91: goto st242;
		case 93: goto tr255;
		case 110: goto st255;
	}
	goto st241;
st255:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof255;
case 255:
	switch( (*( sm->p)) ) {
		case 68: goto st256;
		case 91: goto st242;
		case 93: goto tr255;
		case 100: goto st256;
	}
	goto st241;
st256:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof256;
case 256:
	switch( (*( sm->p)) ) {
		case 69: goto st257;
		case 91: goto st242;
		case 93: goto tr255;
		case 101: goto st257;
	}
	goto st241;
st257:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof257;
case 257:
	switch( (*( sm->p)) ) {
		case 68: goto st258;
		case 91: goto st242;
		case 93: goto tr255;
		case 100: goto st258;
	}
	goto st241;
st258:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof258;
case 258:
	switch( (*( sm->p)) ) {
		case 61: goto tr2;
		case 91: goto st242;
		case 93: goto tr255;
	}
	goto st241;
tr255:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1083;
st1083:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1083;
case 1083:
#line 4909 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto st1083;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st1083;
	goto tr1173;
st1084:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1084;
case 1084:
	if ( (*( sm->p)) == 32 )
		goto st1084;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st1084;
	goto tr1175;
st259:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof259;
case 259:
	switch( (*( sm->p)) ) {
		case 91: goto tr273;
		case 93: goto tr2;
	}
	goto tr272;
tr272:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st260;
st260:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof260;
case 260:
#line 4939 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 91: goto st261;
		case 93: goto tr276;
	}
	goto st260;
tr273:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st261;
st261:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof261;
case 261:
#line 4951 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 83: goto st262;
		case 91: goto st261;
		case 93: goto tr276;
		case 115: goto st262;
	}
	goto st260;
st262:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof262;
case 262:
	switch( (*( sm->p)) ) {
		case 69: goto st263;
		case 91: goto st261;
		case 93: goto tr276;
		case 101: goto st263;
	}
	goto st260;
st263:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof263;
case 263:
	switch( (*( sm->p)) ) {
		case 67: goto st264;
		case 91: goto st261;
		case 93: goto tr276;
		case 99: goto st264;
	}
	goto st260;
st264:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof264;
case 264:
	switch( (*( sm->p)) ) {
		case 84: goto st265;
		case 91: goto st261;
		case 93: goto tr276;
		case 116: goto st265;
	}
	goto st260;
st265:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof265;
case 265:
	switch( (*( sm->p)) ) {
		case 73: goto st266;
		case 91: goto st261;
		case 93: goto tr276;
		case 105: goto st266;
	}
	goto st260;
st266:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof266;
case 266:
	switch( (*( sm->p)) ) {
		case 79: goto st267;
		case 91: goto st261;
		case 93: goto tr276;
		case 111: goto st267;
	}
	goto st260;
st267:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof267;
case 267:
	switch( (*( sm->p)) ) {
		case 78: goto st268;
		case 91: goto st261;
		case 93: goto tr276;
		case 110: goto st268;
	}
	goto st260;
st268:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof268;
case 268:
	switch( (*( sm->p)) ) {
		case 44: goto st269;
		case 61: goto tr2;
		case 91: goto st261;
		case 93: goto tr276;
	}
	goto st260;
st269:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof269;
case 269:
	switch( (*( sm->p)) ) {
		case 69: goto st270;
		case 91: goto st261;
		case 93: goto tr276;
		case 101: goto st270;
	}
	goto st260;
st270:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof270;
case 270:
	switch( (*( sm->p)) ) {
		case 88: goto st271;
		case 91: goto st261;
		case 93: goto tr276;
		case 120: goto st271;
	}
	goto st260;
st271:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof271;
case 271:
	switch( (*( sm->p)) ) {
		case 80: goto st272;
		case 91: goto st261;
		case 93: goto tr276;
		case 112: goto st272;
	}
	goto st260;
st272:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof272;
case 272:
	switch( (*( sm->p)) ) {
		case 65: goto st273;
		case 91: goto st261;
		case 93: goto tr276;
		case 97: goto st273;
	}
	goto st260;
st273:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof273;
case 273:
	switch( (*( sm->p)) ) {
		case 78: goto st274;
		case 91: goto st261;
		case 93: goto tr276;
		case 110: goto st274;
	}
	goto st260;
st274:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof274;
case 274:
	switch( (*( sm->p)) ) {
		case 68: goto st275;
		case 91: goto st261;
		case 93: goto tr276;
		case 100: goto st275;
	}
	goto st260;
st275:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof275;
case 275:
	switch( (*( sm->p)) ) {
		case 69: goto st276;
		case 91: goto st261;
		case 93: goto tr276;
		case 101: goto st276;
	}
	goto st260;
st276:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof276;
case 276:
	switch( (*( sm->p)) ) {
		case 68: goto st277;
		case 91: goto st261;
		case 93: goto tr276;
		case 100: goto st277;
	}
	goto st260;
st277:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof277;
case 277:
	switch( (*( sm->p)) ) {
		case 61: goto tr2;
		case 91: goto st261;
		case 93: goto tr276;
	}
	goto st260;
tr276:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1085;
st1085:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1085;
case 1085:
#line 5140 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto st1085;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st1085;
	goto tr1176;
st1086:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1086;
case 1086:
	if ( (*( sm->p)) == 32 )
		goto st1086;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st1086;
	goto tr1178;
st278:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof278;
case 278:
	switch( (*( sm->p)) ) {
		case 79: goto st279;
		case 111: goto st279;
	}
	goto tr2;
st279:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof279;
case 279:
	switch( (*( sm->p)) ) {
		case 73: goto st280;
		case 105: goto st280;
	}
	goto tr2;
st280:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof280;
case 280:
	switch( (*( sm->p)) ) {
		case 76: goto st281;
		case 108: goto st281;
	}
	goto tr2;
st281:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof281;
case 281:
	switch( (*( sm->p)) ) {
		case 69: goto st282;
		case 101: goto st282;
	}
	goto tr2;
st282:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof282;
case 282:
	switch( (*( sm->p)) ) {
		case 82: goto st283;
		case 114: goto st283;
	}
	goto tr2;
st283:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof283;
case 283:
	switch( (*( sm->p)) ) {
		case 83: goto st284;
		case 93: goto st1087;
		case 115: goto st284;
	}
	goto tr2;
st284:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof284;
case 284:
	if ( (*( sm->p)) == 93 )
		goto st1087;
	goto tr2;
st1087:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1087;
case 1087:
	if ( (*( sm->p)) == 32 )
		goto st1087;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st1087;
	goto tr1179;
st285:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof285;
case 285:
	switch( (*( sm->p)) ) {
		case 65: goto st286;
		case 97: goto st286;
	}
	goto tr2;
st286:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof286;
case 286:
	switch( (*( sm->p)) ) {
		case 66: goto st287;
		case 98: goto st287;
	}
	goto tr2;
st287:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof287;
case 287:
	switch( (*( sm->p)) ) {
		case 76: goto st288;
		case 108: goto st288;
	}
	goto tr2;
st288:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof288;
case 288:
	switch( (*( sm->p)) ) {
		case 69: goto st289;
		case 101: goto st289;
	}
	goto tr2;
st289:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof289;
case 289:
	if ( (*( sm->p)) == 93 )
		goto tr304;
	goto tr2;
tr305:
#line 246 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{ append_html_escaped((*( sm->p))); }}
	goto st1088;
tr310:
#line 211 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_B, "</strong>"); }}
	goto st1088;
tr311:
#line 213 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_I, "</em>"); }}
	goto st1088;
tr313:
#line 215 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_S, "</s>"); }}
	goto st1088;
tr316:
#line 239 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (ignored_sup_sub_tags > 0) {
      ignored_sup_sub_tags--;
    } else {
      dstack_close_inline(INLINE_SUB, "</sub>");
    }
  }}
	goto st1088;
tr317:
#line 225 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (ignored_sup_sub_tags > 0) {
      ignored_sup_sub_tags--;
    } else {
      dstack_close_inline(INLINE_SUP, "</sup>");
    }
  }}
	goto st1088;
tr318:
#line 217 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_U, "</u>"); }}
	goto st1088;
tr319:
#line 210 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_B, "<strong>"); }}
	goto st1088;
tr320:
#line 212 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_I, "<em>"); }}
	goto st1088;
tr322:
#line 214 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_S, "<s>"); }}
	goto st1088;
tr325:
#line 232 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_count(INLINE_SUP) + dstack_count(INLINE_SUB) < 3) {
      dstack_open_inline(INLINE_SUB, "<sub>");
    } else {
      ignored_sup_sub_tags++;
    }
  }}
	goto st1088;
tr326:
#line 218 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_count(INLINE_SUP) + dstack_count(INLINE_SUB) < 3) {
      dstack_open_inline(INLINE_SUP, "<sup>");
    } else {
      ignored_sup_sub_tags++;
    }
  }}
	goto st1088;
tr327:
#line 216 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_U, "<u>"); }}
	goto st1088;
tr1180:
#line 246 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ append_html_escaped((*( sm->p))); }}
	goto st1088;
tr1182:
#line 246 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_html_escaped((*( sm->p))); }}
	goto st1088;
st1088:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1088;
case 1088:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 5343 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 91 )
		goto tr1181;
	goto tr1180;
tr1181:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1089;
st1089:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1089;
case 1089:
#line 5353 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 47: goto st290;
		case 66: goto st298;
		case 73: goto st299;
		case 83: goto st300;
		case 85: goto st304;
		case 98: goto st298;
		case 105: goto st299;
		case 115: goto st300;
		case 117: goto st304;
	}
	goto tr1182;
st290:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof290;
case 290:
	switch( (*( sm->p)) ) {
		case 66: goto st291;
		case 73: goto st292;
		case 83: goto st293;
		case 85: goto st297;
		case 98: goto st291;
		case 105: goto st292;
		case 115: goto st293;
		case 117: goto st297;
	}
	goto tr305;
st291:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof291;
case 291:
	if ( (*( sm->p)) == 93 )
		goto tr310;
	goto tr305;
st292:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof292;
case 292:
	if ( (*( sm->p)) == 93 )
		goto tr311;
	goto tr305;
st293:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof293;
case 293:
	switch( (*( sm->p)) ) {
		case 85: goto st294;
		case 93: goto tr313;
		case 117: goto st294;
	}
	goto tr305;
st294:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof294;
case 294:
	switch( (*( sm->p)) ) {
		case 66: goto st295;
		case 80: goto st296;
		case 98: goto st295;
		case 112: goto st296;
	}
	goto tr305;
st295:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof295;
case 295:
	if ( (*( sm->p)) == 93 )
		goto tr316;
	goto tr305;
st296:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof296;
case 296:
	if ( (*( sm->p)) == 93 )
		goto tr317;
	goto tr305;
st297:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof297;
case 297:
	if ( (*( sm->p)) == 93 )
		goto tr318;
	goto tr305;
st298:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof298;
case 298:
	if ( (*( sm->p)) == 93 )
		goto tr319;
	goto tr305;
st299:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof299;
case 299:
	if ( (*( sm->p)) == 93 )
		goto tr320;
	goto tr305;
st300:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof300;
case 300:
	switch( (*( sm->p)) ) {
		case 85: goto st301;
		case 93: goto tr322;
		case 117: goto st301;
	}
	goto tr305;
st301:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof301;
case 301:
	switch( (*( sm->p)) ) {
		case 66: goto st302;
		case 80: goto st303;
		case 98: goto st302;
		case 112: goto st303;
	}
	goto tr305;
st302:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof302;
case 302:
	if ( (*( sm->p)) == 93 )
		goto tr325;
	goto tr305;
st303:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof303;
case 303:
	if ( (*( sm->p)) == 93 )
		goto tr326;
	goto tr305;
st304:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof304;
case 304:
	if ( (*( sm->p)) == 93 )
		goto tr327;
	goto tr305;
tr328:
#line 1 "NONE"
	{	switch( ( sm->act) ) {
	case 79:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }
	break;
	case 80:
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
	case 82:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }
	break;
	}
	}
	goto st1090;
tr330:
#line 551 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr341:
#line 435 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [/spoiler]");
    dstack_close_before_block();

    if (dstack_check(INLINE_SPOILER)) {
      dstack_close_inline(INLINE_SPOILER, "</span>");
    } else if (dstack_close_block(BLOCK_SPOILER, "</div>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st1090;
tr343:
#line 545 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_close_block(BLOCK_TD, "</td>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st1090;
tr344:
#line 561 "ext/dtext/dtext.cpp.rl"
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
	goto st1090;
tr366:
#line 579 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }}
	goto st1090;
tr384:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 336 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_named_url({ b1, b2 }, { a1, a2 });
  }}
	goto st1090;
tr400:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 352 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_unnamed_url({ a1, a2 });
  }}
	goto st1090;
tr568:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 259 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("<a id=\"");
    std::string lowercased_tag = std::string(a1, a2 - a1);
    std::transform(lowercased_tag.begin(), lowercased_tag.end(), lowercased_tag.begin(), [](unsigned char c) { return std::tolower(c); });
    append_uri_escaped(lowercased_tag);
    append("\"></a>");
  }}
	goto st1090;
tr575:
#line 363 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_B, "</strong>"); }}
	goto st1090;
tr583:
#line 424 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if(options.allow_color) {
      dstack_close_inline(INLINE_COLOR, "</span>");
    }
    {goto st1090;}
  }}
	goto st1090;
tr584:
#line 365 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_I, "</em>"); }}
	goto st1090;
tr586:
#line 367 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_S, "</s>"); }}
	goto st1090;
tr589:
#line 391 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (ignored_sup_sub_tags > 0) {
      ignored_sup_sub_tags--;
    } else {
      dstack_close_inline(INLINE_SUB, "</sub>");
    }
  }}
	goto st1090;
tr590:
#line 377 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (ignored_sup_sub_tags > 0) {
      ignored_sup_sub_tags--;
    } else {
      dstack_close_inline(INLINE_SUP, "</sup>");
    }
  }}
	goto st1090;
tr597:
#line 539 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_close_block(BLOCK_TH, "</th>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st1090;
tr598:
#line 369 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_U, "</u>"); }}
	goto st1090;
tr599:
#line 362 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_B, "<strong>"); }}
	goto st1090;
tr604:
#line 477 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_before_block();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr640:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 409 "ext/dtext/dtext.cpp.rl"
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
    {goto st1090;}
  }}
	goto st1090;
tr648:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 399 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if(options.allow_color) {
      dstack_push(INLINE_COLOR);
      append("<span class=\"dtext-color-");
      append_uri_escaped({ a1, a2 });
      append("\">");
    }
    {goto st1090;}
  }}
	goto st1090;
tr812:
#line 364 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_I, "<em>"); }}
	goto st1090;
tr818:
#line 499 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [quote]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr851:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 506 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [quote=color]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr859:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 513 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [quote=type]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr1026:
#line 366 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_S, "<s>"); }}
	goto st1090;
tr1034:
#line 526 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [section]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr1047:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 526 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [section]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr1070:
#line 431 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_inline(INLINE_SPOILER, "<span class=\"spoiler\">");
  }}
	goto st1090;
tr1073:
#line 384 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_count(INLINE_SUP) + dstack_count(INLINE_SUB) < 3) {
      dstack_open_inline(INLINE_SUB, "<sub>");
    } else {
      ignored_sup_sub_tags++;
    }
  }}
	goto st1090;
tr1074:
#line 370 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_count(INLINE_SUP) + dstack_count(INLINE_SUB) < 3) {
      dstack_open_inline(INLINE_SUP, "<sup>");
    } else {
      ignored_sup_sub_tags++;
    }
  }}
	goto st1090;
tr1079:
#line 455 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_before_block();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr1080:
#line 368 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_U, "<u>"); }}
	goto st1090;
tr1088:
#line 316 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_wiki_link({ a1, a2 }, { a1, a2 });
  }}
	goto st1090;
tr1094:
#line 320 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_wiki_link({ a1, a2 }, { b1, b2 });
  }}
	goto st1090;
tr1109:
#line 312 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_post_search_link({ a1, a2 }, { b1, b2 });
  }}
	goto st1090;
tr1110:
#line 308 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_post_search_link({ a1, a2 }, { a1, a2 });
  }}
	goto st1090;
tr1188:
#line 579 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }}
	goto st1090;
tr1209:
#line 254 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1090;goto st1145;}}
  }}
	goto st1090;
tr1211:
#line 561 "ext/dtext/dtext.cpp.rl"
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
	goto st1090;
tr1216:
#line 551 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr1218:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 356 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline list");
    {( sm->p) = (( ts + 1))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr1220:
#line 449 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr1222:
#line 520 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/quote]");
    dstack_close_until(BLOCK_QUOTE);
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr1223:
#line 533 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/expand]");
    dstack_close_until(BLOCK_SECTION);
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1090;
tr1224:
#line 575 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append(' ');
  }}
	goto st1090;
tr1225:
#line 579 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }}
	goto st1090;
tr1227:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 324 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    const char* match_end = b2;
    const char* url_start = b1;
    const char* url_end = find_boundary_c(match_end - 1, url_start, match_end) + 1;

    append_named_url({ url_start, url_end }, { a1, a2 });

    if (url_end < match_end) {
      append_html_escaped({ url_end, match_end });
    }
  }}
	goto st1090;
tr1232:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 297 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("alias", "tag-alias", "/tag_aliases/"); }}
	goto st1090;
tr1234:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 305 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("appeal", "appeal", "/appeals/"); }}
	goto st1090;
tr1236:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 294 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("artist", "artist", "/artists/"); }}
	goto st1090;
tr1241:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 295 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("ban", "ban", "/bans/"); }}
	goto st1090;
tr1243:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 303 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("blip", "blip", "/blips/"); }}
	goto st1090;
tr1245:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 296 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("BUR", "bulk-update-request", "/bulk_update_requests/"); }}
	goto st1090;
tr1248:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 291 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("comment", "comment", "/comments/"); }}
	goto st1090;
tr1252:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 287 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("flag", "post-flag", "/post_flags/"); }}
	goto st1090;
tr1254:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 289 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("forum", "forum-post", "/forum_posts/"); }}
	goto st1090;
tr1257:
#line 340 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    const char* match_end = te;
    const char* url_start = ts;
    const char* url_end = find_boundary_c(match_end - 1, url_start, match_end) + 1;

    append_unnamed_url({ url_start, url_end });

    if (url_end < match_end) {
      append_html_escaped({ url_end, match_end });
    }
  }}
	goto st1090;
tr1259:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 298 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("implication", "tag-implication", "/tag_implications/"); }}
	goto st1090;
tr1262:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 299 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("mod action", "mod-action", "/mod_actions/"); }}
	goto st1090;
tr1265:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 288 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("note", "note", "/notes/"); }}
	goto st1090;
tr1268:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 292 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("pool", "pool", "/pools/"); }}
	goto st1090;
tr1270:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 285 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("post", "post", "/posts/"); }}
	goto st1090;
tr1272:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 286 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("post changes", "post-changes-for", "/post_versions?search[post_id]="); }}
	goto st1090;
tr1275:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 300 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("record", "user-feedback", "/user_feedbacks/"); }}
	goto st1090;
tr1278:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 302 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("set", "set", "/post_sets/"); }}
	goto st1090;
tr1284:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 306 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("takedown", "takedown", "/takedowns/"); }}
	goto st1090;
tr1286:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 267 "ext/dtext/dtext.cpp.rl"
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
	goto st1090;
tr1288:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 304 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("ticket", "ticket", "/tickets/"); }}
	goto st1090;
tr1290:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 290 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("topic", "forum-topic", "/forum_topics/"); }}
	goto st1090;
tr1293:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 293 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("user", "user", "/users/"); }}
	goto st1090;
tr1296:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 301 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("wiki", "wiki-page", "/wiki_pages/"); }}
	goto st1090;
tr1308:
#line 483 "ext/dtext/dtext.cpp.rl"
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
	goto st1090;
tr1309:
#line 461 "ext/dtext/dtext.cpp.rl"
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
	goto st1090;
tr1310:
#line 250 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("`");
  }}
	goto st1090;
st1090:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1090;
case 1090:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 6058 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr1189;
		case 13: goto st1098;
		case 34: goto tr1191;
		case 60: goto tr1192;
		case 65: goto tr1193;
		case 66: goto tr1194;
		case 67: goto tr1195;
		case 70: goto tr1196;
		case 72: goto tr1197;
		case 73: goto tr1198;
		case 77: goto tr1199;
		case 78: goto tr1200;
		case 80: goto tr1201;
		case 82: goto tr1202;
		case 83: goto tr1203;
		case 84: goto tr1204;
		case 85: goto tr1205;
		case 87: goto tr1206;
		case 91: goto tr1207;
		case 92: goto st1143;
		case 96: goto tr1209;
		case 97: goto tr1193;
		case 98: goto tr1194;
		case 99: goto tr1195;
		case 102: goto tr1196;
		case 104: goto tr1197;
		case 105: goto tr1198;
		case 109: goto tr1199;
		case 110: goto tr1200;
		case 112: goto tr1201;
		case 114: goto tr1202;
		case 115: goto tr1203;
		case 116: goto tr1204;
		case 117: goto tr1205;
		case 119: goto tr1206;
		case 123: goto tr1210;
	}
	goto tr1188;
tr1189:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 561 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 80;}
	goto st1091;
st1091:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1091;
case 1091:
#line 6105 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr329;
		case 13: goto st305;
		case 42: goto tr1213;
		case 72: goto st320;
		case 91: goto st322;
		case 104: goto st320;
	}
	goto tr1211;
tr329:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 551 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 79;}
	goto st1092;
st1092:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1092;
case 1092:
#line 6122 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr329;
		case 13: goto st305;
		case 91: goto st306;
	}
	goto tr1216;
st305:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof305;
case 305:
	if ( (*( sm->p)) == 10 )
		goto tr329;
	goto tr328;
st306:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof306;
case 306:
	if ( (*( sm->p)) == 47 )
		goto st307;
	goto tr330;
st307:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof307;
case 307:
	switch( (*( sm->p)) ) {
		case 83: goto st308;
		case 84: goto st316;
		case 115: goto st308;
		case 116: goto st316;
	}
	goto tr330;
st308:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof308;
case 308:
	switch( (*( sm->p)) ) {
		case 80: goto st309;
		case 112: goto st309;
	}
	goto tr330;
st309:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof309;
case 309:
	switch( (*( sm->p)) ) {
		case 79: goto st310;
		case 111: goto st310;
	}
	goto tr328;
st310:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof310;
case 310:
	switch( (*( sm->p)) ) {
		case 73: goto st311;
		case 105: goto st311;
	}
	goto tr328;
st311:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof311;
case 311:
	switch( (*( sm->p)) ) {
		case 76: goto st312;
		case 108: goto st312;
	}
	goto tr328;
st312:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof312;
case 312:
	switch( (*( sm->p)) ) {
		case 69: goto st313;
		case 101: goto st313;
	}
	goto tr328;
st313:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof313;
case 313:
	switch( (*( sm->p)) ) {
		case 82: goto st314;
		case 114: goto st314;
	}
	goto tr328;
st314:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof314;
case 314:
	switch( (*( sm->p)) ) {
		case 83: goto st315;
		case 93: goto tr341;
		case 115: goto st315;
	}
	goto tr328;
st315:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof315;
case 315:
	if ( (*( sm->p)) == 93 )
		goto tr341;
	goto tr328;
st316:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof316;
case 316:
	switch( (*( sm->p)) ) {
		case 68: goto st317;
		case 100: goto st317;
	}
	goto tr328;
st317:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof317;
case 317:
	if ( (*( sm->p)) == 93 )
		goto tr343;
	goto tr328;
tr1213:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st318;
st318:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof318;
case 318:
#line 6247 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr345;
		case 32: goto tr345;
		case 42: goto st318;
	}
	goto tr344;
tr345:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st319;
st319:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof319;
case 319:
#line 6260 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr348;
		case 10: goto tr344;
		case 13: goto tr344;
		case 32: goto tr348;
	}
	goto tr347;
tr347:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st1093;
st1093:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1093;
case 1093:
#line 6274 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr1218;
		case 13: goto tr1218;
	}
	goto st1093;
tr348:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st1094;
st1094:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1094;
case 1094:
#line 6286 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr348;
		case 10: goto tr1218;
		case 13: goto tr1218;
		case 32: goto tr348;
	}
	goto tr347;
st320:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof320;
case 320:
	if ( 49 <= (*( sm->p)) && (*( sm->p)) <= 54 )
		goto tr349;
	goto tr344;
tr349:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st321;
st321:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof321;
case 321:
#line 6307 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 46 )
		goto tr350;
	goto tr344;
tr350:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1095;
st1095:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1095;
case 1095:
#line 6317 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto st1095;
		case 32: goto st1095;
	}
	goto tr1220;
st322:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof322;
case 322:
	if ( (*( sm->p)) == 47 )
		goto st323;
	goto tr344;
st323:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof323;
case 323:
	switch( (*( sm->p)) ) {
		case 81: goto st324;
		case 83: goto st329;
		case 84: goto st316;
		case 113: goto st324;
		case 115: goto st329;
		case 116: goto st316;
	}
	goto tr344;
st324:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof324;
case 324:
	switch( (*( sm->p)) ) {
		case 85: goto st325;
		case 117: goto st325;
	}
	goto tr328;
st325:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof325;
case 325:
	switch( (*( sm->p)) ) {
		case 79: goto st326;
		case 111: goto st326;
	}
	goto tr328;
st326:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof326;
case 326:
	switch( (*( sm->p)) ) {
		case 84: goto st327;
		case 116: goto st327;
	}
	goto tr328;
st327:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof327;
case 327:
	switch( (*( sm->p)) ) {
		case 69: goto st328;
		case 101: goto st328;
	}
	goto tr328;
st328:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof328;
case 328:
	_widec = (*( sm->p));
	if ( 93 <= (*( sm->p)) && (*( sm->p)) <= 93 ) {
		_widec = (short)(128 + ((*( sm->p)) - -128));
		if ( 
#line 97 "ext/dtext/dtext.cpp.rl"
 dstack_is_open(BLOCK_QUOTE)  ) _widec += 256;
	}
	if ( _widec == 605 )
		goto st1096;
	goto tr328;
st1096:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1096;
case 1096:
	switch( (*( sm->p)) ) {
		case 9: goto st1096;
		case 32: goto st1096;
	}
	goto tr1222;
st329:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof329;
case 329:
	switch( (*( sm->p)) ) {
		case 69: goto st330;
		case 80: goto st309;
		case 101: goto st330;
		case 112: goto st309;
	}
	goto tr344;
st330:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof330;
case 330:
	switch( (*( sm->p)) ) {
		case 67: goto st331;
		case 99: goto st331;
	}
	goto tr328;
st331:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof331;
case 331:
	switch( (*( sm->p)) ) {
		case 84: goto st332;
		case 116: goto st332;
	}
	goto tr328;
st332:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof332;
case 332:
	switch( (*( sm->p)) ) {
		case 73: goto st333;
		case 105: goto st333;
	}
	goto tr328;
st333:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof333;
case 333:
	switch( (*( sm->p)) ) {
		case 79: goto st334;
		case 111: goto st334;
	}
	goto tr328;
st334:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof334;
case 334:
	switch( (*( sm->p)) ) {
		case 78: goto st335;
		case 110: goto st335;
	}
	goto tr328;
st335:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof335;
case 335:
	_widec = (*( sm->p));
	if ( 93 <= (*( sm->p)) && (*( sm->p)) <= 93 ) {
		_widec = (short)(640 + ((*( sm->p)) - -128));
		if ( 
#line 98 "ext/dtext/dtext.cpp.rl"
 dstack_is_open(BLOCK_SECTION)  ) _widec += 256;
	}
	if ( _widec == 1117 )
		goto st1097;
	goto tr328;
st1097:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1097;
case 1097:
	switch( (*( sm->p)) ) {
		case 9: goto st1097;
		case 32: goto st1097;
	}
	goto tr1223;
st1098:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1098;
case 1098:
	if ( (*( sm->p)) == 10 )
		goto tr1189;
	goto tr1224;
tr1191:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1099;
st1099:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1099;
case 1099:
#line 6492 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 34 )
		goto tr1225;
	goto tr1226;
tr1226:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st336;
st336:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof336;
case 336:
#line 6502 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 34 )
		goto tr368;
	goto st336;
tr368:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st337;
st337:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof337;
case 337:
#line 6512 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 58 )
		goto st338;
	goto tr366;
st338:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof338;
case 338:
	switch( (*( sm->p)) ) {
		case 35: goto tr370;
		case 47: goto tr370;
		case 72: goto tr371;
		case 91: goto st347;
		case 104: goto tr371;
	}
	goto tr366;
tr370:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st339;
st339:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof339;
case 339:
#line 6534 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto tr366;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr366;
	goto st1100;
st1100:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1100;
case 1100:
	if ( (*( sm->p)) == 32 )
		goto tr1227;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr1227;
	goto st1100;
tr371:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st340;
st340:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof340;
case 340:
#line 6555 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st341;
		case 116: goto st341;
	}
	goto tr366;
st341:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof341;
case 341:
	switch( (*( sm->p)) ) {
		case 84: goto st342;
		case 116: goto st342;
	}
	goto tr366;
st342:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof342;
case 342:
	switch( (*( sm->p)) ) {
		case 80: goto st343;
		case 112: goto st343;
	}
	goto tr366;
st343:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof343;
case 343:
	switch( (*( sm->p)) ) {
		case 58: goto st344;
		case 83: goto st346;
		case 115: goto st346;
	}
	goto tr366;
st344:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof344;
case 344:
	if ( (*( sm->p)) == 47 )
		goto st345;
	goto tr366;
st345:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof345;
case 345:
	if ( (*( sm->p)) == 47 )
		goto st339;
	goto tr366;
st346:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof346;
case 346:
	if ( (*( sm->p)) == 58 )
		goto st344;
	goto tr366;
st347:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof347;
case 347:
	switch( (*( sm->p)) ) {
		case 35: goto tr381;
		case 47: goto tr381;
		case 72: goto tr382;
		case 104: goto tr382;
	}
	goto tr366;
tr381:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st348;
st348:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof348;
case 348:
#line 6627 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto tr366;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr366;
	goto st349;
st349:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof349;
case 349:
	switch( (*( sm->p)) ) {
		case 32: goto tr366;
		case 93: goto tr384;
	}
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr366;
	goto st349;
tr382:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st350;
st350:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof350;
case 350:
#line 6650 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st351;
		case 116: goto st351;
	}
	goto tr366;
st351:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof351;
case 351:
	switch( (*( sm->p)) ) {
		case 84: goto st352;
		case 116: goto st352;
	}
	goto tr366;
st352:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof352;
case 352:
	switch( (*( sm->p)) ) {
		case 80: goto st353;
		case 112: goto st353;
	}
	goto tr366;
st353:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof353;
case 353:
	switch( (*( sm->p)) ) {
		case 58: goto st354;
		case 83: goto st356;
		case 115: goto st356;
	}
	goto tr366;
st354:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof354;
case 354:
	if ( (*( sm->p)) == 47 )
		goto st355;
	goto tr366;
st355:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof355;
case 355:
	if ( (*( sm->p)) == 47 )
		goto st348;
	goto tr366;
st356:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof356;
case 356:
	if ( (*( sm->p)) == 58 )
		goto st354;
	goto tr366;
tr1192:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1101;
st1101:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1101;
case 1101:
#line 6711 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto tr1228;
		case 104: goto tr1228;
	}
	goto tr1225;
tr1228:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st357;
st357:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof357;
case 357:
#line 6723 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st358;
		case 116: goto st358;
	}
	goto tr366;
st358:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof358;
case 358:
	switch( (*( sm->p)) ) {
		case 84: goto st359;
		case 116: goto st359;
	}
	goto tr366;
st359:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof359;
case 359:
	switch( (*( sm->p)) ) {
		case 80: goto st360;
		case 112: goto st360;
	}
	goto tr366;
st360:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof360;
case 360:
	switch( (*( sm->p)) ) {
		case 58: goto st361;
		case 83: goto st365;
		case 115: goto st365;
	}
	goto tr366;
st361:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof361;
case 361:
	if ( (*( sm->p)) == 47 )
		goto st362;
	goto tr366;
st362:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof362;
case 362:
	if ( (*( sm->p)) == 47 )
		goto st363;
	goto tr366;
st363:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof363;
case 363:
	if ( (*( sm->p)) == 32 )
		goto tr366;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr366;
	goto st364;
st364:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof364;
case 364:
	switch( (*( sm->p)) ) {
		case 32: goto tr366;
		case 62: goto tr400;
	}
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr366;
	goto st364;
st365:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof365;
case 365:
	if ( (*( sm->p)) == 58 )
		goto st361;
	goto tr366;
tr1193:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1102;
st1102:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1102;
case 1102:
#line 6804 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st366;
		case 80: goto st372;
		case 82: goto st379;
		case 108: goto st366;
		case 112: goto st372;
		case 114: goto st379;
	}
	goto tr1225;
st366:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof366;
case 366:
	switch( (*( sm->p)) ) {
		case 73: goto st367;
		case 105: goto st367;
	}
	goto tr366;
st367:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof367;
case 367:
	switch( (*( sm->p)) ) {
		case 65: goto st368;
		case 97: goto st368;
	}
	goto tr366;
st368:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof368;
case 368:
	switch( (*( sm->p)) ) {
		case 83: goto st369;
		case 115: goto st369;
	}
	goto tr366;
st369:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof369;
case 369:
	if ( (*( sm->p)) == 32 )
		goto st370;
	goto tr366;
st370:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof370;
case 370:
	if ( (*( sm->p)) == 35 )
		goto st371;
	goto tr366;
st371:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof371;
case 371:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr406;
	goto tr366;
tr406:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1103;
st1103:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1103;
case 1103:
#line 6868 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1103;
	goto tr1232;
st372:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof372;
case 372:
	switch( (*( sm->p)) ) {
		case 80: goto st373;
		case 112: goto st373;
	}
	goto tr366;
st373:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof373;
case 373:
	switch( (*( sm->p)) ) {
		case 69: goto st374;
		case 101: goto st374;
	}
	goto tr366;
st374:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof374;
case 374:
	switch( (*( sm->p)) ) {
		case 65: goto st375;
		case 97: goto st375;
	}
	goto tr366;
st375:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof375;
case 375:
	switch( (*( sm->p)) ) {
		case 76: goto st376;
		case 108: goto st376;
	}
	goto tr366;
st376:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof376;
case 376:
	if ( (*( sm->p)) == 32 )
		goto st377;
	goto tr366;
st377:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof377;
case 377:
	if ( (*( sm->p)) == 35 )
		goto st378;
	goto tr366;
st378:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof378;
case 378:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr413;
	goto tr366;
tr413:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1104;
st1104:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1104;
case 1104:
#line 6935 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1104;
	goto tr1234;
st379:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof379;
case 379:
	switch( (*( sm->p)) ) {
		case 84: goto st380;
		case 116: goto st380;
	}
	goto tr366;
st380:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof380;
case 380:
	switch( (*( sm->p)) ) {
		case 73: goto st381;
		case 105: goto st381;
	}
	goto tr366;
st381:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof381;
case 381:
	switch( (*( sm->p)) ) {
		case 83: goto st382;
		case 115: goto st382;
	}
	goto tr366;
st382:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof382;
case 382:
	switch( (*( sm->p)) ) {
		case 84: goto st383;
		case 116: goto st383;
	}
	goto tr366;
st383:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof383;
case 383:
	if ( (*( sm->p)) == 32 )
		goto st384;
	goto tr366;
st384:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof384;
case 384:
	if ( (*( sm->p)) == 35 )
		goto st385;
	goto tr366;
st385:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof385;
case 385:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr420;
	goto tr366;
tr420:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1105;
st1105:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1105;
case 1105:
#line 7002 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1105;
	goto tr1236;
tr1194:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1106;
st1106:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1106;
case 1106:
#line 7012 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st386;
		case 76: goto st390;
		case 85: goto st395;
		case 97: goto st386;
		case 108: goto st390;
		case 117: goto st395;
	}
	goto tr1225;
st386:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof386;
case 386:
	switch( (*( sm->p)) ) {
		case 78: goto st387;
		case 110: goto st387;
	}
	goto tr366;
st387:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof387;
case 387:
	if ( (*( sm->p)) == 32 )
		goto st388;
	goto tr366;
st388:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof388;
case 388:
	if ( (*( sm->p)) == 35 )
		goto st389;
	goto tr366;
st389:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof389;
case 389:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr424;
	goto tr366;
tr424:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1107;
st1107:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1107;
case 1107:
#line 7058 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1107;
	goto tr1241;
st390:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof390;
case 390:
	switch( (*( sm->p)) ) {
		case 73: goto st391;
		case 105: goto st391;
	}
	goto tr366;
st391:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof391;
case 391:
	switch( (*( sm->p)) ) {
		case 80: goto st392;
		case 112: goto st392;
	}
	goto tr366;
st392:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof392;
case 392:
	if ( (*( sm->p)) == 32 )
		goto st393;
	goto tr366;
st393:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof393;
case 393:
	if ( (*( sm->p)) == 35 )
		goto st394;
	goto tr366;
st394:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof394;
case 394:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr429;
	goto tr366;
tr429:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1108;
st1108:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1108;
case 1108:
#line 7107 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1108;
	goto tr1243;
st395:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof395;
case 395:
	switch( (*( sm->p)) ) {
		case 82: goto st396;
		case 114: goto st396;
	}
	goto tr366;
st396:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof396;
case 396:
	if ( (*( sm->p)) == 32 )
		goto st397;
	goto tr366;
st397:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof397;
case 397:
	if ( (*( sm->p)) == 35 )
		goto st398;
	goto tr366;
st398:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof398;
case 398:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr433;
	goto tr366;
tr433:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1109;
st1109:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1109;
case 1109:
#line 7147 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1109;
	goto tr1245;
tr1195:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1110;
st1110:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1110;
case 1110:
#line 7157 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st399;
		case 111: goto st399;
	}
	goto tr1225;
st399:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof399;
case 399:
	switch( (*( sm->p)) ) {
		case 77: goto st400;
		case 109: goto st400;
	}
	goto tr366;
st400:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof400;
case 400:
	switch( (*( sm->p)) ) {
		case 77: goto st401;
		case 109: goto st401;
	}
	goto tr366;
st401:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof401;
case 401:
	switch( (*( sm->p)) ) {
		case 69: goto st402;
		case 101: goto st402;
	}
	goto tr366;
st402:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof402;
case 402:
	switch( (*( sm->p)) ) {
		case 78: goto st403;
		case 110: goto st403;
	}
	goto tr366;
st403:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof403;
case 403:
	switch( (*( sm->p)) ) {
		case 84: goto st404;
		case 116: goto st404;
	}
	goto tr366;
st404:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof404;
case 404:
	if ( (*( sm->p)) == 32 )
		goto st405;
	goto tr366;
st405:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof405;
case 405:
	if ( (*( sm->p)) == 35 )
		goto st406;
	goto tr366;
st406:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof406;
case 406:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr441;
	goto tr366;
tr441:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1111;
st1111:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1111;
case 1111:
#line 7235 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1111;
	goto tr1248;
tr1196:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1112;
st1112:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1112;
case 1112:
#line 7245 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st407;
		case 79: goto st412;
		case 108: goto st407;
		case 111: goto st412;
	}
	goto tr1225;
st407:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof407;
case 407:
	switch( (*( sm->p)) ) {
		case 65: goto st408;
		case 97: goto st408;
	}
	goto tr366;
st408:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof408;
case 408:
	switch( (*( sm->p)) ) {
		case 71: goto st409;
		case 103: goto st409;
	}
	goto tr366;
st409:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof409;
case 409:
	if ( (*( sm->p)) == 32 )
		goto st410;
	goto tr366;
st410:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof410;
case 410:
	if ( (*( sm->p)) == 35 )
		goto st411;
	goto tr366;
st411:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof411;
case 411:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr446;
	goto tr366;
tr446:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1113;
st1113:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1113;
case 1113:
#line 7298 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1113;
	goto tr1252;
st412:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof412;
case 412:
	switch( (*( sm->p)) ) {
		case 82: goto st413;
		case 114: goto st413;
	}
	goto tr366;
st413:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof413;
case 413:
	switch( (*( sm->p)) ) {
		case 85: goto st414;
		case 117: goto st414;
	}
	goto tr366;
st414:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof414;
case 414:
	switch( (*( sm->p)) ) {
		case 77: goto st415;
		case 109: goto st415;
	}
	goto tr366;
st415:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof415;
case 415:
	if ( (*( sm->p)) == 32 )
		goto st416;
	goto tr366;
st416:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof416;
case 416:
	if ( (*( sm->p)) == 35 )
		goto st417;
	goto tr366;
st417:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof417;
case 417:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr452;
	goto tr366;
tr452:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1114;
st1114:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1114;
case 1114:
#line 7356 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1114;
	goto tr1254;
tr1197:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1115;
st1115:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1115;
case 1115:
#line 7366 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st418;
		case 116: goto st418;
	}
	goto tr1225;
st418:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof418;
case 418:
	switch( (*( sm->p)) ) {
		case 84: goto st419;
		case 116: goto st419;
	}
	goto tr366;
st419:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof419;
case 419:
	switch( (*( sm->p)) ) {
		case 80: goto st420;
		case 112: goto st420;
	}
	goto tr366;
st420:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof420;
case 420:
	switch( (*( sm->p)) ) {
		case 58: goto st421;
		case 83: goto st424;
		case 115: goto st424;
	}
	goto tr366;
st421:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof421;
case 421:
	if ( (*( sm->p)) == 47 )
		goto st422;
	goto tr366;
st422:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof422;
case 422:
	if ( (*( sm->p)) == 47 )
		goto st423;
	goto tr366;
st423:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof423;
case 423:
	if ( (*( sm->p)) == 32 )
		goto tr366;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr366;
	goto st1116;
st1116:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1116;
case 1116:
	if ( (*( sm->p)) == 32 )
		goto tr1257;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr1257;
	goto st1116;
st424:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof424;
case 424:
	if ( (*( sm->p)) == 58 )
		goto st421;
	goto tr366;
tr1198:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1117;
st1117:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1117;
case 1117:
#line 7445 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 77: goto st425;
		case 109: goto st425;
	}
	goto tr1225;
st425:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof425;
case 425:
	switch( (*( sm->p)) ) {
		case 80: goto st426;
		case 112: goto st426;
	}
	goto tr366;
st426:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof426;
case 426:
	switch( (*( sm->p)) ) {
		case 76: goto st427;
		case 108: goto st427;
	}
	goto tr366;
st427:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof427;
case 427:
	switch( (*( sm->p)) ) {
		case 73: goto st428;
		case 105: goto st428;
	}
	goto tr366;
st428:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof428;
case 428:
	switch( (*( sm->p)) ) {
		case 67: goto st429;
		case 99: goto st429;
	}
	goto tr366;
st429:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof429;
case 429:
	switch( (*( sm->p)) ) {
		case 65: goto st430;
		case 97: goto st430;
	}
	goto tr366;
st430:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof430;
case 430:
	switch( (*( sm->p)) ) {
		case 84: goto st431;
		case 116: goto st431;
	}
	goto tr366;
st431:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof431;
case 431:
	switch( (*( sm->p)) ) {
		case 73: goto st432;
		case 105: goto st432;
	}
	goto tr366;
st432:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof432;
case 432:
	switch( (*( sm->p)) ) {
		case 79: goto st433;
		case 111: goto st433;
	}
	goto tr366;
st433:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof433;
case 433:
	switch( (*( sm->p)) ) {
		case 78: goto st434;
		case 110: goto st434;
	}
	goto tr366;
st434:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof434;
case 434:
	if ( (*( sm->p)) == 32 )
		goto st435;
	goto tr366;
st435:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof435;
case 435:
	if ( (*( sm->p)) == 35 )
		goto st436;
	goto tr366;
st436:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof436;
case 436:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr471;
	goto tr366;
tr471:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1118;
st1118:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1118;
case 1118:
#line 7559 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1118;
	goto tr1259;
tr1199:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1119;
st1119:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1119;
case 1119:
#line 7569 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st437;
		case 111: goto st437;
	}
	goto tr1225;
st437:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof437;
case 437:
	switch( (*( sm->p)) ) {
		case 68: goto st438;
		case 100: goto st438;
	}
	goto tr366;
st438:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof438;
case 438:
	if ( (*( sm->p)) == 32 )
		goto st439;
	goto tr366;
st439:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof439;
case 439:
	switch( (*( sm->p)) ) {
		case 65: goto st440;
		case 97: goto st440;
	}
	goto tr366;
st440:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof440;
case 440:
	switch( (*( sm->p)) ) {
		case 67: goto st441;
		case 99: goto st441;
	}
	goto tr366;
st441:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof441;
case 441:
	switch( (*( sm->p)) ) {
		case 84: goto st442;
		case 116: goto st442;
	}
	goto tr366;
st442:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof442;
case 442:
	switch( (*( sm->p)) ) {
		case 73: goto st443;
		case 105: goto st443;
	}
	goto tr366;
st443:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof443;
case 443:
	switch( (*( sm->p)) ) {
		case 79: goto st444;
		case 111: goto st444;
	}
	goto tr366;
st444:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof444;
case 444:
	switch( (*( sm->p)) ) {
		case 78: goto st445;
		case 110: goto st445;
	}
	goto tr366;
st445:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof445;
case 445:
	if ( (*( sm->p)) == 32 )
		goto st446;
	goto tr366;
st446:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof446;
case 446:
	if ( (*( sm->p)) == 35 )
		goto st447;
	goto tr366;
st447:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof447;
case 447:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr482;
	goto tr366;
tr482:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1120;
st1120:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1120;
case 1120:
#line 7672 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1120;
	goto tr1262;
tr1200:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1121;
st1121:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1121;
case 1121:
#line 7682 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st448;
		case 111: goto st448;
	}
	goto tr1225;
st448:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof448;
case 448:
	switch( (*( sm->p)) ) {
		case 84: goto st449;
		case 116: goto st449;
	}
	goto tr366;
st449:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof449;
case 449:
	switch( (*( sm->p)) ) {
		case 69: goto st450;
		case 101: goto st450;
	}
	goto tr366;
st450:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof450;
case 450:
	if ( (*( sm->p)) == 32 )
		goto st451;
	goto tr366;
st451:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof451;
case 451:
	if ( (*( sm->p)) == 35 )
		goto st452;
	goto tr366;
st452:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof452;
case 452:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr487;
	goto tr366;
tr487:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1122;
st1122:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1122;
case 1122:
#line 7733 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1122;
	goto tr1265;
tr1201:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1123;
st1123:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1123;
case 1123:
#line 7743 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st453;
		case 111: goto st453;
	}
	goto tr1225;
st453:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof453;
case 453:
	switch( (*( sm->p)) ) {
		case 79: goto st454;
		case 83: goto st458;
		case 111: goto st454;
		case 115: goto st458;
	}
	goto tr366;
st454:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof454;
case 454:
	switch( (*( sm->p)) ) {
		case 76: goto st455;
		case 108: goto st455;
	}
	goto tr366;
st455:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof455;
case 455:
	if ( (*( sm->p)) == 32 )
		goto st456;
	goto tr366;
st456:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof456;
case 456:
	if ( (*( sm->p)) == 35 )
		goto st457;
	goto tr366;
st457:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof457;
case 457:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr493;
	goto tr366;
tr493:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1124;
st1124:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1124;
case 1124:
#line 7796 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1124;
	goto tr1268;
st458:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof458;
case 458:
	switch( (*( sm->p)) ) {
		case 84: goto st459;
		case 116: goto st459;
	}
	goto tr366;
st459:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof459;
case 459:
	if ( (*( sm->p)) == 32 )
		goto st460;
	goto tr366;
st460:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof460;
case 460:
	switch( (*( sm->p)) ) {
		case 35: goto st461;
		case 67: goto st462;
		case 99: goto st462;
	}
	goto tr366;
st461:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof461;
case 461:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr498;
	goto tr366;
tr498:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1125;
st1125:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1125;
case 1125:
#line 7839 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1125;
	goto tr1270;
st462:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof462;
case 462:
	switch( (*( sm->p)) ) {
		case 72: goto st463;
		case 104: goto st463;
	}
	goto tr366;
st463:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof463;
case 463:
	switch( (*( sm->p)) ) {
		case 65: goto st464;
		case 97: goto st464;
	}
	goto tr366;
st464:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof464;
case 464:
	switch( (*( sm->p)) ) {
		case 78: goto st465;
		case 110: goto st465;
	}
	goto tr366;
st465:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof465;
case 465:
	switch( (*( sm->p)) ) {
		case 71: goto st466;
		case 103: goto st466;
	}
	goto tr366;
st466:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof466;
case 466:
	switch( (*( sm->p)) ) {
		case 69: goto st467;
		case 101: goto st467;
	}
	goto tr366;
st467:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof467;
case 467:
	switch( (*( sm->p)) ) {
		case 83: goto st468;
		case 115: goto st468;
	}
	goto tr366;
st468:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof468;
case 468:
	if ( (*( sm->p)) == 32 )
		goto st469;
	goto tr366;
st469:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof469;
case 469:
	if ( (*( sm->p)) == 35 )
		goto st470;
	goto tr366;
st470:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof470;
case 470:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr507;
	goto tr366;
tr507:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1126;
st1126:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1126;
case 1126:
#line 7924 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1126;
	goto tr1272;
tr1202:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1127;
st1127:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1127;
case 1127:
#line 7934 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st471;
		case 101: goto st471;
	}
	goto tr1225;
st471:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof471;
case 471:
	switch( (*( sm->p)) ) {
		case 67: goto st472;
		case 99: goto st472;
	}
	goto tr366;
st472:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof472;
case 472:
	switch( (*( sm->p)) ) {
		case 79: goto st473;
		case 111: goto st473;
	}
	goto tr366;
st473:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof473;
case 473:
	switch( (*( sm->p)) ) {
		case 82: goto st474;
		case 114: goto st474;
	}
	goto tr366;
st474:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof474;
case 474:
	switch( (*( sm->p)) ) {
		case 68: goto st475;
		case 100: goto st475;
	}
	goto tr366;
st475:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof475;
case 475:
	if ( (*( sm->p)) == 32 )
		goto st476;
	goto tr366;
st476:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof476;
case 476:
	if ( (*( sm->p)) == 35 )
		goto st477;
	goto tr366;
st477:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof477;
case 477:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr514;
	goto tr366;
tr514:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1128;
st1128:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1128;
case 1128:
#line 8003 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1128;
	goto tr1275;
tr1203:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1129;
st1129:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1129;
case 1129:
#line 8013 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st478;
		case 101: goto st478;
	}
	goto tr1225;
st478:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof478;
case 478:
	switch( (*( sm->p)) ) {
		case 84: goto st479;
		case 116: goto st479;
	}
	goto tr366;
st479:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof479;
case 479:
	if ( (*( sm->p)) == 32 )
		goto st480;
	goto tr366;
st480:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof480;
case 480:
	if ( (*( sm->p)) == 35 )
		goto st481;
	goto tr366;
st481:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof481;
case 481:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr518;
	goto tr366;
tr518:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1130;
st1130:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1130;
case 1130:
#line 8055 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1130;
	goto tr1278;
tr1204:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1131;
st1131:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1131;
case 1131:
#line 8065 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st482;
		case 72: goto st500;
		case 73: goto st506;
		case 79: goto st513;
		case 97: goto st482;
		case 104: goto st500;
		case 105: goto st506;
		case 111: goto st513;
	}
	goto tr1225;
st482:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof482;
case 482:
	switch( (*( sm->p)) ) {
		case 75: goto st483;
		case 107: goto st483;
	}
	goto tr366;
st483:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof483;
case 483:
	switch( (*( sm->p)) ) {
		case 69: goto st484;
		case 101: goto st484;
	}
	goto tr366;
st484:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof484;
case 484:
	switch( (*( sm->p)) ) {
		case 32: goto st485;
		case 68: goto st486;
		case 100: goto st486;
	}
	goto tr366;
st485:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof485;
case 485:
	switch( (*( sm->p)) ) {
		case 68: goto st486;
		case 100: goto st486;
	}
	goto tr366;
st486:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof486;
case 486:
	switch( (*( sm->p)) ) {
		case 79: goto st487;
		case 111: goto st487;
	}
	goto tr366;
st487:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof487;
case 487:
	switch( (*( sm->p)) ) {
		case 87: goto st488;
		case 119: goto st488;
	}
	goto tr366;
st488:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof488;
case 488:
	switch( (*( sm->p)) ) {
		case 78: goto st489;
		case 110: goto st489;
	}
	goto tr366;
st489:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof489;
case 489:
	if ( (*( sm->p)) == 32 )
		goto st490;
	goto tr366;
st490:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof490;
case 490:
	switch( (*( sm->p)) ) {
		case 35: goto st491;
		case 82: goto st492;
		case 114: goto st492;
	}
	goto tr366;
st491:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof491;
case 491:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr529;
	goto tr366;
tr529:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1132;
st1132:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1132;
case 1132:
#line 8171 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1132;
	goto tr1284;
st492:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof492;
case 492:
	switch( (*( sm->p)) ) {
		case 69: goto st493;
		case 101: goto st493;
	}
	goto tr366;
st493:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof493;
case 493:
	switch( (*( sm->p)) ) {
		case 81: goto st494;
		case 113: goto st494;
	}
	goto tr366;
st494:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof494;
case 494:
	switch( (*( sm->p)) ) {
		case 85: goto st495;
		case 117: goto st495;
	}
	goto tr366;
st495:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof495;
case 495:
	switch( (*( sm->p)) ) {
		case 69: goto st496;
		case 101: goto st496;
	}
	goto tr366;
st496:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof496;
case 496:
	switch( (*( sm->p)) ) {
		case 83: goto st497;
		case 115: goto st497;
	}
	goto tr366;
st497:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof497;
case 497:
	switch( (*( sm->p)) ) {
		case 84: goto st498;
		case 116: goto st498;
	}
	goto tr366;
st498:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof498;
case 498:
	if ( (*( sm->p)) == 32 )
		goto st499;
	goto tr366;
st499:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof499;
case 499:
	if ( (*( sm->p)) == 35 )
		goto st491;
	goto tr366;
st500:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof500;
case 500:
	switch( (*( sm->p)) ) {
		case 85: goto st501;
		case 117: goto st501;
	}
	goto tr366;
st501:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof501;
case 501:
	switch( (*( sm->p)) ) {
		case 77: goto st502;
		case 109: goto st502;
	}
	goto tr366;
st502:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof502;
case 502:
	switch( (*( sm->p)) ) {
		case 66: goto st503;
		case 98: goto st503;
	}
	goto tr366;
st503:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof503;
case 503:
	if ( (*( sm->p)) == 32 )
		goto st504;
	goto tr366;
st504:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof504;
case 504:
	if ( (*( sm->p)) == 35 )
		goto st505;
	goto tr366;
st505:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof505;
case 505:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr542;
	goto tr366;
tr542:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1133;
st1133:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1133;
case 1133:
#line 8297 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1133;
	goto tr1286;
st506:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof506;
case 506:
	switch( (*( sm->p)) ) {
		case 67: goto st507;
		case 99: goto st507;
	}
	goto tr366;
st507:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof507;
case 507:
	switch( (*( sm->p)) ) {
		case 75: goto st508;
		case 107: goto st508;
	}
	goto tr366;
st508:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof508;
case 508:
	switch( (*( sm->p)) ) {
		case 69: goto st509;
		case 101: goto st509;
	}
	goto tr366;
st509:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof509;
case 509:
	switch( (*( sm->p)) ) {
		case 84: goto st510;
		case 116: goto st510;
	}
	goto tr366;
st510:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof510;
case 510:
	if ( (*( sm->p)) == 32 )
		goto st511;
	goto tr366;
st511:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof511;
case 511:
	if ( (*( sm->p)) == 35 )
		goto st512;
	goto tr366;
st512:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof512;
case 512:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr549;
	goto tr366;
tr549:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1134;
st1134:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1134;
case 1134:
#line 8364 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1134;
	goto tr1288;
st513:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof513;
case 513:
	switch( (*( sm->p)) ) {
		case 80: goto st514;
		case 112: goto st514;
	}
	goto tr366;
st514:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof514;
case 514:
	switch( (*( sm->p)) ) {
		case 73: goto st515;
		case 105: goto st515;
	}
	goto tr366;
st515:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof515;
case 515:
	switch( (*( sm->p)) ) {
		case 67: goto st516;
		case 99: goto st516;
	}
	goto tr366;
st516:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof516;
case 516:
	if ( (*( sm->p)) == 32 )
		goto st517;
	goto tr366;
st517:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof517;
case 517:
	if ( (*( sm->p)) == 35 )
		goto st518;
	goto tr366;
st518:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof518;
case 518:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr555;
	goto tr366;
tr555:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1135;
st1135:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1135;
case 1135:
#line 8422 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1135;
	goto tr1290;
tr1205:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1136;
st1136:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1136;
case 1136:
#line 8432 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 83: goto st519;
		case 115: goto st519;
	}
	goto tr1225;
st519:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof519;
case 519:
	switch( (*( sm->p)) ) {
		case 69: goto st520;
		case 101: goto st520;
	}
	goto tr366;
st520:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof520;
case 520:
	switch( (*( sm->p)) ) {
		case 82: goto st521;
		case 114: goto st521;
	}
	goto tr366;
st521:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof521;
case 521:
	if ( (*( sm->p)) == 32 )
		goto st522;
	goto tr366;
st522:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof522;
case 522:
	if ( (*( sm->p)) == 35 )
		goto st523;
	goto tr366;
st523:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof523;
case 523:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr560;
	goto tr366;
tr560:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1137;
st1137:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1137;
case 1137:
#line 8483 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1137;
	goto tr1293;
tr1206:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1138;
st1138:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1138;
case 1138:
#line 8493 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 73: goto st524;
		case 105: goto st524;
	}
	goto tr1225;
st524:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof524;
case 524:
	switch( (*( sm->p)) ) {
		case 75: goto st525;
		case 107: goto st525;
	}
	goto tr366;
st525:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof525;
case 525:
	switch( (*( sm->p)) ) {
		case 73: goto st526;
		case 105: goto st526;
	}
	goto tr366;
st526:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof526;
case 526:
	if ( (*( sm->p)) == 32 )
		goto st527;
	goto tr366;
st527:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof527;
case 527:
	if ( (*( sm->p)) == 35 )
		goto st528;
	goto tr366;
st528:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof528;
case 528:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr565;
	goto tr366;
tr565:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1139;
st1139:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1139;
case 1139:
#line 8544 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st1139;
	goto tr1296;
tr1207:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 579 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 82;}
	goto st1140;
st1140:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1140;
case 1140:
#line 8555 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 35: goto st529;
		case 47: goto st531;
		case 66: goto st552;
		case 67: goto st553;
		case 73: goto st762;
		case 81: goto st763;
		case 83: goto st970;
		case 84: goto st1015;
		case 85: goto st1020;
		case 91: goto st1021;
		case 98: goto st552;
		case 99: goto st553;
		case 105: goto st762;
		case 113: goto st763;
		case 115: goto st970;
		case 116: goto st1015;
		case 117: goto st1020;
	}
	goto tr1225;
st529:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof529;
case 529:
	switch( (*( sm->p)) ) {
		case 45: goto tr566;
		case 95: goto tr566;
	}
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto tr566;
	} else if ( (*( sm->p)) > 90 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
			goto tr566;
	} else
		goto tr566;
	goto tr366;
tr566:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st530;
st530:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof530;
case 530:
#line 8599 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 45: goto st530;
		case 93: goto tr568;
		case 95: goto st530;
	}
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st530;
	} else if ( (*( sm->p)) > 90 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
			goto st530;
	} else
		goto st530;
	goto tr366;
st531:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof531;
case 531:
	switch( (*( sm->p)) ) {
		case 66: goto st532;
		case 67: goto st533;
		case 73: goto st540;
		case 81: goto st324;
		case 83: goto st541;
		case 84: goto st545;
		case 85: goto st551;
		case 98: goto st532;
		case 99: goto st533;
		case 105: goto st540;
		case 113: goto st324;
		case 115: goto st541;
		case 116: goto st545;
		case 117: goto st551;
	}
	goto tr366;
st532:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof532;
case 532:
	if ( (*( sm->p)) == 93 )
		goto tr575;
	goto tr366;
st533:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof533;
case 533:
	switch( (*( sm->p)) ) {
		case 79: goto st534;
		case 111: goto st534;
	}
	goto tr366;
st534:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof534;
case 534:
	switch( (*( sm->p)) ) {
		case 68: goto st535;
		case 76: goto st537;
		case 100: goto st535;
		case 108: goto st537;
	}
	goto tr366;
st535:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof535;
case 535:
	switch( (*( sm->p)) ) {
		case 69: goto st536;
		case 101: goto st536;
	}
	goto tr366;
st536:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof536;
case 536:
	if ( (*( sm->p)) == 93 )
		goto st1141;
	goto tr366;
st1141:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1141;
case 1141:
	if ( (*( sm->p)) == 32 )
		goto st1141;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st1141;
	goto tr1308;
st537:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof537;
case 537:
	switch( (*( sm->p)) ) {
		case 79: goto st538;
		case 111: goto st538;
	}
	goto tr366;
st538:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof538;
case 538:
	switch( (*( sm->p)) ) {
		case 82: goto st539;
		case 114: goto st539;
	}
	goto tr366;
st539:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof539;
case 539:
	if ( (*( sm->p)) == 93 )
		goto tr583;
	goto tr366;
st540:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof540;
case 540:
	if ( (*( sm->p)) == 93 )
		goto tr584;
	goto tr366;
st541:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof541;
case 541:
	switch( (*( sm->p)) ) {
		case 69: goto st330;
		case 80: goto st309;
		case 85: goto st542;
		case 93: goto tr586;
		case 101: goto st330;
		case 112: goto st309;
		case 117: goto st542;
	}
	goto tr366;
st542:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof542;
case 542:
	switch( (*( sm->p)) ) {
		case 66: goto st543;
		case 80: goto st544;
		case 98: goto st543;
		case 112: goto st544;
	}
	goto tr366;
st543:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof543;
case 543:
	if ( (*( sm->p)) == 93 )
		goto tr589;
	goto tr366;
st544:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof544;
case 544:
	if ( (*( sm->p)) == 93 )
		goto tr590;
	goto tr366;
st545:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof545;
case 545:
	switch( (*( sm->p)) ) {
		case 65: goto st546;
		case 68: goto st317;
		case 72: goto st550;
		case 97: goto st546;
		case 100: goto st317;
		case 104: goto st550;
	}
	goto tr366;
st546:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof546;
case 546:
	switch( (*( sm->p)) ) {
		case 66: goto st547;
		case 98: goto st547;
	}
	goto tr366;
st547:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof547;
case 547:
	switch( (*( sm->p)) ) {
		case 76: goto st548;
		case 108: goto st548;
	}
	goto tr366;
st548:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof548;
case 548:
	switch( (*( sm->p)) ) {
		case 69: goto st549;
		case 101: goto st549;
	}
	goto tr366;
st549:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof549;
case 549:
	if ( (*( sm->p)) == 93 )
		goto st1142;
	goto tr366;
st1142:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1142;
case 1142:
	if ( (*( sm->p)) == 32 )
		goto st1142;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st1142;
	goto tr1309;
st550:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof550;
case 550:
	if ( (*( sm->p)) == 93 )
		goto tr597;
	goto tr366;
st551:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof551;
case 551:
	if ( (*( sm->p)) == 93 )
		goto tr598;
	goto tr366;
st552:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof552;
case 552:
	if ( (*( sm->p)) == 93 )
		goto tr599;
	goto tr366;
st553:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof553;
case 553:
	switch( (*( sm->p)) ) {
		case 79: goto st554;
		case 111: goto st554;
	}
	goto tr366;
st554:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof554;
case 554:
	switch( (*( sm->p)) ) {
		case 68: goto st555;
		case 76: goto st557;
		case 100: goto st555;
		case 108: goto st557;
	}
	goto tr366;
st555:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof555;
case 555:
	switch( (*( sm->p)) ) {
		case 69: goto st556;
		case 101: goto st556;
	}
	goto tr366;
st556:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof556;
case 556:
	if ( (*( sm->p)) == 93 )
		goto tr604;
	goto tr366;
st557:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof557;
case 557:
	switch( (*( sm->p)) ) {
		case 79: goto st558;
		case 111: goto st558;
	}
	goto tr366;
st558:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof558;
case 558:
	switch( (*( sm->p)) ) {
		case 82: goto st559;
		case 114: goto st559;
	}
	goto tr366;
st559:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof559;
case 559:
	if ( (*( sm->p)) == 61 )
		goto st560;
	goto tr366;
st560:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof560;
case 560:
	switch( (*( sm->p)) ) {
		case 35: goto tr608;
		case 65: goto tr609;
		case 66: goto tr610;
		case 67: goto tr611;
		case 68: goto tr612;
		case 70: goto tr613;
		case 71: goto tr614;
		case 73: goto tr615;
		case 74: goto tr616;
		case 76: goto tr617;
		case 77: goto tr618;
		case 79: goto tr619;
		case 80: goto tr620;
		case 83: goto tr621;
		case 97: goto tr622;
		case 98: goto tr623;
		case 99: goto tr624;
		case 100: goto tr625;
		case 102: goto tr627;
		case 103: goto tr628;
		case 105: goto tr629;
		case 106: goto tr630;
		case 108: goto tr631;
		case 109: goto tr632;
		case 111: goto tr633;
		case 112: goto tr634;
		case 115: goto tr635;
	}
	if ( 101 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto tr626;
	goto tr366;
tr608:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st561;
st561:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof561;
case 561:
#line 8938 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st562;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st562;
	} else
		goto st562;
	goto tr366;
st562:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof562;
case 562:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st563;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st563;
	} else
		goto st563;
	goto tr366;
st563:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof563;
case 563:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st564;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st564;
	} else
		goto st564;
	goto tr366;
st564:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof564;
case 564:
	if ( (*( sm->p)) == 93 )
		goto tr640;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st565;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st565;
	} else
		goto st565;
	goto tr366;
st565:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof565;
case 565:
	if ( (*( sm->p)) == 93 )
		goto tr640;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st566;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st566;
	} else
		goto st566;
	goto tr366;
st566:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof566;
case 566:
	if ( (*( sm->p)) == 93 )
		goto tr640;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st567;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st567;
	} else
		goto st567;
	goto tr366;
st567:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof567;
case 567:
	if ( (*( sm->p)) == 93 )
		goto tr640;
	goto tr366;
tr609:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st568;
st568:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof568;
case 568:
#line 9032 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 68: goto st569;
		case 82: goto st573;
		case 100: goto st569;
		case 114: goto st573;
	}
	goto tr366;
st569:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof569;
case 569:
	switch( (*( sm->p)) ) {
		case 77: goto st570;
		case 109: goto st570;
	}
	goto tr366;
st570:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof570;
case 570:
	switch( (*( sm->p)) ) {
		case 73: goto st571;
		case 105: goto st571;
	}
	goto tr366;
st571:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof571;
case 571:
	switch( (*( sm->p)) ) {
		case 78: goto st572;
		case 110: goto st572;
	}
	goto tr366;
st572:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof572;
case 572:
	if ( (*( sm->p)) == 93 )
		goto tr648;
	goto tr366;
st573:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof573;
case 573:
	switch( (*( sm->p)) ) {
		case 84: goto st574;
		case 116: goto st574;
	}
	goto tr366;
st574:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof574;
case 574:
	switch( (*( sm->p)) ) {
		case 73: goto st575;
		case 93: goto tr648;
		case 105: goto st575;
	}
	goto tr366;
st575:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof575;
case 575:
	switch( (*( sm->p)) ) {
		case 83: goto st576;
		case 115: goto st576;
	}
	goto tr366;
st576:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof576;
case 576:
	switch( (*( sm->p)) ) {
		case 84: goto st572;
		case 116: goto st572;
	}
	goto tr366;
tr610:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st577;
st577:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof577;
case 577:
#line 9117 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st578;
		case 108: goto st578;
	}
	goto tr366;
st578:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof578;
case 578:
	switch( (*( sm->p)) ) {
		case 79: goto st579;
		case 111: goto st579;
	}
	goto tr366;
st579:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof579;
case 579:
	switch( (*( sm->p)) ) {
		case 67: goto st580;
		case 99: goto st580;
	}
	goto tr366;
st580:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof580;
case 580:
	switch( (*( sm->p)) ) {
		case 75: goto st581;
		case 107: goto st581;
	}
	goto tr366;
st581:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof581;
case 581:
	switch( (*( sm->p)) ) {
		case 69: goto st582;
		case 101: goto st582;
	}
	goto tr366;
st582:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof582;
case 582:
	switch( (*( sm->p)) ) {
		case 68: goto st572;
		case 100: goto st572;
	}
	goto tr366;
tr611:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st583;
st583:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof583;
case 583:
#line 9174 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st584;
		case 79: goto st591;
		case 104: goto st584;
		case 111: goto st591;
	}
	goto tr366;
st584:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof584;
case 584:
	switch( (*( sm->p)) ) {
		case 65: goto st585;
		case 93: goto tr648;
		case 97: goto st585;
	}
	goto tr366;
st585:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof585;
case 585:
	switch( (*( sm->p)) ) {
		case 82: goto st586;
		case 114: goto st586;
	}
	goto tr366;
st586:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof586;
case 586:
	switch( (*( sm->p)) ) {
		case 65: goto st587;
		case 93: goto tr648;
		case 97: goto st587;
	}
	goto tr366;
st587:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof587;
case 587:
	switch( (*( sm->p)) ) {
		case 67: goto st588;
		case 99: goto st588;
	}
	goto tr366;
st588:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof588;
case 588:
	switch( (*( sm->p)) ) {
		case 84: goto st589;
		case 116: goto st589;
	}
	goto tr366;
st589:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof589;
case 589:
	switch( (*( sm->p)) ) {
		case 69: goto st590;
		case 101: goto st590;
	}
	goto tr366;
st590:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof590;
case 590:
	switch( (*( sm->p)) ) {
		case 82: goto st572;
		case 114: goto st572;
	}
	goto tr366;
st591:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof591;
case 591:
	switch( (*( sm->p)) ) {
		case 78: goto st592;
		case 80: goto st599;
		case 110: goto st592;
		case 112: goto st599;
	}
	goto tr366;
st592:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof592;
case 592:
	switch( (*( sm->p)) ) {
		case 84: goto st593;
		case 116: goto st593;
	}
	goto tr366;
st593:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof593;
case 593:
	switch( (*( sm->p)) ) {
		case 82: goto st594;
		case 93: goto tr648;
		case 114: goto st594;
	}
	goto tr366;
st594:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof594;
case 594:
	switch( (*( sm->p)) ) {
		case 73: goto st595;
		case 105: goto st595;
	}
	goto tr366;
st595:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof595;
case 595:
	switch( (*( sm->p)) ) {
		case 66: goto st596;
		case 98: goto st596;
	}
	goto tr366;
st596:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof596;
case 596:
	switch( (*( sm->p)) ) {
		case 85: goto st597;
		case 117: goto st597;
	}
	goto tr366;
st597:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof597;
case 597:
	switch( (*( sm->p)) ) {
		case 84: goto st598;
		case 116: goto st598;
	}
	goto tr366;
st598:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof598;
case 598:
	switch( (*( sm->p)) ) {
		case 79: goto st590;
		case 111: goto st590;
	}
	goto tr366;
st599:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof599;
case 599:
	switch( (*( sm->p)) ) {
		case 89: goto st600;
		case 121: goto st600;
	}
	goto tr366;
st600:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof600;
case 600:
	switch( (*( sm->p)) ) {
		case 82: goto st601;
		case 93: goto tr648;
		case 114: goto st601;
	}
	goto tr366;
st601:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof601;
case 601:
	switch( (*( sm->p)) ) {
		case 73: goto st602;
		case 105: goto st602;
	}
	goto tr366;
st602:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof602;
case 602:
	switch( (*( sm->p)) ) {
		case 71: goto st603;
		case 103: goto st603;
	}
	goto tr366;
st603:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof603;
case 603:
	switch( (*( sm->p)) ) {
		case 72: goto st576;
		case 104: goto st576;
	}
	goto tr366;
tr612:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st604;
st604:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof604;
case 604:
#line 9374 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 73: goto st605;
		case 105: goto st605;
	}
	goto tr366;
st605:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof605;
case 605:
	switch( (*( sm->p)) ) {
		case 82: goto st606;
		case 114: goto st606;
	}
	goto tr366;
st606:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof606;
case 606:
	switch( (*( sm->p)) ) {
		case 69: goto st607;
		case 93: goto tr648;
		case 101: goto st607;
	}
	goto tr366;
st607:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof607;
case 607:
	switch( (*( sm->p)) ) {
		case 67: goto st608;
		case 99: goto st608;
	}
	goto tr366;
st608:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof608;
case 608:
	switch( (*( sm->p)) ) {
		case 84: goto st609;
		case 116: goto st609;
	}
	goto tr366;
st609:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof609;
case 609:
	switch( (*( sm->p)) ) {
		case 79: goto st590;
		case 93: goto tr648;
		case 111: goto st590;
	}
	goto tr366;
tr613:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st610;
st610:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof610;
case 610:
#line 9433 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st611;
		case 82: goto st621;
		case 111: goto st611;
		case 114: goto st621;
	}
	goto tr366;
st611:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof611;
case 611:
	switch( (*( sm->p)) ) {
		case 82: goto st612;
		case 114: goto st612;
	}
	goto tr366;
st612:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof612;
case 612:
	switch( (*( sm->p)) ) {
		case 77: goto st613;
		case 109: goto st613;
	}
	goto tr366;
st613:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof613;
case 613:
	switch( (*( sm->p)) ) {
		case 69: goto st614;
		case 101: goto st614;
	}
	goto tr366;
st614:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof614;
case 614:
	switch( (*( sm->p)) ) {
		case 82: goto st615;
		case 114: goto st615;
	}
	goto tr366;
st615:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof615;
case 615:
	switch( (*( sm->p)) ) {
		case 45: goto st616;
		case 93: goto tr648;
	}
	goto tr366;
st616:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof616;
case 616:
	switch( (*( sm->p)) ) {
		case 83: goto st617;
		case 115: goto st617;
	}
	goto tr366;
st617:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof617;
case 617:
	switch( (*( sm->p)) ) {
		case 84: goto st618;
		case 116: goto st618;
	}
	goto tr366;
st618:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof618;
case 618:
	switch( (*( sm->p)) ) {
		case 65: goto st619;
		case 97: goto st619;
	}
	goto tr366;
st619:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof619;
case 619:
	switch( (*( sm->p)) ) {
		case 70: goto st620;
		case 102: goto st620;
	}
	goto tr366;
st620:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof620;
case 620:
	switch( (*( sm->p)) ) {
		case 70: goto st572;
		case 102: goto st572;
	}
	goto tr366;
st621:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof621;
case 621:
	switch( (*( sm->p)) ) {
		case 65: goto st622;
		case 72: goto st625;
		case 93: goto tr648;
		case 97: goto st622;
		case 104: goto st625;
	}
	goto tr366;
st622:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof622;
case 622:
	switch( (*( sm->p)) ) {
		case 78: goto st623;
		case 110: goto st623;
	}
	goto tr366;
st623:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof623;
case 623:
	switch( (*( sm->p)) ) {
		case 67: goto st624;
		case 99: goto st624;
	}
	goto tr366;
st624:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof624;
case 624:
	switch( (*( sm->p)) ) {
		case 72: goto st625;
		case 93: goto tr648;
		case 104: goto st625;
	}
	goto tr366;
st625:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof625;
case 625:
	switch( (*( sm->p)) ) {
		case 73: goto st626;
		case 105: goto st626;
	}
	goto tr366;
st626:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof626;
case 626:
	switch( (*( sm->p)) ) {
		case 83: goto st627;
		case 115: goto st627;
	}
	goto tr366;
st627:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof627;
case 627:
	switch( (*( sm->p)) ) {
		case 69: goto st572;
		case 101: goto st572;
	}
	goto tr366;
tr614:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st628;
st628:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof628;
case 628:
#line 9604 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st629;
		case 101: goto st629;
	}
	goto tr366;
st629:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof629;
case 629:
	switch( (*( sm->p)) ) {
		case 78: goto st630;
		case 110: goto st630;
	}
	goto tr366;
st630:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof630;
case 630:
	switch( (*( sm->p)) ) {
		case 69: goto st631;
		case 93: goto tr648;
		case 101: goto st631;
	}
	goto tr366;
st631:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof631;
case 631:
	switch( (*( sm->p)) ) {
		case 82: goto st632;
		case 114: goto st632;
	}
	goto tr366;
st632:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof632;
case 632:
	switch( (*( sm->p)) ) {
		case 65: goto st633;
		case 97: goto st633;
	}
	goto tr366;
st633:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof633;
case 633:
	switch( (*( sm->p)) ) {
		case 76: goto st572;
		case 108: goto st572;
	}
	goto tr366;
tr615:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st634;
st634:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof634;
case 634:
#line 9662 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st635;
		case 110: goto st635;
	}
	goto tr366;
st635:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof635;
case 635:
	switch( (*( sm->p)) ) {
		case 86: goto st636;
		case 118: goto st636;
	}
	goto tr366;
st636:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof636;
case 636:
	switch( (*( sm->p)) ) {
		case 65: goto st637;
		case 93: goto tr648;
		case 97: goto st637;
	}
	goto tr366;
st637:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof637;
case 637:
	switch( (*( sm->p)) ) {
		case 76: goto st638;
		case 108: goto st638;
	}
	goto tr366;
st638:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof638;
case 638:
	switch( (*( sm->p)) ) {
		case 73: goto st582;
		case 105: goto st582;
	}
	goto tr366;
tr616:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st639;
st639:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof639;
case 639:
#line 9711 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st640;
		case 97: goto st640;
	}
	goto tr366;
st640:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof640;
case 640:
	switch( (*( sm->p)) ) {
		case 78: goto st641;
		case 110: goto st641;
	}
	goto tr366;
st641:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof641;
case 641:
	switch( (*( sm->p)) ) {
		case 73: goto st597;
		case 93: goto tr648;
		case 105: goto st597;
	}
	goto tr366;
tr617:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st642;
st642:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof642;
case 642:
#line 9742 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st643;
		case 111: goto st643;
	}
	goto tr366;
st643:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof643;
case 643:
	switch( (*( sm->p)) ) {
		case 82: goto st644;
		case 114: goto st644;
	}
	goto tr366;
st644:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof644;
case 644:
	switch( (*( sm->p)) ) {
		case 69: goto st572;
		case 93: goto tr648;
		case 101: goto st572;
	}
	goto tr366;
tr618:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st645;
st645:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof645;
case 645:
#line 9773 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st646;
		case 79: goto st649;
		case 101: goto st646;
		case 111: goto st649;
	}
	goto tr366;
st646:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof646;
case 646:
	switch( (*( sm->p)) ) {
		case 77: goto st647;
		case 84: goto st648;
		case 109: goto st647;
		case 116: goto st648;
	}
	goto tr366;
st647:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof647;
case 647:
	switch( (*( sm->p)) ) {
		case 66: goto st589;
		case 98: goto st589;
	}
	goto tr366;
st648:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof648;
case 648:
	switch( (*( sm->p)) ) {
		case 65: goto st572;
		case 97: goto st572;
	}
	goto tr366;
st649:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof649;
case 649:
	switch( (*( sm->p)) ) {
		case 68: goto st650;
		case 100: goto st650;
	}
	goto tr366;
st650:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof650;
case 650:
	switch( (*( sm->p)) ) {
		case 69: goto st651;
		case 93: goto tr648;
		case 101: goto st651;
	}
	goto tr366;
st651:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof651;
case 651:
	switch( (*( sm->p)) ) {
		case 82: goto st652;
		case 114: goto st652;
	}
	goto tr366;
st652:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof652;
case 652:
	switch( (*( sm->p)) ) {
		case 65: goto st597;
		case 97: goto st597;
	}
	goto tr366;
tr619:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st653;
st653:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof653;
case 653:
#line 9853 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 67: goto st572;
		case 99: goto st572;
	}
	goto tr366;
tr620:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st654;
st654:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof654;
case 654:
#line 9865 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st655;
		case 114: goto st655;
	}
	goto tr366;
st655:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof655;
case 655:
	switch( (*( sm->p)) ) {
		case 73: goto st656;
		case 105: goto st656;
	}
	goto tr366;
st656:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof656;
case 656:
	switch( (*( sm->p)) ) {
		case 86: goto st657;
		case 118: goto st657;
	}
	goto tr366;
st657:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof657;
case 657:
	switch( (*( sm->p)) ) {
		case 73: goto st658;
		case 93: goto tr648;
		case 105: goto st658;
	}
	goto tr366;
st658:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof658;
case 658:
	switch( (*( sm->p)) ) {
		case 76: goto st659;
		case 108: goto st659;
	}
	goto tr366;
st659:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof659;
case 659:
	switch( (*( sm->p)) ) {
		case 69: goto st660;
		case 101: goto st660;
	}
	goto tr366;
st660:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof660;
case 660:
	switch( (*( sm->p)) ) {
		case 71: goto st581;
		case 103: goto st581;
	}
	goto tr366;
tr621:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st661;
st661:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof661;
case 661:
#line 9932 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st662;
		case 112: goto st662;
	}
	goto tr366;
st662:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof662;
case 662:
	switch( (*( sm->p)) ) {
		case 69: goto st663;
		case 101: goto st663;
	}
	goto tr366;
st663:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof663;
case 663:
	switch( (*( sm->p)) ) {
		case 67: goto st664;
		case 99: goto st664;
	}
	goto tr366;
st664:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof664;
case 664:
	switch( (*( sm->p)) ) {
		case 73: goto st665;
		case 93: goto tr648;
		case 105: goto st665;
	}
	goto tr366;
st665:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof665;
case 665:
	switch( (*( sm->p)) ) {
		case 69: goto st666;
		case 101: goto st666;
	}
	goto tr366;
st666:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof666;
case 666:
	switch( (*( sm->p)) ) {
		case 83: goto st572;
		case 115: goto st572;
	}
	goto tr366;
tr622:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st667;
st667:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof667;
case 667:
#line 9990 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 68: goto st569;
		case 82: goto st573;
		case 93: goto tr640;
		case 100: goto st669;
		case 114: goto st673;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr626:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st668;
st668:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof668;
case 668:
#line 10007 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr640;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st669:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof669;
case 669:
	switch( (*( sm->p)) ) {
		case 77: goto st570;
		case 93: goto tr640;
		case 109: goto st670;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st670:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof670;
case 670:
	switch( (*( sm->p)) ) {
		case 73: goto st571;
		case 93: goto tr640;
		case 105: goto st671;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st671:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof671;
case 671:
	switch( (*( sm->p)) ) {
		case 78: goto st572;
		case 93: goto tr640;
		case 110: goto st672;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st672:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof672;
case 672:
	if ( (*( sm->p)) == 93 )
		goto tr648;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st673:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof673;
case 673:
	switch( (*( sm->p)) ) {
		case 84: goto st574;
		case 93: goto tr640;
		case 116: goto st674;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st674:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof674;
case 674:
	switch( (*( sm->p)) ) {
		case 73: goto st575;
		case 93: goto tr648;
		case 105: goto st675;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st675:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof675;
case 675:
	switch( (*( sm->p)) ) {
		case 83: goto st576;
		case 93: goto tr640;
		case 115: goto st676;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st676:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof676;
case 676:
	switch( (*( sm->p)) ) {
		case 84: goto st572;
		case 93: goto tr640;
		case 116: goto st672;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr623:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st677;
st677:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof677;
case 677:
#line 10112 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st578;
		case 93: goto tr640;
		case 108: goto st678;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st678:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof678;
case 678:
	switch( (*( sm->p)) ) {
		case 79: goto st579;
		case 93: goto tr640;
		case 111: goto st679;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st679:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof679;
case 679:
	switch( (*( sm->p)) ) {
		case 67: goto st580;
		case 93: goto tr640;
		case 99: goto st680;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st680:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof680;
case 680:
	switch( (*( sm->p)) ) {
		case 75: goto st581;
		case 93: goto tr640;
		case 107: goto st681;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st681:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof681;
case 681:
	switch( (*( sm->p)) ) {
		case 69: goto st582;
		case 93: goto tr640;
		case 101: goto st682;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st682:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof682;
case 682:
	switch( (*( sm->p)) ) {
		case 68: goto st572;
		case 93: goto tr640;
		case 100: goto st672;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr624:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st683;
st683:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof683;
case 683:
#line 10187 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st584;
		case 79: goto st591;
		case 93: goto tr640;
		case 104: goto st684;
		case 111: goto st691;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st684:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof684;
case 684:
	switch( (*( sm->p)) ) {
		case 65: goto st585;
		case 93: goto tr648;
		case 97: goto st685;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st685:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof685;
case 685:
	switch( (*( sm->p)) ) {
		case 82: goto st586;
		case 93: goto tr640;
		case 114: goto st686;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st686:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof686;
case 686:
	switch( (*( sm->p)) ) {
		case 65: goto st587;
		case 93: goto tr648;
		case 97: goto st687;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st687:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof687;
case 687:
	switch( (*( sm->p)) ) {
		case 67: goto st588;
		case 93: goto tr640;
		case 99: goto st688;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st688:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof688;
case 688:
	switch( (*( sm->p)) ) {
		case 84: goto st589;
		case 93: goto tr640;
		case 116: goto st689;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st689:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof689;
case 689:
	switch( (*( sm->p)) ) {
		case 69: goto st590;
		case 93: goto tr640;
		case 101: goto st690;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st690:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof690;
case 690:
	switch( (*( sm->p)) ) {
		case 82: goto st572;
		case 93: goto tr640;
		case 114: goto st672;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st691:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof691;
case 691:
	switch( (*( sm->p)) ) {
		case 78: goto st592;
		case 80: goto st599;
		case 93: goto tr640;
		case 110: goto st692;
		case 112: goto st699;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st692:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof692;
case 692:
	switch( (*( sm->p)) ) {
		case 84: goto st593;
		case 93: goto tr640;
		case 116: goto st693;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st693:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof693;
case 693:
	switch( (*( sm->p)) ) {
		case 82: goto st594;
		case 93: goto tr648;
		case 114: goto st694;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st694:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof694;
case 694:
	switch( (*( sm->p)) ) {
		case 73: goto st595;
		case 93: goto tr640;
		case 105: goto st695;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st695:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof695;
case 695:
	switch( (*( sm->p)) ) {
		case 66: goto st596;
		case 93: goto tr640;
		case 98: goto st696;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st696:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof696;
case 696:
	switch( (*( sm->p)) ) {
		case 85: goto st597;
		case 93: goto tr640;
		case 117: goto st697;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st697:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof697;
case 697:
	switch( (*( sm->p)) ) {
		case 84: goto st598;
		case 93: goto tr640;
		case 116: goto st698;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st698:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof698;
case 698:
	switch( (*( sm->p)) ) {
		case 79: goto st590;
		case 93: goto tr640;
		case 111: goto st690;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st699:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof699;
case 699:
	switch( (*( sm->p)) ) {
		case 89: goto st600;
		case 93: goto tr640;
		case 121: goto st700;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st700:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof700;
case 700:
	switch( (*( sm->p)) ) {
		case 82: goto st601;
		case 93: goto tr648;
		case 114: goto st701;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st701:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof701;
case 701:
	switch( (*( sm->p)) ) {
		case 73: goto st602;
		case 93: goto tr640;
		case 105: goto st702;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st702:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof702;
case 702:
	switch( (*( sm->p)) ) {
		case 71: goto st603;
		case 93: goto tr640;
		case 103: goto st703;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st703:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof703;
case 703:
	switch( (*( sm->p)) ) {
		case 72: goto st576;
		case 93: goto tr640;
		case 104: goto st676;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr625:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st704;
st704:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof704;
case 704:
#line 10446 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 73: goto st605;
		case 93: goto tr640;
		case 105: goto st705;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st705:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof705;
case 705:
	switch( (*( sm->p)) ) {
		case 82: goto st606;
		case 93: goto tr640;
		case 114: goto st706;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st706:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof706;
case 706:
	switch( (*( sm->p)) ) {
		case 69: goto st607;
		case 93: goto tr648;
		case 101: goto st707;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st707:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof707;
case 707:
	switch( (*( sm->p)) ) {
		case 67: goto st608;
		case 93: goto tr640;
		case 99: goto st708;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st708:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof708;
case 708:
	switch( (*( sm->p)) ) {
		case 84: goto st609;
		case 93: goto tr640;
		case 116: goto st709;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st709:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof709;
case 709:
	switch( (*( sm->p)) ) {
		case 79: goto st590;
		case 93: goto tr648;
		case 111: goto st690;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr627:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st710;
st710:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof710;
case 710:
#line 10521 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st611;
		case 82: goto st621;
		case 93: goto tr640;
		case 111: goto st711;
		case 114: goto st716;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st711:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof711;
case 711:
	switch( (*( sm->p)) ) {
		case 82: goto st612;
		case 93: goto tr640;
		case 114: goto st712;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st712:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof712;
case 712:
	switch( (*( sm->p)) ) {
		case 77: goto st613;
		case 93: goto tr640;
		case 109: goto st713;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st713:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof713;
case 713:
	switch( (*( sm->p)) ) {
		case 69: goto st614;
		case 93: goto tr640;
		case 101: goto st714;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st714:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof714;
case 714:
	switch( (*( sm->p)) ) {
		case 82: goto st615;
		case 93: goto tr640;
		case 114: goto st715;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st715:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof715;
case 715:
	switch( (*( sm->p)) ) {
		case 45: goto st616;
		case 93: goto tr648;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st716:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof716;
case 716:
	switch( (*( sm->p)) ) {
		case 65: goto st622;
		case 72: goto st625;
		case 93: goto tr648;
		case 97: goto st717;
		case 104: goto st720;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st717:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof717;
case 717:
	switch( (*( sm->p)) ) {
		case 78: goto st623;
		case 93: goto tr640;
		case 110: goto st718;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st718:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof718;
case 718:
	switch( (*( sm->p)) ) {
		case 67: goto st624;
		case 93: goto tr640;
		case 99: goto st719;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st719:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof719;
case 719:
	switch( (*( sm->p)) ) {
		case 72: goto st625;
		case 93: goto tr648;
		case 104: goto st720;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st720:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof720;
case 720:
	switch( (*( sm->p)) ) {
		case 73: goto st626;
		case 93: goto tr640;
		case 105: goto st721;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st721:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof721;
case 721:
	switch( (*( sm->p)) ) {
		case 83: goto st627;
		case 93: goto tr640;
		case 115: goto st722;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st722:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof722;
case 722:
	switch( (*( sm->p)) ) {
		case 69: goto st572;
		case 93: goto tr640;
		case 101: goto st672;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr628:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st723;
st723:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof723;
case 723:
#line 10683 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st629;
		case 93: goto tr640;
		case 101: goto st724;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st724:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof724;
case 724:
	switch( (*( sm->p)) ) {
		case 78: goto st630;
		case 93: goto tr640;
		case 110: goto st725;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st725:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof725;
case 725:
	switch( (*( sm->p)) ) {
		case 69: goto st631;
		case 93: goto tr648;
		case 101: goto st726;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st726:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof726;
case 726:
	switch( (*( sm->p)) ) {
		case 82: goto st632;
		case 93: goto tr640;
		case 114: goto st727;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st727:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof727;
case 727:
	switch( (*( sm->p)) ) {
		case 65: goto st633;
		case 93: goto tr640;
		case 97: goto st728;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st728:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof728;
case 728:
	switch( (*( sm->p)) ) {
		case 76: goto st572;
		case 93: goto tr640;
		case 108: goto st672;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr629:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st729;
st729:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof729;
case 729:
#line 10758 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st635;
		case 93: goto tr640;
		case 110: goto st730;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st730:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof730;
case 730:
	switch( (*( sm->p)) ) {
		case 86: goto st636;
		case 93: goto tr640;
		case 118: goto st731;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st731:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof731;
case 731:
	switch( (*( sm->p)) ) {
		case 65: goto st637;
		case 93: goto tr648;
		case 97: goto st732;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st732:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof732;
case 732:
	switch( (*( sm->p)) ) {
		case 76: goto st638;
		case 93: goto tr640;
		case 108: goto st733;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st733:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof733;
case 733:
	switch( (*( sm->p)) ) {
		case 73: goto st582;
		case 93: goto tr640;
		case 105: goto st682;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr630:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st734;
st734:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof734;
case 734:
#line 10821 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st640;
		case 93: goto tr640;
		case 97: goto st735;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st735:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof735;
case 735:
	switch( (*( sm->p)) ) {
		case 78: goto st641;
		case 93: goto tr640;
		case 110: goto st736;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st736:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof736;
case 736:
	switch( (*( sm->p)) ) {
		case 73: goto st597;
		case 93: goto tr648;
		case 105: goto st697;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr631:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st737;
st737:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof737;
case 737:
#line 10860 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st643;
		case 93: goto tr640;
		case 111: goto st738;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st738:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof738;
case 738:
	switch( (*( sm->p)) ) {
		case 82: goto st644;
		case 93: goto tr640;
		case 114: goto st739;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st739:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof739;
case 739:
	switch( (*( sm->p)) ) {
		case 69: goto st572;
		case 93: goto tr648;
		case 101: goto st672;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr632:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st740;
st740:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof740;
case 740:
#line 10899 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st646;
		case 79: goto st649;
		case 93: goto tr640;
		case 101: goto st741;
		case 111: goto st744;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st741:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof741;
case 741:
	switch( (*( sm->p)) ) {
		case 77: goto st647;
		case 84: goto st648;
		case 93: goto tr640;
		case 109: goto st742;
		case 116: goto st743;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st742:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof742;
case 742:
	switch( (*( sm->p)) ) {
		case 66: goto st589;
		case 93: goto tr640;
		case 98: goto st689;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st743:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof743;
case 743:
	switch( (*( sm->p)) ) {
		case 65: goto st572;
		case 93: goto tr640;
		case 97: goto st672;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st744:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof744;
case 744:
	switch( (*( sm->p)) ) {
		case 68: goto st650;
		case 93: goto tr640;
		case 100: goto st745;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st745:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof745;
case 745:
	switch( (*( sm->p)) ) {
		case 69: goto st651;
		case 93: goto tr648;
		case 101: goto st746;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st746:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof746;
case 746:
	switch( (*( sm->p)) ) {
		case 82: goto st652;
		case 93: goto tr640;
		case 114: goto st747;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st747:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof747;
case 747:
	switch( (*( sm->p)) ) {
		case 65: goto st597;
		case 93: goto tr640;
		case 97: goto st697;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr633:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st748;
st748:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof748;
case 748:
#line 11002 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 67: goto st572;
		case 93: goto tr640;
		case 99: goto st672;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr634:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st749;
st749:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof749;
case 749:
#line 11017 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st655;
		case 93: goto tr640;
		case 114: goto st750;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st750:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof750;
case 750:
	switch( (*( sm->p)) ) {
		case 73: goto st656;
		case 93: goto tr640;
		case 105: goto st751;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st751:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof751;
case 751:
	switch( (*( sm->p)) ) {
		case 86: goto st657;
		case 93: goto tr640;
		case 118: goto st752;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st752:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof752;
case 752:
	switch( (*( sm->p)) ) {
		case 73: goto st658;
		case 93: goto tr648;
		case 105: goto st753;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st753:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof753;
case 753:
	switch( (*( sm->p)) ) {
		case 76: goto st659;
		case 93: goto tr640;
		case 108: goto st754;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st754:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof754;
case 754:
	switch( (*( sm->p)) ) {
		case 69: goto st660;
		case 93: goto tr640;
		case 101: goto st755;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st755:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof755;
case 755:
	switch( (*( sm->p)) ) {
		case 71: goto st581;
		case 93: goto tr640;
		case 103: goto st681;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
tr635:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st756;
st756:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof756;
case 756:
#line 11104 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st662;
		case 93: goto tr640;
		case 112: goto st757;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st757:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof757;
case 757:
	switch( (*( sm->p)) ) {
		case 69: goto st663;
		case 93: goto tr640;
		case 101: goto st758;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st758:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof758;
case 758:
	switch( (*( sm->p)) ) {
		case 67: goto st664;
		case 93: goto tr640;
		case 99: goto st759;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st759:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof759;
case 759:
	switch( (*( sm->p)) ) {
		case 73: goto st665;
		case 93: goto tr648;
		case 105: goto st760;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st760:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof760;
case 760:
	switch( (*( sm->p)) ) {
		case 69: goto st666;
		case 93: goto tr640;
		case 101: goto st761;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st761:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof761;
case 761:
	switch( (*( sm->p)) ) {
		case 83: goto st572;
		case 93: goto tr640;
		case 115: goto st672;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st668;
	goto tr366;
st762:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof762;
case 762:
	if ( (*( sm->p)) == 93 )
		goto tr812;
	goto tr366;
st763:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof763;
case 763:
	switch( (*( sm->p)) ) {
		case 85: goto st764;
		case 117: goto st764;
	}
	goto tr366;
st764:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof764;
case 764:
	switch( (*( sm->p)) ) {
		case 79: goto st765;
		case 111: goto st765;
	}
	goto tr366;
st765:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof765;
case 765:
	switch( (*( sm->p)) ) {
		case 84: goto st766;
		case 116: goto st766;
	}
	goto tr366;
st766:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof766;
case 766:
	switch( (*( sm->p)) ) {
		case 69: goto st767;
		case 101: goto st767;
	}
	goto tr366;
st767:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof767;
case 767:
	switch( (*( sm->p)) ) {
		case 61: goto st768;
		case 93: goto tr818;
	}
	goto tr366;
st768:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof768;
case 768:
	switch( (*( sm->p)) ) {
		case 35: goto tr819;
		case 65: goto tr820;
		case 66: goto tr821;
		case 67: goto tr822;
		case 68: goto tr823;
		case 70: goto tr824;
		case 71: goto tr825;
		case 73: goto tr826;
		case 74: goto tr827;
		case 76: goto tr828;
		case 77: goto tr829;
		case 79: goto tr830;
		case 80: goto tr831;
		case 83: goto tr832;
		case 97: goto tr833;
		case 98: goto tr834;
		case 99: goto tr835;
		case 100: goto tr836;
		case 102: goto tr838;
		case 103: goto tr839;
		case 105: goto tr840;
		case 106: goto tr841;
		case 108: goto tr842;
		case 109: goto tr843;
		case 111: goto tr844;
		case 112: goto tr845;
		case 115: goto tr846;
	}
	if ( 101 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto tr837;
	goto tr366;
tr819:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st769;
st769:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof769;
case 769:
#line 11267 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st770;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st770;
	} else
		goto st770;
	goto tr366;
st770:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof770;
case 770:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st771;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st771;
	} else
		goto st771;
	goto tr366;
st771:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof771;
case 771:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st772;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st772;
	} else
		goto st772;
	goto tr366;
st772:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof772;
case 772:
	if ( (*( sm->p)) == 93 )
		goto tr851;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st773;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st773;
	} else
		goto st773;
	goto tr366;
st773:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof773;
case 773:
	if ( (*( sm->p)) == 93 )
		goto tr851;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st774;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st774;
	} else
		goto st774;
	goto tr366;
st774:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof774;
case 774:
	if ( (*( sm->p)) == 93 )
		goto tr851;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st775;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st775;
	} else
		goto st775;
	goto tr366;
st775:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof775;
case 775:
	if ( (*( sm->p)) == 93 )
		goto tr851;
	goto tr366;
tr820:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st776;
st776:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof776;
case 776:
#line 11361 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 68: goto st777;
		case 82: goto st781;
		case 100: goto st777;
		case 114: goto st781;
	}
	goto tr366;
st777:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof777;
case 777:
	switch( (*( sm->p)) ) {
		case 77: goto st778;
		case 109: goto st778;
	}
	goto tr366;
st778:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof778;
case 778:
	switch( (*( sm->p)) ) {
		case 73: goto st779;
		case 105: goto st779;
	}
	goto tr366;
st779:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof779;
case 779:
	switch( (*( sm->p)) ) {
		case 78: goto st780;
		case 110: goto st780;
	}
	goto tr366;
st780:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof780;
case 780:
	if ( (*( sm->p)) == 93 )
		goto tr859;
	goto tr366;
st781:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof781;
case 781:
	switch( (*( sm->p)) ) {
		case 84: goto st782;
		case 116: goto st782;
	}
	goto tr366;
st782:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof782;
case 782:
	switch( (*( sm->p)) ) {
		case 73: goto st783;
		case 93: goto tr859;
		case 105: goto st783;
	}
	goto tr366;
st783:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof783;
case 783:
	switch( (*( sm->p)) ) {
		case 83: goto st784;
		case 115: goto st784;
	}
	goto tr366;
st784:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof784;
case 784:
	switch( (*( sm->p)) ) {
		case 84: goto st780;
		case 116: goto st780;
	}
	goto tr366;
tr821:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st785;
st785:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof785;
case 785:
#line 11446 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st786;
		case 108: goto st786;
	}
	goto tr366;
st786:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof786;
case 786:
	switch( (*( sm->p)) ) {
		case 79: goto st787;
		case 111: goto st787;
	}
	goto tr366;
st787:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof787;
case 787:
	switch( (*( sm->p)) ) {
		case 67: goto st788;
		case 99: goto st788;
	}
	goto tr366;
st788:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof788;
case 788:
	switch( (*( sm->p)) ) {
		case 75: goto st789;
		case 107: goto st789;
	}
	goto tr366;
st789:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof789;
case 789:
	switch( (*( sm->p)) ) {
		case 69: goto st790;
		case 101: goto st790;
	}
	goto tr366;
st790:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof790;
case 790:
	switch( (*( sm->p)) ) {
		case 68: goto st780;
		case 100: goto st780;
	}
	goto tr366;
tr822:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st791;
st791:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof791;
case 791:
#line 11503 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st792;
		case 79: goto st799;
		case 104: goto st792;
		case 111: goto st799;
	}
	goto tr366;
st792:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof792;
case 792:
	switch( (*( sm->p)) ) {
		case 65: goto st793;
		case 93: goto tr859;
		case 97: goto st793;
	}
	goto tr366;
st793:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof793;
case 793:
	switch( (*( sm->p)) ) {
		case 82: goto st794;
		case 114: goto st794;
	}
	goto tr366;
st794:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof794;
case 794:
	switch( (*( sm->p)) ) {
		case 65: goto st795;
		case 93: goto tr859;
		case 97: goto st795;
	}
	goto tr366;
st795:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof795;
case 795:
	switch( (*( sm->p)) ) {
		case 67: goto st796;
		case 99: goto st796;
	}
	goto tr366;
st796:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof796;
case 796:
	switch( (*( sm->p)) ) {
		case 84: goto st797;
		case 116: goto st797;
	}
	goto tr366;
st797:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof797;
case 797:
	switch( (*( sm->p)) ) {
		case 69: goto st798;
		case 101: goto st798;
	}
	goto tr366;
st798:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof798;
case 798:
	switch( (*( sm->p)) ) {
		case 82: goto st780;
		case 114: goto st780;
	}
	goto tr366;
st799:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof799;
case 799:
	switch( (*( sm->p)) ) {
		case 78: goto st800;
		case 80: goto st807;
		case 110: goto st800;
		case 112: goto st807;
	}
	goto tr366;
st800:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof800;
case 800:
	switch( (*( sm->p)) ) {
		case 84: goto st801;
		case 116: goto st801;
	}
	goto tr366;
st801:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof801;
case 801:
	switch( (*( sm->p)) ) {
		case 82: goto st802;
		case 93: goto tr859;
		case 114: goto st802;
	}
	goto tr366;
st802:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof802;
case 802:
	switch( (*( sm->p)) ) {
		case 73: goto st803;
		case 105: goto st803;
	}
	goto tr366;
st803:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof803;
case 803:
	switch( (*( sm->p)) ) {
		case 66: goto st804;
		case 98: goto st804;
	}
	goto tr366;
st804:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof804;
case 804:
	switch( (*( sm->p)) ) {
		case 85: goto st805;
		case 117: goto st805;
	}
	goto tr366;
st805:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof805;
case 805:
	switch( (*( sm->p)) ) {
		case 84: goto st806;
		case 116: goto st806;
	}
	goto tr366;
st806:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof806;
case 806:
	switch( (*( sm->p)) ) {
		case 79: goto st798;
		case 111: goto st798;
	}
	goto tr366;
st807:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof807;
case 807:
	switch( (*( sm->p)) ) {
		case 89: goto st808;
		case 121: goto st808;
	}
	goto tr366;
st808:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof808;
case 808:
	switch( (*( sm->p)) ) {
		case 82: goto st809;
		case 93: goto tr859;
		case 114: goto st809;
	}
	goto tr366;
st809:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof809;
case 809:
	switch( (*( sm->p)) ) {
		case 73: goto st810;
		case 105: goto st810;
	}
	goto tr366;
st810:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof810;
case 810:
	switch( (*( sm->p)) ) {
		case 71: goto st811;
		case 103: goto st811;
	}
	goto tr366;
st811:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof811;
case 811:
	switch( (*( sm->p)) ) {
		case 72: goto st784;
		case 104: goto st784;
	}
	goto tr366;
tr823:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st812;
st812:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof812;
case 812:
#line 11703 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 73: goto st813;
		case 105: goto st813;
	}
	goto tr366;
st813:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof813;
case 813:
	switch( (*( sm->p)) ) {
		case 82: goto st814;
		case 114: goto st814;
	}
	goto tr366;
st814:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof814;
case 814:
	switch( (*( sm->p)) ) {
		case 69: goto st815;
		case 93: goto tr859;
		case 101: goto st815;
	}
	goto tr366;
st815:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof815;
case 815:
	switch( (*( sm->p)) ) {
		case 67: goto st816;
		case 99: goto st816;
	}
	goto tr366;
st816:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof816;
case 816:
	switch( (*( sm->p)) ) {
		case 84: goto st817;
		case 116: goto st817;
	}
	goto tr366;
st817:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof817;
case 817:
	switch( (*( sm->p)) ) {
		case 79: goto st798;
		case 93: goto tr859;
		case 111: goto st798;
	}
	goto tr366;
tr824:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st818;
st818:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof818;
case 818:
#line 11762 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st819;
		case 82: goto st829;
		case 111: goto st819;
		case 114: goto st829;
	}
	goto tr366;
st819:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof819;
case 819:
	switch( (*( sm->p)) ) {
		case 82: goto st820;
		case 114: goto st820;
	}
	goto tr366;
st820:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof820;
case 820:
	switch( (*( sm->p)) ) {
		case 77: goto st821;
		case 109: goto st821;
	}
	goto tr366;
st821:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof821;
case 821:
	switch( (*( sm->p)) ) {
		case 69: goto st822;
		case 101: goto st822;
	}
	goto tr366;
st822:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof822;
case 822:
	switch( (*( sm->p)) ) {
		case 82: goto st823;
		case 114: goto st823;
	}
	goto tr366;
st823:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof823;
case 823:
	switch( (*( sm->p)) ) {
		case 45: goto st824;
		case 93: goto tr859;
	}
	goto tr366;
st824:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof824;
case 824:
	switch( (*( sm->p)) ) {
		case 83: goto st825;
		case 115: goto st825;
	}
	goto tr366;
st825:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof825;
case 825:
	switch( (*( sm->p)) ) {
		case 84: goto st826;
		case 116: goto st826;
	}
	goto tr366;
st826:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof826;
case 826:
	switch( (*( sm->p)) ) {
		case 65: goto st827;
		case 97: goto st827;
	}
	goto tr366;
st827:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof827;
case 827:
	switch( (*( sm->p)) ) {
		case 70: goto st828;
		case 102: goto st828;
	}
	goto tr366;
st828:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof828;
case 828:
	switch( (*( sm->p)) ) {
		case 70: goto st780;
		case 102: goto st780;
	}
	goto tr366;
st829:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof829;
case 829:
	switch( (*( sm->p)) ) {
		case 65: goto st830;
		case 72: goto st833;
		case 93: goto tr859;
		case 97: goto st830;
		case 104: goto st833;
	}
	goto tr366;
st830:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof830;
case 830:
	switch( (*( sm->p)) ) {
		case 78: goto st831;
		case 110: goto st831;
	}
	goto tr366;
st831:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof831;
case 831:
	switch( (*( sm->p)) ) {
		case 67: goto st832;
		case 99: goto st832;
	}
	goto tr366;
st832:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof832;
case 832:
	switch( (*( sm->p)) ) {
		case 72: goto st833;
		case 93: goto tr859;
		case 104: goto st833;
	}
	goto tr366;
st833:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof833;
case 833:
	switch( (*( sm->p)) ) {
		case 73: goto st834;
		case 105: goto st834;
	}
	goto tr366;
st834:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof834;
case 834:
	switch( (*( sm->p)) ) {
		case 83: goto st835;
		case 115: goto st835;
	}
	goto tr366;
st835:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof835;
case 835:
	switch( (*( sm->p)) ) {
		case 69: goto st780;
		case 101: goto st780;
	}
	goto tr366;
tr825:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st836;
st836:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof836;
case 836:
#line 11933 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st837;
		case 101: goto st837;
	}
	goto tr366;
st837:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof837;
case 837:
	switch( (*( sm->p)) ) {
		case 78: goto st838;
		case 110: goto st838;
	}
	goto tr366;
st838:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof838;
case 838:
	switch( (*( sm->p)) ) {
		case 69: goto st839;
		case 93: goto tr859;
		case 101: goto st839;
	}
	goto tr366;
st839:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof839;
case 839:
	switch( (*( sm->p)) ) {
		case 82: goto st840;
		case 114: goto st840;
	}
	goto tr366;
st840:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof840;
case 840:
	switch( (*( sm->p)) ) {
		case 65: goto st841;
		case 97: goto st841;
	}
	goto tr366;
st841:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof841;
case 841:
	switch( (*( sm->p)) ) {
		case 76: goto st780;
		case 108: goto st780;
	}
	goto tr366;
tr826:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st842;
st842:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof842;
case 842:
#line 11991 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st843;
		case 110: goto st843;
	}
	goto tr366;
st843:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof843;
case 843:
	switch( (*( sm->p)) ) {
		case 86: goto st844;
		case 118: goto st844;
	}
	goto tr366;
st844:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof844;
case 844:
	switch( (*( sm->p)) ) {
		case 65: goto st845;
		case 93: goto tr859;
		case 97: goto st845;
	}
	goto tr366;
st845:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof845;
case 845:
	switch( (*( sm->p)) ) {
		case 76: goto st846;
		case 108: goto st846;
	}
	goto tr366;
st846:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof846;
case 846:
	switch( (*( sm->p)) ) {
		case 73: goto st790;
		case 105: goto st790;
	}
	goto tr366;
tr827:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st847;
st847:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof847;
case 847:
#line 12040 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st848;
		case 97: goto st848;
	}
	goto tr366;
st848:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof848;
case 848:
	switch( (*( sm->p)) ) {
		case 78: goto st849;
		case 110: goto st849;
	}
	goto tr366;
st849:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof849;
case 849:
	switch( (*( sm->p)) ) {
		case 73: goto st805;
		case 93: goto tr859;
		case 105: goto st805;
	}
	goto tr366;
tr828:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st850;
st850:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof850;
case 850:
#line 12071 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st851;
		case 111: goto st851;
	}
	goto tr366;
st851:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof851;
case 851:
	switch( (*( sm->p)) ) {
		case 82: goto st852;
		case 114: goto st852;
	}
	goto tr366;
st852:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof852;
case 852:
	switch( (*( sm->p)) ) {
		case 69: goto st780;
		case 93: goto tr859;
		case 101: goto st780;
	}
	goto tr366;
tr829:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st853;
st853:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof853;
case 853:
#line 12102 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st854;
		case 79: goto st857;
		case 101: goto st854;
		case 111: goto st857;
	}
	goto tr366;
st854:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof854;
case 854:
	switch( (*( sm->p)) ) {
		case 77: goto st855;
		case 84: goto st856;
		case 109: goto st855;
		case 116: goto st856;
	}
	goto tr366;
st855:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof855;
case 855:
	switch( (*( sm->p)) ) {
		case 66: goto st797;
		case 98: goto st797;
	}
	goto tr366;
st856:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof856;
case 856:
	switch( (*( sm->p)) ) {
		case 65: goto st780;
		case 97: goto st780;
	}
	goto tr366;
st857:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof857;
case 857:
	switch( (*( sm->p)) ) {
		case 68: goto st858;
		case 100: goto st858;
	}
	goto tr366;
st858:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof858;
case 858:
	switch( (*( sm->p)) ) {
		case 69: goto st859;
		case 93: goto tr859;
		case 101: goto st859;
	}
	goto tr366;
st859:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof859;
case 859:
	switch( (*( sm->p)) ) {
		case 82: goto st860;
		case 114: goto st860;
	}
	goto tr366;
st860:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof860;
case 860:
	switch( (*( sm->p)) ) {
		case 65: goto st805;
		case 97: goto st805;
	}
	goto tr366;
tr830:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st861;
st861:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof861;
case 861:
#line 12182 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 67: goto st780;
		case 99: goto st780;
	}
	goto tr366;
tr831:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st862;
st862:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof862;
case 862:
#line 12194 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st863;
		case 114: goto st863;
	}
	goto tr366;
st863:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof863;
case 863:
	switch( (*( sm->p)) ) {
		case 73: goto st864;
		case 105: goto st864;
	}
	goto tr366;
st864:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof864;
case 864:
	switch( (*( sm->p)) ) {
		case 86: goto st865;
		case 118: goto st865;
	}
	goto tr366;
st865:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof865;
case 865:
	switch( (*( sm->p)) ) {
		case 73: goto st866;
		case 93: goto tr859;
		case 105: goto st866;
	}
	goto tr366;
st866:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof866;
case 866:
	switch( (*( sm->p)) ) {
		case 76: goto st867;
		case 108: goto st867;
	}
	goto tr366;
st867:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof867;
case 867:
	switch( (*( sm->p)) ) {
		case 69: goto st868;
		case 101: goto st868;
	}
	goto tr366;
st868:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof868;
case 868:
	switch( (*( sm->p)) ) {
		case 71: goto st789;
		case 103: goto st789;
	}
	goto tr366;
tr832:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st869;
st869:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof869;
case 869:
#line 12261 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st870;
		case 112: goto st870;
	}
	goto tr366;
st870:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof870;
case 870:
	switch( (*( sm->p)) ) {
		case 69: goto st871;
		case 101: goto st871;
	}
	goto tr366;
st871:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof871;
case 871:
	switch( (*( sm->p)) ) {
		case 67: goto st872;
		case 99: goto st872;
	}
	goto tr366;
st872:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof872;
case 872:
	switch( (*( sm->p)) ) {
		case 73: goto st873;
		case 93: goto tr859;
		case 105: goto st873;
	}
	goto tr366;
st873:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof873;
case 873:
	switch( (*( sm->p)) ) {
		case 69: goto st874;
		case 101: goto st874;
	}
	goto tr366;
st874:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof874;
case 874:
	switch( (*( sm->p)) ) {
		case 83: goto st780;
		case 115: goto st780;
	}
	goto tr366;
tr833:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st875;
st875:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof875;
case 875:
#line 12319 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 68: goto st777;
		case 82: goto st781;
		case 93: goto tr851;
		case 100: goto st877;
		case 114: goto st881;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr837:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st876;
st876:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof876;
case 876:
#line 12336 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr851;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st877:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof877;
case 877:
	switch( (*( sm->p)) ) {
		case 77: goto st778;
		case 93: goto tr851;
		case 109: goto st878;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st878:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof878;
case 878:
	switch( (*( sm->p)) ) {
		case 73: goto st779;
		case 93: goto tr851;
		case 105: goto st879;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st879:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof879;
case 879:
	switch( (*( sm->p)) ) {
		case 78: goto st780;
		case 93: goto tr851;
		case 110: goto st880;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st880:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof880;
case 880:
	if ( (*( sm->p)) == 93 )
		goto tr859;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st881:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof881;
case 881:
	switch( (*( sm->p)) ) {
		case 84: goto st782;
		case 93: goto tr851;
		case 116: goto st882;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st882:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof882;
case 882:
	switch( (*( sm->p)) ) {
		case 73: goto st783;
		case 93: goto tr859;
		case 105: goto st883;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st883:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof883;
case 883:
	switch( (*( sm->p)) ) {
		case 83: goto st784;
		case 93: goto tr851;
		case 115: goto st884;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st884:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof884;
case 884:
	switch( (*( sm->p)) ) {
		case 84: goto st780;
		case 93: goto tr851;
		case 116: goto st880;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr834:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st885;
st885:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof885;
case 885:
#line 12441 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st786;
		case 93: goto tr851;
		case 108: goto st886;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st886:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof886;
case 886:
	switch( (*( sm->p)) ) {
		case 79: goto st787;
		case 93: goto tr851;
		case 111: goto st887;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st887:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof887;
case 887:
	switch( (*( sm->p)) ) {
		case 67: goto st788;
		case 93: goto tr851;
		case 99: goto st888;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st888:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof888;
case 888:
	switch( (*( sm->p)) ) {
		case 75: goto st789;
		case 93: goto tr851;
		case 107: goto st889;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st889:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof889;
case 889:
	switch( (*( sm->p)) ) {
		case 69: goto st790;
		case 93: goto tr851;
		case 101: goto st890;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st890:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof890;
case 890:
	switch( (*( sm->p)) ) {
		case 68: goto st780;
		case 93: goto tr851;
		case 100: goto st880;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr835:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st891;
st891:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof891;
case 891:
#line 12516 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st792;
		case 79: goto st799;
		case 93: goto tr851;
		case 104: goto st892;
		case 111: goto st899;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st892:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof892;
case 892:
	switch( (*( sm->p)) ) {
		case 65: goto st793;
		case 93: goto tr859;
		case 97: goto st893;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st893:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof893;
case 893:
	switch( (*( sm->p)) ) {
		case 82: goto st794;
		case 93: goto tr851;
		case 114: goto st894;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st894:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof894;
case 894:
	switch( (*( sm->p)) ) {
		case 65: goto st795;
		case 93: goto tr859;
		case 97: goto st895;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st895:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof895;
case 895:
	switch( (*( sm->p)) ) {
		case 67: goto st796;
		case 93: goto tr851;
		case 99: goto st896;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st896:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof896;
case 896:
	switch( (*( sm->p)) ) {
		case 84: goto st797;
		case 93: goto tr851;
		case 116: goto st897;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st897:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof897;
case 897:
	switch( (*( sm->p)) ) {
		case 69: goto st798;
		case 93: goto tr851;
		case 101: goto st898;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st898:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof898;
case 898:
	switch( (*( sm->p)) ) {
		case 82: goto st780;
		case 93: goto tr851;
		case 114: goto st880;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st899:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof899;
case 899:
	switch( (*( sm->p)) ) {
		case 78: goto st800;
		case 80: goto st807;
		case 93: goto tr851;
		case 110: goto st900;
		case 112: goto st907;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st900:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof900;
case 900:
	switch( (*( sm->p)) ) {
		case 84: goto st801;
		case 93: goto tr851;
		case 116: goto st901;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st901:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof901;
case 901:
	switch( (*( sm->p)) ) {
		case 82: goto st802;
		case 93: goto tr859;
		case 114: goto st902;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st902:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof902;
case 902:
	switch( (*( sm->p)) ) {
		case 73: goto st803;
		case 93: goto tr851;
		case 105: goto st903;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st903:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof903;
case 903:
	switch( (*( sm->p)) ) {
		case 66: goto st804;
		case 93: goto tr851;
		case 98: goto st904;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st904:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof904;
case 904:
	switch( (*( sm->p)) ) {
		case 85: goto st805;
		case 93: goto tr851;
		case 117: goto st905;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st905:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof905;
case 905:
	switch( (*( sm->p)) ) {
		case 84: goto st806;
		case 93: goto tr851;
		case 116: goto st906;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st906:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof906;
case 906:
	switch( (*( sm->p)) ) {
		case 79: goto st798;
		case 93: goto tr851;
		case 111: goto st898;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st907:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof907;
case 907:
	switch( (*( sm->p)) ) {
		case 89: goto st808;
		case 93: goto tr851;
		case 121: goto st908;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st908:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof908;
case 908:
	switch( (*( sm->p)) ) {
		case 82: goto st809;
		case 93: goto tr859;
		case 114: goto st909;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st909:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof909;
case 909:
	switch( (*( sm->p)) ) {
		case 73: goto st810;
		case 93: goto tr851;
		case 105: goto st910;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st910:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof910;
case 910:
	switch( (*( sm->p)) ) {
		case 71: goto st811;
		case 93: goto tr851;
		case 103: goto st911;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st911:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof911;
case 911:
	switch( (*( sm->p)) ) {
		case 72: goto st784;
		case 93: goto tr851;
		case 104: goto st884;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr836:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st912;
st912:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof912;
case 912:
#line 12775 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 73: goto st813;
		case 93: goto tr851;
		case 105: goto st913;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st913:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof913;
case 913:
	switch( (*( sm->p)) ) {
		case 82: goto st814;
		case 93: goto tr851;
		case 114: goto st914;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st914:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof914;
case 914:
	switch( (*( sm->p)) ) {
		case 69: goto st815;
		case 93: goto tr859;
		case 101: goto st915;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st915:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof915;
case 915:
	switch( (*( sm->p)) ) {
		case 67: goto st816;
		case 93: goto tr851;
		case 99: goto st916;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st916:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof916;
case 916:
	switch( (*( sm->p)) ) {
		case 84: goto st817;
		case 93: goto tr851;
		case 116: goto st917;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st917:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof917;
case 917:
	switch( (*( sm->p)) ) {
		case 79: goto st798;
		case 93: goto tr859;
		case 111: goto st898;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr838:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st918;
st918:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof918;
case 918:
#line 12850 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st819;
		case 82: goto st829;
		case 93: goto tr851;
		case 111: goto st919;
		case 114: goto st924;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st919:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof919;
case 919:
	switch( (*( sm->p)) ) {
		case 82: goto st820;
		case 93: goto tr851;
		case 114: goto st920;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st920:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof920;
case 920:
	switch( (*( sm->p)) ) {
		case 77: goto st821;
		case 93: goto tr851;
		case 109: goto st921;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st921:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof921;
case 921:
	switch( (*( sm->p)) ) {
		case 69: goto st822;
		case 93: goto tr851;
		case 101: goto st922;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st922:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof922;
case 922:
	switch( (*( sm->p)) ) {
		case 82: goto st823;
		case 93: goto tr851;
		case 114: goto st923;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st923:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof923;
case 923:
	switch( (*( sm->p)) ) {
		case 45: goto st824;
		case 93: goto tr859;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st924:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof924;
case 924:
	switch( (*( sm->p)) ) {
		case 65: goto st830;
		case 72: goto st833;
		case 93: goto tr859;
		case 97: goto st925;
		case 104: goto st928;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st925:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof925;
case 925:
	switch( (*( sm->p)) ) {
		case 78: goto st831;
		case 93: goto tr851;
		case 110: goto st926;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st926:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof926;
case 926:
	switch( (*( sm->p)) ) {
		case 67: goto st832;
		case 93: goto tr851;
		case 99: goto st927;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st927:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof927;
case 927:
	switch( (*( sm->p)) ) {
		case 72: goto st833;
		case 93: goto tr859;
		case 104: goto st928;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st928:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof928;
case 928:
	switch( (*( sm->p)) ) {
		case 73: goto st834;
		case 93: goto tr851;
		case 105: goto st929;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st929:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof929;
case 929:
	switch( (*( sm->p)) ) {
		case 83: goto st835;
		case 93: goto tr851;
		case 115: goto st930;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st930:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof930;
case 930:
	switch( (*( sm->p)) ) {
		case 69: goto st780;
		case 93: goto tr851;
		case 101: goto st880;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr839:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st931;
st931:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof931;
case 931:
#line 13012 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st837;
		case 93: goto tr851;
		case 101: goto st932;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st932:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof932;
case 932:
	switch( (*( sm->p)) ) {
		case 78: goto st838;
		case 93: goto tr851;
		case 110: goto st933;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st933:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof933;
case 933:
	switch( (*( sm->p)) ) {
		case 69: goto st839;
		case 93: goto tr859;
		case 101: goto st934;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st934:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof934;
case 934:
	switch( (*( sm->p)) ) {
		case 82: goto st840;
		case 93: goto tr851;
		case 114: goto st935;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st935:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof935;
case 935:
	switch( (*( sm->p)) ) {
		case 65: goto st841;
		case 93: goto tr851;
		case 97: goto st936;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st936:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof936;
case 936:
	switch( (*( sm->p)) ) {
		case 76: goto st780;
		case 93: goto tr851;
		case 108: goto st880;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr840:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st937;
st937:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof937;
case 937:
#line 13087 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st843;
		case 93: goto tr851;
		case 110: goto st938;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st938:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof938;
case 938:
	switch( (*( sm->p)) ) {
		case 86: goto st844;
		case 93: goto tr851;
		case 118: goto st939;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st939:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof939;
case 939:
	switch( (*( sm->p)) ) {
		case 65: goto st845;
		case 93: goto tr859;
		case 97: goto st940;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st940:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof940;
case 940:
	switch( (*( sm->p)) ) {
		case 76: goto st846;
		case 93: goto tr851;
		case 108: goto st941;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st941:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof941;
case 941:
	switch( (*( sm->p)) ) {
		case 73: goto st790;
		case 93: goto tr851;
		case 105: goto st890;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr841:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st942;
st942:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof942;
case 942:
#line 13150 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st848;
		case 93: goto tr851;
		case 97: goto st943;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st943:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof943;
case 943:
	switch( (*( sm->p)) ) {
		case 78: goto st849;
		case 93: goto tr851;
		case 110: goto st944;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st944:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof944;
case 944:
	switch( (*( sm->p)) ) {
		case 73: goto st805;
		case 93: goto tr859;
		case 105: goto st905;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr842:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st945;
st945:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof945;
case 945:
#line 13189 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st851;
		case 93: goto tr851;
		case 111: goto st946;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st946:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof946;
case 946:
	switch( (*( sm->p)) ) {
		case 82: goto st852;
		case 93: goto tr851;
		case 114: goto st947;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st947:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof947;
case 947:
	switch( (*( sm->p)) ) {
		case 69: goto st780;
		case 93: goto tr859;
		case 101: goto st880;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr843:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st948;
st948:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof948;
case 948:
#line 13228 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st854;
		case 79: goto st857;
		case 93: goto tr851;
		case 101: goto st949;
		case 111: goto st952;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st949:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof949;
case 949:
	switch( (*( sm->p)) ) {
		case 77: goto st855;
		case 84: goto st856;
		case 93: goto tr851;
		case 109: goto st950;
		case 116: goto st951;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st950:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof950;
case 950:
	switch( (*( sm->p)) ) {
		case 66: goto st797;
		case 93: goto tr851;
		case 98: goto st897;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st951:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof951;
case 951:
	switch( (*( sm->p)) ) {
		case 65: goto st780;
		case 93: goto tr851;
		case 97: goto st880;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st952:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof952;
case 952:
	switch( (*( sm->p)) ) {
		case 68: goto st858;
		case 93: goto tr851;
		case 100: goto st953;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st953:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof953;
case 953:
	switch( (*( sm->p)) ) {
		case 69: goto st859;
		case 93: goto tr859;
		case 101: goto st954;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st954:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof954;
case 954:
	switch( (*( sm->p)) ) {
		case 82: goto st860;
		case 93: goto tr851;
		case 114: goto st955;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st955:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof955;
case 955:
	switch( (*( sm->p)) ) {
		case 65: goto st805;
		case 93: goto tr851;
		case 97: goto st905;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr844:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st956;
st956:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof956;
case 956:
#line 13331 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 67: goto st780;
		case 93: goto tr851;
		case 99: goto st880;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr845:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st957;
st957:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof957;
case 957:
#line 13346 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st863;
		case 93: goto tr851;
		case 114: goto st958;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st958:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof958;
case 958:
	switch( (*( sm->p)) ) {
		case 73: goto st864;
		case 93: goto tr851;
		case 105: goto st959;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st959:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof959;
case 959:
	switch( (*( sm->p)) ) {
		case 86: goto st865;
		case 93: goto tr851;
		case 118: goto st960;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st960:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof960;
case 960:
	switch( (*( sm->p)) ) {
		case 73: goto st866;
		case 93: goto tr859;
		case 105: goto st961;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st961:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof961;
case 961:
	switch( (*( sm->p)) ) {
		case 76: goto st867;
		case 93: goto tr851;
		case 108: goto st962;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st962:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof962;
case 962:
	switch( (*( sm->p)) ) {
		case 69: goto st868;
		case 93: goto tr851;
		case 101: goto st963;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st963:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof963;
case 963:
	switch( (*( sm->p)) ) {
		case 71: goto st789;
		case 93: goto tr851;
		case 103: goto st889;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
tr846:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st964;
st964:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof964;
case 964:
#line 13433 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st870;
		case 93: goto tr851;
		case 112: goto st965;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st965:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof965;
case 965:
	switch( (*( sm->p)) ) {
		case 69: goto st871;
		case 93: goto tr851;
		case 101: goto st966;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st966:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof966;
case 966:
	switch( (*( sm->p)) ) {
		case 67: goto st872;
		case 93: goto tr851;
		case 99: goto st967;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st967:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof967;
case 967:
	switch( (*( sm->p)) ) {
		case 73: goto st873;
		case 93: goto tr859;
		case 105: goto st968;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st968:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof968;
case 968:
	switch( (*( sm->p)) ) {
		case 69: goto st874;
		case 93: goto tr851;
		case 101: goto st969;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st969:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof969;
case 969:
	switch( (*( sm->p)) ) {
		case 83: goto st780;
		case 93: goto tr851;
		case 115: goto st880;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st876;
	goto tr366;
st970:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof970;
case 970:
	switch( (*( sm->p)) ) {
		case 69: goto st971;
		case 80: goto st1005;
		case 85: goto st1012;
		case 93: goto tr1026;
		case 101: goto st971;
		case 112: goto st1005;
		case 117: goto st1012;
	}
	goto tr366;
st971:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof971;
case 971:
	switch( (*( sm->p)) ) {
		case 67: goto st972;
		case 99: goto st972;
	}
	goto tr366;
st972:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof972;
case 972:
	switch( (*( sm->p)) ) {
		case 84: goto st973;
		case 116: goto st973;
	}
	goto tr366;
st973:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof973;
case 973:
	switch( (*( sm->p)) ) {
		case 73: goto st974;
		case 105: goto st974;
	}
	goto tr366;
st974:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof974;
case 974:
	switch( (*( sm->p)) ) {
		case 79: goto st975;
		case 111: goto st975;
	}
	goto tr366;
st975:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof975;
case 975:
	switch( (*( sm->p)) ) {
		case 78: goto st976;
		case 110: goto st976;
	}
	goto tr366;
st976:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof976;
case 976:
	switch( (*( sm->p)) ) {
		case 44: goto st977;
		case 61: goto st986;
		case 93: goto tr1034;
	}
	goto tr366;
st977:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof977;
case 977:
	switch( (*( sm->p)) ) {
		case 69: goto st978;
		case 101: goto st978;
	}
	goto tr366;
st978:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof978;
case 978:
	switch( (*( sm->p)) ) {
		case 88: goto st979;
		case 120: goto st979;
	}
	goto tr366;
st979:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof979;
case 979:
	switch( (*( sm->p)) ) {
		case 80: goto st980;
		case 112: goto st980;
	}
	goto tr366;
st980:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof980;
case 980:
	switch( (*( sm->p)) ) {
		case 65: goto st981;
		case 97: goto st981;
	}
	goto tr366;
st981:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof981;
case 981:
	switch( (*( sm->p)) ) {
		case 78: goto st982;
		case 110: goto st982;
	}
	goto tr366;
st982:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof982;
case 982:
	switch( (*( sm->p)) ) {
		case 68: goto st983;
		case 100: goto st983;
	}
	goto tr366;
st983:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof983;
case 983:
	switch( (*( sm->p)) ) {
		case 69: goto st984;
		case 101: goto st984;
	}
	goto tr366;
st984:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof984;
case 984:
	switch( (*( sm->p)) ) {
		case 68: goto st985;
		case 100: goto st985;
	}
	goto tr366;
st985:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof985;
case 985:
	switch( (*( sm->p)) ) {
		case 61: goto st986;
		case 93: goto tr1034;
	}
	goto tr366;
st986:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof986;
case 986:
	switch( (*( sm->p)) ) {
		case 91: goto tr1044;
		case 93: goto tr366;
	}
	goto tr1043;
tr1043:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st987;
st987:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof987;
case 987:
#line 13667 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 91: goto st988;
		case 93: goto tr1047;
	}
	goto st987;
tr1044:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st988;
st988:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof988;
case 988:
#line 13679 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 83: goto st989;
		case 91: goto st988;
		case 93: goto tr1047;
		case 115: goto st989;
	}
	goto st987;
st989:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof989;
case 989:
	switch( (*( sm->p)) ) {
		case 69: goto st990;
		case 91: goto st988;
		case 93: goto tr1047;
		case 101: goto st990;
	}
	goto st987;
st990:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof990;
case 990:
	switch( (*( sm->p)) ) {
		case 67: goto st991;
		case 91: goto st988;
		case 93: goto tr1047;
		case 99: goto st991;
	}
	goto st987;
st991:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof991;
case 991:
	switch( (*( sm->p)) ) {
		case 84: goto st992;
		case 91: goto st988;
		case 93: goto tr1047;
		case 116: goto st992;
	}
	goto st987;
st992:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof992;
case 992:
	switch( (*( sm->p)) ) {
		case 73: goto st993;
		case 91: goto st988;
		case 93: goto tr1047;
		case 105: goto st993;
	}
	goto st987;
st993:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof993;
case 993:
	switch( (*( sm->p)) ) {
		case 79: goto st994;
		case 91: goto st988;
		case 93: goto tr1047;
		case 111: goto st994;
	}
	goto st987;
st994:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof994;
case 994:
	switch( (*( sm->p)) ) {
		case 78: goto st995;
		case 91: goto st988;
		case 93: goto tr1047;
		case 110: goto st995;
	}
	goto st987;
st995:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof995;
case 995:
	switch( (*( sm->p)) ) {
		case 44: goto st996;
		case 61: goto tr366;
		case 91: goto st988;
		case 93: goto tr1047;
	}
	goto st987;
st996:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof996;
case 996:
	switch( (*( sm->p)) ) {
		case 69: goto st997;
		case 91: goto st988;
		case 93: goto tr1047;
		case 101: goto st997;
	}
	goto st987;
st997:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof997;
case 997:
	switch( (*( sm->p)) ) {
		case 88: goto st998;
		case 91: goto st988;
		case 93: goto tr1047;
		case 120: goto st998;
	}
	goto st987;
st998:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof998;
case 998:
	switch( (*( sm->p)) ) {
		case 80: goto st999;
		case 91: goto st988;
		case 93: goto tr1047;
		case 112: goto st999;
	}
	goto st987;
st999:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof999;
case 999:
	switch( (*( sm->p)) ) {
		case 65: goto st1000;
		case 91: goto st988;
		case 93: goto tr1047;
		case 97: goto st1000;
	}
	goto st987;
st1000:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1000;
case 1000:
	switch( (*( sm->p)) ) {
		case 78: goto st1001;
		case 91: goto st988;
		case 93: goto tr1047;
		case 110: goto st1001;
	}
	goto st987;
st1001:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1001;
case 1001:
	switch( (*( sm->p)) ) {
		case 68: goto st1002;
		case 91: goto st988;
		case 93: goto tr1047;
		case 100: goto st1002;
	}
	goto st987;
st1002:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1002;
case 1002:
	switch( (*( sm->p)) ) {
		case 69: goto st1003;
		case 91: goto st988;
		case 93: goto tr1047;
		case 101: goto st1003;
	}
	goto st987;
st1003:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1003;
case 1003:
	switch( (*( sm->p)) ) {
		case 68: goto st1004;
		case 91: goto st988;
		case 93: goto tr1047;
		case 100: goto st1004;
	}
	goto st987;
st1004:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1004;
case 1004:
	switch( (*( sm->p)) ) {
		case 61: goto tr366;
		case 91: goto st988;
		case 93: goto tr1047;
	}
	goto st987;
st1005:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1005;
case 1005:
	switch( (*( sm->p)) ) {
		case 79: goto st1006;
		case 111: goto st1006;
	}
	goto tr366;
st1006:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1006;
case 1006:
	switch( (*( sm->p)) ) {
		case 73: goto st1007;
		case 105: goto st1007;
	}
	goto tr366;
st1007:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1007;
case 1007:
	switch( (*( sm->p)) ) {
		case 76: goto st1008;
		case 108: goto st1008;
	}
	goto tr366;
st1008:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1008;
case 1008:
	switch( (*( sm->p)) ) {
		case 69: goto st1009;
		case 101: goto st1009;
	}
	goto tr366;
st1009:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1009;
case 1009:
	switch( (*( sm->p)) ) {
		case 82: goto st1010;
		case 114: goto st1010;
	}
	goto tr366;
st1010:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1010;
case 1010:
	switch( (*( sm->p)) ) {
		case 83: goto st1011;
		case 93: goto tr1070;
		case 115: goto st1011;
	}
	goto tr366;
st1011:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1011;
case 1011:
	if ( (*( sm->p)) == 93 )
		goto tr1070;
	goto tr366;
st1012:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1012;
case 1012:
	switch( (*( sm->p)) ) {
		case 66: goto st1013;
		case 80: goto st1014;
		case 98: goto st1013;
		case 112: goto st1014;
	}
	goto tr366;
st1013:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1013;
case 1013:
	if ( (*( sm->p)) == 93 )
		goto tr1073;
	goto tr366;
st1014:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1014;
case 1014:
	if ( (*( sm->p)) == 93 )
		goto tr1074;
	goto tr366;
st1015:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1015;
case 1015:
	switch( (*( sm->p)) ) {
		case 65: goto st1016;
		case 97: goto st1016;
	}
	goto tr366;
st1016:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1016;
case 1016:
	switch( (*( sm->p)) ) {
		case 66: goto st1017;
		case 98: goto st1017;
	}
	goto tr366;
st1017:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1017;
case 1017:
	switch( (*( sm->p)) ) {
		case 76: goto st1018;
		case 108: goto st1018;
	}
	goto tr366;
st1018:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1018;
case 1018:
	switch( (*( sm->p)) ) {
		case 69: goto st1019;
		case 101: goto st1019;
	}
	goto tr366;
st1019:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1019;
case 1019:
	if ( (*( sm->p)) == 93 )
		goto tr1079;
	goto tr366;
st1020:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1020;
case 1020:
	if ( (*( sm->p)) == 93 )
		goto tr1080;
	goto tr366;
st1021:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1021;
case 1021:
	switch( (*( sm->p)) ) {
		case 91: goto tr1082;
		case 93: goto tr366;
		case 124: goto tr1083;
	}
	goto tr1081;
tr1081:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1022;
st1022:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1022;
case 1022:
#line 14015 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 91: goto st1023;
		case 93: goto tr1086;
		case 124: goto tr1087;
	}
	goto st1022;
tr1082:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1023;
st1023:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1023;
case 1023:
#line 14028 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 91: goto tr366;
		case 93: goto tr1086;
		case 124: goto tr1087;
	}
	goto st1022;
tr1086:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1024;
st1024:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1024;
case 1024:
#line 14041 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr1088;
	goto tr366;
tr1087:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1025;
st1025:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1025;
case 1025:
#line 14051 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 91: goto tr1090;
		case 93: goto tr366;
		case 124: goto tr366;
	}
	goto tr1089;
tr1089:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st1026;
st1026:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1026;
case 1026:
#line 14064 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 91: goto st1027;
		case 93: goto tr1093;
		case 124: goto tr366;
	}
	goto st1026;
tr1090:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st1027;
st1027:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1027;
case 1027:
#line 14077 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 91: goto tr366;
		case 93: goto tr1093;
		case 124: goto tr366;
	}
	goto st1026;
tr1093:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
	goto st1028;
st1028:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1028;
case 1028:
#line 14090 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr1094;
	goto tr366;
tr1083:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1029;
st1029:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1029;
case 1029:
#line 14100 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 91: goto st1030;
		case 93: goto tr1086;
		case 124: goto tr366;
	}
	goto st1029;
st1030:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1030;
case 1030:
	switch( (*( sm->p)) ) {
		case 91: goto tr366;
		case 93: goto tr1086;
		case 124: goto tr366;
	}
	goto st1029;
st1143:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1143;
case 1143:
	if ( (*( sm->p)) == 96 )
		goto tr1310;
	goto tr1225;
tr1210:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1144;
st1144:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1144;
case 1144:
#line 14130 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 123 )
		goto st1031;
	goto tr1225;
st1031:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1031;
case 1031:
	switch( (*( sm->p)) ) {
		case 123: goto tr1098;
		case 124: goto tr1099;
		case 125: goto tr366;
	}
	goto tr1097;
tr1097:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1032;
st1032:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1032;
case 1032:
#line 14150 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 123: goto st1033;
		case 124: goto tr1102;
		case 125: goto tr1103;
	}
	goto st1032;
tr1098:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1033;
st1033:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1033;
case 1033:
#line 14163 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 123: goto tr366;
		case 124: goto tr1102;
		case 125: goto tr1103;
	}
	goto st1032;
tr1102:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1034;
st1034:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1034;
case 1034:
#line 14176 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 123 )
		goto tr1105;
	if ( 124 <= (*( sm->p)) && (*( sm->p)) <= 125 )
		goto tr366;
	goto tr1104;
tr1104:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st1035;
st1035:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1035;
case 1035:
#line 14188 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 123: goto st1036;
		case 124: goto tr366;
		case 125: goto tr1108;
	}
	goto st1035;
tr1105:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st1036;
st1036:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1036;
case 1036:
#line 14201 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 125 )
		goto tr1108;
	if ( 123 <= (*( sm->p)) && (*( sm->p)) <= 124 )
		goto tr366;
	goto st1035;
tr1108:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
	goto st1037;
st1037:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1037;
case 1037:
#line 14213 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 125 )
		goto tr1109;
	goto tr366;
tr1103:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1038;
st1038:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1038;
case 1038:
#line 14223 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 125 )
		goto tr1110;
	goto tr366;
tr1099:
#line 92 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st1039;
st1039:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1039;
case 1039:
#line 14233 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 123: goto st1040;
		case 124: goto tr366;
		case 125: goto tr1103;
	}
	goto st1039;
st1040:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1040;
case 1040:
	if ( (*( sm->p)) == 125 )
		goto tr1103;
	if ( 123 <= (*( sm->p)) && (*( sm->p)) <= 124 )
		goto tr366;
	goto st1039;
tr1312:
#line 595 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_html_escaped((*( sm->p)));
  }}
	goto st1145;
tr1314:
#line 590 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_inline(INLINE_CODE, "</span>");
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1145;
tr1315:
#line 595 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_html_escaped((*( sm->p)));
  }}
	goto st1145;
tr1316:
#line 586 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("`");
  }}
	goto st1145;
st1145:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1145;
case 1145:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 14275 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 92: goto st1146;
		case 96: goto tr1314;
	}
	goto tr1312;
st1146:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1146;
case 1146:
	if ( (*( sm->p)) == 96 )
		goto tr1316;
	goto tr1315;
tr1113:
#line 610 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    append_html_escaped((*( sm->p)));
  }}
	goto st1147;
tr1118:
#line 601 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_check(BLOCK_CODE)) {
      dstack_rewind();
    } else {
      append("[/code]");
    }
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st1147;
tr1317:
#line 610 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_html_escaped((*( sm->p)));
  }}
	goto st1147;
tr1319:
#line 610 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_html_escaped((*( sm->p)));
  }}
	goto st1147;
st1147:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1147;
case 1147:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 14318 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 91 )
		goto tr1318;
	goto tr1317;
tr1318:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1148;
st1148:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1148;
case 1148:
#line 14328 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 47 )
		goto st1041;
	goto tr1319;
st1041:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1041;
case 1041:
	switch( (*( sm->p)) ) {
		case 67: goto st1042;
		case 99: goto st1042;
	}
	goto tr1113;
st1042:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1042;
case 1042:
	switch( (*( sm->p)) ) {
		case 79: goto st1043;
		case 111: goto st1043;
	}
	goto tr1113;
st1043:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1043;
case 1043:
	switch( (*( sm->p)) ) {
		case 68: goto st1044;
		case 100: goto st1044;
	}
	goto tr1113;
st1044:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1044;
case 1044:
	switch( (*( sm->p)) ) {
		case 69: goto st1045;
		case 101: goto st1045;
	}
	goto tr1113;
st1045:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1045;
case 1045:
	if ( (*( sm->p)) == 93 )
		goto tr1118;
	goto tr1113;
tr1119:
#line 656 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}}
	goto st1149;
tr1128:
#line 650 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_close_block(BLOCK_TABLE, "</table>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st1149;
tr1132:
#line 628 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_block(BLOCK_TBODY, "</tbody>");
  }}
	goto st1149;
tr1136:
#line 620 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_block(BLOCK_THEAD, "</thead>");
  }}
	goto st1149;
tr1137:
#line 641 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_block(BLOCK_TR, "</tr>");
  }}
	goto st1149;
tr1145:
#line 624 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TBODY, "<tbody>");
  }}
	goto st1149;
tr1146:
#line 645 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1149;goto st1090;}}
  }}
	goto st1149;
tr1148:
#line 632 "ext/dtext/dtext.cpp.rl"
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
{( (sm->stack.data()))[( sm->top)++] = 1149;goto st1090;}}
  }}
	goto st1149;
tr1151:
#line 616 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_THEAD, "<thead>");
  }}
	goto st1149;
tr1152:
#line 637 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TR, "<tr>");
  }}
	goto st1149;
tr1321:
#line 656 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;}
	goto st1149;
tr1323:
#line 656 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;}
	goto st1149;
st1149:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1149;
case 1149:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 14462 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 91 )
		goto tr1322;
	goto tr1321;
tr1322:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st1150;
st1150:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1150;
case 1150:
#line 14472 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 47: goto st1046;
		case 84: goto st1061;
		case 116: goto st1061;
	}
	goto tr1323;
st1046:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1046;
case 1046:
	switch( (*( sm->p)) ) {
		case 84: goto st1047;
		case 116: goto st1047;
	}
	goto tr1119;
st1047:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1047;
case 1047:
	switch( (*( sm->p)) ) {
		case 65: goto st1048;
		case 66: goto st1052;
		case 72: goto st1056;
		case 82: goto st1060;
		case 97: goto st1048;
		case 98: goto st1052;
		case 104: goto st1056;
		case 114: goto st1060;
	}
	goto tr1119;
st1048:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1048;
case 1048:
	switch( (*( sm->p)) ) {
		case 66: goto st1049;
		case 98: goto st1049;
	}
	goto tr1119;
st1049:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1049;
case 1049:
	switch( (*( sm->p)) ) {
		case 76: goto st1050;
		case 108: goto st1050;
	}
	goto tr1119;
st1050:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1050;
case 1050:
	switch( (*( sm->p)) ) {
		case 69: goto st1051;
		case 101: goto st1051;
	}
	goto tr1119;
st1051:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1051;
case 1051:
	if ( (*( sm->p)) == 93 )
		goto tr1128;
	goto tr1119;
st1052:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1052;
case 1052:
	switch( (*( sm->p)) ) {
		case 79: goto st1053;
		case 111: goto st1053;
	}
	goto tr1119;
st1053:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1053;
case 1053:
	switch( (*( sm->p)) ) {
		case 68: goto st1054;
		case 100: goto st1054;
	}
	goto tr1119;
st1054:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1054;
case 1054:
	switch( (*( sm->p)) ) {
		case 89: goto st1055;
		case 121: goto st1055;
	}
	goto tr1119;
st1055:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1055;
case 1055:
	if ( (*( sm->p)) == 93 )
		goto tr1132;
	goto tr1119;
st1056:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1056;
case 1056:
	switch( (*( sm->p)) ) {
		case 69: goto st1057;
		case 101: goto st1057;
	}
	goto tr1119;
st1057:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1057;
case 1057:
	switch( (*( sm->p)) ) {
		case 65: goto st1058;
		case 97: goto st1058;
	}
	goto tr1119;
st1058:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1058;
case 1058:
	switch( (*( sm->p)) ) {
		case 68: goto st1059;
		case 100: goto st1059;
	}
	goto tr1119;
st1059:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1059;
case 1059:
	if ( (*( sm->p)) == 93 )
		goto tr1136;
	goto tr1119;
st1060:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1060;
case 1060:
	if ( (*( sm->p)) == 93 )
		goto tr1137;
	goto tr1119;
st1061:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1061;
case 1061:
	switch( (*( sm->p)) ) {
		case 66: goto st1062;
		case 68: goto st1066;
		case 72: goto st1067;
		case 82: goto st1071;
		case 98: goto st1062;
		case 100: goto st1066;
		case 104: goto st1067;
		case 114: goto st1071;
	}
	goto tr1119;
st1062:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1062;
case 1062:
	switch( (*( sm->p)) ) {
		case 79: goto st1063;
		case 111: goto st1063;
	}
	goto tr1119;
st1063:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1063;
case 1063:
	switch( (*( sm->p)) ) {
		case 68: goto st1064;
		case 100: goto st1064;
	}
	goto tr1119;
st1064:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1064;
case 1064:
	switch( (*( sm->p)) ) {
		case 89: goto st1065;
		case 121: goto st1065;
	}
	goto tr1119;
st1065:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1065;
case 1065:
	if ( (*( sm->p)) == 93 )
		goto tr1145;
	goto tr1119;
st1066:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1066;
case 1066:
	if ( (*( sm->p)) == 93 )
		goto tr1146;
	goto tr1119;
st1067:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1067;
case 1067:
	switch( (*( sm->p)) ) {
		case 69: goto st1068;
		case 93: goto tr1148;
		case 101: goto st1068;
	}
	goto tr1119;
st1068:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1068;
case 1068:
	switch( (*( sm->p)) ) {
		case 65: goto st1069;
		case 97: goto st1069;
	}
	goto tr1119;
st1069:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1069;
case 1069:
	switch( (*( sm->p)) ) {
		case 68: goto st1070;
		case 100: goto st1070;
	}
	goto tr1119;
st1070:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1070;
case 1070:
	if ( (*( sm->p)) == 93 )
		goto tr1151;
	goto tr1119;
st1071:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1071;
case 1071:
	if ( (*( sm->p)) == 93 )
		goto tr1152;
	goto tr1119;
	}
	_test_eof1072:  sm->cs = 1072; goto _test_eof; 
	_test_eof1073:  sm->cs = 1073; goto _test_eof; 
	_test_eof0:  sm->cs = 0; goto _test_eof; 
	_test_eof1074:  sm->cs = 1074; goto _test_eof; 
	_test_eof1075:  sm->cs = 1075; goto _test_eof; 
	_test_eof1:  sm->cs = 1; goto _test_eof; 
	_test_eof1076:  sm->cs = 1076; goto _test_eof; 
	_test_eof1077:  sm->cs = 1077; goto _test_eof; 
	_test_eof2:  sm->cs = 2; goto _test_eof; 
	_test_eof1078:  sm->cs = 1078; goto _test_eof; 
	_test_eof3:  sm->cs = 3; goto _test_eof; 
	_test_eof1079:  sm->cs = 1079; goto _test_eof; 
	_test_eof1080:  sm->cs = 1080; goto _test_eof; 
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
	_test_eof1081:  sm->cs = 1081; goto _test_eof; 
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
	_test_eof151:  sm->cs = 151; goto _test_eof; 
	_test_eof152:  sm->cs = 152; goto _test_eof; 
	_test_eof153:  sm->cs = 153; goto _test_eof; 
	_test_eof154:  sm->cs = 154; goto _test_eof; 
	_test_eof155:  sm->cs = 155; goto _test_eof; 
	_test_eof156:  sm->cs = 156; goto _test_eof; 
	_test_eof157:  sm->cs = 157; goto _test_eof; 
	_test_eof158:  sm->cs = 158; goto _test_eof; 
	_test_eof159:  sm->cs = 159; goto _test_eof; 
	_test_eof160:  sm->cs = 160; goto _test_eof; 
	_test_eof161:  sm->cs = 161; goto _test_eof; 
	_test_eof162:  sm->cs = 162; goto _test_eof; 
	_test_eof163:  sm->cs = 163; goto _test_eof; 
	_test_eof164:  sm->cs = 164; goto _test_eof; 
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
	_test_eof195:  sm->cs = 195; goto _test_eof; 
	_test_eof196:  sm->cs = 196; goto _test_eof; 
	_test_eof197:  sm->cs = 197; goto _test_eof; 
	_test_eof198:  sm->cs = 198; goto _test_eof; 
	_test_eof199:  sm->cs = 199; goto _test_eof; 
	_test_eof200:  sm->cs = 200; goto _test_eof; 
	_test_eof201:  sm->cs = 201; goto _test_eof; 
	_test_eof202:  sm->cs = 202; goto _test_eof; 
	_test_eof203:  sm->cs = 203; goto _test_eof; 
	_test_eof204:  sm->cs = 204; goto _test_eof; 
	_test_eof205:  sm->cs = 205; goto _test_eof; 
	_test_eof206:  sm->cs = 206; goto _test_eof; 
	_test_eof207:  sm->cs = 207; goto _test_eof; 
	_test_eof208:  sm->cs = 208; goto _test_eof; 
	_test_eof209:  sm->cs = 209; goto _test_eof; 
	_test_eof210:  sm->cs = 210; goto _test_eof; 
	_test_eof211:  sm->cs = 211; goto _test_eof; 
	_test_eof212:  sm->cs = 212; goto _test_eof; 
	_test_eof213:  sm->cs = 213; goto _test_eof; 
	_test_eof214:  sm->cs = 214; goto _test_eof; 
	_test_eof215:  sm->cs = 215; goto _test_eof; 
	_test_eof216:  sm->cs = 216; goto _test_eof; 
	_test_eof217:  sm->cs = 217; goto _test_eof; 
	_test_eof218:  sm->cs = 218; goto _test_eof; 
	_test_eof219:  sm->cs = 219; goto _test_eof; 
	_test_eof220:  sm->cs = 220; goto _test_eof; 
	_test_eof221:  sm->cs = 221; goto _test_eof; 
	_test_eof222:  sm->cs = 222; goto _test_eof; 
	_test_eof223:  sm->cs = 223; goto _test_eof; 
	_test_eof1082:  sm->cs = 1082; goto _test_eof; 
	_test_eof224:  sm->cs = 224; goto _test_eof; 
	_test_eof225:  sm->cs = 225; goto _test_eof; 
	_test_eof226:  sm->cs = 226; goto _test_eof; 
	_test_eof227:  sm->cs = 227; goto _test_eof; 
	_test_eof228:  sm->cs = 228; goto _test_eof; 
	_test_eof229:  sm->cs = 229; goto _test_eof; 
	_test_eof230:  sm->cs = 230; goto _test_eof; 
	_test_eof231:  sm->cs = 231; goto _test_eof; 
	_test_eof232:  sm->cs = 232; goto _test_eof; 
	_test_eof233:  sm->cs = 233; goto _test_eof; 
	_test_eof234:  sm->cs = 234; goto _test_eof; 
	_test_eof235:  sm->cs = 235; goto _test_eof; 
	_test_eof236:  sm->cs = 236; goto _test_eof; 
	_test_eof237:  sm->cs = 237; goto _test_eof; 
	_test_eof238:  sm->cs = 238; goto _test_eof; 
	_test_eof239:  sm->cs = 239; goto _test_eof; 
	_test_eof240:  sm->cs = 240; goto _test_eof; 
	_test_eof241:  sm->cs = 241; goto _test_eof; 
	_test_eof242:  sm->cs = 242; goto _test_eof; 
	_test_eof243:  sm->cs = 243; goto _test_eof; 
	_test_eof244:  sm->cs = 244; goto _test_eof; 
	_test_eof245:  sm->cs = 245; goto _test_eof; 
	_test_eof246:  sm->cs = 246; goto _test_eof; 
	_test_eof247:  sm->cs = 247; goto _test_eof; 
	_test_eof248:  sm->cs = 248; goto _test_eof; 
	_test_eof249:  sm->cs = 249; goto _test_eof; 
	_test_eof250:  sm->cs = 250; goto _test_eof; 
	_test_eof251:  sm->cs = 251; goto _test_eof; 
	_test_eof252:  sm->cs = 252; goto _test_eof; 
	_test_eof253:  sm->cs = 253; goto _test_eof; 
	_test_eof254:  sm->cs = 254; goto _test_eof; 
	_test_eof255:  sm->cs = 255; goto _test_eof; 
	_test_eof256:  sm->cs = 256; goto _test_eof; 
	_test_eof257:  sm->cs = 257; goto _test_eof; 
	_test_eof258:  sm->cs = 258; goto _test_eof; 
	_test_eof1083:  sm->cs = 1083; goto _test_eof; 
	_test_eof1084:  sm->cs = 1084; goto _test_eof; 
	_test_eof259:  sm->cs = 259; goto _test_eof; 
	_test_eof260:  sm->cs = 260; goto _test_eof; 
	_test_eof261:  sm->cs = 261; goto _test_eof; 
	_test_eof262:  sm->cs = 262; goto _test_eof; 
	_test_eof263:  sm->cs = 263; goto _test_eof; 
	_test_eof264:  sm->cs = 264; goto _test_eof; 
	_test_eof265:  sm->cs = 265; goto _test_eof; 
	_test_eof266:  sm->cs = 266; goto _test_eof; 
	_test_eof267:  sm->cs = 267; goto _test_eof; 
	_test_eof268:  sm->cs = 268; goto _test_eof; 
	_test_eof269:  sm->cs = 269; goto _test_eof; 
	_test_eof270:  sm->cs = 270; goto _test_eof; 
	_test_eof271:  sm->cs = 271; goto _test_eof; 
	_test_eof272:  sm->cs = 272; goto _test_eof; 
	_test_eof273:  sm->cs = 273; goto _test_eof; 
	_test_eof274:  sm->cs = 274; goto _test_eof; 
	_test_eof275:  sm->cs = 275; goto _test_eof; 
	_test_eof276:  sm->cs = 276; goto _test_eof; 
	_test_eof277:  sm->cs = 277; goto _test_eof; 
	_test_eof1085:  sm->cs = 1085; goto _test_eof; 
	_test_eof1086:  sm->cs = 1086; goto _test_eof; 
	_test_eof278:  sm->cs = 278; goto _test_eof; 
	_test_eof279:  sm->cs = 279; goto _test_eof; 
	_test_eof280:  sm->cs = 280; goto _test_eof; 
	_test_eof281:  sm->cs = 281; goto _test_eof; 
	_test_eof282:  sm->cs = 282; goto _test_eof; 
	_test_eof283:  sm->cs = 283; goto _test_eof; 
	_test_eof284:  sm->cs = 284; goto _test_eof; 
	_test_eof1087:  sm->cs = 1087; goto _test_eof; 
	_test_eof285:  sm->cs = 285; goto _test_eof; 
	_test_eof286:  sm->cs = 286; goto _test_eof; 
	_test_eof287:  sm->cs = 287; goto _test_eof; 
	_test_eof288:  sm->cs = 288; goto _test_eof; 
	_test_eof289:  sm->cs = 289; goto _test_eof; 
	_test_eof1088:  sm->cs = 1088; goto _test_eof; 
	_test_eof1089:  sm->cs = 1089; goto _test_eof; 
	_test_eof290:  sm->cs = 290; goto _test_eof; 
	_test_eof291:  sm->cs = 291; goto _test_eof; 
	_test_eof292:  sm->cs = 292; goto _test_eof; 
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
	_test_eof1090:  sm->cs = 1090; goto _test_eof; 
	_test_eof1091:  sm->cs = 1091; goto _test_eof; 
	_test_eof1092:  sm->cs = 1092; goto _test_eof; 
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
	_test_eof316:  sm->cs = 316; goto _test_eof; 
	_test_eof317:  sm->cs = 317; goto _test_eof; 
	_test_eof318:  sm->cs = 318; goto _test_eof; 
	_test_eof319:  sm->cs = 319; goto _test_eof; 
	_test_eof1093:  sm->cs = 1093; goto _test_eof; 
	_test_eof1094:  sm->cs = 1094; goto _test_eof; 
	_test_eof320:  sm->cs = 320; goto _test_eof; 
	_test_eof321:  sm->cs = 321; goto _test_eof; 
	_test_eof1095:  sm->cs = 1095; goto _test_eof; 
	_test_eof322:  sm->cs = 322; goto _test_eof; 
	_test_eof323:  sm->cs = 323; goto _test_eof; 
	_test_eof324:  sm->cs = 324; goto _test_eof; 
	_test_eof325:  sm->cs = 325; goto _test_eof; 
	_test_eof326:  sm->cs = 326; goto _test_eof; 
	_test_eof327:  sm->cs = 327; goto _test_eof; 
	_test_eof328:  sm->cs = 328; goto _test_eof; 
	_test_eof1096:  sm->cs = 1096; goto _test_eof; 
	_test_eof329:  sm->cs = 329; goto _test_eof; 
	_test_eof330:  sm->cs = 330; goto _test_eof; 
	_test_eof331:  sm->cs = 331; goto _test_eof; 
	_test_eof332:  sm->cs = 332; goto _test_eof; 
	_test_eof333:  sm->cs = 333; goto _test_eof; 
	_test_eof334:  sm->cs = 334; goto _test_eof; 
	_test_eof335:  sm->cs = 335; goto _test_eof; 
	_test_eof1097:  sm->cs = 1097; goto _test_eof; 
	_test_eof1098:  sm->cs = 1098; goto _test_eof; 
	_test_eof1099:  sm->cs = 1099; goto _test_eof; 
	_test_eof336:  sm->cs = 336; goto _test_eof; 
	_test_eof337:  sm->cs = 337; goto _test_eof; 
	_test_eof338:  sm->cs = 338; goto _test_eof; 
	_test_eof339:  sm->cs = 339; goto _test_eof; 
	_test_eof1100:  sm->cs = 1100; goto _test_eof; 
	_test_eof340:  sm->cs = 340; goto _test_eof; 
	_test_eof341:  sm->cs = 341; goto _test_eof; 
	_test_eof342:  sm->cs = 342; goto _test_eof; 
	_test_eof343:  sm->cs = 343; goto _test_eof; 
	_test_eof344:  sm->cs = 344; goto _test_eof; 
	_test_eof345:  sm->cs = 345; goto _test_eof; 
	_test_eof346:  sm->cs = 346; goto _test_eof; 
	_test_eof347:  sm->cs = 347; goto _test_eof; 
	_test_eof348:  sm->cs = 348; goto _test_eof; 
	_test_eof349:  sm->cs = 349; goto _test_eof; 
	_test_eof350:  sm->cs = 350; goto _test_eof; 
	_test_eof351:  sm->cs = 351; goto _test_eof; 
	_test_eof352:  sm->cs = 352; goto _test_eof; 
	_test_eof353:  sm->cs = 353; goto _test_eof; 
	_test_eof354:  sm->cs = 354; goto _test_eof; 
	_test_eof355:  sm->cs = 355; goto _test_eof; 
	_test_eof356:  sm->cs = 356; goto _test_eof; 
	_test_eof1101:  sm->cs = 1101; goto _test_eof; 
	_test_eof357:  sm->cs = 357; goto _test_eof; 
	_test_eof358:  sm->cs = 358; goto _test_eof; 
	_test_eof359:  sm->cs = 359; goto _test_eof; 
	_test_eof360:  sm->cs = 360; goto _test_eof; 
	_test_eof361:  sm->cs = 361; goto _test_eof; 
	_test_eof362:  sm->cs = 362; goto _test_eof; 
	_test_eof363:  sm->cs = 363; goto _test_eof; 
	_test_eof364:  sm->cs = 364; goto _test_eof; 
	_test_eof365:  sm->cs = 365; goto _test_eof; 
	_test_eof1102:  sm->cs = 1102; goto _test_eof; 
	_test_eof366:  sm->cs = 366; goto _test_eof; 
	_test_eof367:  sm->cs = 367; goto _test_eof; 
	_test_eof368:  sm->cs = 368; goto _test_eof; 
	_test_eof369:  sm->cs = 369; goto _test_eof; 
	_test_eof370:  sm->cs = 370; goto _test_eof; 
	_test_eof371:  sm->cs = 371; goto _test_eof; 
	_test_eof1103:  sm->cs = 1103; goto _test_eof; 
	_test_eof372:  sm->cs = 372; goto _test_eof; 
	_test_eof373:  sm->cs = 373; goto _test_eof; 
	_test_eof374:  sm->cs = 374; goto _test_eof; 
	_test_eof375:  sm->cs = 375; goto _test_eof; 
	_test_eof376:  sm->cs = 376; goto _test_eof; 
	_test_eof377:  sm->cs = 377; goto _test_eof; 
	_test_eof378:  sm->cs = 378; goto _test_eof; 
	_test_eof1104:  sm->cs = 1104; goto _test_eof; 
	_test_eof379:  sm->cs = 379; goto _test_eof; 
	_test_eof380:  sm->cs = 380; goto _test_eof; 
	_test_eof381:  sm->cs = 381; goto _test_eof; 
	_test_eof382:  sm->cs = 382; goto _test_eof; 
	_test_eof383:  sm->cs = 383; goto _test_eof; 
	_test_eof384:  sm->cs = 384; goto _test_eof; 
	_test_eof385:  sm->cs = 385; goto _test_eof; 
	_test_eof1105:  sm->cs = 1105; goto _test_eof; 
	_test_eof1106:  sm->cs = 1106; goto _test_eof; 
	_test_eof386:  sm->cs = 386; goto _test_eof; 
	_test_eof387:  sm->cs = 387; goto _test_eof; 
	_test_eof388:  sm->cs = 388; goto _test_eof; 
	_test_eof389:  sm->cs = 389; goto _test_eof; 
	_test_eof1107:  sm->cs = 1107; goto _test_eof; 
	_test_eof390:  sm->cs = 390; goto _test_eof; 
	_test_eof391:  sm->cs = 391; goto _test_eof; 
	_test_eof392:  sm->cs = 392; goto _test_eof; 
	_test_eof393:  sm->cs = 393; goto _test_eof; 
	_test_eof394:  sm->cs = 394; goto _test_eof; 
	_test_eof1108:  sm->cs = 1108; goto _test_eof; 
	_test_eof395:  sm->cs = 395; goto _test_eof; 
	_test_eof396:  sm->cs = 396; goto _test_eof; 
	_test_eof397:  sm->cs = 397; goto _test_eof; 
	_test_eof398:  sm->cs = 398; goto _test_eof; 
	_test_eof1109:  sm->cs = 1109; goto _test_eof; 
	_test_eof1110:  sm->cs = 1110; goto _test_eof; 
	_test_eof399:  sm->cs = 399; goto _test_eof; 
	_test_eof400:  sm->cs = 400; goto _test_eof; 
	_test_eof401:  sm->cs = 401; goto _test_eof; 
	_test_eof402:  sm->cs = 402; goto _test_eof; 
	_test_eof403:  sm->cs = 403; goto _test_eof; 
	_test_eof404:  sm->cs = 404; goto _test_eof; 
	_test_eof405:  sm->cs = 405; goto _test_eof; 
	_test_eof406:  sm->cs = 406; goto _test_eof; 
	_test_eof1111:  sm->cs = 1111; goto _test_eof; 
	_test_eof1112:  sm->cs = 1112; goto _test_eof; 
	_test_eof407:  sm->cs = 407; goto _test_eof; 
	_test_eof408:  sm->cs = 408; goto _test_eof; 
	_test_eof409:  sm->cs = 409; goto _test_eof; 
	_test_eof410:  sm->cs = 410; goto _test_eof; 
	_test_eof411:  sm->cs = 411; goto _test_eof; 
	_test_eof1113:  sm->cs = 1113; goto _test_eof; 
	_test_eof412:  sm->cs = 412; goto _test_eof; 
	_test_eof413:  sm->cs = 413; goto _test_eof; 
	_test_eof414:  sm->cs = 414; goto _test_eof; 
	_test_eof415:  sm->cs = 415; goto _test_eof; 
	_test_eof416:  sm->cs = 416; goto _test_eof; 
	_test_eof417:  sm->cs = 417; goto _test_eof; 
	_test_eof1114:  sm->cs = 1114; goto _test_eof; 
	_test_eof1115:  sm->cs = 1115; goto _test_eof; 
	_test_eof418:  sm->cs = 418; goto _test_eof; 
	_test_eof419:  sm->cs = 419; goto _test_eof; 
	_test_eof420:  sm->cs = 420; goto _test_eof; 
	_test_eof421:  sm->cs = 421; goto _test_eof; 
	_test_eof422:  sm->cs = 422; goto _test_eof; 
	_test_eof423:  sm->cs = 423; goto _test_eof; 
	_test_eof1116:  sm->cs = 1116; goto _test_eof; 
	_test_eof424:  sm->cs = 424; goto _test_eof; 
	_test_eof1117:  sm->cs = 1117; goto _test_eof; 
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
	_test_eof1118:  sm->cs = 1118; goto _test_eof; 
	_test_eof1119:  sm->cs = 1119; goto _test_eof; 
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
	_test_eof1120:  sm->cs = 1120; goto _test_eof; 
	_test_eof1121:  sm->cs = 1121; goto _test_eof; 
	_test_eof448:  sm->cs = 448; goto _test_eof; 
	_test_eof449:  sm->cs = 449; goto _test_eof; 
	_test_eof450:  sm->cs = 450; goto _test_eof; 
	_test_eof451:  sm->cs = 451; goto _test_eof; 
	_test_eof452:  sm->cs = 452; goto _test_eof; 
	_test_eof1122:  sm->cs = 1122; goto _test_eof; 
	_test_eof1123:  sm->cs = 1123; goto _test_eof; 
	_test_eof453:  sm->cs = 453; goto _test_eof; 
	_test_eof454:  sm->cs = 454; goto _test_eof; 
	_test_eof455:  sm->cs = 455; goto _test_eof; 
	_test_eof456:  sm->cs = 456; goto _test_eof; 
	_test_eof457:  sm->cs = 457; goto _test_eof; 
	_test_eof1124:  sm->cs = 1124; goto _test_eof; 
	_test_eof458:  sm->cs = 458; goto _test_eof; 
	_test_eof459:  sm->cs = 459; goto _test_eof; 
	_test_eof460:  sm->cs = 460; goto _test_eof; 
	_test_eof461:  sm->cs = 461; goto _test_eof; 
	_test_eof1125:  sm->cs = 1125; goto _test_eof; 
	_test_eof462:  sm->cs = 462; goto _test_eof; 
	_test_eof463:  sm->cs = 463; goto _test_eof; 
	_test_eof464:  sm->cs = 464; goto _test_eof; 
	_test_eof465:  sm->cs = 465; goto _test_eof; 
	_test_eof466:  sm->cs = 466; goto _test_eof; 
	_test_eof467:  sm->cs = 467; goto _test_eof; 
	_test_eof468:  sm->cs = 468; goto _test_eof; 
	_test_eof469:  sm->cs = 469; goto _test_eof; 
	_test_eof470:  sm->cs = 470; goto _test_eof; 
	_test_eof1126:  sm->cs = 1126; goto _test_eof; 
	_test_eof1127:  sm->cs = 1127; goto _test_eof; 
	_test_eof471:  sm->cs = 471; goto _test_eof; 
	_test_eof472:  sm->cs = 472; goto _test_eof; 
	_test_eof473:  sm->cs = 473; goto _test_eof; 
	_test_eof474:  sm->cs = 474; goto _test_eof; 
	_test_eof475:  sm->cs = 475; goto _test_eof; 
	_test_eof476:  sm->cs = 476; goto _test_eof; 
	_test_eof477:  sm->cs = 477; goto _test_eof; 
	_test_eof1128:  sm->cs = 1128; goto _test_eof; 
	_test_eof1129:  sm->cs = 1129; goto _test_eof; 
	_test_eof478:  sm->cs = 478; goto _test_eof; 
	_test_eof479:  sm->cs = 479; goto _test_eof; 
	_test_eof480:  sm->cs = 480; goto _test_eof; 
	_test_eof481:  sm->cs = 481; goto _test_eof; 
	_test_eof1130:  sm->cs = 1130; goto _test_eof; 
	_test_eof1131:  sm->cs = 1131; goto _test_eof; 
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
	_test_eof1132:  sm->cs = 1132; goto _test_eof; 
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
	_test_eof1133:  sm->cs = 1133; goto _test_eof; 
	_test_eof506:  sm->cs = 506; goto _test_eof; 
	_test_eof507:  sm->cs = 507; goto _test_eof; 
	_test_eof508:  sm->cs = 508; goto _test_eof; 
	_test_eof509:  sm->cs = 509; goto _test_eof; 
	_test_eof510:  sm->cs = 510; goto _test_eof; 
	_test_eof511:  sm->cs = 511; goto _test_eof; 
	_test_eof512:  sm->cs = 512; goto _test_eof; 
	_test_eof1134:  sm->cs = 1134; goto _test_eof; 
	_test_eof513:  sm->cs = 513; goto _test_eof; 
	_test_eof514:  sm->cs = 514; goto _test_eof; 
	_test_eof515:  sm->cs = 515; goto _test_eof; 
	_test_eof516:  sm->cs = 516; goto _test_eof; 
	_test_eof517:  sm->cs = 517; goto _test_eof; 
	_test_eof518:  sm->cs = 518; goto _test_eof; 
	_test_eof1135:  sm->cs = 1135; goto _test_eof; 
	_test_eof1136:  sm->cs = 1136; goto _test_eof; 
	_test_eof519:  sm->cs = 519; goto _test_eof; 
	_test_eof520:  sm->cs = 520; goto _test_eof; 
	_test_eof521:  sm->cs = 521; goto _test_eof; 
	_test_eof522:  sm->cs = 522; goto _test_eof; 
	_test_eof523:  sm->cs = 523; goto _test_eof; 
	_test_eof1137:  sm->cs = 1137; goto _test_eof; 
	_test_eof1138:  sm->cs = 1138; goto _test_eof; 
	_test_eof524:  sm->cs = 524; goto _test_eof; 
	_test_eof525:  sm->cs = 525; goto _test_eof; 
	_test_eof526:  sm->cs = 526; goto _test_eof; 
	_test_eof527:  sm->cs = 527; goto _test_eof; 
	_test_eof528:  sm->cs = 528; goto _test_eof; 
	_test_eof1139:  sm->cs = 1139; goto _test_eof; 
	_test_eof1140:  sm->cs = 1140; goto _test_eof; 
	_test_eof529:  sm->cs = 529; goto _test_eof; 
	_test_eof530:  sm->cs = 530; goto _test_eof; 
	_test_eof531:  sm->cs = 531; goto _test_eof; 
	_test_eof532:  sm->cs = 532; goto _test_eof; 
	_test_eof533:  sm->cs = 533; goto _test_eof; 
	_test_eof534:  sm->cs = 534; goto _test_eof; 
	_test_eof535:  sm->cs = 535; goto _test_eof; 
	_test_eof536:  sm->cs = 536; goto _test_eof; 
	_test_eof1141:  sm->cs = 1141; goto _test_eof; 
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
	_test_eof1142:  sm->cs = 1142; goto _test_eof; 
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
	_test_eof697:  sm->cs = 697; goto _test_eof; 
	_test_eof698:  sm->cs = 698; goto _test_eof; 
	_test_eof699:  sm->cs = 699; goto _test_eof; 
	_test_eof700:  sm->cs = 700; goto _test_eof; 
	_test_eof701:  sm->cs = 701; goto _test_eof; 
	_test_eof702:  sm->cs = 702; goto _test_eof; 
	_test_eof703:  sm->cs = 703; goto _test_eof; 
	_test_eof704:  sm->cs = 704; goto _test_eof; 
	_test_eof705:  sm->cs = 705; goto _test_eof; 
	_test_eof706:  sm->cs = 706; goto _test_eof; 
	_test_eof707:  sm->cs = 707; goto _test_eof; 
	_test_eof708:  sm->cs = 708; goto _test_eof; 
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
	_test_eof735:  sm->cs = 735; goto _test_eof; 
	_test_eof736:  sm->cs = 736; goto _test_eof; 
	_test_eof737:  sm->cs = 737; goto _test_eof; 
	_test_eof738:  sm->cs = 738; goto _test_eof; 
	_test_eof739:  sm->cs = 739; goto _test_eof; 
	_test_eof740:  sm->cs = 740; goto _test_eof; 
	_test_eof741:  sm->cs = 741; goto _test_eof; 
	_test_eof742:  sm->cs = 742; goto _test_eof; 
	_test_eof743:  sm->cs = 743; goto _test_eof; 
	_test_eof744:  sm->cs = 744; goto _test_eof; 
	_test_eof745:  sm->cs = 745; goto _test_eof; 
	_test_eof746:  sm->cs = 746; goto _test_eof; 
	_test_eof747:  sm->cs = 747; goto _test_eof; 
	_test_eof748:  sm->cs = 748; goto _test_eof; 
	_test_eof749:  sm->cs = 749; goto _test_eof; 
	_test_eof750:  sm->cs = 750; goto _test_eof; 
	_test_eof751:  sm->cs = 751; goto _test_eof; 
	_test_eof752:  sm->cs = 752; goto _test_eof; 
	_test_eof753:  sm->cs = 753; goto _test_eof; 
	_test_eof754:  sm->cs = 754; goto _test_eof; 
	_test_eof755:  sm->cs = 755; goto _test_eof; 
	_test_eof756:  sm->cs = 756; goto _test_eof; 
	_test_eof757:  sm->cs = 757; goto _test_eof; 
	_test_eof758:  sm->cs = 758; goto _test_eof; 
	_test_eof759:  sm->cs = 759; goto _test_eof; 
	_test_eof760:  sm->cs = 760; goto _test_eof; 
	_test_eof761:  sm->cs = 761; goto _test_eof; 
	_test_eof762:  sm->cs = 762; goto _test_eof; 
	_test_eof763:  sm->cs = 763; goto _test_eof; 
	_test_eof764:  sm->cs = 764; goto _test_eof; 
	_test_eof765:  sm->cs = 765; goto _test_eof; 
	_test_eof766:  sm->cs = 766; goto _test_eof; 
	_test_eof767:  sm->cs = 767; goto _test_eof; 
	_test_eof768:  sm->cs = 768; goto _test_eof; 
	_test_eof769:  sm->cs = 769; goto _test_eof; 
	_test_eof770:  sm->cs = 770; goto _test_eof; 
	_test_eof771:  sm->cs = 771; goto _test_eof; 
	_test_eof772:  sm->cs = 772; goto _test_eof; 
	_test_eof773:  sm->cs = 773; goto _test_eof; 
	_test_eof774:  sm->cs = 774; goto _test_eof; 
	_test_eof775:  sm->cs = 775; goto _test_eof; 
	_test_eof776:  sm->cs = 776; goto _test_eof; 
	_test_eof777:  sm->cs = 777; goto _test_eof; 
	_test_eof778:  sm->cs = 778; goto _test_eof; 
	_test_eof779:  sm->cs = 779; goto _test_eof; 
	_test_eof780:  sm->cs = 780; goto _test_eof; 
	_test_eof781:  sm->cs = 781; goto _test_eof; 
	_test_eof782:  sm->cs = 782; goto _test_eof; 
	_test_eof783:  sm->cs = 783; goto _test_eof; 
	_test_eof784:  sm->cs = 784; goto _test_eof; 
	_test_eof785:  sm->cs = 785; goto _test_eof; 
	_test_eof786:  sm->cs = 786; goto _test_eof; 
	_test_eof787:  sm->cs = 787; goto _test_eof; 
	_test_eof788:  sm->cs = 788; goto _test_eof; 
	_test_eof789:  sm->cs = 789; goto _test_eof; 
	_test_eof790:  sm->cs = 790; goto _test_eof; 
	_test_eof791:  sm->cs = 791; goto _test_eof; 
	_test_eof792:  sm->cs = 792; goto _test_eof; 
	_test_eof793:  sm->cs = 793; goto _test_eof; 
	_test_eof794:  sm->cs = 794; goto _test_eof; 
	_test_eof795:  sm->cs = 795; goto _test_eof; 
	_test_eof796:  sm->cs = 796; goto _test_eof; 
	_test_eof797:  sm->cs = 797; goto _test_eof; 
	_test_eof798:  sm->cs = 798; goto _test_eof; 
	_test_eof799:  sm->cs = 799; goto _test_eof; 
	_test_eof800:  sm->cs = 800; goto _test_eof; 
	_test_eof801:  sm->cs = 801; goto _test_eof; 
	_test_eof802:  sm->cs = 802; goto _test_eof; 
	_test_eof803:  sm->cs = 803; goto _test_eof; 
	_test_eof804:  sm->cs = 804; goto _test_eof; 
	_test_eof805:  sm->cs = 805; goto _test_eof; 
	_test_eof806:  sm->cs = 806; goto _test_eof; 
	_test_eof807:  sm->cs = 807; goto _test_eof; 
	_test_eof808:  sm->cs = 808; goto _test_eof; 
	_test_eof809:  sm->cs = 809; goto _test_eof; 
	_test_eof810:  sm->cs = 810; goto _test_eof; 
	_test_eof811:  sm->cs = 811; goto _test_eof; 
	_test_eof812:  sm->cs = 812; goto _test_eof; 
	_test_eof813:  sm->cs = 813; goto _test_eof; 
	_test_eof814:  sm->cs = 814; goto _test_eof; 
	_test_eof815:  sm->cs = 815; goto _test_eof; 
	_test_eof816:  sm->cs = 816; goto _test_eof; 
	_test_eof817:  sm->cs = 817; goto _test_eof; 
	_test_eof818:  sm->cs = 818; goto _test_eof; 
	_test_eof819:  sm->cs = 819; goto _test_eof; 
	_test_eof820:  sm->cs = 820; goto _test_eof; 
	_test_eof821:  sm->cs = 821; goto _test_eof; 
	_test_eof822:  sm->cs = 822; goto _test_eof; 
	_test_eof823:  sm->cs = 823; goto _test_eof; 
	_test_eof824:  sm->cs = 824; goto _test_eof; 
	_test_eof825:  sm->cs = 825; goto _test_eof; 
	_test_eof826:  sm->cs = 826; goto _test_eof; 
	_test_eof827:  sm->cs = 827; goto _test_eof; 
	_test_eof828:  sm->cs = 828; goto _test_eof; 
	_test_eof829:  sm->cs = 829; goto _test_eof; 
	_test_eof830:  sm->cs = 830; goto _test_eof; 
	_test_eof831:  sm->cs = 831; goto _test_eof; 
	_test_eof832:  sm->cs = 832; goto _test_eof; 
	_test_eof833:  sm->cs = 833; goto _test_eof; 
	_test_eof834:  sm->cs = 834; goto _test_eof; 
	_test_eof835:  sm->cs = 835; goto _test_eof; 
	_test_eof836:  sm->cs = 836; goto _test_eof; 
	_test_eof837:  sm->cs = 837; goto _test_eof; 
	_test_eof838:  sm->cs = 838; goto _test_eof; 
	_test_eof839:  sm->cs = 839; goto _test_eof; 
	_test_eof840:  sm->cs = 840; goto _test_eof; 
	_test_eof841:  sm->cs = 841; goto _test_eof; 
	_test_eof842:  sm->cs = 842; goto _test_eof; 
	_test_eof843:  sm->cs = 843; goto _test_eof; 
	_test_eof844:  sm->cs = 844; goto _test_eof; 
	_test_eof845:  sm->cs = 845; goto _test_eof; 
	_test_eof846:  sm->cs = 846; goto _test_eof; 
	_test_eof847:  sm->cs = 847; goto _test_eof; 
	_test_eof848:  sm->cs = 848; goto _test_eof; 
	_test_eof849:  sm->cs = 849; goto _test_eof; 
	_test_eof850:  sm->cs = 850; goto _test_eof; 
	_test_eof851:  sm->cs = 851; goto _test_eof; 
	_test_eof852:  sm->cs = 852; goto _test_eof; 
	_test_eof853:  sm->cs = 853; goto _test_eof; 
	_test_eof854:  sm->cs = 854; goto _test_eof; 
	_test_eof855:  sm->cs = 855; goto _test_eof; 
	_test_eof856:  sm->cs = 856; goto _test_eof; 
	_test_eof857:  sm->cs = 857; goto _test_eof; 
	_test_eof858:  sm->cs = 858; goto _test_eof; 
	_test_eof859:  sm->cs = 859; goto _test_eof; 
	_test_eof860:  sm->cs = 860; goto _test_eof; 
	_test_eof861:  sm->cs = 861; goto _test_eof; 
	_test_eof862:  sm->cs = 862; goto _test_eof; 
	_test_eof863:  sm->cs = 863; goto _test_eof; 
	_test_eof864:  sm->cs = 864; goto _test_eof; 
	_test_eof865:  sm->cs = 865; goto _test_eof; 
	_test_eof866:  sm->cs = 866; goto _test_eof; 
	_test_eof867:  sm->cs = 867; goto _test_eof; 
	_test_eof868:  sm->cs = 868; goto _test_eof; 
	_test_eof869:  sm->cs = 869; goto _test_eof; 
	_test_eof870:  sm->cs = 870; goto _test_eof; 
	_test_eof871:  sm->cs = 871; goto _test_eof; 
	_test_eof872:  sm->cs = 872; goto _test_eof; 
	_test_eof873:  sm->cs = 873; goto _test_eof; 
	_test_eof874:  sm->cs = 874; goto _test_eof; 
	_test_eof875:  sm->cs = 875; goto _test_eof; 
	_test_eof876:  sm->cs = 876; goto _test_eof; 
	_test_eof877:  sm->cs = 877; goto _test_eof; 
	_test_eof878:  sm->cs = 878; goto _test_eof; 
	_test_eof879:  sm->cs = 879; goto _test_eof; 
	_test_eof880:  sm->cs = 880; goto _test_eof; 
	_test_eof881:  sm->cs = 881; goto _test_eof; 
	_test_eof882:  sm->cs = 882; goto _test_eof; 
	_test_eof883:  sm->cs = 883; goto _test_eof; 
	_test_eof884:  sm->cs = 884; goto _test_eof; 
	_test_eof885:  sm->cs = 885; goto _test_eof; 
	_test_eof886:  sm->cs = 886; goto _test_eof; 
	_test_eof887:  sm->cs = 887; goto _test_eof; 
	_test_eof888:  sm->cs = 888; goto _test_eof; 
	_test_eof889:  sm->cs = 889; goto _test_eof; 
	_test_eof890:  sm->cs = 890; goto _test_eof; 
	_test_eof891:  sm->cs = 891; goto _test_eof; 
	_test_eof892:  sm->cs = 892; goto _test_eof; 
	_test_eof893:  sm->cs = 893; goto _test_eof; 
	_test_eof894:  sm->cs = 894; goto _test_eof; 
	_test_eof895:  sm->cs = 895; goto _test_eof; 
	_test_eof896:  sm->cs = 896; goto _test_eof; 
	_test_eof897:  sm->cs = 897; goto _test_eof; 
	_test_eof898:  sm->cs = 898; goto _test_eof; 
	_test_eof899:  sm->cs = 899; goto _test_eof; 
	_test_eof900:  sm->cs = 900; goto _test_eof; 
	_test_eof901:  sm->cs = 901; goto _test_eof; 
	_test_eof902:  sm->cs = 902; goto _test_eof; 
	_test_eof903:  sm->cs = 903; goto _test_eof; 
	_test_eof904:  sm->cs = 904; goto _test_eof; 
	_test_eof905:  sm->cs = 905; goto _test_eof; 
	_test_eof906:  sm->cs = 906; goto _test_eof; 
	_test_eof907:  sm->cs = 907; goto _test_eof; 
	_test_eof908:  sm->cs = 908; goto _test_eof; 
	_test_eof909:  sm->cs = 909; goto _test_eof; 
	_test_eof910:  sm->cs = 910; goto _test_eof; 
	_test_eof911:  sm->cs = 911; goto _test_eof; 
	_test_eof912:  sm->cs = 912; goto _test_eof; 
	_test_eof913:  sm->cs = 913; goto _test_eof; 
	_test_eof914:  sm->cs = 914; goto _test_eof; 
	_test_eof915:  sm->cs = 915; goto _test_eof; 
	_test_eof916:  sm->cs = 916; goto _test_eof; 
	_test_eof917:  sm->cs = 917; goto _test_eof; 
	_test_eof918:  sm->cs = 918; goto _test_eof; 
	_test_eof919:  sm->cs = 919; goto _test_eof; 
	_test_eof920:  sm->cs = 920; goto _test_eof; 
	_test_eof921:  sm->cs = 921; goto _test_eof; 
	_test_eof922:  sm->cs = 922; goto _test_eof; 
	_test_eof923:  sm->cs = 923; goto _test_eof; 
	_test_eof924:  sm->cs = 924; goto _test_eof; 
	_test_eof925:  sm->cs = 925; goto _test_eof; 
	_test_eof926:  sm->cs = 926; goto _test_eof; 
	_test_eof927:  sm->cs = 927; goto _test_eof; 
	_test_eof928:  sm->cs = 928; goto _test_eof; 
	_test_eof929:  sm->cs = 929; goto _test_eof; 
	_test_eof930:  sm->cs = 930; goto _test_eof; 
	_test_eof931:  sm->cs = 931; goto _test_eof; 
	_test_eof932:  sm->cs = 932; goto _test_eof; 
	_test_eof933:  sm->cs = 933; goto _test_eof; 
	_test_eof934:  sm->cs = 934; goto _test_eof; 
	_test_eof935:  sm->cs = 935; goto _test_eof; 
	_test_eof936:  sm->cs = 936; goto _test_eof; 
	_test_eof937:  sm->cs = 937; goto _test_eof; 
	_test_eof938:  sm->cs = 938; goto _test_eof; 
	_test_eof939:  sm->cs = 939; goto _test_eof; 
	_test_eof940:  sm->cs = 940; goto _test_eof; 
	_test_eof941:  sm->cs = 941; goto _test_eof; 
	_test_eof942:  sm->cs = 942; goto _test_eof; 
	_test_eof943:  sm->cs = 943; goto _test_eof; 
	_test_eof944:  sm->cs = 944; goto _test_eof; 
	_test_eof945:  sm->cs = 945; goto _test_eof; 
	_test_eof946:  sm->cs = 946; goto _test_eof; 
	_test_eof947:  sm->cs = 947; goto _test_eof; 
	_test_eof948:  sm->cs = 948; goto _test_eof; 
	_test_eof949:  sm->cs = 949; goto _test_eof; 
	_test_eof950:  sm->cs = 950; goto _test_eof; 
	_test_eof951:  sm->cs = 951; goto _test_eof; 
	_test_eof952:  sm->cs = 952; goto _test_eof; 
	_test_eof953:  sm->cs = 953; goto _test_eof; 
	_test_eof954:  sm->cs = 954; goto _test_eof; 
	_test_eof955:  sm->cs = 955; goto _test_eof; 
	_test_eof956:  sm->cs = 956; goto _test_eof; 
	_test_eof957:  sm->cs = 957; goto _test_eof; 
	_test_eof958:  sm->cs = 958; goto _test_eof; 
	_test_eof959:  sm->cs = 959; goto _test_eof; 
	_test_eof960:  sm->cs = 960; goto _test_eof; 
	_test_eof961:  sm->cs = 961; goto _test_eof; 
	_test_eof962:  sm->cs = 962; goto _test_eof; 
	_test_eof963:  sm->cs = 963; goto _test_eof; 
	_test_eof964:  sm->cs = 964; goto _test_eof; 
	_test_eof965:  sm->cs = 965; goto _test_eof; 
	_test_eof966:  sm->cs = 966; goto _test_eof; 
	_test_eof967:  sm->cs = 967; goto _test_eof; 
	_test_eof968:  sm->cs = 968; goto _test_eof; 
	_test_eof969:  sm->cs = 969; goto _test_eof; 
	_test_eof970:  sm->cs = 970; goto _test_eof; 
	_test_eof971:  sm->cs = 971; goto _test_eof; 
	_test_eof972:  sm->cs = 972; goto _test_eof; 
	_test_eof973:  sm->cs = 973; goto _test_eof; 
	_test_eof974:  sm->cs = 974; goto _test_eof; 
	_test_eof975:  sm->cs = 975; goto _test_eof; 
	_test_eof976:  sm->cs = 976; goto _test_eof; 
	_test_eof977:  sm->cs = 977; goto _test_eof; 
	_test_eof978:  sm->cs = 978; goto _test_eof; 
	_test_eof979:  sm->cs = 979; goto _test_eof; 
	_test_eof980:  sm->cs = 980; goto _test_eof; 
	_test_eof981:  sm->cs = 981; goto _test_eof; 
	_test_eof982:  sm->cs = 982; goto _test_eof; 
	_test_eof983:  sm->cs = 983; goto _test_eof; 
	_test_eof984:  sm->cs = 984; goto _test_eof; 
	_test_eof985:  sm->cs = 985; goto _test_eof; 
	_test_eof986:  sm->cs = 986; goto _test_eof; 
	_test_eof987:  sm->cs = 987; goto _test_eof; 
	_test_eof988:  sm->cs = 988; goto _test_eof; 
	_test_eof989:  sm->cs = 989; goto _test_eof; 
	_test_eof990:  sm->cs = 990; goto _test_eof; 
	_test_eof991:  sm->cs = 991; goto _test_eof; 
	_test_eof992:  sm->cs = 992; goto _test_eof; 
	_test_eof993:  sm->cs = 993; goto _test_eof; 
	_test_eof994:  sm->cs = 994; goto _test_eof; 
	_test_eof995:  sm->cs = 995; goto _test_eof; 
	_test_eof996:  sm->cs = 996; goto _test_eof; 
	_test_eof997:  sm->cs = 997; goto _test_eof; 
	_test_eof998:  sm->cs = 998; goto _test_eof; 
	_test_eof999:  sm->cs = 999; goto _test_eof; 
	_test_eof1000:  sm->cs = 1000; goto _test_eof; 
	_test_eof1001:  sm->cs = 1001; goto _test_eof; 
	_test_eof1002:  sm->cs = 1002; goto _test_eof; 
	_test_eof1003:  sm->cs = 1003; goto _test_eof; 
	_test_eof1004:  sm->cs = 1004; goto _test_eof; 
	_test_eof1005:  sm->cs = 1005; goto _test_eof; 
	_test_eof1006:  sm->cs = 1006; goto _test_eof; 
	_test_eof1007:  sm->cs = 1007; goto _test_eof; 
	_test_eof1008:  sm->cs = 1008; goto _test_eof; 
	_test_eof1009:  sm->cs = 1009; goto _test_eof; 
	_test_eof1010:  sm->cs = 1010; goto _test_eof; 
	_test_eof1011:  sm->cs = 1011; goto _test_eof; 
	_test_eof1012:  sm->cs = 1012; goto _test_eof; 
	_test_eof1013:  sm->cs = 1013; goto _test_eof; 
	_test_eof1014:  sm->cs = 1014; goto _test_eof; 
	_test_eof1015:  sm->cs = 1015; goto _test_eof; 
	_test_eof1016:  sm->cs = 1016; goto _test_eof; 
	_test_eof1017:  sm->cs = 1017; goto _test_eof; 
	_test_eof1018:  sm->cs = 1018; goto _test_eof; 
	_test_eof1019:  sm->cs = 1019; goto _test_eof; 
	_test_eof1020:  sm->cs = 1020; goto _test_eof; 
	_test_eof1021:  sm->cs = 1021; goto _test_eof; 
	_test_eof1022:  sm->cs = 1022; goto _test_eof; 
	_test_eof1023:  sm->cs = 1023; goto _test_eof; 
	_test_eof1024:  sm->cs = 1024; goto _test_eof; 
	_test_eof1025:  sm->cs = 1025; goto _test_eof; 
	_test_eof1026:  sm->cs = 1026; goto _test_eof; 
	_test_eof1027:  sm->cs = 1027; goto _test_eof; 
	_test_eof1028:  sm->cs = 1028; goto _test_eof; 
	_test_eof1029:  sm->cs = 1029; goto _test_eof; 
	_test_eof1030:  sm->cs = 1030; goto _test_eof; 
	_test_eof1143:  sm->cs = 1143; goto _test_eof; 
	_test_eof1144:  sm->cs = 1144; goto _test_eof; 
	_test_eof1031:  sm->cs = 1031; goto _test_eof; 
	_test_eof1032:  sm->cs = 1032; goto _test_eof; 
	_test_eof1033:  sm->cs = 1033; goto _test_eof; 
	_test_eof1034:  sm->cs = 1034; goto _test_eof; 
	_test_eof1035:  sm->cs = 1035; goto _test_eof; 
	_test_eof1036:  sm->cs = 1036; goto _test_eof; 
	_test_eof1037:  sm->cs = 1037; goto _test_eof; 
	_test_eof1038:  sm->cs = 1038; goto _test_eof; 
	_test_eof1039:  sm->cs = 1039; goto _test_eof; 
	_test_eof1040:  sm->cs = 1040; goto _test_eof; 
	_test_eof1145:  sm->cs = 1145; goto _test_eof; 
	_test_eof1146:  sm->cs = 1146; goto _test_eof; 
	_test_eof1147:  sm->cs = 1147; goto _test_eof; 
	_test_eof1148:  sm->cs = 1148; goto _test_eof; 
	_test_eof1041:  sm->cs = 1041; goto _test_eof; 
	_test_eof1042:  sm->cs = 1042; goto _test_eof; 
	_test_eof1043:  sm->cs = 1043; goto _test_eof; 
	_test_eof1044:  sm->cs = 1044; goto _test_eof; 
	_test_eof1045:  sm->cs = 1045; goto _test_eof; 
	_test_eof1149:  sm->cs = 1149; goto _test_eof; 
	_test_eof1150:  sm->cs = 1150; goto _test_eof; 
	_test_eof1046:  sm->cs = 1046; goto _test_eof; 
	_test_eof1047:  sm->cs = 1047; goto _test_eof; 
	_test_eof1048:  sm->cs = 1048; goto _test_eof; 
	_test_eof1049:  sm->cs = 1049; goto _test_eof; 
	_test_eof1050:  sm->cs = 1050; goto _test_eof; 
	_test_eof1051:  sm->cs = 1051; goto _test_eof; 
	_test_eof1052:  sm->cs = 1052; goto _test_eof; 
	_test_eof1053:  sm->cs = 1053; goto _test_eof; 
	_test_eof1054:  sm->cs = 1054; goto _test_eof; 
	_test_eof1055:  sm->cs = 1055; goto _test_eof; 
	_test_eof1056:  sm->cs = 1056; goto _test_eof; 
	_test_eof1057:  sm->cs = 1057; goto _test_eof; 
	_test_eof1058:  sm->cs = 1058; goto _test_eof; 
	_test_eof1059:  sm->cs = 1059; goto _test_eof; 
	_test_eof1060:  sm->cs = 1060; goto _test_eof; 
	_test_eof1061:  sm->cs = 1061; goto _test_eof; 
	_test_eof1062:  sm->cs = 1062; goto _test_eof; 
	_test_eof1063:  sm->cs = 1063; goto _test_eof; 
	_test_eof1064:  sm->cs = 1064; goto _test_eof; 
	_test_eof1065:  sm->cs = 1065; goto _test_eof; 
	_test_eof1066:  sm->cs = 1066; goto _test_eof; 
	_test_eof1067:  sm->cs = 1067; goto _test_eof; 
	_test_eof1068:  sm->cs = 1068; goto _test_eof; 
	_test_eof1069:  sm->cs = 1069; goto _test_eof; 
	_test_eof1070:  sm->cs = 1070; goto _test_eof; 
	_test_eof1071:  sm->cs = 1071; goto _test_eof; 

	_test_eof: {}
	if ( ( sm->p) == ( sm->eof) )
	{
	switch (  sm->cs ) {
	case 1073: goto tr0;
	case 0: goto tr0;
	case 1074: goto tr1160;
	case 1075: goto tr1160;
	case 1: goto tr2;
	case 1076: goto tr1161;
	case 1077: goto tr1161;
	case 2: goto tr2;
	case 1078: goto tr1160;
	case 3: goto tr2;
	case 1079: goto tr1164;
	case 1080: goto tr1160;
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
	case 1081: goto tr1171;
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
	case 151: goto tr2;
	case 152: goto tr2;
	case 153: goto tr2;
	case 154: goto tr2;
	case 155: goto tr2;
	case 156: goto tr2;
	case 157: goto tr2;
	case 158: goto tr2;
	case 159: goto tr2;
	case 160: goto tr2;
	case 161: goto tr2;
	case 162: goto tr2;
	case 163: goto tr2;
	case 164: goto tr2;
	case 165: goto tr2;
	case 166: goto tr2;
	case 167: goto tr2;
	case 168: goto tr2;
	case 169: goto tr2;
	case 170: goto tr2;
	case 171: goto tr2;
	case 172: goto tr2;
	case 173: goto tr2;
	case 174: goto tr2;
	case 175: goto tr2;
	case 176: goto tr2;
	case 177: goto tr2;
	case 178: goto tr2;
	case 179: goto tr2;
	case 180: goto tr2;
	case 181: goto tr2;
	case 182: goto tr2;
	case 183: goto tr2;
	case 184: goto tr2;
	case 185: goto tr2;
	case 186: goto tr2;
	case 187: goto tr2;
	case 188: goto tr2;
	case 189: goto tr2;
	case 190: goto tr2;
	case 191: goto tr2;
	case 192: goto tr2;
	case 193: goto tr2;
	case 194: goto tr2;
	case 195: goto tr2;
	case 196: goto tr2;
	case 197: goto tr2;
	case 198: goto tr2;
	case 199: goto tr2;
	case 200: goto tr2;
	case 201: goto tr2;
	case 202: goto tr2;
	case 203: goto tr2;
	case 204: goto tr2;
	case 205: goto tr2;
	case 206: goto tr2;
	case 207: goto tr2;
	case 208: goto tr2;
	case 209: goto tr2;
	case 210: goto tr2;
	case 211: goto tr2;
	case 212: goto tr2;
	case 213: goto tr2;
	case 214: goto tr2;
	case 215: goto tr2;
	case 216: goto tr2;
	case 217: goto tr2;
	case 218: goto tr2;
	case 219: goto tr2;
	case 220: goto tr2;
	case 221: goto tr2;
	case 222: goto tr2;
	case 223: goto tr2;
	case 1082: goto tr1172;
	case 224: goto tr2;
	case 225: goto tr2;
	case 226: goto tr2;
	case 227: goto tr2;
	case 228: goto tr2;
	case 229: goto tr2;
	case 230: goto tr2;
	case 231: goto tr2;
	case 232: goto tr2;
	case 233: goto tr2;
	case 234: goto tr2;
	case 235: goto tr2;
	case 236: goto tr2;
	case 237: goto tr2;
	case 238: goto tr2;
	case 239: goto tr2;
	case 240: goto tr2;
	case 241: goto tr2;
	case 242: goto tr2;
	case 243: goto tr2;
	case 244: goto tr2;
	case 245: goto tr2;
	case 246: goto tr2;
	case 247: goto tr2;
	case 248: goto tr2;
	case 249: goto tr2;
	case 250: goto tr2;
	case 251: goto tr2;
	case 252: goto tr2;
	case 253: goto tr2;
	case 254: goto tr2;
	case 255: goto tr2;
	case 256: goto tr2;
	case 257: goto tr2;
	case 258: goto tr2;
	case 1083: goto tr1173;
	case 1084: goto tr1175;
	case 259: goto tr2;
	case 260: goto tr2;
	case 261: goto tr2;
	case 262: goto tr2;
	case 263: goto tr2;
	case 264: goto tr2;
	case 265: goto tr2;
	case 266: goto tr2;
	case 267: goto tr2;
	case 268: goto tr2;
	case 269: goto tr2;
	case 270: goto tr2;
	case 271: goto tr2;
	case 272: goto tr2;
	case 273: goto tr2;
	case 274: goto tr2;
	case 275: goto tr2;
	case 276: goto tr2;
	case 277: goto tr2;
	case 1085: goto tr1176;
	case 1086: goto tr1178;
	case 278: goto tr2;
	case 279: goto tr2;
	case 280: goto tr2;
	case 281: goto tr2;
	case 282: goto tr2;
	case 283: goto tr2;
	case 284: goto tr2;
	case 1087: goto tr1179;
	case 285: goto tr2;
	case 286: goto tr2;
	case 287: goto tr2;
	case 288: goto tr2;
	case 289: goto tr2;
	case 1089: goto tr1182;
	case 290: goto tr305;
	case 291: goto tr305;
	case 292: goto tr305;
	case 293: goto tr305;
	case 294: goto tr305;
	case 295: goto tr305;
	case 296: goto tr305;
	case 297: goto tr305;
	case 298: goto tr305;
	case 299: goto tr305;
	case 300: goto tr305;
	case 301: goto tr305;
	case 302: goto tr305;
	case 303: goto tr305;
	case 304: goto tr305;
	case 1091: goto tr1211;
	case 1092: goto tr1216;
	case 305: goto tr328;
	case 306: goto tr330;
	case 307: goto tr330;
	case 308: goto tr330;
	case 309: goto tr328;
	case 310: goto tr328;
	case 311: goto tr328;
	case 312: goto tr328;
	case 313: goto tr328;
	case 314: goto tr328;
	case 315: goto tr328;
	case 316: goto tr328;
	case 317: goto tr328;
	case 318: goto tr344;
	case 319: goto tr344;
	case 1093: goto tr1218;
	case 1094: goto tr1218;
	case 320: goto tr344;
	case 321: goto tr344;
	case 1095: goto tr1220;
	case 322: goto tr344;
	case 323: goto tr344;
	case 324: goto tr328;
	case 325: goto tr328;
	case 326: goto tr328;
	case 327: goto tr328;
	case 328: goto tr328;
	case 1096: goto tr1222;
	case 329: goto tr344;
	case 330: goto tr328;
	case 331: goto tr328;
	case 332: goto tr328;
	case 333: goto tr328;
	case 334: goto tr328;
	case 335: goto tr328;
	case 1097: goto tr1223;
	case 1098: goto tr1224;
	case 1099: goto tr1225;
	case 336: goto tr366;
	case 337: goto tr366;
	case 338: goto tr366;
	case 339: goto tr366;
	case 1100: goto tr1227;
	case 340: goto tr366;
	case 341: goto tr366;
	case 342: goto tr366;
	case 343: goto tr366;
	case 344: goto tr366;
	case 345: goto tr366;
	case 346: goto tr366;
	case 347: goto tr366;
	case 348: goto tr366;
	case 349: goto tr366;
	case 350: goto tr366;
	case 351: goto tr366;
	case 352: goto tr366;
	case 353: goto tr366;
	case 354: goto tr366;
	case 355: goto tr366;
	case 356: goto tr366;
	case 1101: goto tr1225;
	case 357: goto tr366;
	case 358: goto tr366;
	case 359: goto tr366;
	case 360: goto tr366;
	case 361: goto tr366;
	case 362: goto tr366;
	case 363: goto tr366;
	case 364: goto tr366;
	case 365: goto tr366;
	case 1102: goto tr1225;
	case 366: goto tr366;
	case 367: goto tr366;
	case 368: goto tr366;
	case 369: goto tr366;
	case 370: goto tr366;
	case 371: goto tr366;
	case 1103: goto tr1232;
	case 372: goto tr366;
	case 373: goto tr366;
	case 374: goto tr366;
	case 375: goto tr366;
	case 376: goto tr366;
	case 377: goto tr366;
	case 378: goto tr366;
	case 1104: goto tr1234;
	case 379: goto tr366;
	case 380: goto tr366;
	case 381: goto tr366;
	case 382: goto tr366;
	case 383: goto tr366;
	case 384: goto tr366;
	case 385: goto tr366;
	case 1105: goto tr1236;
	case 1106: goto tr1225;
	case 386: goto tr366;
	case 387: goto tr366;
	case 388: goto tr366;
	case 389: goto tr366;
	case 1107: goto tr1241;
	case 390: goto tr366;
	case 391: goto tr366;
	case 392: goto tr366;
	case 393: goto tr366;
	case 394: goto tr366;
	case 1108: goto tr1243;
	case 395: goto tr366;
	case 396: goto tr366;
	case 397: goto tr366;
	case 398: goto tr366;
	case 1109: goto tr1245;
	case 1110: goto tr1225;
	case 399: goto tr366;
	case 400: goto tr366;
	case 401: goto tr366;
	case 402: goto tr366;
	case 403: goto tr366;
	case 404: goto tr366;
	case 405: goto tr366;
	case 406: goto tr366;
	case 1111: goto tr1248;
	case 1112: goto tr1225;
	case 407: goto tr366;
	case 408: goto tr366;
	case 409: goto tr366;
	case 410: goto tr366;
	case 411: goto tr366;
	case 1113: goto tr1252;
	case 412: goto tr366;
	case 413: goto tr366;
	case 414: goto tr366;
	case 415: goto tr366;
	case 416: goto tr366;
	case 417: goto tr366;
	case 1114: goto tr1254;
	case 1115: goto tr1225;
	case 418: goto tr366;
	case 419: goto tr366;
	case 420: goto tr366;
	case 421: goto tr366;
	case 422: goto tr366;
	case 423: goto tr366;
	case 1116: goto tr1257;
	case 424: goto tr366;
	case 1117: goto tr1225;
	case 425: goto tr366;
	case 426: goto tr366;
	case 427: goto tr366;
	case 428: goto tr366;
	case 429: goto tr366;
	case 430: goto tr366;
	case 431: goto tr366;
	case 432: goto tr366;
	case 433: goto tr366;
	case 434: goto tr366;
	case 435: goto tr366;
	case 436: goto tr366;
	case 1118: goto tr1259;
	case 1119: goto tr1225;
	case 437: goto tr366;
	case 438: goto tr366;
	case 439: goto tr366;
	case 440: goto tr366;
	case 441: goto tr366;
	case 442: goto tr366;
	case 443: goto tr366;
	case 444: goto tr366;
	case 445: goto tr366;
	case 446: goto tr366;
	case 447: goto tr366;
	case 1120: goto tr1262;
	case 1121: goto tr1225;
	case 448: goto tr366;
	case 449: goto tr366;
	case 450: goto tr366;
	case 451: goto tr366;
	case 452: goto tr366;
	case 1122: goto tr1265;
	case 1123: goto tr1225;
	case 453: goto tr366;
	case 454: goto tr366;
	case 455: goto tr366;
	case 456: goto tr366;
	case 457: goto tr366;
	case 1124: goto tr1268;
	case 458: goto tr366;
	case 459: goto tr366;
	case 460: goto tr366;
	case 461: goto tr366;
	case 1125: goto tr1270;
	case 462: goto tr366;
	case 463: goto tr366;
	case 464: goto tr366;
	case 465: goto tr366;
	case 466: goto tr366;
	case 467: goto tr366;
	case 468: goto tr366;
	case 469: goto tr366;
	case 470: goto tr366;
	case 1126: goto tr1272;
	case 1127: goto tr1225;
	case 471: goto tr366;
	case 472: goto tr366;
	case 473: goto tr366;
	case 474: goto tr366;
	case 475: goto tr366;
	case 476: goto tr366;
	case 477: goto tr366;
	case 1128: goto tr1275;
	case 1129: goto tr1225;
	case 478: goto tr366;
	case 479: goto tr366;
	case 480: goto tr366;
	case 481: goto tr366;
	case 1130: goto tr1278;
	case 1131: goto tr1225;
	case 482: goto tr366;
	case 483: goto tr366;
	case 484: goto tr366;
	case 485: goto tr366;
	case 486: goto tr366;
	case 487: goto tr366;
	case 488: goto tr366;
	case 489: goto tr366;
	case 490: goto tr366;
	case 491: goto tr366;
	case 1132: goto tr1284;
	case 492: goto tr366;
	case 493: goto tr366;
	case 494: goto tr366;
	case 495: goto tr366;
	case 496: goto tr366;
	case 497: goto tr366;
	case 498: goto tr366;
	case 499: goto tr366;
	case 500: goto tr366;
	case 501: goto tr366;
	case 502: goto tr366;
	case 503: goto tr366;
	case 504: goto tr366;
	case 505: goto tr366;
	case 1133: goto tr1286;
	case 506: goto tr366;
	case 507: goto tr366;
	case 508: goto tr366;
	case 509: goto tr366;
	case 510: goto tr366;
	case 511: goto tr366;
	case 512: goto tr366;
	case 1134: goto tr1288;
	case 513: goto tr366;
	case 514: goto tr366;
	case 515: goto tr366;
	case 516: goto tr366;
	case 517: goto tr366;
	case 518: goto tr366;
	case 1135: goto tr1290;
	case 1136: goto tr1225;
	case 519: goto tr366;
	case 520: goto tr366;
	case 521: goto tr366;
	case 522: goto tr366;
	case 523: goto tr366;
	case 1137: goto tr1293;
	case 1138: goto tr1225;
	case 524: goto tr366;
	case 525: goto tr366;
	case 526: goto tr366;
	case 527: goto tr366;
	case 528: goto tr366;
	case 1139: goto tr1296;
	case 1140: goto tr1225;
	case 529: goto tr366;
	case 530: goto tr366;
	case 531: goto tr366;
	case 532: goto tr366;
	case 533: goto tr366;
	case 534: goto tr366;
	case 535: goto tr366;
	case 536: goto tr366;
	case 1141: goto tr1308;
	case 537: goto tr366;
	case 538: goto tr366;
	case 539: goto tr366;
	case 540: goto tr366;
	case 541: goto tr366;
	case 542: goto tr366;
	case 543: goto tr366;
	case 544: goto tr366;
	case 545: goto tr366;
	case 546: goto tr366;
	case 547: goto tr366;
	case 548: goto tr366;
	case 549: goto tr366;
	case 1142: goto tr1309;
	case 550: goto tr366;
	case 551: goto tr366;
	case 552: goto tr366;
	case 553: goto tr366;
	case 554: goto tr366;
	case 555: goto tr366;
	case 556: goto tr366;
	case 557: goto tr366;
	case 558: goto tr366;
	case 559: goto tr366;
	case 560: goto tr366;
	case 561: goto tr366;
	case 562: goto tr366;
	case 563: goto tr366;
	case 564: goto tr366;
	case 565: goto tr366;
	case 566: goto tr366;
	case 567: goto tr366;
	case 568: goto tr366;
	case 569: goto tr366;
	case 570: goto tr366;
	case 571: goto tr366;
	case 572: goto tr366;
	case 573: goto tr366;
	case 574: goto tr366;
	case 575: goto tr366;
	case 576: goto tr366;
	case 577: goto tr366;
	case 578: goto tr366;
	case 579: goto tr366;
	case 580: goto tr366;
	case 581: goto tr366;
	case 582: goto tr366;
	case 583: goto tr366;
	case 584: goto tr366;
	case 585: goto tr366;
	case 586: goto tr366;
	case 587: goto tr366;
	case 588: goto tr366;
	case 589: goto tr366;
	case 590: goto tr366;
	case 591: goto tr366;
	case 592: goto tr366;
	case 593: goto tr366;
	case 594: goto tr366;
	case 595: goto tr366;
	case 596: goto tr366;
	case 597: goto tr366;
	case 598: goto tr366;
	case 599: goto tr366;
	case 600: goto tr366;
	case 601: goto tr366;
	case 602: goto tr366;
	case 603: goto tr366;
	case 604: goto tr366;
	case 605: goto tr366;
	case 606: goto tr366;
	case 607: goto tr366;
	case 608: goto tr366;
	case 609: goto tr366;
	case 610: goto tr366;
	case 611: goto tr366;
	case 612: goto tr366;
	case 613: goto tr366;
	case 614: goto tr366;
	case 615: goto tr366;
	case 616: goto tr366;
	case 617: goto tr366;
	case 618: goto tr366;
	case 619: goto tr366;
	case 620: goto tr366;
	case 621: goto tr366;
	case 622: goto tr366;
	case 623: goto tr366;
	case 624: goto tr366;
	case 625: goto tr366;
	case 626: goto tr366;
	case 627: goto tr366;
	case 628: goto tr366;
	case 629: goto tr366;
	case 630: goto tr366;
	case 631: goto tr366;
	case 632: goto tr366;
	case 633: goto tr366;
	case 634: goto tr366;
	case 635: goto tr366;
	case 636: goto tr366;
	case 637: goto tr366;
	case 638: goto tr366;
	case 639: goto tr366;
	case 640: goto tr366;
	case 641: goto tr366;
	case 642: goto tr366;
	case 643: goto tr366;
	case 644: goto tr366;
	case 645: goto tr366;
	case 646: goto tr366;
	case 647: goto tr366;
	case 648: goto tr366;
	case 649: goto tr366;
	case 650: goto tr366;
	case 651: goto tr366;
	case 652: goto tr366;
	case 653: goto tr366;
	case 654: goto tr366;
	case 655: goto tr366;
	case 656: goto tr366;
	case 657: goto tr366;
	case 658: goto tr366;
	case 659: goto tr366;
	case 660: goto tr366;
	case 661: goto tr366;
	case 662: goto tr366;
	case 663: goto tr366;
	case 664: goto tr366;
	case 665: goto tr366;
	case 666: goto tr366;
	case 667: goto tr366;
	case 668: goto tr366;
	case 669: goto tr366;
	case 670: goto tr366;
	case 671: goto tr366;
	case 672: goto tr366;
	case 673: goto tr366;
	case 674: goto tr366;
	case 675: goto tr366;
	case 676: goto tr366;
	case 677: goto tr366;
	case 678: goto tr366;
	case 679: goto tr366;
	case 680: goto tr366;
	case 681: goto tr366;
	case 682: goto tr366;
	case 683: goto tr366;
	case 684: goto tr366;
	case 685: goto tr366;
	case 686: goto tr366;
	case 687: goto tr366;
	case 688: goto tr366;
	case 689: goto tr366;
	case 690: goto tr366;
	case 691: goto tr366;
	case 692: goto tr366;
	case 693: goto tr366;
	case 694: goto tr366;
	case 695: goto tr366;
	case 696: goto tr366;
	case 697: goto tr366;
	case 698: goto tr366;
	case 699: goto tr366;
	case 700: goto tr366;
	case 701: goto tr366;
	case 702: goto tr366;
	case 703: goto tr366;
	case 704: goto tr366;
	case 705: goto tr366;
	case 706: goto tr366;
	case 707: goto tr366;
	case 708: goto tr366;
	case 709: goto tr366;
	case 710: goto tr366;
	case 711: goto tr366;
	case 712: goto tr366;
	case 713: goto tr366;
	case 714: goto tr366;
	case 715: goto tr366;
	case 716: goto tr366;
	case 717: goto tr366;
	case 718: goto tr366;
	case 719: goto tr366;
	case 720: goto tr366;
	case 721: goto tr366;
	case 722: goto tr366;
	case 723: goto tr366;
	case 724: goto tr366;
	case 725: goto tr366;
	case 726: goto tr366;
	case 727: goto tr366;
	case 728: goto tr366;
	case 729: goto tr366;
	case 730: goto tr366;
	case 731: goto tr366;
	case 732: goto tr366;
	case 733: goto tr366;
	case 734: goto tr366;
	case 735: goto tr366;
	case 736: goto tr366;
	case 737: goto tr366;
	case 738: goto tr366;
	case 739: goto tr366;
	case 740: goto tr366;
	case 741: goto tr366;
	case 742: goto tr366;
	case 743: goto tr366;
	case 744: goto tr366;
	case 745: goto tr366;
	case 746: goto tr366;
	case 747: goto tr366;
	case 748: goto tr366;
	case 749: goto tr366;
	case 750: goto tr366;
	case 751: goto tr366;
	case 752: goto tr366;
	case 753: goto tr366;
	case 754: goto tr366;
	case 755: goto tr366;
	case 756: goto tr366;
	case 757: goto tr366;
	case 758: goto tr366;
	case 759: goto tr366;
	case 760: goto tr366;
	case 761: goto tr366;
	case 762: goto tr366;
	case 763: goto tr366;
	case 764: goto tr366;
	case 765: goto tr366;
	case 766: goto tr366;
	case 767: goto tr366;
	case 768: goto tr366;
	case 769: goto tr366;
	case 770: goto tr366;
	case 771: goto tr366;
	case 772: goto tr366;
	case 773: goto tr366;
	case 774: goto tr366;
	case 775: goto tr366;
	case 776: goto tr366;
	case 777: goto tr366;
	case 778: goto tr366;
	case 779: goto tr366;
	case 780: goto tr366;
	case 781: goto tr366;
	case 782: goto tr366;
	case 783: goto tr366;
	case 784: goto tr366;
	case 785: goto tr366;
	case 786: goto tr366;
	case 787: goto tr366;
	case 788: goto tr366;
	case 789: goto tr366;
	case 790: goto tr366;
	case 791: goto tr366;
	case 792: goto tr366;
	case 793: goto tr366;
	case 794: goto tr366;
	case 795: goto tr366;
	case 796: goto tr366;
	case 797: goto tr366;
	case 798: goto tr366;
	case 799: goto tr366;
	case 800: goto tr366;
	case 801: goto tr366;
	case 802: goto tr366;
	case 803: goto tr366;
	case 804: goto tr366;
	case 805: goto tr366;
	case 806: goto tr366;
	case 807: goto tr366;
	case 808: goto tr366;
	case 809: goto tr366;
	case 810: goto tr366;
	case 811: goto tr366;
	case 812: goto tr366;
	case 813: goto tr366;
	case 814: goto tr366;
	case 815: goto tr366;
	case 816: goto tr366;
	case 817: goto tr366;
	case 818: goto tr366;
	case 819: goto tr366;
	case 820: goto tr366;
	case 821: goto tr366;
	case 822: goto tr366;
	case 823: goto tr366;
	case 824: goto tr366;
	case 825: goto tr366;
	case 826: goto tr366;
	case 827: goto tr366;
	case 828: goto tr366;
	case 829: goto tr366;
	case 830: goto tr366;
	case 831: goto tr366;
	case 832: goto tr366;
	case 833: goto tr366;
	case 834: goto tr366;
	case 835: goto tr366;
	case 836: goto tr366;
	case 837: goto tr366;
	case 838: goto tr366;
	case 839: goto tr366;
	case 840: goto tr366;
	case 841: goto tr366;
	case 842: goto tr366;
	case 843: goto tr366;
	case 844: goto tr366;
	case 845: goto tr366;
	case 846: goto tr366;
	case 847: goto tr366;
	case 848: goto tr366;
	case 849: goto tr366;
	case 850: goto tr366;
	case 851: goto tr366;
	case 852: goto tr366;
	case 853: goto tr366;
	case 854: goto tr366;
	case 855: goto tr366;
	case 856: goto tr366;
	case 857: goto tr366;
	case 858: goto tr366;
	case 859: goto tr366;
	case 860: goto tr366;
	case 861: goto tr366;
	case 862: goto tr366;
	case 863: goto tr366;
	case 864: goto tr366;
	case 865: goto tr366;
	case 866: goto tr366;
	case 867: goto tr366;
	case 868: goto tr366;
	case 869: goto tr366;
	case 870: goto tr366;
	case 871: goto tr366;
	case 872: goto tr366;
	case 873: goto tr366;
	case 874: goto tr366;
	case 875: goto tr366;
	case 876: goto tr366;
	case 877: goto tr366;
	case 878: goto tr366;
	case 879: goto tr366;
	case 880: goto tr366;
	case 881: goto tr366;
	case 882: goto tr366;
	case 883: goto tr366;
	case 884: goto tr366;
	case 885: goto tr366;
	case 886: goto tr366;
	case 887: goto tr366;
	case 888: goto tr366;
	case 889: goto tr366;
	case 890: goto tr366;
	case 891: goto tr366;
	case 892: goto tr366;
	case 893: goto tr366;
	case 894: goto tr366;
	case 895: goto tr366;
	case 896: goto tr366;
	case 897: goto tr366;
	case 898: goto tr366;
	case 899: goto tr366;
	case 900: goto tr366;
	case 901: goto tr366;
	case 902: goto tr366;
	case 903: goto tr366;
	case 904: goto tr366;
	case 905: goto tr366;
	case 906: goto tr366;
	case 907: goto tr366;
	case 908: goto tr366;
	case 909: goto tr366;
	case 910: goto tr366;
	case 911: goto tr366;
	case 912: goto tr366;
	case 913: goto tr366;
	case 914: goto tr366;
	case 915: goto tr366;
	case 916: goto tr366;
	case 917: goto tr366;
	case 918: goto tr366;
	case 919: goto tr366;
	case 920: goto tr366;
	case 921: goto tr366;
	case 922: goto tr366;
	case 923: goto tr366;
	case 924: goto tr366;
	case 925: goto tr366;
	case 926: goto tr366;
	case 927: goto tr366;
	case 928: goto tr366;
	case 929: goto tr366;
	case 930: goto tr366;
	case 931: goto tr366;
	case 932: goto tr366;
	case 933: goto tr366;
	case 934: goto tr366;
	case 935: goto tr366;
	case 936: goto tr366;
	case 937: goto tr366;
	case 938: goto tr366;
	case 939: goto tr366;
	case 940: goto tr366;
	case 941: goto tr366;
	case 942: goto tr366;
	case 943: goto tr366;
	case 944: goto tr366;
	case 945: goto tr366;
	case 946: goto tr366;
	case 947: goto tr366;
	case 948: goto tr366;
	case 949: goto tr366;
	case 950: goto tr366;
	case 951: goto tr366;
	case 952: goto tr366;
	case 953: goto tr366;
	case 954: goto tr366;
	case 955: goto tr366;
	case 956: goto tr366;
	case 957: goto tr366;
	case 958: goto tr366;
	case 959: goto tr366;
	case 960: goto tr366;
	case 961: goto tr366;
	case 962: goto tr366;
	case 963: goto tr366;
	case 964: goto tr366;
	case 965: goto tr366;
	case 966: goto tr366;
	case 967: goto tr366;
	case 968: goto tr366;
	case 969: goto tr366;
	case 970: goto tr366;
	case 971: goto tr366;
	case 972: goto tr366;
	case 973: goto tr366;
	case 974: goto tr366;
	case 975: goto tr366;
	case 976: goto tr366;
	case 977: goto tr366;
	case 978: goto tr366;
	case 979: goto tr366;
	case 980: goto tr366;
	case 981: goto tr366;
	case 982: goto tr366;
	case 983: goto tr366;
	case 984: goto tr366;
	case 985: goto tr366;
	case 986: goto tr366;
	case 987: goto tr366;
	case 988: goto tr366;
	case 989: goto tr366;
	case 990: goto tr366;
	case 991: goto tr366;
	case 992: goto tr366;
	case 993: goto tr366;
	case 994: goto tr366;
	case 995: goto tr366;
	case 996: goto tr366;
	case 997: goto tr366;
	case 998: goto tr366;
	case 999: goto tr366;
	case 1000: goto tr366;
	case 1001: goto tr366;
	case 1002: goto tr366;
	case 1003: goto tr366;
	case 1004: goto tr366;
	case 1005: goto tr366;
	case 1006: goto tr366;
	case 1007: goto tr366;
	case 1008: goto tr366;
	case 1009: goto tr366;
	case 1010: goto tr366;
	case 1011: goto tr366;
	case 1012: goto tr366;
	case 1013: goto tr366;
	case 1014: goto tr366;
	case 1015: goto tr366;
	case 1016: goto tr366;
	case 1017: goto tr366;
	case 1018: goto tr366;
	case 1019: goto tr366;
	case 1020: goto tr366;
	case 1021: goto tr366;
	case 1022: goto tr366;
	case 1023: goto tr366;
	case 1024: goto tr366;
	case 1025: goto tr366;
	case 1026: goto tr366;
	case 1027: goto tr366;
	case 1028: goto tr366;
	case 1029: goto tr366;
	case 1030: goto tr366;
	case 1143: goto tr1225;
	case 1144: goto tr1225;
	case 1031: goto tr366;
	case 1032: goto tr366;
	case 1033: goto tr366;
	case 1034: goto tr366;
	case 1035: goto tr366;
	case 1036: goto tr366;
	case 1037: goto tr366;
	case 1038: goto tr366;
	case 1039: goto tr366;
	case 1040: goto tr366;
	case 1146: goto tr1315;
	case 1148: goto tr1319;
	case 1041: goto tr1113;
	case 1042: goto tr1113;
	case 1043: goto tr1113;
	case 1044: goto tr1113;
	case 1045: goto tr1113;
	case 1150: goto tr1323;
	case 1046: goto tr1119;
	case 1047: goto tr1119;
	case 1048: goto tr1119;
	case 1049: goto tr1119;
	case 1050: goto tr1119;
	case 1051: goto tr1119;
	case 1052: goto tr1119;
	case 1053: goto tr1119;
	case 1054: goto tr1119;
	case 1055: goto tr1119;
	case 1056: goto tr1119;
	case 1057: goto tr1119;
	case 1058: goto tr1119;
	case 1059: goto tr1119;
	case 1060: goto tr1119;
	case 1061: goto tr1119;
	case 1062: goto tr1119;
	case 1063: goto tr1119;
	case 1064: goto tr1119;
	case 1065: goto tr1119;
	case 1066: goto tr1119;
	case 1067: goto tr1119;
	case 1068: goto tr1119;
	case 1069: goto tr1119;
	case 1070: goto tr1119;
	case 1071: goto tr1119;
	}
	}

	}

#line 1212 "ext/dtext/dtext.cpp.rl"

  sm->dstack_close_all();

  return DTextResult { sm->output, sm->posts };
}
