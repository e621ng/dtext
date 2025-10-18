require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for spoilers, like [spoiler]this is a spoiler[/spoiler].
class DTextSpoilerTest < Minitest::Test
  include DTextTestHelper

  def test_spoilers
    assert_parse("<p>this is <span class=\"spoiler\">an inline spoiler</span>.</p>", "this is [spoiler]an inline spoiler[/spoiler].")
    assert_parse("<p>this is <span class=\"spoiler\">an inline spoiler</span>.</p>", "this is [SPOILERS]an inline spoiler[/SPOILERS].")
    assert_parse("<p>this is</p><div class=\"spoiler\"><p>a block spoiler</p></div><p>.</p>", "this is\n\n[spoiler]\na block spoiler\n[/spoiler].")
    assert_parse("<p>this is</p><div class=\"spoiler\"><p>a block spoiler</p></div><p>.</p>", "this is\n\n[SPOILERS]\na block spoiler\n[/SPOILERS].")
    assert_parse("<div class=\"spoiler\"><p>this is a spoiler with no closing tag</p><p>new text</p></div>", "[spoiler]this is a spoiler with no closing tag\n\nnew text")
    assert_parse("<div class=\"spoiler\"><p>this is a spoiler with no closing tag<br>new text</p></div>", "[spoiler]this is a spoiler with no closing tag\nnew text")
    assert_parse("<div class=\"spoiler\"><p>this is a block spoiler with no closing tag</p></div>", "[spoiler]\nthis is a block spoiler with no closing tag")
    assert_parse("<div class=\"spoiler\"><p>this is <span class=\"spoiler\">a nested</span> spoiler</p></div>", "[spoiler]this is [spoiler]a nested[/spoiler] spoiler[/spoiler]")

    # assert_parse('<div class="spoiler"><h4>Blah</h4></div>', "[spoiler]\nh4. Blah\n[/spoiler]")
    assert_parse(%{<div class="spoiler"><h4>Blah\n[/spoiler]</h4></div>}, "[spoiler]\nh4. Blah\n[/spoiler]") # XXX wrong

    # assert_parse('<p>First sentence</p><p>[/spoiler] Second sentence.</p>', "First sentence\n\n[/spoiler] Second sentence.")
    assert_parse("<p>First sentence</p>\n\n[/spoiler] Second sentence.", "First sentence\n\n[/spoiler] Second sentence.") # XXX wrong

    assert_parse('<p>inline <em>foo</em></p><div class="spoiler"><p>blah blah</p></div>', "inline [i]foo\n\n[spoiler]blah blah[/spoiler]")
    assert_parse('<p>inline <span class="spoiler"> foo</span></p><div class="spoiler"><p>blah blah</p></div>', "inline [spoiler] foo\n\n[spoiler]blah blah[/spoiler]")
  end
end
