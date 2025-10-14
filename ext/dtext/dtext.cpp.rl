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

%%{
machine dtext;

access sm->;
variable p sm->p;
variable pe sm->pe;
variable eof sm->eof;
variable top sm->top;
variable ts sm->ts;
variable te sm->te;
variable act sm->act;
variable stack (sm->stack.data());

prepush {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
}

action mark_a1 { a1 = p; }
action mark_a2 { a2 = p; }
action mark_b1 { b1 = p; }
action mark_b2 { b2 = p; }

action in_quote { dstack_is_open(BLOCK_QUOTE) }
action in_section { dstack_is_open(BLOCK_SECTION) }

newline = '\r\n' | '\n';

nonnewline = any - (newline | '\r');
nonquote = ^'"';
nonbracket = ^']';
nonpipe = ^'|';
nonpipebracket = nonpipe & nonbracket;
noncurly = ^'}';
nonpipecurly = nonpipe & noncurly;

url = 'http'i 's'i? '://' ^space+;
delimited_url = '<' url >mark_a1 %mark_a2 :>> '>';
internal_url = [/#] ^space+;
basic_textile_link = '"' nonquote+ >mark_a1 %mark_a2 '":' (url | internal_url) >mark_b1 %mark_b2;
bracketed_textile_link = '"' nonquote+ >mark_a1 %mark_a2 '":[' (url | internal_url) >mark_b1 %mark_b2 :>> ']';

basic_wiki_link = '[[' (nonbracket nonpipebracket*) >mark_a1 %mark_a2 ']]';
aliased_wiki_link = '[[' nonpipebracket+ >mark_a1 %mark_a2 '|' nonpipebracket+ >mark_b1 %mark_b2 ']]';

basic_post_search_link = '{{' (noncurly nonpipecurly*) >mark_a1 %mark_a2 '}}';
aliased_post_search_link = '{{' nonpipecurly+ >mark_a1 %mark_a2 '|' nonpipecurly+ >mark_b1 %mark_b2 '}}';

spoilers_open = '[spoiler'i 's'i? ']';
spoilers_close = '[/spoiler'i 's'i? ']';

color_name = (
    'gen'i ('eral'i)?
  | 'art'i ('ist'i)?
  | 'cont'i ('ributor'i)?
  | 'copy'i ('right'i)?
  | 'char'i ('acter'i)?
  | 'spec'i ('ies'i)?
  | 'inv'i ('alid'i)?
  | 'meta'i
  | 'lor'i ('e'i)?
  );

color_value = (([a-z]+ - color_name) | '#'i[0-9a-fA-F]{3,6});

color_open = '[color='i color_value >mark_a1 %mark_a2 ']';
color_typed = '[color='i color_name >mark_a1 %mark_a2 ']';
color_close = '[/color]'i;

id = digit+ >mark_a1 %mark_a2;
page = digit+ >mark_b1 %mark_b2;

post_id = 'post #'i id;
post_changes_for_id = 'post changes #'i id;
thumb_id = 'thumb #'i id;
post_flag_id = 'flag #'i id;
note_id = 'note #'i id;
forum_post_id = 'forum #'i id;
forum_topic_id = 'topic #'i id;
comment_id = 'comment #'i id;
pool_id = 'pool #'i id;
user_id = 'user #'i id;
artist_id = 'artist #'i id;
ban_id = 'ban #'i id;
bulk_update_request_id = 'bur #'i id;
tag_alias_id = 'alias #'i id;
tag_implication_id = 'implication #'i id;
mod_action_id = 'mod action #'i id;
user_feedback_id = 'record #'i id;
wiki_page_id = 'wiki #'i id;
set_id = 'set #'i id;
blip_id = 'blip #'i id;
takedown_id = 'take'i ' 'i? 'down 'i 'request 'i? '#'i id;
ticket_id = 'ticket #'i id;

ws = ' ' | '\t';
nonperiod = graph - ('.' | '"');
header = 'h'i [123456] >mark_a1 %mark_a2 '.' ws*;

section_open = '[section]'i;
section_open_expanded = '[section,expanded]'i;
section_close = '[/section'i (']' when in_section);
section_open_aliased = '[section='i (nonbracket+ >mark_a1 %mark_a2) ']';
section_open_aliased_expanded = '[section,expanded='i (nonbracket+ >mark_a1 %mark_a2) ']';

quote_open = '[quote]'i;
quote_open_colored = '[quote='i ([a-z]+|'#'i[0-9a-fA-F]{3,6}) >mark_a1 %mark_a2 ']';
quote_open_colored_typed = '[quote='i ('art'i('ist'i)?|'char'i('acter'i)?|'copy'i('right'i)?|'spec'i('ies'i)?|'inv'i('alid'i)?|'meta'i|'lore'i) >mark_a1 %mark_a2 ']';
quote_close = '[/quote'i (']' when in_quote);

internal_anchor = '[#' ((alnum | [_\-])+ >mark_a1 %mark_a2) ']';

list_item = '*'+ >mark_a1 %mark_a2 ws+ nonnewline+ >mark_b1 %mark_b2;

basic_inline := |*
  '[b]'i    => { dstack_open_inline(INLINE_B, "<strong>"); };
  '[/b]'i   => { dstack_close_inline(INLINE_B, "</strong>"); };
  '[i]'i    => { dstack_open_inline(INLINE_I, "<em>"); };
  '[/i]'i   => { dstack_close_inline(INLINE_I, "</em>"); };
  '[s]'i    => { dstack_open_inline(INLINE_S, "<s>"); };
  '[/s]'i   => { dstack_close_inline(INLINE_S, "</s>"); };
  '[u]'i    => { dstack_open_inline(INLINE_U, "<u>"); };
  '[/u]'i   => { dstack_close_inline(INLINE_U, "</u>"); };
  '[sup]'i  => { dstack_open_inline(INLINE_SUP, "<sup>"); };
  '[/sup]'i => { dstack_close_inline(INLINE_SUP, "</sup>"); };
  '[sub]'i  => { dstack_open_inline(INLINE_SUB, "<sub>"); };
  '[/sub]'i => { dstack_close_inline(INLINE_SUB, "</sub>"); };
  any => { append_html_escaped(fc); };
*|;

inline := |*
  '\\`' => {
    append("`");
  };

  '`' => {
    dstack_open_inline(INLINE_CODE, "<span class=\"inline-code\">");
    fcall inline_code;
  };

  internal_anchor => {
    append("<a id=\"");
    std::string lowercased_tag = std::string(a1, a2 - a1);
    std::transform(lowercased_tag.begin(), lowercased_tag.end(), lowercased_tag.begin(), [](unsigned char c) { return std::tolower(c); });
    append_uri_escaped(lowercased_tag);
    append("\"></a>");
  };

  thumb_id => {
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
  };

  post_id => { append_id_link("post", "post", "/posts/"); };
  post_changes_for_id => { append_id_link("post changes", "post-changes-for", "/post_versions?search[post_id]="); };
  post_flag_id => { append_id_link("flag", "post-flag", "/post_flags/"); };
  note_id => { append_id_link("note", "note", "/notes/"); };
  forum_post_id => { append_id_link("forum", "forum-post", "/forum_posts/"); };
  forum_topic_id => { append_id_link("topic", "forum-topic", "/forum_topics/"); };
  comment_id => { append_id_link("comment", "comment", "/comments/"); };
  pool_id => { append_id_link("pool", "pool", "/pools/"); };
  user_id => { append_id_link("user", "user", "/users/"); };
  artist_id => { append_id_link("artist", "artist", "/artists/"); };
  ban_id => { append_id_link("ban", "ban", "/bans/"); };
  bulk_update_request_id => { append_id_link("BUR", "bulk-update-request", "/bulk_update_requests/"); };
  tag_alias_id => { append_id_link("alias", "tag-alias", "/tag_aliases/"); };
  tag_implication_id => { append_id_link("implication", "tag-implication", "/tag_implications/"); };
  mod_action_id => { append_id_link("mod action", "mod-action", "/mod_actions/"); };
  user_feedback_id => { append_id_link("record", "user-feedback", "/user_feedbacks/"); };
  wiki_page_id => { append_id_link("wiki", "wiki-page", "/wiki_pages/"); };
  set_id => { append_id_link("set", "set", "/post_sets/"); };
  blip_id => { append_id_link("blip", "blip", "/blips/"); };
  ticket_id => { append_id_link("ticket", "ticket", "/tickets/"); };
  takedown_id => { append_id_link("takedown", "takedown", "/takedowns/"); };

  basic_post_search_link => {
    append_post_search_link({ a1, a2 }, { a1, a2 });
  };

  aliased_post_search_link => {
    append_post_search_link({ a1, a2 }, { b1, b2 });
  };

  basic_wiki_link => {
    append_wiki_link({ a1, a2 }, { a1, a2 });
  };

  aliased_wiki_link => {
    append_wiki_link({ a1, a2 }, { b1, b2 });
  };

  basic_textile_link => {
    const char* match_end = b2;
    const char* url_start = b1;
    const char* url_end = find_boundary_c(match_end - 1) + 1;

    append_named_url({ url_start, url_end }, { a1, a2 });

    if (url_end < match_end) {
      append_html_escaped({ url_end, match_end });
    }
  };

  bracketed_textile_link => {
    append_named_url({ b1, b2 }, { a1, a2 });
  };

  url => {
    const char* match_end = te;
    const char* url_start = ts;
    const char* url_end = find_boundary_c(match_end - 1) + 1;

    append_unnamed_url({ url_start, url_end });

    if (url_end < match_end) {
      append_html_escaped({ url_end, match_end });
    }
  };

  delimited_url => {
    append_unnamed_url({ a1, a2 });
  };

  newline list_item => {
    g_debug("inline list");
    fexec ts + 1;
    fret;
  };

  '[b]'i  => { dstack_open_inline(INLINE_B, "<strong>"); };
  '[/b]'i => { dstack_close_inline(INLINE_B, "</strong>"); };
  '[i]'i  => { dstack_open_inline(INLINE_I, "<em>"); };
  '[/i]'i => { dstack_close_inline(INLINE_I, "</em>"); };
  '[s]'i  => { dstack_open_inline(INLINE_S, "<s>"); };
  '[/s]'i => { dstack_close_inline(INLINE_S, "</s>"); };
  '[u]'i  => { dstack_open_inline(INLINE_U, "<u>"); };
  '[/u]'i => { dstack_close_inline(INLINE_U, "</u>"); };
  '[sup]'i  => { dstack_open_inline(INLINE_SUP, "<sup>"); };
  '[/sup]'i => { dstack_close_inline(INLINE_SUP, "</sup>"); };
  '[sub]'i  => { dstack_open_inline(INLINE_SUB, "<sub>"); };
  '[/sub]'i => { dstack_close_inline(INLINE_SUB, "</sub>"); };

  color_typed => {
    if(options.allow_color) {
      dstack_push(INLINE_COLOR);
      append("<span class=\"dtext-color-");
      append_uri_escaped({ a1, a2 });
      append("\">");
    }
    fgoto inline;
  };

  color_open => {
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
    fgoto inline;
  };

  color_close => {
    if(options.allow_color) {
      dstack_close_inline(INLINE_COLOR, "</span>");
    }
    fgoto inline;
  };

  spoilers_open => {
    dstack_open_inline(INLINE_SPOILER, "<span class=\"spoiler\">");
  };

  newline* spoilers_close => {
    g_debug("inline [/spoiler]");
    dstack_close_before_block();

    if (dstack_check(INLINE_SPOILER)) {
      dstack_close_inline(INLINE_SPOILER, "</span>");
    } else if (dstack_close_block(BLOCK_SPOILER, "</div>")) {
      fret;
    }
  };

  # these are block level elements that should kick us out of the inline
  # scanner

  newline header => {
    dstack_close_leaf_blocks();
    fexec ts;
    fret;
  };

  '[table]'i => {
    dstack_close_before_block();
    fexec ts;
    fret;
  };

  '[/table]'i space* => {
    g_debug("inline [/table]");
    dstack_close_before_block();

    if (dstack_check(BLOCK_LI)) {
      dstack_close_list();
    }

    if (dstack_check(BLOCK_TABLE)) {
      dstack_rewind();
      fret;
    } else {
      append_block("[/table]");
    }
  };

  '[code]'i => {
    dstack_close_before_block();
    fexec ts;
    fret;
  };

  '[/code]'i space* => {
    g_debug("inline [/code]");
    dstack_close_before_block();

    if (dstack_check(BLOCK_LI)) {
      dstack_close_list();
    }

    if (dstack_check(BLOCK_CODE)) {
      dstack_rewind();
      fret;
    } else {
      append_block("[/code]");
    }
  };

  quote_open => {
    g_debug("inline [quote]");
    dstack_close_leaf_blocks();
    fexec ts;
    fret;
  };

  newline? quote_close ws* => {
    g_debug("inline [/quote]");
    dstack_close_until(BLOCK_QUOTE);
    fret;
  };

  (section_open | section_open_expanded | section_open_aliased | section_open_aliased_expanded) => {
    g_debug("inline [section]");
    dstack_close_leaf_blocks();
    fexec ts;
    fret;
  };

  newline? section_close ws* => {
    g_debug("inline [/expand]");
    dstack_close_until(BLOCK_SECTION);
    fret;
  };

  '[/th]'i => {
    if (dstack_close_block(BLOCK_TH, "</th>")) {
      fret;
    }
  };

  newline* '[/td]'i => {
    if (dstack_close_block(BLOCK_TD, "</td>")) {
      fret;
    }
  };

  newline{2,} => {
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    fexec ts;
    fret;
  };

  newline => {
    g_debug("inline newline");

    if (header_mode) {
      dstack_close_leaf_blocks();
      fret;
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_list();
      fret;
    } else {
      append("<br>");
    }
  };

  '\r' => {
    append(' ');
  };

  any => {
    g_debug("inline char: %c", fc);
    append_html_escaped(fc);
  };
*|;

inline_code := |*
  '\\`' => {
    append("`");
  };

  '`' => {
    dstack_close_inline(INLINE_CODE, "</span>");
    fret;
  };

  any => {
    append_html_escaped(fc);
  };
*|;

code := |*
  '[/code]'i => {
    if (dstack_check(BLOCK_CODE)) {
      dstack_rewind();
    } else {
      append("[/code]");
    }
    fret;
  };

  any => {
    append_html_escaped(fc);
  };
*|;

table := |*
  '[thead]'i => {
    dstack_open_block(BLOCK_THEAD, "<thead>");
  };

  '[/thead]'i => {
    dstack_close_block(BLOCK_THEAD, "</thead>");
  };

  '[tbody]'i => {
    dstack_open_block(BLOCK_TBODY, "<tbody>");
  };

  '[/tbody]'i => {
    dstack_close_block(BLOCK_TBODY, "</tbody>");
  };

  '[th]'i => {
    dstack_open_block(BLOCK_TH, "<th>");
    fcall inline;
  };

  '[tr]'i => {
    dstack_open_block(BLOCK_TR, "<tr>");
  };

  '[/tr]'i => {
    dstack_close_block(BLOCK_TR, "</tr>");
  };

  '[td]'i => {
    dstack_open_block(BLOCK_TD, "<td>");
    fcall inline;
  };

  '[/table]'i => {
    if (dstack_close_block(BLOCK_TABLE, "</table>")) {
      fret;
    }
  };

  any;
*|;

main := |*
  header => {
    static element_t blocks[] = { BLOCK_H1, BLOCK_H2, BLOCK_H3, BLOCK_H4, BLOCK_H5, BLOCK_H6 };
    char header = *a1;
    element_t block = blocks[header - '1'];

    dstack_open_block(block, "<h");
    append_block(header);
    append_block(">");

    header_mode = true;
    fcall inline;
  };

  quote_open space* => {
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote>");
  };

  spoilers_open space* => {
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_SPOILER, "<div class=\"spoiler\">");
  };

  spoilers_close => {
    g_debug("block [/spoiler]");
    dstack_close_before_block();
    if (dstack_check( BLOCK_SPOILER)) {
      g_debug("  rewind");
      dstack_rewind();
    }
  };

  '[code]'i space* => {
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_CODE, "<pre>");
    fcall code;
  };

  section_open space* => {
    append_section({}, false);
  };

  section_open_expanded space* => {
    append_section({}, true);
  };

  section_open_aliased space* => {
    g_debug("block [section=]");
    append_section({ a1, a2 }, false);
  };

  section_open_aliased_expanded space* => {
    g_debug("block expanded [section=]");
    append_section({ a1, a2 }, true);
  };

  '[table]'i => {
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_TABLE, "<table class=\"striped\">");
    fcall table;
  };

  list_item => {
    g_debug("block list");
    dstack_open_list(a2 - a1);
    fexec b1;
    fcall inline;
  };

  newline{2,} => {
    g_debug("block newline2");

    if (header_mode) {
      dstack_close_leaf_blocks();
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_until(BLOCK_UL);
    } else {
      dstack_close_before_block();
    }
  };

  newline => {
    g_debug("block newline");
  };

  any => {
    g_debug("block char: %c", fc);
    fhold;

    if (dstack.empty() || dstack_check(BLOCK_QUOTE) || dstack_check(BLOCK_SPOILER) || dstack_check(BLOCK_SECTION)) {
      dstack_open_block(BLOCK_P, "<p>");
    }

    fcall inline;
  };
*|;

}%%

%% write data;

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

  %% write init nocs;
  %% write exec;

  sm->dstack_close_all();

  return DTextResult { sm->output, sm->posts };
}
