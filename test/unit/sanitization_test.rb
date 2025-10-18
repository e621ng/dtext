require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for DText content sanitization. Ensures that potentially dangerous characters are properly escaped.
class DTextSanitizationTest < Minitest::Test
  include DTextTestHelper

  # test the assert_parse helper from DTextTestHelper
  def test_args
    assert_parse(nil, nil)
    assert_parse("", "")
    assert_raises(TypeError) { DText.parse(42) }
  end

  def test_sanitize_heart
    assert_parse('<p>&lt;3</p>', "<3")
  end

  def test_sanitize_less_than
    assert_parse('<p>&lt;</p>', "<")
  end

  def test_sanitize_greater_than
    assert_parse('<p>&gt;</p>', ">")
  end

  def test_sanitize_ampersand
    assert_parse('<p>&amp;</p>', "&")
  end

  def test_url_xss
    e = assert_raises(DText::Error) do
      DText.parse(%("url":/page\xF4">x\xFA<b>xss\xFA</b>))
    end
    assert_equal("invalid byte sequence in UTF-8", e.message)
  end

  def test_extra_newlines
    assert_parse('<p>a</p><p>b</p>', "a\n\n\n\n\n\n\nb\n\n\n\n")
  end

  def test_boundary_exploit
    assert_parse('<p>@mack&lt;</p>', "@mack<")
  end

  def test_null_bytes
    e = assert_raises(DText::Error) { DText.parse("foo\0bar") }
    assert_equal("invalid byte sequence in UTF-8", e.message)
  end

  def test_mention_xss
    e = assert_raises(DText::Error) do
      DText.parse("@user\xF4<b>xss\xFA</b>")
    end
    assert_equal("invalid byte sequence in UTF-8", e.message)
  end

  def test_inline_mode
    assert_equal("hello", DText.parse("hello", inline: true)[0].strip)
  end
end
