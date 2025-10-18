require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for quote blocks like [quote]...[/quote].
class DTextQuoteTest < Minitest::Test
  include DTextTestHelper

  def test_quote_blocks
    assert_parse('<blockquote><p>test</p></blockquote>', "[quote]\ntest\n[/quote]")

    assert_parse('<blockquote><p>test</p></blockquote>', "[quote]\ntest\n[/quote] ")
    assert_parse('<blockquote><p>test</p></blockquote><p>blah</p>', "[quote]\ntest\n[/quote] blah")
    assert_parse('<blockquote><p>test</p></blockquote><p>blah</p>', "[quote]\ntest\n[/quote] \nblah")
    assert_parse('<blockquote><p>test</p></blockquote><p>blah</p>', "[quote]\ntest\n[/quote]\nblah")
    assert_parse('<blockquote><p>test</p></blockquote><p> blah</p>', "[quote]\ntest\n[/quote]\n blah") # XXX should ignore space

    assert_parse('<p>test<br>[/quote] blah</p>', "test\n[/quote] blah")
    assert_parse('<p>test<br>[/quote]</p><ul><li>blah</li></ul>', "test\n[/quote]\n* blah")

    assert_parse('<blockquote><p>test</p></blockquote><h4>See also</h4>', "[quote]\ntest\n[/quote]\nh4. See also")
    assert_parse('<blockquote><p>test</p></blockquote><div class="spoiler"><p>blah</p></div>', "[quote]\ntest\n[/quote]\n[spoiler]blah[/spoiler]")

    assert_parse("<p>inline </p><blockquote><p>blah blah</p></blockquote>", "inline [quote]blah blah[/quote]")
    assert_parse("<p>inline <em>foo </em></p><blockquote><p>blah blah</p></blockquote>", "inline [i]foo [quote]blah blah[/quote]")
    assert_parse('<p>inline <span class="spoiler">foo </span></p><blockquote><p>blah blah</p></blockquote>', "inline [spoiler]foo [quote]blah blah[/quote]")

    assert_parse("<p>inline <em>foo</em></p><blockquote><p>blah blah</p></blockquote>", "inline [i]foo\n\n[quote]blah blah[/quote]")
    assert_parse('<p>inline <span class="spoiler">foo </span></p><blockquote><p>blah blah</p></blockquote>', "inline [spoiler]\n\nfoo [quote]blah blah[/quote]")
  end

  def test_quote_blocks_with_list
    assert_parse("<blockquote><ul><li>hello</li><li>there</li></ul></blockquote><p>abc</p>", "[quote]\n* hello\n* there\n[/quote]\nabc")
    assert_parse("<blockquote><ul><li>hello</li><li>there</li></ul></blockquote><p>abc</p>", "[quote]\n* hello\n* there\n\n[/quote]\nabc")
  end

  def test_quote_with_unclosed_tags
    assert_parse('<blockquote><p><strong>foo</strong></p></blockquote>', "[quote][b]foo[/quote]")
    assert_parse('<blockquote><blockquote><p>foo</p></blockquote></blockquote>', "[quote][quote]foo[/quote]")
    assert_parse('<blockquote><div class="spoiler"><p>foo</p></div></blockquote>', "[quote][spoiler]foo[/quote]")
    # code-related case moved to code_test.rb
    assert_parse('<blockquote><details><summary></summary><div><p>foo</p></div></details></blockquote>', "[quote][section]foo[/quote]")
    assert_parse('<blockquote><table class="striped"><td>foo</td></table></blockquote>', "[quote][table][td]foo[/quote]")
    assert_parse('<blockquote><ul><li>foo</li></ul></blockquote>', "[quote]* foo[/quote]")
    assert_parse('<blockquote><h1>foo</h1></blockquote>', "[quote]h1. foo[/quote]")
  end

  def test_quote_blocks_nested
    assert_parse("<blockquote><p>a</p><blockquote><p>b</p></blockquote><p>c</p></blockquote>", "[quote]\na\n[quote]\nb\n[/quote]\nc\n[/quote]")
  end

  def test_quote_blocks_nested_spoiler
    assert_parse("<blockquote><p>a<br><span class=\"spoiler\">blah</span><br>c</p></blockquote>", "[quote]\na\n[spoiler]blah[/spoiler]\nc[/quote]")
    assert_parse("<blockquote><p>a</p><div class=\"spoiler\"><p>blah</p></div><p>c</p></blockquote>", "[quote]\na\n\n[spoiler]blah[/spoiler]\n\nc[/quote]")
    assert_parse('<details><summary></summary><div><div class="spoiler"><ul><li>blah</li></ul></div></div></details>', "[section]\n[spoiler]\n* blah\n[/spoiler]\n[/section]")

    assert_parse('<details><summary></summary><div><div class="spoiler"><ul><li>blah</li></ul></div></div></details>', "[section]\n[spoiler]\n* blah\n[/spoiler]\n[/section]")
  end

  def test_quote_blocks_nested_expand
    assert_parse("<blockquote><p>a</p><details><summary></summary><div><p>b</p></div></details><p>c</p></blockquote>", "[quote]\na\n[section]\nb\n[/section]\nc\n[/quote]")
  end
end
