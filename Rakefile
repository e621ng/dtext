# ENV["G_MESSAGES_DEBUG"] = "all"

begin
  require "rake/extensiontask"
  require "rubygems/package_task"
  require "bundler"

  Bundler.setup(:default, :development)
rescue LoadError, Bundler::BundlerError => e
  $stderr.puts e.message
  $stderr.puts "Run `bundle install` to install missing gems"
  exit 1
end

Rake::ExtensionTask.new "dtext" do |ext|
  # this goes here to ensure ragel runs *before* the extension is compiled.
  task :compile => ["ext/dtext/dtext.cpp", "ext/dtext/rb_dtext.cpp"]
  ext.lib_dir = "lib/dtext"
end

# Smart file task: checks dependencies normally, but can be forced
file "ext/dtext/dtext.cpp" => Dir["ext/dtext/dtext.{cpp.rl,h}", "Rakefile"] do
  sh "ragel -G2 -C ext/dtext/dtext.cpp.rl -o ext/dtext/dtext.cpp"
end

# Force regeneration task for explicit compilation
desc "Force recompilation of Ragel files and compile extension"
task "compile:force" do
  # Remove the file to force regeneration, then compile
  rm_f "ext/dtext/dtext.cpp"
  Rake::Task["compile"].invoke
end

def run_dtext(*args)
  ruby "-Ilib", "-rdtext", *args
end

task test_inline_ragel: :compile do
  Bundler.with_unbundled_env do
    run_dtext "-e", 'puts DText.parse("hello\r\nworld")'
  end
end

task test: :compile do
  Bundler.with_unbundled_env do
    Dir["test/unit/*_test.rb"].sort.each do |test_file|
      run_dtext test_file
    end
  end
end

task reference_compare: :compile do
  Bundler.with_unbundled_env do
    run_dtext "test/reference_compare.rb"
  end
end

task reference_generate: :compile do
  Bundler.with_unbundled_env do
    run_dtext "test/reference_generate.rb"
  end
end

task default: :test
