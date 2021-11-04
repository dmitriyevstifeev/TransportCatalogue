#pragma once

#include <string>
#include <vector>

#include "json_reader.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

using namespace transcat;

void SerializeTransportCatalogue(const std::string &file_name,
                                 const transcat::TransportCatalogue &transport_catalogue,
                                 const transcat::RenderSettings &render_settings,
                                 const transcat::RoutingSettings &routing_settings,
                                 const std::shared_ptr<transcat::TransportRouter>& transport_router);
void DeserializeTransportCatalogue(const std::string &file_name,
                                   std::vector<InfoQuery> &queries_to_add,
                                   transcat::RenderSettings &render_settings,
                                   transcat::RoutingSettings &routing_settings,
                                   transcat::queries::QueryManager *queryManager,
                                   const transcat::TransportCatalogue &transport_catalogue);
