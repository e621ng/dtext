require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for lists
class DTextListTest < Minitest::Test
  include DTextTestHelper

  def test_lists
    assert_parse('<ul><li>a</li></ul>', '* a')
    assert_parse('<ul><li>a</li><li>b</li></ul>', "* a\n* b")
    assert_parse('<ul><li>a</li><li>b</li><li>c</li></ul>', "* a\n* b\n* c")
    assert_parse('<ul><li>a</li></ul><p> </p>', "* a\n ") # XXX should strip space

    assert_parse('<ul><li>a</li><li>b</li></ul>', "* a\r\n* b")
    assert_parse('<ul><li>a</li></ul><ul><li>b</li></ul>', "* a\n\n* b")
    assert_parse('<ul><li>a</li><li>b</li><li>c</li></ul>', "* a\r\n* b\r\n* c")

    assert_parse('<ul><li>a</li><ul><li>b</li></ul></ul>', "* a\n** b")
    assert_parse('<ul><li>a</li><ul><li>b</li><ul><li>c</li></ul></ul></ul>', "* a\n** b\n*** c")
    # assert_parse('<ul><ul><ul><li>a</li></ul><li>b</li></ul><li>c</li></ul>', "*** a\n**\n b\n* c")
    assert_parse('<ul><ul><ul><li>a</li></ul></ul><li>b</li></ul>', "*** a\n* b")
    assert_parse('<ul><ul><ul><li>a</li></ul></ul></ul>', "*** a")

    assert_parse('<ul><li>a</li></ul><p>b</p><ul><li>c</li></ul>', "* a\nb\n* c")

    assert_parse('<p>a<br>b</p><ul><li>c</li><li>d</li></ul>', "a\nb\n* c\n* d")
    assert_parse('<p>a</p><ul><li>b</li></ul><p>c</p><ul><li>d</li></ul><p>e</p><p>another one</p>', "a\n* b\nc\n* d\ne\n\nanother one")
    assert_parse('<p>a</p><ul><li>b</li></ul><p>c</p><ul><ul><li>d</li></ul></ul><p>e</p><p>another one</p>', "a\n* b\nc\n** d\ne\n\nanother one")

    assert_parse('<ul><li><a class="dtext-link dtext-id-link dtext-post-id-link" href="/posts/1">post #1</a></li></ul>', "* post #1")

    assert_parse('<ul><li><em>a</em></li><li>b</li></ul>', "* [i]a[/i]\n* b")
    assert_parse('<ul><li><em>a</em></li><li>b</li></ul>', "* [i]a\n* b")
    assert_parse('<p><em>a</em></p><ul><li>b</li><li>c</li></ul>', "[i]a\n* b\n* c")

    # assert_parse('<ul><li></li></ul><h4>See also</h4><ul><li>a</li></ul>', "* h4. See also\n* a")
    assert_parse('<ul><li>h4. See also</li><li>a</li></ul>', "* h4. See also\n* a") # XXX wrong?

    assert_parse('<ul><li>a</li></ul><h4>See also</h4>', "* a\nh4. See also")
    assert_parse('<h4><em>See also</em></h4><ul><li>a</li></ul>', "h4. [i]See also\n* a")
    assert_parse('<ul><li><em>a</em></li></ul><h4>See also</h4>', "* [i]a\nh4. See also")

    assert_parse('<h4>See also</h4><ul><li>a</li></ul>', "h4. See also\n* a")
    assert_parse('<h4>See also</h4><ul><li>a</li><li>h4. External links</li></ul>', "h4. See also\n* a\n* h4. External links")

    # assert_parse('<p>a</p><div class="spoiler"><ul><li>b</li><li>c</li></ul></div><p>d</p>', "a\n[spoilers]\n* b\n* c\n[/spoilers]\nd")

    assert_parse('<p>a</p><blockquote><ul><li>b</li><li>c</li></ul></blockquote><p>d</p>', "a\n[quote]\n* b\n* c\n[/quote]\nd")
    assert_parse('<p>a</p><details><summary></summary><div><ul><li>b</li><li>c</li></ul></div></details><p>d</p>', "a\n[section]\n* b\n* c\n[/section]\nd")

    assert_parse('<p>a</p><blockquote><ul><li>b</li><li>c</li></ul><p>d</p></blockquote>', "a\n[quote]\n* b\n* c\n\nd")
    assert_parse('<p>a</p><details><summary></summary><div><ul><li>b</li><li>c</li></ul><p>d</p></div></details>', "a\n[section]\n* b\n* c\n\nd")

    assert_parse('<p>*</p>', "*")
    assert_parse('<p>*a</p>', "*a")
    assert_parse('<p>***</p>', "***")
    assert_parse('<p>*<br>*<br>*</p>', "*\n*\n*")
    assert_parse('<p>* <br>blah</p>', "* \r\nblah")
  end
end
