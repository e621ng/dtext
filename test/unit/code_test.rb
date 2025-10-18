require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for code blocks and inline code, like `[code]block[/code]` and `inline`.
class DTextCodeTest < Minitest::Test
  include DTextTestHelper

  def test_inline_code_backticks
    assert_parse(%{<p><span class="inline-code">`what`</span></p>}, "`\\`what\\``")
  end

  def test_literal_backticks
    assert_parse(%{<p>`what`</p>}, "\\`what\\`")
  end

  def test_inline_code_spaced
    assert_parse("<p><span class=\"inline-code\">x</span> <span class=\"inline-code\">y</span></p>", "`x` `y`")
  end

  def test_inline_code_tag
    assert_parse('<p>hello </p><pre>tag</pre>', "hello [code]tag[/code]")
  end

  def test_code_block
    assert_parse("<pre>for (i=0; i&lt;5; ++i) {\n  printf(1);\n}\n\nexit(1);</pre>", "[code]for (i=0; i<5; ++i) {\n  printf(1);\n}\n\nexit(1);")
  end

  def test_code_block_inline_contexts
    assert_parse("<p>inline</p><pre>[/i]\n</pre>", "inline\n\n[code]\n[/i]\n[/code]")
    assert_parse('<p>inline</p><pre>[/i]</pre>', "inline\n\n[code][/i][/code]")
    assert_parse("<p><em>inline</em></p><pre>[/i]\n</pre>", "[i]inline\n\n[code]\n[/i]\n[/code]")
    assert_parse('<p><em>inline</em></p><pre>[/i]</pre>', "[i]inline\n\n[code][/i][/code]")
  end

  def test_expand_with_nested_code
    assert_parse("<details><summary></summary><div><pre>hello\n</pre></div></details>", "[section]\n[code]\nhello\n[/code]\n[/section]")
  end

  def test_quote_with_unclosed_code
    assert_parse('<blockquote><pre>foo[/quote]</pre></blockquote>', "[quote][code]foo[/quote]")
  end

  def test_inline_code_empty
    assert_parse('<p><span class="inline-code"></span></p>', "``")
  end

  def test_inline_code_single_space
    assert_parse('<p><span class="inline-code"> </span></p>', "` `")
  end

  def test_inline_code_unclosed
    assert_parse('<p><span class="inline-code">unclosed</span></p>', "`unclosed")
  end

  def test_inline_code_with_newlines
    assert_parse("<p><span class=\"inline-code\">multi\nline</span></p>", "`multi\nline`")
  end

  def test_inline_code_html_escaping
    assert_parse('<p><span class="inline-code">&lt;script&gt;alert(&quot;xss&quot;)&lt;/script&gt;</span></p>', "`<script>alert(\"xss\")</script>`")
  end

  def test_inline_code_bbcode_not_processed
    assert_parse('<p><span class="inline-code">[b]bold[/b]</span></p>', "`[b]bold[/b]`")
  end

  def test_inline_code_in_text
    assert_parse('<p>text<span class="inline-code">code</span>more</p>', "text`code`more")
  end

  def test_inline_code_nested_backticks
    assert_parse('<p><span class="inline-code">nested </span>code<span class="inline-code"></span></p>', "`nested `code``")
  end

  def test_inline_code_inline_mode
    assert_parse('<span class="inline-code">code</span>', "`code`", inline: true)
  end

  def test_literal_backticks_inline_mode
    assert_parse('`what`', "\\`what\\`", inline: true)
  end

  def test_mixed_escaped_and_code
    assert_parse('<p>\\`escaped<span class="inline-code">code</span></p>', "\\\\`escaped`code`")
  end
end
