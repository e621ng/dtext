require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for url links, like "link to site":https://example.com/info#details and "another link":/relative/path, http://example.com/page, and anchor links.
class DTextURLLinkTest < Minitest::Test
  include DTextTestHelper

  def test_relative_urls
    assert_parse('<p><a class="dtext-link dtext-id-link dtext-post-id-link" href="http://danbooru.donmai.us/posts/1234">post #1234</a></p>', "post #1234", base_url: "http://danbooru.donmai.us")
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="http://danbooru.donmai.us/posts">posts</a></p>', '"posts":/posts', base_url: "http://danbooru.donmai.us")
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="https://example.com/posts">posts</a></p>', '"posts":https://example.com/posts', base_url: "https://e621.net")
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-wiki-link" href="https://e621.net/wiki_pages/show_or_new?title=abc">abc</a></p>', '[[abc]]', base_url: "https://e621.net")
  end

  def test_utf8_links
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="/posts?tags=approver:葉月">7893</a></p>', '"7893":/posts?tags=approver:葉月')
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="/posts?tags=approver:葉月">7893</a></p>', '"7893":[/posts?tags=approver:葉月]')
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="http://danbooru.donmai.us/posts?tags=approver:葉月">http://danbooru.donmai.us/posts?tags=approver:葉月</a></p>', 'http://danbooru.donmai.us/posts?tags=approver:葉月')
  end

  def test_new_style_link_boundaries
    assert_parse('<p>a 「<a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">title</a>」 b</p>', 'a 「"title":[http://test.com]」 b')
  end

  def test_old_style_link_boundaries
    assert_parse('<p>a 「<a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">title</a>」 b</p>', 'a 「"title":http://test.com」 b')
  end

  def test_urls
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a> b</p>', 'a http://test.com b')
  end

  def test_urls_case_insensitive
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="Https://test.com">Https://test.com</a> b</p>', 'a Https://test.com b')
  end

  def test_urls_with_newline
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a><br>b</p>', "http://test.com\nb")
  end

  def test_urls_with_paths
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com/~bob/image.jpg">http://test.com/~bob/image.jpg</a> b</p>', 'a http://test.com/~bob/image.jpg b')
  end

  def test_urls_with_fragment
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com/home.html#toc">http://test.com/home.html#toc</a> b</p>', 'a http://test.com/home.html#toc b')
  end

  def test_urls_with_params
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="https://test.com/?a=b&amp;c=d#abc">https://test.com/?a=b&amp;c=d#abc</a></p>', "https://test.com/?a=b&c=d#abc")
  end

  def test_auto_urls
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>. b</p>', 'a http://test.com. b')
  end

  def test_auto_urls_in_parentheses
    assert_parse('<p>a (<a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>) b</p>', 'a (http://test.com) b')
  end

  def test_old_style_links
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">test</a></p>', '"test":http://test.com')
  end

  def test_old_style_links_with_inline_tags
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com"><em>test</em></a></p>', '"[i]test[/i]":http://test.com')
  end

  def test_old_style_links_with_nested_links
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">post #1</a></p>', '"post #1":http://test.com')
  end

  def test_old_style_links_with_special_entities
    assert_parse('<p>&quot;1&quot; <a rel="nofollow" class="dtext-link dtext-external-link" href="http://three.com">2 &amp; 3</a></p>', '"1" "2 & 3":http://three.com')
  end

  def test_new_style_links
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">test</a></p>', '"test":[http://test.com]')
  end

  def test_new_style_links_with_inline_tags
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com/(parentheses)"><em>test</em></a></p>', '"[i]test[/i]":[http://test.com/(parentheses)]')
  end

  def test_new_style_links_with_nested_links
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">post #1</a></p>', '"post #1":[http://test.com]')
  end

  def test_new_style_links_with_parentheses
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com/(parentheses)">test</a></p>', '"test":[http://test.com/(parentheses)]')
    assert_parse('<p>(<a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com/(parentheses)">test</a>)</p>', '("test":[http://test.com/(parentheses)])')
    assert_parse('<p>[<a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com/(parentheses)">test</a>]</p>', '["test":[http://test.com/(parentheses)]]')
  end

  def test_fragment_only_urls
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="#toc">test</a></p>', '"test":#toc')
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="#toc">test</a></p>', '"test":[#toc]')
  end

  def test_auto_url_boundaries
    assert_parse('<p>a （<a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>） b</p>', 'a （http://test.com） b')
    assert_parse('<p>a 〜<a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>〜 b</p>', 'a 〜http://test.com〜 b')
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>　 b</p>', 'a http://test.com　 b')
  end

  def test_delimited_links
    dtext = '(blah <https://en.wikipedia.org/wiki/Orange_(fruit)>).'
    html = '<p>(blah <a rel="nofollow" class="dtext-link" href="https://en.wikipedia.org/wiki/Orange_(fruit)">https://en.wikipedia.org/wiki/Orange_(fruit)</a>).</p>'
    assert_parse(html, dtext)
  end

  def test_internal_anchor
    assert_parse("<p><a id=\"b\"></a></p>", "[#B]")

    assert_parse("<p>[#test.abc]</p>", "[#test.abc]")
  end
end