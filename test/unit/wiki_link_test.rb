require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for wiki links, like [[wiki_page]] and [[wiki_page|Some Text]].
class DTextWikiLinkTest < Minitest::Test
  include DTextTestHelper

  def test_wiki_links
    assert_parse("<p>a <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=b\">b</a> c</p>", "a [[b]] c")
  end

  def test_wiki_links_aliased
    assert_parse("<p><a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=wiki_page\">Some Text</a></p>", "[[wiki page|Some Text]]")
  end

  def test_wiki_links_spoiler
    assert_parse("<p>a <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=spoiler\">spoiler</a> c</p>", "a [[spoiler]] c")
  end

  def test_wiki_links_utf8
    assert_parse("<p><a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=pok%C3%A9mon\">pokémon</a></p>", "[[pokémon]]")
  end

  def test_wiki_links_lowercase_utf8
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-wiki-link" href="/wiki_pages/show_or_new?title=%C5%8Ckami">ŌkamI</a></p>', "[[ŌkamI]]")
  end

  def test_wiki_links_edge
    assert_parse("<p>[[|_|]]</p>", "[[|_|]]")
    assert_parse("<p>[[||_||]]</p>", "[[||_||]]")
  end

  def test_wiki_links_nested_b
    assert_parse("<p><strong>[[</strong>tag<strong>]]</strong></p>", "[b][[[/b]tag[b]]][/b]")
  end
  def test_complex_links_1
    assert_parse("<p><a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=1\">2 3</a> | <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=4\">5 6</a></p>", "[[1|2 3]] | [[4|5 6]]")
  end

  def test_complex_links_2
    assert_parse("<p>Tags <strong>(<a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=howto%3Atag\">Tagging Guidelines</a> | <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=howto%3Atag_checklist\">Tag Checklist</a> | <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=tag_groups\">Tag Groups</a>)</strong></p>", "Tags [b]([[howto:tag|Tagging Guidelines]] | [[howto:tag_checklist|Tag Checklist]] | [[Tag Groups]])[/b]")
  end

  def test_anchored_wiki_link
    assert_parse("<p><a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=avoid_posting#a\">ABC 123</a></p>", "[[avoid_posting#A|ABC 123]]")
  end

  def test_wiki_link_xss
    e = assert_raises(DText::Error) do
      DText.parse("[[\xFA<script \xFA>alert(42); //\xFA</script \xFA>]]")
    end
    assert_equal("invalid byte sequence in UTF-8", e.message)
  end
end