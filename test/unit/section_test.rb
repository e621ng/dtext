require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for [section]...[/section] blocks.
class DTextSectionTest < Minitest::Test
  include DTextTestHelper

  def test_section_expand
    assert_parse("<details><summary></summary><div><p>hello world</p></div></details>", "[section]hello world[/section]")

    assert_parse("<p>inline <em>foo </em></p><details><summary></summary><div><p>blah blah</p></div></details>", "inline [i]foo [section]blah blah[/section]")
    assert_parse('<p>inline <span class="spoiler">foo </span></p><details><summary></summary><div><p>blah blah</p></div></details>', "inline [spoiler]foo [section]blah blah[/section]")

    assert_parse("<p>inline <em>foo</em></p><details><summary></summary><div><p>blah blah</p></div></details>", "inline [i]foo\n\n[section]blah blah[/section]")
    assert_parse('<p>inline <span class="spoiler">foo</span></p><details><summary></summary><div><p>blah blah</p></div></details>', "inline [spoiler]foo\n\n[section]blah blah[/section]")

    assert_parse("<p>inline </p><details><summary></summary><div><p>blah blah</p></div></details>", "inline [section]blah blah[/section]")

    assert_parse('<details><summary></summary><div><p>test</p></div></details>', "[section]\ntest\n[/section] ")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><p>blah</p>', "[section]\ntest\n[/section] blah")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><p>blah</p>', "[section]\ntest\n[/section] \nblah")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><p>blah</p>', "[section]\ntest\n[/section]\nblah")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><p> blah</p>', "[section]\ntest\n[/section]\n blah") # XXX should ignore space

    assert_parse("<p>[/section]</p>", "[/section]")
    assert_parse("<p>foo [/section] bar</p>", "foo [/section] bar")
    assert_parse('<p>test<br>[/section] blah</p>', "test\n[/section] blah")
    assert_parse('<p>test<br>[/section]</p><ul><li>blah</li></ul>', "test\n[/section]\n* blah")

    assert_parse('<details><summary></summary><div><p>test</p></div></details><h4>See also</h4>', "[section]\ntest\n[/section]\nh4. See also")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><div class="spoiler"><p>blah</p></div>', "[section]\ntest\n[/section]\n[spoiler]blah[/spoiler]")
  end

  def test_section_expand_missing_close
    assert_parse("<details><summary></summary><div><p>a</p></div></details>", "[section]a")
  end

  def test_section_aliased_expand
    assert_parse("<details><summary>hello</summary><div><p>blah blah</p></div></details>", "[section=hello]blah blah[/section]")

    assert_parse("<p>inline <em>foo </em></p><details><summary>title</summary><div><p>blah blah</p></div></details>", "inline [i]foo [section=title]blah blah[/section]")
    assert_parse('<p>inline <span class="spoiler">foo </span></p><details><summary>title</summary><div><p>blah blah</p></div></details>', "inline [spoiler]foo [section=title]blah blah[/section]")
  end

  def test_section_expand_with_nested_list
    assert_parse("<details><summary></summary><div><ul><li>a</li><li>b</li></ul></div></details><p>c</p>", "[section]\n* a\n* b\n[/section]\nc")
  end

end
