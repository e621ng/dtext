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
  spec.extensions = ["ext/dtext/extconf.rb"]

  spec.metadata["homepage_uri"] = spec.homepage
  spec.metadata["source_code_uri"] = spec.homepage

  spec.files = [
    "lib/dtext.rb",
    "lib/dtext/dtext.so",
    "lib/dtext/version.rb",
  ]

  spec.add_development_dependency(%q<minitest>, ["~> 5.10"])
  spec.add_development_dependency(%q<rake-compiler>, ["~> 1.0"])
end
