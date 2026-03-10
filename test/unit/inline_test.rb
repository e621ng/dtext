require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

class DTextInlineTest < Minitest::Test
  include DTextTestHelper
  
  def test_inline
    assert_parse("<p>a</p>", "a", inline: false)
    assert_parse("a", "a", inline: true)

    assert_parse("<blockquote><p>a</p></blockquote>", "[quote]a[/quote]", inline: false)
    assert_parse("<blockquote class=\"dtext-quote-color\" style=\"border-left-color:red\"><p>a</p></blockquote>", "[quote=red]a[/quote]", inline: false)

    assert_parse("a", "[quote]a[/quote]", inline: true)
    assert_parse("a", "[quote=red]a[/quote]", inline: true)
  end
end
