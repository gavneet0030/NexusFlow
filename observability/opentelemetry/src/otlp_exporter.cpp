#include "otlp_exporter.hpp"

#include "trace.pb.h"
#include "trace_service.pb.h"

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#else
#include <curl/curl.h>
#endif

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

namespace nexusflow::observability {

namespace {

std::string make_otlp_trace_id(
    std::uint64_t id
)
{
    std::array<unsigned char, 16> bytes{};

    // OTLP trace_id is exactly 16 bytes.
    // Preserve the internal 64-bit ID in the lower 8 bytes.
    for (int i = 0; i < 8; ++i) {
        bytes[8 + i] =
            static_cast<unsigned char>(
                (id >> (56 - i * 8)) & 0xFF
            );
    }

    return std::string(
        reinterpret_cast<const char*>(bytes.data()),
        bytes.size()
    );
}

std::string make_otlp_span_id(
    std::uint64_t id
)
{
    std::array<unsigned char, 8> bytes{};

    for (int i = 0; i < 8; ++i) {
        bytes[i] =
            static_cast<unsigned char>(
                (id >> (56 - i * 8)) & 0xFF
            );
    }

    return std::string(
        reinterpret_cast<const char*>(bytes.data()),
        bytes.size()
    );
}

#ifdef _WIN32

void print_winhttp_error(
    const char* operation
)
{
    std::cerr
        << "WINHTTP FAILURE: "
        << operation
        << " ERROR="
        << GetLastError()
        << "\n";
}

bool export_http(
    const std::string& body,
    long& status
)
{
    HINTERNET session =
        WinHttpOpen(
            L"NexusFlow-OTLP/1.0",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

    if (!session) {
        print_winhttp_error("WinHttpOpen");
        return false;
    }

    HINTERNET connection =
        WinHttpConnect(
            session,
            L"127.0.0.1",
            4318,
            0
        );

    if (!connection) {
        print_winhttp_error("WinHttpConnect");
        WinHttpCloseHandle(session);
        return false;
    }

    HINTERNET request_handle =
        WinHttpOpenRequest(
            connection,
            L"POST",
            L"/v1/traces",
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0
        );

    if (!request_handle) {
        print_winhttp_error("WinHttpOpenRequest");
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    const wchar_t* headers =
        L"Content-Type: application/x-protobuf\r\n"
        L"Accept: application/x-protobuf\r\n";

    BOOL sent =
        WinHttpSendRequest(
            request_handle,
            headers,
            static_cast<DWORD>(-1L),
            const_cast<char*>(body.data()),
            static_cast<DWORD>(body.size()),
            static_cast<DWORD>(body.size()),
            0
        );

    if (!sent) {
        print_winhttp_error("WinHttpSendRequest");
        WinHttpCloseHandle(request_handle);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    if (!WinHttpReceiveResponse(
            request_handle,
            nullptr)) {

        print_winhttp_error("WinHttpReceiveResponse");
        WinHttpCloseHandle(request_handle);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD win_status = 0;
    DWORD status_size = sizeof(win_status);

    if (!WinHttpQueryHeaders(
            request_handle,
            WINHTTP_QUERY_STATUS_CODE |
                WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &win_status,
            &status_size,
            WINHTTP_NO_HEADER_INDEX)) {

        print_winhttp_error("WinHttpQueryHeaders");
        WinHttpCloseHandle(request_handle);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    status = static_cast<long>(win_status);

    DWORD available = 0;

    if (WinHttpQueryDataAvailable(
            request_handle,
            &available)) {

        if (available > 0) {
            std::string response(
                available,
                '\0'
            );

            DWORD downloaded = 0;

            if (WinHttpReadData(
                    request_handle,
                    response.data(),
                    available,
                    &downloaded)) {

                response.resize(downloaded);

                std::cerr
                    << "OTLP RESPONSE: "
                    << response
                    << "\n";
            }
        }
    }

    WinHttpCloseHandle(request_handle);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return true;
}

#else

bool export_http(
    const std::string& body,
    long& status
)
{
    CURL* curl = curl_easy_init();

    if (!curl) {
        std::cerr
            << "CURL FAILURE: INITIALIZATION\n";
        return false;
    }

    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(
        headers,
        "Content-Type: application/x-protobuf"
    );

    headers = curl_slist_append(
        headers,
        "Accept: application/x-protobuf"
    );

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        "http://127.0.0.1:4318/v1/traces"
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        body.data()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDSIZE,
        static_cast<long>(body.size())
    );

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "NexusFlow-OTLP/1.0"
    );

    CURLcode result =
        curl_easy_perform(curl);

    if (result != CURLE_OK) {
        std::cerr
            << "CURL FAILURE: "
            << curl_easy_strerror(result)
            << "\n";

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return false;
    }

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &status
    );

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return true;
}

#endif

}

OtlpExporter::OtlpExporter(
    std::string endpoint
)
    : endpoint_(std::move(endpoint)),
      healthy_(!endpoint_.empty())
{
}

bool OtlpExporter::export_span(
    const SpanRecord& span
)
{
    std::cerr
        << "OTLP STEP 1: EXPORT ENTER\n";

    if (!healthy_) {
        std::cerr
            << "OTLP FAILURE: EXPORTER UNHEALTHY\n";
        return false;
    }

    if (span.name.empty()) {
        std::cerr
            << "OTLP FAILURE: EMPTY SPAN NAME\n";
        return false;
    }

    if (span.context.trace_id == 0) {
        std::cerr
            << "OTLP FAILURE: TRACE ID ZERO\n";
        return false;
    }

    if (span.context.span_id == 0) {
        std::cerr
            << "OTLP FAILURE: SPAN ID ZERO\n";
        return false;
    }

    if (span.start_ns == 0) {
        std::cerr
            << "OTLP FAILURE: START TIME ZERO\n";
        return false;
    }

    if (span.end_ns < span.start_ns) {
        std::cerr
            << "OTLP FAILURE: INVALID TIME RANGE\n";
        return false;
    }

    std::cerr
        << "OTLP STEP 2: SPAN VALIDATION PASS\n";

    opentelemetry::proto::trace::v1::Span proto_span;

    proto_span.set_name(span.name);

    const std::string trace_id =
        make_otlp_trace_id(
            span.context.trace_id
        );

    const std::string span_id =
        make_otlp_span_id(
            span.context.span_id
        );

    if (trace_id.size() != 16) {
        std::cerr
            << "OTLP FAILURE: TRACE ID NOT 16 BYTES\n";
        return false;
    }

    if (span_id.size() != 8) {
        std::cerr
            << "OTLP FAILURE: SPAN ID NOT 8 BYTES\n";
        return false;
    }

    proto_span.set_trace_id(trace_id);
    proto_span.set_span_id(span_id);

    proto_span.set_start_time_unix_nano(
        span.start_ns
    );

    proto_span.set_end_time_unix_nano(
        span.end_ns
    );

    proto_span.set_kind(
        opentelemetry::proto::trace::v1::
            SPAN_KIND_INTERNAL
    );

    opentelemetry::proto::collector::trace::v1::
        ExportTraceServiceRequest request;

    auto* resource =
        request.add_resource_spans();

    auto* service_attribute =
        resource->mutable_resource()
            ->add_attributes();

    service_attribute->set_key(
        "service.name"
    );

    service_attribute->mutable_value()
        ->set_string_value(
            "NexusFlow"
        );

    auto* scope_spans =
        resource->add_scope_spans();

    scope_spans->add_spans()
        ->CopyFrom(proto_span);

    std::string body;

    if (!request.SerializeToString(&body)) {
        std::cerr
            << "OTLP FAILURE: PROTOBUF SERIALIZATION\n";
        return false;
    }

    std::cerr
        << "OTLP STEP 3: PROTOBUF SERIALIZATION PASS\n";

    std::cerr
        << "OTLP TRACE ID BYTES: "
        << trace_id.size()
        << "\n";

    std::cerr
        << "OTLP SPAN ID BYTES: "
        << span_id.size()
        << "\n";

    std::cerr
        << "OTLP PAYLOAD BYTES: "
        << body.size()
        << "\n";

    long status = 0;

    if (!export_http(body, status)) {
        return false;
    }

    std::cerr
        << "OTLP HTTP STATUS: "
        << status
        << "\n";

    if (status >= 200 && status < 300) {
        std::cerr
            << "OTLP HTTP EXPORT: PASS\n";
        return true;
    }

    std::cerr
        << "OTLP HTTP EXPORT: FAIL\n";

    return false;
}

bool OtlpExporter::healthy() const noexcept
{
    return healthy_;
}

const std::string& OtlpExporter::endpoint() const noexcept
{
    return endpoint_;
}

}
