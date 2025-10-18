require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for search links, like {{tag}}.
class DTextSearchLinkTest < Minitest::Test
  include DTextTestHelper

  def test_inline_tags
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=tag">tag</a></p>', "{{tag}}")
  end

  def test_inline_tags_conjunction
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=tag1%20tag2">tag1 tag2</a></p>', "{{tag1 tag2}}")
  end

  def test_inline_tags_special_entities
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=%3C3">&lt;3</a></p>', "{{<3}}")
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=%20%22%23%26%2B%3C%3E%3F"> &quot;#&amp;+&lt;&gt;?</a></p>', '{{ "#&+<>?}}')
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=%E6%9D%B1%E6%96%B9">東方</a></p>', "{{東方}}")
  end

  def test_inline_tags_lowercase_utf8
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=n%20%CE%A9%20p">n Ω P</a></p>', "{{n Ω P}}")
  end

  def test_inline_tags_aliased
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=fox%20smile">Best Search</a></p>', "{{fox smile|Best Search}}")
  end
end