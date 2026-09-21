#pragma once

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "nexusflow.grpc.pb.h"
#include "api/grpc/nexusflow_grpc_service.hpp"

namespace nexusflow::grpc {

class NexusFlowGrpcServer {
public:
    NexusFlowGrpcServer(
        const std::string& address,
        std::size_t queue_capacity = 4096,
        std::size_t max_workers = 8
    );

    ~NexusFlowGrpcServer();

    bool start();
    void stop();

    bool running() const;

private:
    class ServiceImpl final
        : public nexusflow::grpc::NexusFlowService::Service {
    public:
        explicit ServiceImpl(NexusFlowGrpcServer& owner);

        ::grpc::Status SubmitEvent(
            ::grpc::ServerContext* context,
            const ::nexusflow::grpc::EventRequest* request,
            ::nexusflow::grpc::EventResponse* response
        ) override;

        ::grpc::Status Health(
            ::grpc::ServerContext* context,
            const ::nexusflow::grpc::HealthRequest* request,
            ::nexusflow::grpc::HealthResponse* response
        ) override;

    private:
        NexusFlowGrpcServer& owner_;
    };

    std::string address_;
    std::size_t queue_capacity_;
    std::size_t max_workers_;

    std::unique_ptr<::grpc::Server> server_;
    std::unique_ptr<ServiceImpl> service_;
    std::unique_ptr<NexusFlowGrpcService> pipeline_;
};

} // namespace nexusflow::grpc
