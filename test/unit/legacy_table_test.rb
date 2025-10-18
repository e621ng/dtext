require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Tests for legacy tables, like [ltable]text | text[/ltable]
class DTextLegacyTableTest < Minitest::Test
  include DTextTestHelper
  LSTART = "[ltable]"
  LEND   = "[/ltable]"

  def test_table_legacy
    assert_parse("<table class=\"striped\"><thead><tr><th>test1 \\\| test2 </th><th> test2</th></tr></thead><tbody><tr><td>abc </td><td> 123</td></tr></tbody></table>", <<~END)
    #{LSTART}
    test1 \\\| test2 | test2
    abc | 123
    #{LEND}
    END

  assert_parse("<table class=\"striped\"><thead><tr><th>test1 </th><th> test2</th></tr></thead><tbody><tr><td>abc </td><td> 123</td></tr></tbody></table><table class=\"striped\"><thead><tr><th>test1 </th><th> test2</th></tr></thead><tbody><tr><td>abc </td><td> 123</td></tr></tbody></table>", <<~END)
    #{LSTART}
    test1 | test2
    abc | 123
    #{LEND}
    #{LSTART}
    test1 | test2
    abc | 123
    #{LEND}
    END

  assert_parse("<table class=\"striped\"><thead><tr><th>test1</th></tr></thead><tbody></tbody></table>", <<~END)
    #{LSTART}
    test1
    #{LEND}
    END

  assert_parse("<table class=\"striped\"><thead><tr><th>test1</th></tr></thead><tbody><tr><td>test2</td></tr></tbody></table>", <<~END)
    #{LSTART}test1
    test2#{LEND}
    END
  end

  def assert_legacy_table_parse(html, table)
    assert_parse(html, <<~'LEND')
    #{LSTART}
    #{table}
    #{LEND}
    LEND
  end
end
