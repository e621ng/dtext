require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for tables, like [table][tr][td]text[/td][/tr][/table]
class DTextTableTest < Minitest::Test
  include DTextTestHelper

  def test_table
    assert_parse("<table class=\"striped\"><tr><td>text</td></tr></table>", "[table][tr][td]text[/td][/tr][/table]")

    assert_parse("<table class=\"striped\"><thead><tr><th>header</th></tr></thead><tbody><tr><td><a class=\"dtext-link dtext-id-link dtext-post-id-link\" href=\"/posts/100\">post #100</a></td></tr></tbody></table>", "[table][thead][tr][th]header[/th][/tr][/thead][tbody][tr][td]post #100[/td][/tr][/tbody][/table]")
    assert_parse("<table class=\"striped\"><thead><tr><th>header</th></tr></thead><tbody><tr><td><a class=\"dtext-link dtext-id-link dtext-post-id-link\" href=\"/posts/100\">post #100</a></td></tr></tbody></table>", "[table]\n[thead]\n[tr]\n[th]header[/th][/tr][/thead][tbody][tr][td]post #100[/td][/tr][/tbody][/table]")

    assert_parse('<p>inline</p><table class="striped"><tr><td>text</td></tr></table>', "inline\n\n[table][tr][td]text[/td][/tr][/table]")
    assert_parse("<p><em>inline</em></p><table class=\"striped\"><tr><td>text</td></tr></table>", "[i]inline\n\n[table][tr][td]text[/td][/tr][/table]")
  end

  def test_table_unclosed_th
    assert_parse('<table class="striped"><th>foo</th></table>', "[table][th]foo")
  end
end
