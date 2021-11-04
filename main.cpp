#include <fstream>
#include <iostream>
#include <string_view>

#include "json_reader.h"
#include "transport_catalogue.h"

using namespace std::literals;
using namespace transcat;

void PrintUsage(std::ostream& stream = std::cerr) {
  stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {

  if (argc != 2) {
    PrintUsage();
    return 1;
  }

  const std::string_view mode(argv[1]);

  if (mode == "make_base"sv) {

    TransportCatalogue tc;
    queries::QueryManager qm(tc);
    qm.ReadJsonRequests(std::cin);
    qm.Serialize();

  } else if (mode == "process_requests"sv) {

    TransportCatalogue tc;
    queries::QueryManager qm(tc);
    qm.ReadJsonRequests(std::cin);
    qm.Deserialize();
    json::Print(qm.GetJSONAnswers(), std::cout);

  } else {
    PrintUsage();
    return 1;
  }
}