require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for ID links like "post #1234" and "user #5678"
class DTextIDLinkTest < Minitest::Test
  include DTextTestHelper

  def assert_parse_id_link(class_name, url, input, display: input)
    assert_parse(%{<p><a class="dtext-link dtext-id-link #{class_name}" href="#{url}">#{display}</a></p>}, input)
  end

  def test_id_links
    assert_parse_id_link("dtext-post-id-link", "/posts/1234", "post #1234")
    assert_parse_id_link("dtext-post-changes-for-id-link", "/post_versions?search[post_id]=1234", "post changes #1234")
    assert_parse_id_link("dtext-post-flag-id-link", "/post_flags/1234", "flag #1234")
    assert_parse_id_link("dtext-note-id-link", "/notes/1234", "note #1234")
    assert_parse_id_link("dtext-forum-post-id-link", "/forum_posts/1234", "forum #1234")
    assert_parse_id_link("dtext-forum-topic-id-link", "/forum_topics/1234", "topic #1234")
    assert_parse_id_link("dtext-comment-id-link", "/comments/1234", "comment #1234")
    assert_parse_id_link("dtext-pool-id-link", "/pools/1234", "pool #1234")
    assert_parse_id_link("dtext-user-id-link", "/users/1234", "user #1234")
    assert_parse_id_link("dtext-artist-id-link", "/artists/1234", "artist #1234")
    assert_parse_id_link("dtext-ban-id-link", "/bans/1234", "ban #1234")
    assert_parse_id_link("dtext-tag-alias-id-link", "/tag_aliases/1234", "alias #1234")
    assert_parse_id_link("dtext-tag-implication-id-link", "/tag_implications/1234", "implication #1234")
    assert_parse_id_link("dtext-mod-action-id-link", "/mod_actions/1234", "mod action #1234")
    assert_parse_id_link("dtext-user-feedback-id-link", "/user_feedbacks/1234", "record #1234")
    assert_parse_id_link("dtext-blip-id-link", "/blips/1234", "blip #1234")
    assert_parse_id_link("dtext-set-id-link", "/post_sets/1234", "set #1234")
    assert_parse_id_link("dtext-takedown-id-link", "/takedowns/1234", "takedown #1234")
    assert_parse_id_link("dtext-takedown-id-link", "/takedowns/1234", "take down #1234", display: "takedown #1234")
    assert_parse_id_link("dtext-takedown-id-link", "/takedowns/1234", "takedown request #1234", display: "takedown #1234")
    assert_parse_id_link("dtext-takedown-id-link", "/takedowns/1234", "take down request #1234", display: "takedown #1234")
    assert_parse_id_link("dtext-ticket-id-link", "/tickets/1234", "ticket #1234")
    assert_parse_id_link("dtext-wiki-page-id-link", "/wiki_pages/1234", "wiki #1234")
  end

  def test_note_id_link
    assert_parse('<p><a class="dtext-link dtext-id-link dtext-note-id-link" href="/notes/1234">note #1234</a></p>', "note #1234")
  end
end