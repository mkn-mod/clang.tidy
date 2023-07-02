

#include <maiken.hpp>

#include "mkn/kul/signal.hpp"

const std::string yArgs = R"(
args:
    -std=c++17
)";

int main(int argc, char* argv[]) {
  mkn::kul::Signal sig;
  try {
    YAML::Node node = mkn::kul::yaml::String(yArgs).root();
    char* argv2[2] = {argv[0], (char*)"-R"};
    auto app = (maiken::Application::CREATE(2, argv2))[0];
    auto loader(maiken::ModuleLoader::LOAD(*app));
    loader->module()->link(*app, node);
    loader->unload();
  } catch (mkn::kul::Exception const& e) {
    KERR << e.what();
    return 2;
  } catch (std::exception const& e) {
    KERR << e.what();
    return 3;
  } catch (...) {
    KERR << "UNKNOWN EXCEPTION TYPE CAUGHT";
    return 5;
  }

  return 0;
}
