require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for thumbnails, like thumb #123
class DTextThumbnailTest < Minitest::Test
  include DTextTestHelper

  def test_thumbnails
    parsed = DText.parse("thumb #123")
    assert_equal(parsed[1], [123])
    assert_equal(parsed[0], "<p><a class=\"dtext-link dtext-id-link dtext-post-id-link thumb-placeholder-link\" data-id=\"123\" href=\"/posts/123\">post #123</a></p>")
    parsed = DText.parse("thumb #123 "*10, max_thumbs: 5)
    assert_equal(parsed[1], [123]*5)
  end

  def test_thumbails_max_count
    assert_parse('<p><a class="dtext-link dtext-id-link dtext-post-id-link" href="/posts/1">post #1</a></p>', "thumb #1", max_thumbs: 0);
  end
end
