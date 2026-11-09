#ifndef LIYONG_OLLAMA_AGENT_AGENTSERVICE_HPP
#define LIYONG_OLLAMA_AGENT_AGENTSERVICE_HPP

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/sync_stream.h>

#include "../include/config/EnvironConfig.hpp"
#include "../proto/agent.grpc.pb.h"

class GRPCAgentServiceImpl final : public agent::AgentService::Service {
public:
    grpc::Status Connect(
        grpc::ServerContext *context,
        grpc::ServerReaderWriter<agent::ServerMessage, agent::AgentMessage> *stream) override;
};

void RunServer(const EnvironConfig &config);

#endif
