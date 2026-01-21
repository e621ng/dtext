#ifndef DTEXT_H
#define DTEXT_H

#include <string>
#include <vector>
#include <stdexcept>

typedef enum element_t {
  DSTACK_EMPTY = 0,
  BLOCK_P,
  BLOCK_QUOTE,
  BLOCK_SECTION,
  BLOCK_SPOILER,
  BLOCK_CODE,
  BLOCK_TABLE,
  BLOCK_THEAD,
  BLOCK_TBODY,
  BLOCK_UL,
  BLOCK_LI,
  BLOCK_TR,
  BLOCK_TH,
  BLOCK_TD,
  BLOCK_H1,
  BLOCK_H2,
  BLOCK_H3,
  BLOCK_H4,
  BLOCK_H5,
  BLOCK_H6,
  INLINE_B,
  INLINE_I,
  INLINE_U,
  INLINE_S,
  INLINE_SUP,
  INLINE_SUB,
  INLINE_COLOR,
  INLINE_SPOILER,
  INLINE_CODE,
} element_t;

class DTextError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct DTextOptions {
  bool f_inline = false;
  bool allow_color = false;
  int max_thumbs = 25;
  std::string base_url;
};

struct DTextResult {
  std::string dtext;
  std::vector<long> posts;
};

class StateMachine {
public:
  static DTextResult parse_dtext(const std::string_view dtext, DTextOptions options);
  static std::string parse_basic_inline(const std::string_view dtext);

private:
  StateMachine(const std::string_view dtext, int initial_state, const DTextOptions options);
  DTextResult parse();

  inline void append(const std::string_view c);
  inline void append(const char c);
  inline void append_block(const std::string_view s);
  inline void append_block(const char s);
  inline void append_html_escaped(char s);
  inline void append_html_escaped(const std::string_view input);
  inline void append_uri_escaped(const std::string_view uri_part, const char whitelist = '-');
  inline void append_url(const char* url);
  inline void append_unnamed_url(const std::string_view url);
  inline void append_named_url(const std::string_view url, const std::string_view title);
  inline void append_post_search_link(const std::string_view tag, const std::string_view title);
  inline void append_wiki_link(const std::string_view tag, const std::string_view title);
  inline void append_id_link(const char * title, const char * id_name, const char * url);
  inline void append_section(const std::string_view summary, bool initially_open);
  inline void append_closing_p();

  inline void dstack_close_leaf_blocks();
  inline void dstack_close_until(element_t element);
  inline void dstack_close_all();
  inline void dstack_close_list();
  inline void dstack_close_inline(element_t type, const char * close_html);
  inline bool dstack_close_block(element_t type, const char * close_html);
  inline void dstack_close_before_block();

  inline void dstack_open_block(element_t type, const char * html);
  inline void dstack_open_list(int depth);
  inline void dstack_open_inline(element_t type, const char * html);
  inline bool dstack_is_open(element_t element);
  inline void dstack_push(element_t element);
  inline bool dstack_check(element_t expected_element);
  inline void dstack_rewind();
  inline int dstack_count(element_t element);
  inline element_t dstack_peek();
  inline element_t dstack_pop();

  DTextOptions options;

  size_t top;
  int cs;
  int act = 0;
  const char * p = NULL;
  const char * pb = NULL;
  const char * pe = NULL;
  const char * eof = NULL;
  const char * ts = NULL;
  const char * te = NULL;
  const char * a1 = NULL;
  const char * a2 = NULL;
  const char * b1 = NULL;
  const char * b2 = NULL;

  bool header_mode = false;
  int ignored_sup_sub_tags = 0;

  std::vector<long> posts;
  std::string output;
  std::vector<int> stack;
  std::vector<element_t> dstack;
};

#endif
