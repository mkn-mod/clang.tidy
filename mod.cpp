/**
Copyright (c) 2026, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "mkn/mod/init.hpp"  // IWYU pragma: keep

#include "mkn/kul/os.hpp"
#include "mkn/kul/log.hpp"
#include "mkn/kul/proc.hpp"
#include "mkn/kul/yaml.hpp"
#include "mkn/kul/lang/cpp/compilers.hpp"

namespace mkn::clang {

class TidyModule : public mkn::mod::Module {
 protected:
  static void VALIDATE_NODE(const YAML::Node& node) {
    using namespace mkn::kul::yaml;
    Validator({NodeValidator("args"), NodeValidator("config"), NodeValidator("checks"),
               NodeValidator("types")})
        .validate(node);
  }

 public:
  void init(mkn::mod::Context& ctx, const YAML::Node& node) KTHROW(std::exception) override {
    VALIDATE_NODE(node);

    std::string const binary = mkn::kul::env::GET("CLANG_TIDY", "clang-tidy");

    std::string clangBinDir;
    std::string const clangHome(mkn::kul::env::GET("CLANG_HOME"));
    if (!clangHome.empty()) {
      mkn::kul::Dir cBin("bin", clangHome);
      if (!cBin) KEXCEPT(mkn::kul::Exception, "$CLANG_HOME/bin does not exist");
      clangBinDir = cBin.real();
    }

    std::unordered_set<std::string> types;
    if (!node["types"]) {
      types = {"cpp", "cxx", "cc", "h", "hpp"};
    } else
      for (const auto& s : mkn::kul::String::SPLIT(node["types"].Scalar(), ":")) types.insert(s);

    // Groups source files by their exact real compile args - the same
    // resolution logic the real build uses for that file - so files sharing
    // a flag set run through one clang-tidy invocation instead of one per
    // file, while still needing no separate "args:" duplication to stay in
    // sync with the real ones. cmd.compiler carries the configured compiler
    // line (e.g. "ccache g++ -std=c++20"), whose trailing flags (-std=c++20)
    // land nowhere else, so they're extracted the same way the real compile
    // step does and merged ahead of cmd.args.
    std::map<std::vector<std::string>, std::vector<std::string>> filesByArgs;
    ctx.per_compiler_command([&](mkn::mod::CompileCommand const& cmd) {
      const std::string name = mkn::kul::File(cmd.in).name();
      if (name.find(".") == std::string::npos) return;
      const std::string type = name.substr(name.rfind(".") + 1);
      if (!types.count(type)) return;

      auto const id = mkn::kul::lang::cpp::Compilers::INSTANCE().identify(cmd.compiler);
      std::vector<std::string> args(id.trailing);
      args.insert(args.end(), cmd.args.begin(), cmd.args.end());

      filesByArgs[args].push_back(cmd.in);
    });

    for (auto const& [args, files] : filesByArgs) {
      mkn::kul::Process p(binary);
      if (!clangBinDir.empty()) {
        mkn::kul::env::Var pa("PATH", clangBinDir, mkn::kul::env::Var::Mode::PREP);
        p.var(pa.name(), pa.toString());
      }

      if (node["config"]) p << std::string("-config=" + node["config"].Scalar());
      if (node["checks"]) p << std::string("-checks=" + node["checks"].Scalar());

      for (std::string const& file : files) p << mkn::kul::File(file).escm();
      p << "--";

      for (std::string const& s : args) p << s;
      if (node["args"])
        for (const auto& s : mkn::kul::cli::asArgs(node["args"].Scalar())) p << s;

      KLOG(DBG) << p;
      p.start();
    }
  }
};

}  // namespace mkn::clang

extern "C" MKN_KUL_PUBLISH mkn::mod::Module* maiken_module_construct() {
  return new mkn ::clang ::TidyModule;
}

extern "C" MKN_KUL_PUBLISH void maiken_module_destruct(mkn::mod::Module* p) { delete p; }
