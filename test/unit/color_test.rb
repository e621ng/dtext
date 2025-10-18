require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for color tags, like [color=red]text[/color], [color=#ffaa11]text[/color], and [color=artist]text[/color].
class DTextColorTest < Minitest::Test
  include DTextTestHelper

  def assert_color(prefix, color)
    str_part = DText.parse("[color=#{color}]test[/color]", allow_color: true)[0]
    assert_equal("#{prefix}#{color}\">test</span></p>", str_part)
  end

  def test_color
    %w(gen general art artist cont contributor copy copyright char character spec species inv invalid meta lor lore).each do |color|
      assert_color("<p><span class=\"dtext-color-", color)
    end
    assert_color("<p><span class=\"dtext-color\" style=\"color:", 'yellow')
    assert_color("<p><span class=\"dtext-color\" style=\"color:", "#ccc")
    assert_color("<p><span class=\"dtext-color\" style=\"color:", "#12345")
    assert_color("<p><span class=\"dtext-color\" style=\"color:", "#1a1")

    assert_parse('<ul><li><span class="dtext-color" style="color:lime">test</span> abc</li></ul>', "* [color=lime]test[/color] abc", allow_color: true)
    assert_parse('<h1><span class="dtext-color" style="color:lime">test</span></h1>', "h1.[color=lime]test[/color]", allow_color: true)
  end

  def test_color_not_allowed
    assert_parse("<p>test</p>", "[color=invalid]test[/color]")
    assert_parse("<p>test</p>", "[color=#123456]test[/color]")

    assert_parse('<ul><li>test abc</li></ul>', "* [color=lime]test[/color] abc")
    assert_parse('<h1>test</h1>', "h1.[color=lime]test[/color]")
  end
end
