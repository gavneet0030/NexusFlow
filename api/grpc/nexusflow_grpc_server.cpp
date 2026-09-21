#include "nexusflow_grpc_server.hpp"

namespace nexusflow::grpc {

NexusFlowGrpcServer::ServiceImpl::ServiceImpl(
    NexusFlowGrpcServer& owner
)
    : owner_(owner) {
}

::grpc::Status NexusFlowGrpcServer::ServiceImpl::SubmitEvent(
    ::grpc::ServerContext*,
    const ::nexusflow::grpc::EventRequest* request,
    ::nexusflow::grpc::EventResponse* response
) {
    if (request == nullptr || response == nullptr) {
        return ::grpc::Status(
            ::grpc::StatusCode::INVALID_ARGUMENT,
            "Invalid request"
        );
    }

    if (!owner_.pipeline_) {
        response->set_accepted(false);
        response->set_message("Service is not running");
        return ::grpc::Status::OK;
    }

    const bool accepted = owner_.pipeline_->submit_event(
        request->event_id(),
        request->timestamp_ms(),
        request->type(),
        request->payload()
    );

    response->set_accepted(accepted);
    response->set_message(
        accepted ? "Event accepted" : "Event rejected"
    );

    return ::grpc::Status::OK;
}

::grpc::Status NexusFlowGrpcServer::ServiceImpl::Health(
    ::grpc::ServerContext*,
    const ::nexusflow::grpc::HealthRequest*,
    ::nexusflow::grpc::HealthResponse* response
) {
    if (response == nullptr) {
        return ::grpc::Status(
            ::grpc::StatusCode::INVALID_ARGUMENT,
            "Invalid response"
        );
    }

    const bool healthy =
        owner_.pipeline_ &&
        owner_.pipeline_->healthy();

    response->set_healthy(healthy);

    if (owner_.pipeline_) {
        response->set_processed_events(
            owner_.pipeline_->processed_events()
        );
    }

    return ::grpc::Status::OK;
}

NexusFlowGrpcServer::NexusFlowGrpcServer(
    const std::string& address,
    std::size_t queue_capacity,
    std::size_t max_workers
)
    : address_(address),
      queue_capacity_(queue_capacity),
      max_workers_(max_workers) {
}

NexusFlowGrpcServer::~NexusFlowGrpcServer() {
    stop();
}

bool NexusFlowGrpcServer::start() {
    if (server_) {
        return false;
    }

    pipeline_ = std::make_unique<NexusFlowGrpcService>(
        queue_capacity_,
        max_workers_
    );

    if (!pipeline_->start()) {
        pipeline_.reset();
        return false;
    }

    service_ = std::make_unique<ServiceImpl>(*this);

    ::grpc::ServerBuilder builder;

    builder.AddListeningPort(
        address_,
        ::grpc::InsecureServerCredentials()
    );

    builder.RegisterService(service_.get());

    server_ = builder.BuildAndStart();

    if (!server_) {
        service_.reset();
        pipeline_->stop();
        pipeline_.reset();
        return false;
    }

    return true;
}

void NexusFlowGrpcServer::stop() {
    if (server_) {
        server_->Shutdown();
        server_.reset();
    }

    service_.reset();

    if (pipeline_) {
        pipeline_->stop();
        pipeline_.reset();
    }
}

bool NexusFlowGrpcServer::running() const {
    return server_ != nullptr;
}

} // namespace nexusflow::grpc
