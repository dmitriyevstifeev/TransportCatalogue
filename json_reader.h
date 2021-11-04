#pragma once

#include<iostream>
#include<memory>
#include<vector>

#include "domain.h"
#include "geo.h"
#include "json.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_router.h"

namespace transcat
  {
    namespace queries
      {
        class QueryManager {
         public:
          QueryManager(TransportCatalogue &tc)
              : tc_(tc)
              {
          }
          void ReadJsonRequests(std::istream &input);
          json::Document GetJSONAnswers();

          void Serialize();
          void Deserialize();
          void SetTransportRouter(std::shared_ptr<transcat::TransportRouter> transport_router);
          const std::shared_ptr<transcat::TransportRouter>& GetTranstoptRouter() const;
          void AddQueriesToTC();
         private:

          std::vector<InfoQuery> queries_to_add_;
          std::vector<Request> requests_;
          TransportCatalogue &tc_;
          std::shared_ptr<transcat::TransportRouter> tr_;
          transcat::RenderSettings render_settings_;
          transcat::RoutingSettings routing_settings_;
          transcat::SerializationSettings serialization_settings_;

        };

      }
  }
