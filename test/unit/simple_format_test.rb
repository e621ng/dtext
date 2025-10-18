require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for simple formatting, like paragraphs, newlines, sub, sup, etc.
class DTextSimpleFormatTest < Minitest::Test
  include DTextTestHelper

  def test_paragraphs
    assert_parse("<p>abc</p>", "abc")
  end

  def test_paragraphs_with_newlines_1
    assert_parse("<p>a<br>b<br>c</p>", "a\nb\nc")
  end

  def test_paragraphs_with_newlines_2
    assert_parse("<p>a</p><p>b</p>", "a\n\nb")
  end

  def test_sub_sup
    assert_parse("<p><sub>test</sub></p>", "[sub]test[/sub]")
    assert_parse("<p><sup>test</sup></p>", "[sup]test[/sup]")
    assert_parse("<p><sub><sub>test</sub></sub></p>", "[sub][sub]test[/sub][/sub]")
    assert_parse("<p><sup><sup>test</sup></sup></p>", "[sup][sup]test[/sup][/sup]")
  end

  def test_old_asterisks
    assert_parse("<p>hello *world* neutral</p>", "hello *world* neutral")
  end

  def test_italics
    assert_parse("<p><em>italic text</em></p>", "[i]italic text[/i]")
    assert_parse("<p><em><em>nested italic text</em></em></p>", "[i][i]nested italic text[/i][/i]")
  end

  def test_bold
    assert_parse("<p><strong>bold text</strong></p>", "[b]bold text[/b]")
    assert_parse("<p><strong><strong>nested bold text</strong></strong></p>", "[b][b]nested bold text[/b][/b]")
  end

  def test_strikethrough
    assert_parse("<p><s>strikethrough text</s></p>", "[s]strikethrough text[/s]")
    assert_parse("<p><s><s>nested strikethrough text</s></s></p>", "[s][s]nested strikethrough text[/s][/s]")
  end

  def test_underline
    assert_parse("<p><u>underline text</u></p>", "[u]underline text[/u]")
    assert_parse("<p><u><u>nested underline text</u></u></p>", "[u][u]nested underline text[/u][/u]")
  end
end