require 'minitest/autorun'
require 'dtext'
require_relative 'test_helper'

# Regression tests for inputs that used to make the parser quadratic. Each case took
# seconds to minutes before the fix and takes milliseconds after it; the budget is
# deliberately generous so slow CI runners do not flake.
class DTextPerformanceTest < Minitest::Test
  BUDGET_SECONDS = 1.0

  def assert_parses_quickly(input)
    started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    DText.parse(input)
    elapsed = Process.clock_gettime(Process::CLOCK_MONOTONIC) - started
    assert_operator elapsed, :<, BUDGET_SECONDS, "parsing #{input.bytesize} bytes took #{elapsed.round(2)}s"
  end

  # E6SEC-302: link / section bodies that could contain their own opener made every
  # opener rescan to the end of the input.
  def test_runs_of_link_openers
    assert_parses_quickly("[" * 400_000)
    assert_parses_quickly("{" * 400_000)
    assert_parses_quickly("[[a" * 150_000)
    assert_parses_quickly("{{a" * 150_000)
    assert_parses_quickly("[section=" * 50_000)
  end

  # E6SEC-303: scanning the element stack on every token.
  def test_deep_element_stack
    stack = "[b]" * 100_000
    assert_parses_quickly(stack + "[sup]" * 100_000)
    assert_parses_quickly(stack + "a\n" * 100_000)
    assert_parses_quickly(stack + "[/quote]" * 100_000)
    assert_parses_quickly(stack + "[/section]" * 100_000)
  end
end
