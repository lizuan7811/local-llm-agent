#include "GRPCAgentServiceImpl.hpp"
#include <iostream>

#include "httplib.h"
#include "config/EnvironConfig.hpp"
#include "spdlog/spdlog.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using agent::AgentMessage;
using agent::ServerMessage;

Status GRPCAgentServiceImpl::Connect(
    ServerContext *context,
    ServerReaderWriter<ServerMessage, AgentMessage> *stream) {
    AgentMessage msg;
    while (stream->Read(&msg)) {
        std::cout << "Receive from Agent, ID: " << msg.agent_id()
                << " | Content: " << msg.payload() << std::endl;

        ServerMessage reply;
        reply.set_command_id("cmd_001");
        reply.set_payload("HEARTBEAT_ACK");
        stream->Write(reply);
    }
    return Status::OK;
}
void RunServer(const EnvironConfig &config) {
    // 組合 gRPC 監聽位址
    std::string server_address = config.grpc.host + ":" + std::to_string(config.grpc.port);

    GRPCAgentServiceImpl agent_service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&agent_service);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    spdlog::info("Agent Server has been Started, Listening on: {}", server_address);

    server->Wait();
}