require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for headers, like h1. Header levels 1-6 are supported.
class DTextHeaderTest < Minitest::Test
  include DTextTestHelper

  def test_headers
    assert_parse("<h1>header</h1>", "h1. header")
    assert_parse("<ul><li>a</li></ul><h1>header</h1><ul><li>list</li></ul>", "* a\n\nh1. header\n* list")
    assert_parse("<p>blah h1. blah</p>", "blah h1. blah")

    assert_parse('<p>text</p><h1>header</h1>', "text\nh1. header")
    assert_parse('<p><em>text</em></p><h1>header</h1>', "[i]text\nh1. header")
    assert_parse('<div class="spoiler"><p>text</p><h1>header</h1></div>', "[spoiler]text\nh1. header")
    assert_parse('<h1>header</h1><p>text</p>', "h1. header\ntext")
    assert_parse('<ul><li>one</li></ul><h1>header</h1>', "* one\nh1. header")
    assert_parse('<ul><li>one</li></ul><h1>header</h1><ul><li>two</li></ul>', "* one\nh1. header\n* two")
    assert_parse('<h1>header</h1><h2>header</h2>', "h1. header\nh2. header")

    assert_parse('<h1><em>header</em></h1><p>blah</p>', "h1. [i]header\nblah")
    assert_parse('<h1><span class="spoiler">header</span></h1><p>blah</p>', "h1. [spoiler]header\nblah")
    assert_parse('<h1><a rel="nofollow" class="dtext-link" href="http://example.com">http://example.com</a></h1><p>blah</p>', %{h1. http://example.com\nblah})
    assert_parse('<h1><a rel="nofollow" class="dtext-link dtext-external-link" href="http://example.com">example</a></h1><p>blah</p>', %{h1. "example":http://example.com\nblah})

    assert_parse('<blockquote><blockquote><h1>header</h1></blockquote></blockquote>', %{[quote]\n\n[quote]\n\nh1. header\n[/quote]\n\n[/quote]})

    assert_parse('<blockquote><h1>header</h1></blockquote><p>one<br>two</p>', %{[quote]\nh1. header\n[/quote]\none\ntwo})
  end
end
