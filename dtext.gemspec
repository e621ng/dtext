# frozen_string_literal: true

require_relative "lib/dtext/version"

Gem::Specification.new do |spec|
  spec.name = "dtext"
  spec.version = DText::VERSION
  spec.authors = ["r888888888", "evazion", "earlopain", "sindrake"]

  spec.summary = "E621 DText parser"
  spec.homepage = "http://github.com/e621ng/dtext"
  spec.license = "MIT"
  spec.required_ruby_version = ">= 3.0.0"
  spec.platform = Gem::Platform::RUBY
  spec.extensions = ["ext/dtext/extconf.rb"]

  spec.metadata["homepage_uri"] = spec.homepage
  spec.metadata["source_code_uri"] = spec.homepage

  # Don't pack the dynamic library (.so), that will be done by
  # `bundle exec rake native`. Pack sources so we always have a
  # source gem available for platforms other than `x86_64-linux-musl`.
  spec.files = [
    "lib/dtext.rb",
    "lib/dtext/version.rb",
    "ext/dtext/extconf.rb",
    "ext/dtext/dtext.cpp",
    "ext/dtext/dtext.h",
    "ext/dtext/rb_dtext.cpp",
  ]

  spec.add_development_dependency(%q<minitest>, ["~> 5.10"])
  spec.add_development_dependency(%q<rake-compiler>, ["~> 1.0"])
end
